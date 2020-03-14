#!/usr/bin/env python3


import os
import geventwebsocket
import weakref
import traceback

from geventwebsocket.server import WebSocketServer
import time
import json

from fendermustang import FenderMustang

SERVER_MAJOR, SERVER_MINOR = 0, 1

START_TIME = int(time.time() * 1000)
def millis():
    return int(time.time() * 1000) - START_TIME

class WebSocketHandler(object):
    clients = weakref.WeakKeyDictionary()
    def __init__(self, websocket, environ):
        self.ws = websocket
        self.environ = environ
        self.clientArgsAsStr = True

    def registerClient(self, client):
        self.clients[self] = {
            "start": time.time(),
            "client": client,
        }
        print('registerClient', client, self.clients)
        return self

    def data(self):
        return self.clients[self]

    def start(self):
        self.sendMessage("FCB2.Server", SERVER_MAJOR, SERVER_MINOR)
        cmd, args = self.recvMessage()
        print('WebSocketHandler.start', cmd, args)
        if cmd == "FCB2.019":
            return FCB2019Hardware(self.ws, self.environ).registerClient(cmd).start()
        elif cmd == "FCB2.Web":
            return FCB2019WebClient(self.ws, self.environ).registerClient(cmd).start()
        return self
        
    def recvMessage(self):
        msg = self.ws.receive()
        msg = json.loads(msg)
        ts, cmd, args = msg['t'], msg['c'], msg['a']
        # ts, cmd, *args = msg.split()
        print("recvMessage: {} {} {}".format(ts, cmd, args))
        return cmd, args

    def sendMessage(self, cmd, *args):
        # self.ws.send("{} {} {}".format(millis(), cmd, ' '.join([ str(x) for x in args])))
        if self.clientArgsAsStr:
            args = [ str(x) for x in args ]
        msg = json.dumps({"t": millis(), "c": cmd, "a": args})
        print("sendMessage: ", msg)
        self.ws.send(msg)

    def broadcast(self, cmd, *args, **filters):
        for client in self.getClients(**filters):
            client.sendMessage(cmd, *args)

    @classmethod
    def getClients(cls, **filters):
        print('getClients: {}'.format(filters))
        # import code; code.interact(local=locals())
        return  { s for s, d in cls.clients.items() for i in d.items() if i in filters.items() } if len(filters) else cls.clients.keys()

    @classmethod
    def getHardware(cls):
        return cls.getClients(client='FCB2.019')

    @staticmethod
    def getWeb(cls):
        return cls.getClients(client='FCB2.Web')

class FCB2019Hardware(WebSocketHandler):
    def start(self):
        self.loop()

    def loop(self):
        ret = None
        while ret is None:
            cmd, args = self.recvMessage()
            ret = getattr(self, 'handle_{}'.format(cmd.lower()))(*args)

    def handle_footswitch(self, fs, isPressed):
        isPressed = (isPressed == 1)
        print('handle_footswitch: {} {}'.format(fs, isPressed))
        self.broadcast('fs', fs, isPressed)

    def handle_pedal_value(self, pedal, valueStr):
        pedal, value = (10+int(pedal)), int(valueStr)
        print('handle_pedal: {} {}'.format(pedal, value))
        #self.broadcast('test', 1, 2, 3, 4, client='FCB2.Web')

    def led_on(self, led):
        self.sendMessage('led', led, 'state', 1)

    def led_off(self, led):
        self.sendMessage('led', led, 'state', 0)

    def led_blink(self, led, onTime=500, offTime=500, mode=None):
        if mode == 'fast':
            onTime, offTime = 100, 400
        elif mode == 'slow':
            onTime, offTime = 500, 500
        elif mode == 'steady':
            onTime, offTime = 5000, 0
        self.sendMessage('led', led, 'blink', onTime, offTime)

    def led_breathe(self, led, onTime=500, offTime=500, rampUpTime=250, rampDownTime=250, mode=None):
        if mode == 'fast':
            onTime, offTime, rampUpTime, rampDownTime = 500, 500, 250, 250
        elif mode == 'slow':
            onTime, offTime, rampUpTime, rampDownTime = 1000, 100, 500, 500
        self.sendMessage('led', led, 'breathe', onTime, offTime, rampUpTime, rampDownTime)

    def led_pedal_sync(self, led, pedal, enable=True):
        self.sendMessage('led', led, 'pedal', pedal, 1 if enable else 0)

class FCB2019WebClient(WebSocketHandler):
    '''
    load .fuse file
    load amp state
    control amp/effect settings
    '''
    def start(self):
        self.clientArgsAsStr = False
        self.fm = self.environ['FenderMustang']
        self.sendMessage("FCB2.init")
        self.loop()

    def loop(self):
        ret = None
        while ret is None:
            cmd, args = self.recvMessage()
            print('rx:', cmd, args)
            ret = getattr(self, 'handle_{}'.format(cmd.lower()))(*args)

    def handle_test(self, *args):
        for client in self.getHardware():
            client.led_pedal_sync(12, 1)

    def handle_start(self, *args):
        presets, name, amp, fx = self.fm.start()
        response = {
            'presets': [p.decode('utf8') for i, p in enumerate(presets) if i<100],
            'name': name.decode('utf8'),
            'amp': amp,
            'fx': fx,
        }
        print(response)
        self.sendMessage('start_r', response)

    def handle_load_preset(self, slot, *args):
        name, amp, fx = self.fm.load_memory_bank(int(slot))
        response = {
            'name': name.decode('utf8'),
            'amp': amp,
            'fx': fx,
        }
        print(response)
        self.sendMessage('load_preset_r', json.dumps(response))

 
def echo_app(environ, start_response):
    print('echo_app', environ)
    websocket = environ.get("wsgi.websocket")

    if websocket is None:
        return http_handler(environ, start_response)

    fm = environ['FenderMustang'] = FenderMustang()

    try:
        fcb2 = WebSocketHandler(websocket, environ)
        print('fcb2', fcb2)
        fcb2.start()
        websocket.close()
    except geventwebsocket.WebSocketError as ex:
        print("{0}: {1}".format(ex.__class__.__name__, ex))
    except Exception as ex:
        traceback.print_exc()


from pprint import pprint
def http_handler(environ, start_response):
    pprint(environ)
    path = os.path.join('./www', environ['PATH_INFO'].lstrip('/'))
    print("looking for {}".format(path))
    if os.path.exists(path):
        print("found {}".format(path))
        start_response("200 OK", [])
        return [open(path, "rb").read()]
    if environ["PATH_INFO"].strip("/") == "version":
        start_response("200 OK", [])
        return [agent]

    else:
        start_response("400 Bad Request", [])

        return ["WebSocket connection is expected here."]


path = os.path.dirname(geventwebsocket.__file__)
agent = bytearray("gevent-websocket/%s" % (geventwebsocket.get_version()),
                  'latin-1')

print("Running %s from %s" % (agent, path))
WebSocketServer(("", 3000), echo_app, debug=True).serve_forever()


#define DISABLE_OTA
//#define DISABLE_LEDS
//#define DISABLE_FOOTSWITCHES
//#define DISABLE_DEBOUNCE
//#define DISABLE_WIFI
//#define DISABLE_PEDALS
//#define DISABLE_WEBSOCKET
//#define DEBUG_WEBSOCKET

#ifndef DISABLE_OTA
#include <ArduinoOTA.h>
#endif
#include <ArduinoWebsockets.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WString.h>

#ifdef WIFI_MULTI
#include <WiFiMulti.h>
WiFiMulti WiFiMulti;
#else

#endif

#include <SparkFunSX1509.h>

#include <SSD1306.h>
#include <OLEDDisplay.h>
#include <OLEDDisplayFonts.h>
#include <SSD1306I2C.h>

#include <Preferences.h>

#include <list>
#include <iterator>
#include <algorithm>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <Statistic.h>

class BLEPreferences {
  private:
    const char * serviceUUID = "d3a591b7-a053-4db9-bcf3-e3fb035c40d4";
    BLEServer * m_ble_server;
    BLEService * m_ble_service;

    void _init_ble_service(void)
    {
      BLEDevice::init("b2"); // nspace.c_str());
      m_ble_server = BLEDevice::createServer();
      m_ble_service = m_ble_server->createService(serviceUUID);
    }

  public:
    typedef bool (*SettingChangedCallback)(BLEPreferences *prefs, String name, String &newValue, String oldValue, bool &needReboot);
    class Setting {
      public:
        String m_uuid;
        String m_description;
        String m_name;
        String m_value;
        SettingChangedCallback m_cb;


        Setting(String uuid, String name, String description, String value, SettingChangedCallback cb = NULL) :
          m_uuid(uuid),
          m_name(name),
          m_description(description),
          m_value(value),
          m_cb(cb)
        {
        }

        Setting(const Setting *src)
        {
          m_uuid = src->m_uuid;
          m_name = src->m_name;
          m_description = src->m_description;
          m_value = src->m_value;
          m_cb = src->m_cb;
        }
    };



    class MyCallbackHandler: public BLECharacteristicCallbacks {
        BLEPreferences *bleprefs;
        const Setting *setting;

        void onWrite(BLECharacteristic *pCharacteristic) {
          String uuid(pCharacteristic->getUUID().toString().c_str());
          String value(pCharacteristic->getValue().c_str());
          Serial.println(uuid);
          Serial.println(value);

          Serial.println(setting->m_uuid);
          Serial.println(setting->m_name);
          Serial.println(setting->m_description);

          if (uuid == setting->m_uuid)
          { bool needReboot = false;
            if (setting->m_cb(bleprefs, setting->m_name, value, setting->m_value, needReboot))
            {
              bleprefs->save(setting->m_name, value);
              if (needReboot)
                ESP.restart();
            }
          }

        }
      public:
        MyCallbackHandler(BLEPreferences *p, const Setting *s) {
          bleprefs = p;
          setting = new Setting(s);
        }
    };

  public:


    BLEPreferences(String ns)
    {
      // TODO: Handle namespace here
    }

    void setup(const std::list<Setting> l)
    {

      _init_ble_service();
      Preferences prefs;

      prefs.begin("b2", true);

      for (auto const&it : l) //(it=l.begin(); it!=l.end(); ++it)
      {
        Serial.print("Setting up ");
        Serial.println(it.m_name);
        BLECharacteristic *pCharacteristic = m_ble_service->createCharacteristic(
                                               it.m_uuid.c_str(),
                                               BLECharacteristic::PROPERTY_READ |
                                               BLECharacteristic::PROPERTY_WRITE
                                             );
        pCharacteristic->setCallbacks(new MyCallbackHandler(this, &it));
        const char *name = it.m_name.c_str();
        const char *default_value = it.m_value.c_str();
        const char *value = prefs.getString(name, default_value).c_str();
        Serial.println(name);
        Serial.println(default_value);
        Serial.println(value);
        pCharacteristic->setValue(value);
      }
      prefs.end();

      m_ble_service->start();
      BLEAdvertising *pAdvertising = m_ble_server->getAdvertising();
      pAdvertising->start();

    }

    void save(String name, String value)
    {
      Preferences prefs;

      prefs.begin("b2", false);
      prefs.putString(name.c_str(), value.c_str());
      prefs.end();
      Serial.println("saved");
    }

    String getValue(String name)
    {
      Preferences prefs;

      prefs.begin("b2", true);
      String ret(prefs.getString(name.c_str()));
      prefs.end();
      Serial.print("getValue ");
      Serial.print(name);
      Serial.print("=");
      Serial.println(ret);

      return ret;
    }
};



#ifndef DISABLE_OTA
void setup_ota(void)
{
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}
#endif

uint8_t ledPin = 16; // Onboard LED reference

const byte PEDAL1_PIN = 35, PEDAL2_PIN = 33;

const byte OLED_I2C_ADDRESS = 0x3c, OLED_I2C_SDA = 5, OLED_I2C_SCL = 4;

SSD1306 display(OLED_I2C_ADDRESS, OLED_I2C_SDA, OLED_I2C_SCL);


const byte SX1509_LEDS_ADDRESS = 0x3E;
const byte SX1509_SWITCHES_ADDRESS = 0x3F;

SX1509 leds, switches;

const byte FOOTSWITCH_DEBOUNCE_TIME = 8;

// SX1509 Pin definitledsn:
//const byte SX1509_LED_PIN = 0; // LED to SX1509's pin 15

void setup_display()
{
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.flipScreenVertically();
}

char displayBuff[15];

#define OLEDprintf(...) {  \
    sprintf(displayBuff, __VA_ARGS__);  \
    OLEDprint(displayBuff); \
  }

void OLEDprint(char *s)
{
  Serial.print(s);
  display.clear();
  display.drawString(0, 0, s);
  display.display();
}

const byte SX1509_I2C_SDA = 17;
const byte SX1509_I2C_SCL = 16;


bool footswitchesPressed = false;
void IRAM_ATTR button(void) {
  footswitchesPressed = true;
}

void setup_sx1509s(void)
{
  OLEDprint("sx1509s...");
  TwoWire *sxwire = new TwoWire(1);
  sxwire->begin(SX1509_I2C_SDA, SX1509_I2C_SCL, 100000);

#ifndef DISABLE_LEDS
  leds.use_wire(sxwire);
  if (!leds.begin(SX1509_LEDS_ADDRESS))
  {
    while (1) ; // If we fail to communicate, loop forever.
  }
  leds.clock(INTERNAL_CLOCK_2MHZ);
  OLEDprint("leds done.");
#endif

#ifndef SWITCHES
  switches.use_wire(sxwire);
  if (!switches.begin(SX1509_SWITCHES_ADDRESS))
  {
    while (1) ; // If we fail to communicate, loop forever.
  }
  switches.clock(INTERNAL_CLOCK_2MHZ);
  for (byte i = 0; i < 16; i++)
  {
    switches.pinMode(i, INPUT_PULLUP);
    switches.enableInterrupt(i, CHANGE);
#ifndef DISABLE_DEBOUNCE
    switches.debouncePin(i);
#endif
  }
#ifndef DISABLE_DEBOUNCE
  switches.debounceTime(FOOTSWITCH_DEBOUNCE_TIME);
#endif

  pinMode(21, INPUT_PULLUP);
  attachInterrupt(21, button, FALLING);

  OLEDprint("switches done.");
#endif
}

#ifndef DISABLE_WIFI
#ifndef DISABLE_WEBSOCKETS
using namespace websockets;
WebsocketsClient client;

const char *websockets_server_host = "192.168.0.13";
const int websockets_server_port = 3000;


typedef enum {
  WS_DISABLED,
  WS_INIT,
  WS_CONNECT,
  WS_CONNECTED,
  WS_DISCONNECTED
} ws_state_t;
ws_state_t ws_state = WS_DISABLED;
void connect_ws()
{
  ws_state = WS_INIT;
}

void onEventsCallback(websockets::WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed");
    ws_state = WS_DISCONNECTED;
  } else if (event == WebsocketsEvent::GotPing) {
#ifdef DEBUG_WEBSOCKET
    Serial.println("Got a Ping!");
#endif
  } else if (event == WebsocketsEvent::GotPong) {
#ifdef DEBUG_WEBSOCKET
    Serial.println("Got a Pong!");
#endif
  }
}

const uint64_t WS_RECONNECT_TIME = 500, WS_PING_INTERVAL = 1000;
uint64_t lastPing = 0, connectAt = 0;
void ws_loop()
{

  switch (ws_state)
  {
    case WS_DISABLED:
      return;
    case WS_INIT:
      client.onEvent(onEventsCallback);
      client.onMessage(onMessageCallback);
      /*client.onMessage([&](WebsocketsMessage message){
        //OLEDprintf("%s", message.data());
        Serial.println(message.data());
        });*/
      ws_state = WS_CONNECT;
      // run callback when events are occuring
      break;
    case WS_CONNECT:
      // Connect to server
      if (millis() < connectAt)
        break;
      if (client.connect(websockets_server_host, websockets_server_port, "/"))
      {
        ws_state = WS_CONNECTED;
        OLEDprintf("ws\nconnected\n");
        lastPing = millis();
      }
      else
      {
        ws_state = WS_DISCONNECTED;
        //OLEDprintf("ws\ndisconnected\n");
      }
      break;

    // Send a message
    //client.send("FCB2.019 init");

    // Send a ping
    //client.ping();
    case WS_CONNECTED:
      if ((millis() - lastPing) > WS_PING_INTERVAL)
      {
        client.ping();
        lastPing = millis();
      }
      if (client.available())
        client.poll();
      break;
    case WS_DISCONNECTED:
      ws_state = WS_CONNECT;
      connectAt = millis() + WS_RECONNECT_TIME;
      break;
    default:
      break;
  }
}

typedef enum {
  FC_INIT,
  FC_INIT_ACK,
} fcb2019_protocol_state_t;
fcb2019_protocol_state_t protocol_state = FC_INIT;
void fcb2019_loop()
{
  switch (protocol_state)
  {
    case FC_INIT:
      client.send("FCB2.019 init");
      protocol_state = FC_INIT_ACK;
      break;
    case FC_INIT_ACK:
      break;
  }
}

#endif

uint8_t wifi_status = -1, wifi_status_prev = -1;
char *wifi_status_str = NULL;
uint8_t wifi_status_changed(void)
{

  switch (wifi_status) {
    case WL_IDLE_STATUS:
      wifi_status_str = "Idle";
      break;
    case WL_NO_SSID_AVAIL:
      wifi_status_str = "SSID not found";
      break;
    case WL_SCAN_COMPLETED:
      wifi_status_str = "Scan Completed";
      break;
    case WL_CONNECTED:
      wifi_status_str = "Connected";
#ifndef DISABLE_OTA
      setup_ota();
#endif
#ifndef DISABLE_WEBSOCKET
      connect_ws();
#endif
      break;
    case WL_CONNECT_FAILED:
      wifi_status_str = "Connect Fail";
      break;
    case WL_CONNECTION_LOST:
      wifi_status_str = "Connect Lost";
      break;
    case WL_DISCONNECTED:
      wifi_status_str = "Disconnected";
      break;
    default:
      wifi_status_str = "wifi ?";
      break;
  }
  OLEDprintf("%s", wifi_status_str);
  wifi_status_prev = wifi_status;
}
#endif
//WebsocketsClient client;

#ifndef DISABLE_PEDALS
class Pedals
{
  private:

    const byte pedalPins[2];
    byte pedalPin;
    byte pedalIdx = 255;
    uint16_t prevVal[2];

    Statistic stats[2];
    Statistic calibStats[2];
    static bool resetCalibration;
    byte sampleCount = 63;
    byte sensitivity = 3; // Lower is more sensitive

    std::vector <byte> ledPins[2];

    void _setup_pedal(byte pin)
    {
      adcAttachPin(pin);
    }

    void _init_loop_first_run(void)
    {
      pedalIdx = 0;
      pedalPin = pedalPins[pedalIdx];
      stats[pedalIdx].clear();
      calibStats[pedalIdx].clear();

    }

    void _process_adc_value(uint16_t val)
    {
      if (resetCalibration)
      {
        calibStats[0].clear();
        calibStats[1].clear();
        resetCalibration = false;
      }
      stats[pedalIdx].add(val);

      if (stats[pedalIdx].count() > sampleCount)
      {
        uint16_t avgVal = (uint16_t)stats[pedalIdx].average();
        uint16_t minVal = calibStats[pedalIdx].minimum();
        uint16_t maxVal = calibStats[pedalIdx].maximum();

        calibStats[pedalIdx].add(avgVal);
        if ((maxVal - minVal) > 2000)
        {
          uint16_t calibVal = map(avgVal, minVal, maxVal, 0, 1024);
          if (abs(prevVal[pedalIdx] - calibVal) > sensitivity)
          {
            Serial.println("ledPins");
            for (auto const & ledPin : ledPins[pedalIdx] )
            {
              Serial.println(ledPin);
              leds.analogWrite(ledPin, map(calibVal, 0, 1024, 0, 255));

            }
            if (valueChanged)
              valueChanged(pedalIdx, calibVal);
            prevVal[pedalIdx] = calibVal;
          }
        } else {
          // TODO: Blink LED since not calibrated
        }
        stats[pedalIdx].clear();
      }
    }

    void _prep_for_next_run(void)
    {
      // Prep for next iteration
      ++pedalIdx;
      pedalIdx %= sizeof(pedalPins);
      pedalPin = pedalPins[pedalIdx];

    }

  public:

    void (*valueChanged)(byte pedalIndex, uint16_t value);

    Pedals() :  pedalPins{ PEDAL2_PIN, PEDAL1_PIN }, prevVal{0}
    {
    }
    void setup(void)
    {
      analogReadResolution(12);
      //analogSetClockDiv(1);
      //analogSetCycles(8);
      //analogSetSamples(64);
      //analogSetAttenuation(ADC_11db);
      _setup_pedal(PEDAL1_PIN);
      _setup_pedal(PEDAL2_PIN);

    }


    void loop(void)
    {
      if (pedalIdx == 255)
      {
        _init_loop_first_run();
        adcStart(pedalPin);
        return;
      }

      if (!adcBusy(pedalPin))
      {
        uint16_t val = adcEnd(pedalPin);
        _process_adc_value(val);
        _prep_for_next_run();
        adcStart(pedalPin);
      }
    }

    void sync_led(byte pedalIdx, byte ledPin, byte state) {
      switch (state) {
        case 0:
          ledPins[pedalIdx].erase(std::remove(ledPins[pedalIdx].begin(), ledPins[pedalIdx].end(), ledPin), ledPins[pedalIdx].end());
          break;
        case 1:
          if (!(std::find(ledPins[pedalIdx].begin(), ledPins[pedalIdx].end(), ledPin) != ledPins[pedalIdx].end()))
            ledPins[pedalIdx].push_back(ledPin);
          break;
        default:
          break;
      }
    }

    static bool onPedalCalibrationReset(BLEPreferences *prefs, String name, String &newValue, String oldValue, bool &needReboot)
    {
      resetCalibration = true;
      /*sensitivity = prefs->getValue("pedalSensitivity").toInt();
        sampleCount = prefs->getValue("pedalSamples").toInt();
        OLEDprintf("pedal %d\nSET %d\n", sensitivity, sampleCount);
      */
      return false;
    }


};

bool Pedals::resetCalibration = false;

#endif

#ifndef DISABLE_FOOTSWITCHES
const char *footswitchMappedNames[] = {
  "FS_UP", //    1
  "FS_7",   //    2
  "FS_10",  //    4
  NULL,   //    8
  NULL,   //   10
  "FS_9",   //   20
  "FS_6",   //   40
  "FS_8",   //   80
  "FS_3",   //  100
  "FS_5",   //  200
  "FS_4",   //  400
  "FS_1",   //  800
  NULL,   // 1000
  NULL,   // 2000
  "FS_DOWN",//4000
  "FS_2",   // 8000hh
};


unsigned int footswitchState = 0;
void footswitches_loop(void)
{
  if (!footswitchesPressed)
    return;

  footswitchesPressed = false;

  unsigned int val = switches.readPins() ^ 0xffff;

  for (byte i = 0; i < 16; i++)
  {
    bool fsPrevState = footswitchState & (0x01 << i);
    bool fsState = val & (0x01 << i);
    if (fsPrevState == false && fsState == true)
      onFootswitchDown(footswitchMappedNames[i]);
    else if (fsPrevState == true && fsState == false)
      onFootswitchUp(footswitchMappedNames[i]);
  }
  footswitchState = val;
}
#endif

#ifndef DISABLE_WIFI
void wifi_loop()
{
#ifdef WIFI_MULTI
  wifi_status = WiFiMulti.run();
#else
  wifi_status = WiFi.status();
#endif
  if (wifi_status != wifi_status_prev)
    wifi_status_changed();
}
#endif

#ifndef DISABLE_LEDS
void setup_leds(void)
{
  for (byte i = 0; i < 16; i++)
    leds.pinMode(i, OUTPUT);
}
void leds_loop(void)
{
  /*
    static byte pin = 0;

    if (pin < 16) {
    leds.pinMode(pin, OUTPUT); // Set LED pin to OUTPUT
    // Blink the LED pin -- ~1000 ms LOW, ~500 ms HIGH:
    //leds.blink(pin, 1000, 500);
    pin++;
    }*/
}
#endif

char hostname[10];
bool onWiFiSettingsChanged(BLEPreferences *prefs, String name, String &newValue, String oldValue, bool &needReboot)
{
  Serial.print("onWiFiSettingsChanged:");
  Serial.println(name);
  Serial.println(newValue);
  Serial.println(oldValue);
  needReboot = true;
  return true;
}

void setup_wifi(BLEPreferences *prefs)
{
  const char *ssid = strdup(prefs->getValue("ssid").c_str());
  const char *password = strdup(prefs->getValue("password").c_str());
  const char *hname = strdup(prefs->getValue("hostname").c_str());

  OLEDprintf("wifi=%s\n%s", ssid, password);

#ifndef DISABLE_WIFI
#ifdef WIFI_MULTI
  WiFiMulti.addAP(ssid, password);
#else
  strncpy(hostname, hname, sizeof(hostname));
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  MDNS.begin(hostname);
#endif
#endif
}

Pedals pedals;

char wsmsg[100];
void wsSendMsg(const char *msgType, uint16_t arg1, uint16_t arg2)
{
  sprintf(wsmsg, "%ld %s %d %d", millis(), msgType, arg1, arg2);
  client.send(wsmsg);
}

void api_reset(std::vector<char *> args)
{
  Serial.println("api_reset");
  ESP.restart();
}

byte ledPinMap[] = {
  15, // n.c.
  0, // 1
  1, // 2
  2, // 3
  3, // 4
  11, // 5
  4, // 6
  5, // 7
  6, // 8
  7, // 9
  12, // 10
  14, // Expr 1
  13, // Expr 2
};
void api_led(std::vector<char *> args)
{
  Serial.println("api_led");
  byte pin = ledPinMap[atoi(args[1])];
  Serial.println(pin);

  if (!strcmp("state", args[2]))
  {
    leds.digitalWrite(pin, 1-atoi(args[3]));
  } else if (!strcmp("intensity", args[2]))
  {
    leds.analogWrite(pin, atoi(args[3]));
  } else if (!strcmp("blink", args[2]))
  {
    leds.blink(pin, atoi(args[3]), atoi(args[4]));
  } else if (!strcmp("breathe", args[2]))
  {
    leds.breathe(pin, atoi(args[3]), atoi(args[4]), atoi(args[5]), atoi(args[6]));
  } else if (!strcmp("pedal", args[2]))
  {
    pedals.sync_led(atoi(args[3]), pin, atoi(args[4]));
  }

}

typedef void (*ws_api_fn_t)(std::vector<char *> args);
typedef struct {
  const char *api;
  ws_api_fn_t callback;
} ws_api_t;

ws_api_t api_handlers[] = {
  { "reset", api_reset },
  { "led", api_led },
  { NULL, NULL }
};

void onMessageCallback(websockets::WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.data());

  char *t, *msg = strdup(message.data().c_str());
  std::vector<char *> args;
  while ((t = strtok_r(msg, " \n", &msg)) != NULL)
  {
    args.push_back(t);
    t = strtok(NULL, " \t\n");
  }
  for (byte i = 0; api_handlers[i].api; i++)
  {
    if (!strncmp(api_handlers[i].api, args.front(), strlen(api_handlers[i].api)))
    {
      api_handlers[i].callback(args);
      break;
    }
  }
  free(msg);
}

#ifndef  DISABLE_PEDALS
void pedalsValueChanged(byte pedalIndex, uint16_t value)
{
  OLEDprintf("PEDAL_%d\n%d\n", pedalIndex + 1, value);
  wsSendMsg("PEDAL_VALUE", pedalIndex, value);
}
#endif



#ifndef DISABLE_FOOTSWITCHES
void onFootswitchDown(const char *fsname)
{
  OLEDprintf("%s\ndown\n", fsname);
  wsSendMsg(fsname, 1, 0);
}

void onFootswitchUp(const char *fsname)
{
  OLEDprintf("%s\nup\n", fsname);
  wsSendMsg(fsname, 0, 0);
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("fcb2019");

  pinMode(ledPin, OUTPUT);

  BLEPreferences xprefs("fcb2019");
  xprefs.setup({
    BLEPreferences::Setting("edabdb60-b8cc-4869-9368-9c5f11f0155e", "ssid", "WiFi SSID", "b2", onWiFiSettingsChanged),
    BLEPreferences::Setting("03f18da6-427c-422c-acc5-95966229efa0", "password", "WiFi Password", "hello", onWiFiSettingsChanged),
    BLEPreferences::Setting("9a9f29dd-c65b-490c-9f7c-34123f2c2f7a", "hostname", "Hostname", "fcb2019", onWiFiSettingsChanged),
    BLEPreferences::Setting("835e92fa-a035-4383-8e8b-26377f65a815", "pedalReset", "Reset Pedal Calibration", "0", Pedals::onPedalCalibrationReset),
    //    BLEPreferences::Setting("ce3b9e47-6fbc-4a64-bf4d-545c2b4b7785", "pedalSamples", "PedalSamples", "63", Pedals::onPedalSettingsChanged),
  });

  setup_display();
  setup_wifi(&xprefs);

  setup_sx1509s();
#ifndef DISABLE_PEDALS
  pedals.setup();
  pedals.valueChanged = pedalsValueChanged;
#endif
#ifndef DISABLE_LEDS
  setup_leds();
#endif
}

void loop() {
#ifndef DISABLE_OTA
  ArduinoOTA.handle();
#endif

#ifndef DISABLE_WIFI
  wifi_loop();
#endif

#ifndef DISABLE_LEDS
  leds_loop();
#endif

#ifndef DISABLE_PEDALS
  pedals.loop();
#endif

#ifndef DISABLE_FOOTSWITCHES
  footswitches_loop();
#endif

#ifndef DISABLE_WEBSOCKETS
  ws_loop();
#endif
}

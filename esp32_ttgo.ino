
//#define DISABLE_LEDS
//#define DISABLE_WIFI

#include <ArduinoWebsockets.h>

#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifdef WIFI_MULTI
#include <WiFiMulti.h>
WiFiMulti WiFiMulti;
#else

#endif

//#include <WebSocketsClient.h>



//#include <WebSocketsServer.h>
//#include <WebSocketsClient.h>
//#include <WebSockets.h>
//#include <SocketIOclient.h>


#include <SparkFunSX1509.h>
  

#include <SH1106.h>
#include <SSD1306.h>
#include <SH1106Spi.h>
#include <OLEDDisplayUi.h>
#include <SH1106Wire.h>
#include <SSD1306Brzo.h>
#include <OLEDDisplay.h>
#include <SH1106Brzo.h>
#include <OLEDDisplayFonts.h>
#include <SSD1306Spi.h>
#include <SSD1306Wire.h>
#include <SSD1306I2C.h>



uint8_t ledPin = 16; // Onboard LED reference

const byte PEDAL1_PIN = 36, PEDAL2_PIN = 39;

SSD1306 display(0x3c, 5, 4); // instance for the OLED. Addr, SDA, SCL


const byte SX1509_LEDS_ADDRESS = 0x3E;  // SX1509 I2C address
const byte SX1509_SWITCHES_ADDRESS = 0x3F;  // SX1509 I2C address

SX1509 leds, switches; // Create an SX1509 object to be used throughout

// SX1509 Pin definitledsn:
const byte SX1509_LED_PIN = 0; // LED to SX1509's pin 15

void setup_display()
{
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
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


bool buttonPressed = false;
void IRAM_ATTR button(void) {
  buttonPressed = true;
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
  OLEDprint("1");
  for (byte i=0; i<16; i++)
  {
    switches.pinMode(i, INPUT_PULLUP);
    switches.enableInterrupt(i, CHANGE);
#ifndef DISABLE_DEBOUNCE
    //switches.debounceTime(32);
    //switches.debouncePin(i);
#endif
  }
  OLEDprint("2");
  
  pinMode(21, INPUT_PULLUP);
  OLEDprint("3");
  attachInterrupt(21, 
                  button, FALLING);
  OLEDprint("4");
 
  OLEDprint("switches done.");
#endif
  


}

void setup_pedal(byte pin)
{
  adcAttachPin(pin);
  analogSetClockDiv(1);  
  adcStart(pin);
}

char hostname[10];
void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  setup_display();
  setup_sx1509s();
  setup_pedal(PEDAL1_PIN);
  setup_pedal(PEDAL2_PIN);
  

#ifndef DISABLE_WIFI
#ifdef WIFI_MULTI
  WiFiMulti.addAP("b2desk", "this is b2!");
#else
  sprintf(hostname, "fcb2019");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin("b2desk", "this is b2!");
  MDNS.begin(hostname);
#endif
#endif
}

using namespace websockets;
WebsocketsClient client;

/*void onMessageCallback( WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectledsnOpened) {
        Serial.println("Connnectledsn Opened");
    } else if(event == WebsocketsEvent::ConnectledsnClosed) {
        Serial.println("Connnectledsn Closed");
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}
*/

const char *websockets_server_host = "192.168.0.13";
const int websockets_server_port = 3000;

void connect_ws() 
{
      // run callback when messages are received
    //client.onMessage(onMessageCallback);

    client.onMessage([&](WebsocketsMessage message){
        OLEDprintf("%s", message.data());
        //Serial.println(message.data());
    });

    
    // run callback when events are occuring
    //client.onEvent(onEventsCallback);

    // Connect to server
    client.connect(websockets_server_host, websockets_server_port, "/");

    // Send a message
    client.send("Hello Server");

    // Send a ping
    client.ping();

}

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
      connect_ws();
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

//WebsocketsClient client;

void pedal_poll(byte pin)
{
  if (!adcBusy(pin))
  {
    uint16_t val = adcEnd(pin);
    Serial.print("pedal ");
    Serial.print(pin);
    Serial.print(": ");
    Serial.println(val);
    adcStart(pin);
  }
}

byte pin = 0;
void loop() {
#ifndef DISABLE_WIFI
#ifdef WIFI_MULTI
  wifi_status = WiFiMulti.run();
#else
  wifi_status = WiFi.status();
#endif        
 
  if (wifi_status != wifi_status_prev)
    wifi_status_changed();
#endif

#ifndef DISABLE_LEDS
  if (pin < 16) {
    leds.pinMode(pin, OUTPUT); // Set LED pin to OUTPUT
    // Blink the LED pin -- ~1000 ms LOW, ~500 ms HIGH:
    leds.blink(pin, 1000, 500);

    pin++;
 }
#endif

/*
  uint16_t analog1 = analogRead(36);
  Serial.println(analog1);

  uint16_t analog2 = analogRead(39);
  Serial.println(analog2);
*/  

  pedal_poll(PEDAL1_PIN);
  pedal_poll(PEDAL2_PIN);
  
//buttonPressed =  true;
if (buttonPressed) {
  buttonPressed = false;
#if 0
 byte val, i;
 char d[32];
 for (i=0; i<8; i++)
 {
   val = switches.digitalRead(i);
   d[i] = val ? '-': 'X';
   //Serial.print(val);
 }
 d[i] = '\n';
 for (i=8; i<16; i++)
 {
   val = switches.digitalRead(i);
   d[i+1] = val ? '-': 'X';
   //Serial.print(val);
 }
 d[i+1] = '\0'; 
  //Serial.println("");
  OLEDprintf(d);
#else
  unsigned int val = switches.readPins() ^ 0xffff;
  OLEDprintf("%x", val);
#endif
 }
 
 //if (wifi_status == WL_CONNECTED)
 if (client.available())
    client.poll(); 
}

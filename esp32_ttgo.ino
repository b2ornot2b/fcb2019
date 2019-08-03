
#define DISABLE_OTA
//#define DISABLE_LEDS
//#define DISABLE_SWITCHES
//#define DISABLE_WIFI
#define DISABLE_PEDALS
#define DISABLE_WEBSOCKET

#include <ArduinoOTA.h>
#include <ArduinoWebsockets.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifdef WIFI_MULTI
#include <WiFiMulti.h>
WiFiMulti WiFiMulti;
#else

#endif


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

const byte PEDAL1_PIN = 36, PEDAL2_PIN = 39;

const byte OLED_I2C_ADDRESS = 0x3c, OLED_I2C_SDA = 5, OLED_I2C_SCL = 4;

SSD1306 display(OLED_I2C_ADDRESS, OLED_I2C_SDA, OLED_I2C_SCL);


const byte SX1509_LEDS_ADDRESS = 0x3E;
const byte SX1509_SWITCHES_ADDRESS = 0x3F;

SX1509 leds, switches;

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
  for (byte i=0; i<16; i++)
  {
    switches.pinMode(i, INPUT_PULLUP);
    switches.enableInterrupt(i, CHANGE);
#ifndef DISABLE_DEBOUNCE
    //switches.debounceTime(32);
    //switches.debouncePin(i);
#endif
  }
  
  pinMode(21, INPUT_PULLUP);
  attachInterrupt(21, button, FALLING);
 
  OLEDprint("switches done.");
#endif
  


}

#ifndef DISABLE_PEDALS
void setup_pedal(byte pin)
{
  OLEDprintf("Pedal %d...", pin);
  adcAttachPin(pin);
  analogSetClockDiv(1);
  analogReadResolution(12);
  analogSetCycles(8);
  analogSetSamples(1);
  analogSetAttenuation(ADC_11db);
  OLEDprintf("Pedal %d", pin);
}
#endif

char hostname[10];
void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  
  setup_display();
  setup_sx1509s();
#ifndef DISABLE_PEDALS
  setup_pedal(PEDAL1_PIN);
  setup_pedal(PEDAL2_PIN);
#endif
  

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

const char *websockets_server_host = "192.168.0.13";
const int websockets_server_port = 3000;

void connect_ws() 
{
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

//WebsocketsClient client;

#ifndef DISABLE_PEDALS
const byte pedalPins[] = { PEDAL1_PIN, PEDAL2_PIN };
byte pedalPinIndex = 0;
byte pedalPin = -1;
void pedals_poll(void)
{
  if (pedalPin == -1)
  {
      pedalPin = pedalPins[pedalPinIndex];
      adcStart(pedalPin);
      return;
  }
  
  if (!adcBusy(pedalPin))
  {
    uint16_t val = adcEnd(pedalPin);
    Serial.print("pedal ");
    Serial.print(pedalPin);
    Serial.print(": ");
    Serial.println(val);
    ++pedalPinIndex;
    pedalPinIndex %= 2;
    pedalPin = pedalPins[pedalPinIndex]; 
    adcStart(pedalPin);
  }
}
#endif

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
  "FS_2",   // 8000   
};
void footswitches_poll(void)
{
  if (!footswitchesPressed)
    return;
    
    footswitchesPressed = false;

  unsigned int val = switches.readPins() ^ 0xffff;
  OLEDprint("");
  //OLEDprintf("val=%x ", val);
  for (byte i=0; val; val >>= 1, i++)
  {
    if (val%2) {
      /*if (buttonsMappedNames[i])
      {OLEDprintf("%s", buttonsMappedNames[i]);
      }else{
      OLEDprintf("???");}*/
    //OLEDprintf(" ( %d %d )\n", val, i);
    OLEDprintf("%s\n", footswitchMappedNames[i]);
    }  
  }
}

byte pin = 0;
void loop() {
#ifdef DISABLE_OTA
  ArduinoOTA.handle();
#endif

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

#ifndef DISABLE_PEDALS
  pedals_poll();
#endif
  footswitches_poll();
  
 
 if (client.available())
    client.poll(); 
}

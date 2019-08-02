


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

SSD1306 display(0x3c, 5, 4); // instance for the OLED. Addr, SDA, SCL


const byte SX1509_ADDRESS = 0x3E;  // SX1509 I2C address
SX1509 io; // Create an SX1509 object to be used throughout

// SX1509 Pin definition:
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

void setup_sx1509(void)
{
  OLEDprint("leds...");
  TwoWire *sxwire = new TwoWire(1);
  sxwire->begin(SX1509_I2C_SDA, SX1509_I2C_SCL, 100000);
  io.use_wire(sxwire);
  if (!io.begin(SX1509_ADDRESS))
  {
    while (1) ; // If we fail to communicate, loop forever.
  }

  
  OLEDprint("leds done.");
io.clock(INTERNAL_CLOCK_2MHZ);  


}

char hostname[10];
void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  setup_display();
  setup_sx1509();

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

}

using namespace websockets;
WebsocketsClient client;

/*void onMessageCallback( WebsocketsMessage message) {
    Serial.print("Got Message: ");
    Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
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

byte pin = 0;
void loop() {
#ifdef WIFI_MULTI
  wifi_status = WiFiMulti.run();
#else
  wifi_status = WiFi.status();
#endif        
 
  if (wifi_status != wifi_status_prev)
    wifi_status_changed();

  if (pin < 16) {
    io.pinMode(pin, OUTPUT); // Set LED pin to OUTPUT
    // Blink the LED pin -- ~1000 ms LOW, ~500 ms HIGH:
    io.blink(pin, 1000, 500);

    pin++;
 }
 //if (wifi_status == WL_CONNECTED)
 if (client.available())
    client.poll(); 
}

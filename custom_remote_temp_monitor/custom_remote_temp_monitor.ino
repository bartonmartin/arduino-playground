#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define BMP280_I2C_ADDRESS  0x76          // BMP280 address on I2C bus
#define ONE_WIRE_BUS D3                   // Data wire is plugged into D3 pin on the D1 mini

#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"


OneWire oneWire(ONE_WIRE_BUS);            // Setup a oneWire instance to communicate with any OneWire device
DallasTemperature sensors(&oneWire);      // Pass oneWire reference to DallasTemperature library
Adafruit_BMP280 bmp;                      // BMP280 is sending data over I2C
ESP8266WebServer server(80);              // 192.168.2.xxx port 80


void setup()
{
  Serial.begin(115200);




  sensors.begin();                        // Start up the DallasTemperature library

  Serial.begin(115200);
  while ( !Serial ) delay(100);           // wait for native usb

  Serial.println(F("BMP280 test"));
  unsigned status;

  status = bmp.begin(BMP280_I2C_ADDRESS);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                     "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,           /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,           /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,          /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,            /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500);       /* Standby time. */


  // wifi settings
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");

  // waiting for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // definition of all endpoints
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);

  // starting http server
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}


/*

   HTTP Server functions

*/


void handleRoot() {
  logEndpointMessage(" / root");
  
  sensors.requestTemperatures();

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");

  String endpoints = textToAhref("/status") + paragraph;


  String temperature = String("BMP280 Temperature = ") + bmp.readTemperature() + String(" Celsius");
  String temperature2 = String("DS18B20 Temperature = ") + sensors.getTempCByIndex(0) + String(" Celsius");
  String pressure = String("Pressure = ") + bmp.readPressure() + String(" Pascal");
  String altitude = String("Approx altitude  = ") + bmp.readAltitude(991.2) + String(" m");

  String style = String("<style>body {background-color: black;color: white;}</style>");
  String title = String("<title>ESP8266 Temperature server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + String("<h1>HTTP server works!</h1>") + paragraph
                + lineBreak
                + String("This is a super simple website, running on an ESP8266 based micro controller :)") + paragraph
                + String("You can see real time temperature and humidy and also you can control a relay.</p>") + paragraph
                + lineBreak
                + temperature + paragraph + lineBreak
                + temperature2 + paragraph + lineBreak
                + pressure + paragraph + lineBreak
                + altitude + paragraph + lineBreak
                + String("Or you call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + String("</body>");
  String rootContent = String("<html>") + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void handleStatus() {
  logEndpointMessage(" / status");
  
  sensors.requestTemperatures();

  int httpRequestCode = 200; // OK

  String temperature = String("BMP280_Temperature:") + bmp.readTemperature() + String("[Celsius],");
  String temperature2 = String("DS18B20_Temperature:") + sensors.getTempCByIndex(0) + String("[Celsius],");
  String pressure = String("Pressure:") + bmp.readPressure() + String("[Pascal],");
  String altitude = String("Altitude:") + bmp.readAltitude(991.2) + String("[m]");

  String message =  temperature + temperature2 + pressure + altitude;
  Serial.println("");
  Serial.println(message);
  Serial.println("");
  server.send(httpRequestCode, "text/plain", message);
}


void handleNotFound() {
  logEndpointMessage("404 - " + server.uri());

  int httpRequestCode = 404; // ERROR
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(httpRequestCode, "text/plain", message);
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  Serial.println(getClientIp() + " is calling " + enpointName);
}


String getClientIp() {
  return server.client().remoteIP().toString();
}


String textToAhref(String text) {
  String endpointText = ip2Str(WiFi.localIP()) + text;
  String ahref = String("<a href=\"") + text + String("\">") + endpointText + String("</a>");
  Serial.println(String("Endpoint: ") + endpointText);
  return ahref;
}


String boolToString(bool b) {
  return b ? "ON" : "OFF";
}


String boolToStringValue(bool b) {
  return b ? "true" : "false";
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

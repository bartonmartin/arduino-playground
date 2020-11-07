#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <ESP8266WebServer.h>

#define ID_RELAY            "1"
#define PIN_RELAY           D1
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"

Adafruit_BMP085 bmp;
boolean isRelayOn = false;


//192.168.2.xxx port 80
ESP8266WebServer server(80);

void setup()
{
  pinMode(PIN_RELAY, OUTPUT);
  Serial.begin(115200);
  //Wire.begin (4, 5);
  if (!bmp.begin())
  {
    Serial.println("Could not find BMP180 or BMP085 sensor at 0x77");
  }

  // setting all rellays to default state == OFF
  switchAllRelays();

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
  server.on("/temp", handleTemp);
  server.on("/pressure", handlePressure);
  server.on("/relay", handleRelay);
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
  Serial.println("");
  Serial.println("someone is calling root");

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");
  String buttons = textToButton("Refresh", "") + " "
                   + textToButton(ID_RELAY);

  String endpoints = textToAhref("/pressure") + paragraph
                     + textToAhref("/temp") + paragraph
                     + textToAhref("/relay?number=1&isOn=true") + paragraph;

  String relaysState = boolToString(isRelayOn) + paragraph;

  String temperature = String("Temperature = ") + bmp.readTemperature() + String(" Celsius");
  String pressure = String("Pressure = ") + bmp.readPressure() + String(" Pascal");

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
                + pressure + paragraph + lineBreak
                + String("<h3>Tap on buttons right here:</h3>") + paragraph + lineBreak
                + buttons + paragraph + lineBreak
                + String("Or you call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<i>Just for debugging purposes, here is <b>relay status:</b>") + paragraph + lineBreak
                + relaysState + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void handleRelay() {
  logEndpointMessage(" / relay");

  int httpRequestCode = 200; // OK
  int relayNumber = 0;

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "number") {
      relayNumber = server.arg(i).toInt();
    }
    if (argumentName == "isOn") {
      isRelayOn = server.arg(i) == "true";
    }
  }

  switch (relayNumber) {
    case 1:
      isRelayOn = isRelayOn;
      digitalWrite(PIN_RELAY, isRelayOn);
      break;
  }

  String status = isRelayOn ? " is ON" : " is OFF";
  String message =  "light number: " + String(relayNumber) + status;
  //server.send(httpRequestCode, "text/plain", message);
  handleRoot();
}


void handleTemp() {
  handleRoot();
}


void handlePressure() {
  handleRoot();
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


void switchAllRelays() {
  digitalWrite(PIN_RELAY, isRelayOn);
}


void logEndpointMessage(String enpointName) {
  Serial.println("---------------------------");
  Serial.println(getClientIp() + " vola " + enpointName);
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


String textToButton(String relayId) {
  String text = String("Light ") + relayId + String(" <br>Switch ") + boolToString(!isRelayOn);
  String url = String("/relay?number=") + relayId + String("&isOn=") + boolToStringValue(!isRelayOn);
  return textToButton(text, url);
}


String textToButton(String text, String url) {
  String endpointText = ip2Str(WiFi.localIP()) + url;
  String ahref = String("<a href=\"") + url + String("\"><button>   ") + text  + String("   </button></a> ");
  Serial.println(String("Endpoint: ") + endpointText);
  return ahref;
}


String relayState(String relayId, String state) {
  return String("Light ") + relayId + String(" : ") + state;
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

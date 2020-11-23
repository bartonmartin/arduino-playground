
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <OneWire.h>


#define PIN_RELAY         D1

#ifndef STASSID
#define STASSID "IoT"
#define STAPSK  "qSUpFC3XyLSLabgQ"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

OneWire  ds(D2);
ESP8266WebServer server(80);
float temperatureData = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  setupWifiServer();
  setupArduinoOTA();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
}


void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  float displayData = getDisplayData();
  temperatureData = displayData != 0 ? displayData : temperatureData;
}


void setupWifiServer() {
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  server.on("/", handleRoot);
  server.on("/switchOff", handleTurnOff);
  server.on("/switchOn", handleTurnOn);

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}


void setupArduinoOTA() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Lite_DS_Relay");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void handleRoot() {
  Serial.println("");
  logEndpointMessage(" / root");

  int httpRequestCode = 200; // OK

  String paragraph = String("<p>");
  String lineBreak = String("<br>");
  String buttons = textToButton("Root", "/");

  String temperature = String("Temperature in Celsius : ") + temperatureData + String(" Celsius (+- 1 Celsius)");

  String endpoints = textToAhref("/switchOn") + paragraph
                     + textToAhref("/switchOff") + paragraph;

  String autoRefresh = String("<meta http-equiv=\"refresh\" content=\"5\">"); //refresh every 5 seconds

  String style = String("<style>body {background-color: black;color: white;}</style>");
  String title = String("<title>ESP8266 OTA server</title>");
  String head = String("<head>") + style + title + String("</head>");
  String body = String("<body>")
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<h4>Tap a button to do something</h4>") + lineBreak
                + buttons + lineBreak
                + paragraph + lineBreak
                + temperature + lineBreak
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("<h4>HTTP server works!</h4>") + paragraph
                + String("This is a super simple website, running on an ESP8266 based micro controller :)") + paragraph
                + String("You can see real time temperature and also you can control a relay.") + paragraph
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("Or call any <b>endpoint</b> via browser or an app:") + paragraph + lineBreak
                + endpoints + paragraph + lineBreak
                + paragraph + lineBreak
                + paragraph + lineBreak
                + String("</i></body>");
  String rootContent = String("<html>") + autoRefresh + head + body + String("</html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void handleTurnOn() {
  logEndpointMessage(" / switchOn");
  digitalWrite(LED_BUILTIN, LOW);
  handleRoot();
}


void handleTurnOff() {
  logEndpointMessage(" / switchOff");
  digitalWrite(LED_BUILTIN, HIGH);
  handleRoot();
}


float getDisplayData() {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;

  //now the DS18b20
  if ( !ds.search(addr))
  {
    ds.reset_search();
    delay(250);
    return 0;
  }

  if (OneWire::crc8(addr, 7) != addr[7])
  {
    Serial.println("CRC is not valid!");
    return 0;
  }

  // the first ROM byte indicates which chip
  switch (addr[0])
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return 0;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);
  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++)
  {
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

  }

  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  return celsius;
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
  return ahref;
}


String textToButton(String text, String url) {
  String endpointText = ip2Str(WiFi.localIP()) + url;
  String ahref = String("<a href=\"") + url + String("\"><button>   ") + text  + String("   </button></a> ");
  return ahref;
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

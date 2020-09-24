// HTTP server co bezi na svabu za dola
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

// definice pinu
#define PIN_LED             D2
#define PIN_BUTTON          D3
#define LED_INDEX           0
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"

//192.168.2.xxx port 80
ESP8266WebServer server(80);

//RGB LED knihovna
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

// dalsi promene
int buttonState1 = LOW;
int buttonState2 = LOW;


void setup(void) {
  // zapnuti debug konzole
  Serial.begin(115200);

  // nastaveni LED aby se sni dalo blikat
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // nastaveni wifi modu, jmena site a hesla
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");

  // cekani na pripojeni k siti
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // definice vsech moznych endpointu
  server.on("/", handleRoot);
  server.on("/vypni", handleTurnOff);
  server.on("/zapni", handleTurnOn);
  server.on("/rgb", handleRGB);
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  // zapiname http server
  server.begin();
  Serial.println("HTTP server started");

  // zapiname RGB LED knihovnu
  pixels.begin();
}


void loop(void) {
  server.handleClient();


  // nacti novou hodnotu pro tento loop
  buttonState2 = digitalRead(PIN_BUTTON);

  if (buttonState1 == LOW && buttonState2 == HIGH) {
    // tlacitko zmenilo svuj stav ;
  }

  // uloz starou hodnotu na dalsi loop
  buttonState1 = buttonState2;

  // pauza 100ms
  delay(100);
}


void handleRoot() {
  logEndpointMessage("/root");

  int httpRequestCode = 200; // OK
  String webTitle = String("Muj ESP8266 web server");
  String body = String("<h1>HTTP server funguje!</h1><br>Muzete volat nasledujici <b>endpointy:</b>");
  String zapni = textToAhref("/zapni");
  String vypni = textToAhref("/vypni");
  String rgb = textToAhref("/rgb?r=0&g=0&b=0");
  String dalsi = String("</p> <p>");
  String rootContent = String("<html><head><title>") + webTitle + String("</title></head><body>") + body + String(" <p>") + zapni + dalsi + vypni + dalsi + rgb + String("</p> </body></html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void handleTurnOn() {
  logEndpointMessage("/zapni");

  int httpRequestCode = 200; // OK
  digitalWrite(LED_BUILTIN, LOW);
  server.send(httpRequestCode, "text/plain", "prave si aktivoval LED pres wifi pomoci prohlizece !!!!");
}


void handleTurnOff() {
  logEndpointMessage("/vypni");

  int httpRequestCode = 200; // OK
  digitalWrite(LED_BUILTIN, HIGH);
  pixels.setPixelColor(LED_INDEX, pixels.Color(0, 0, 0));
  pixels.show();
  server.send(httpRequestCode, "text/plain", "prave si deaktivoval LED pres wifi pomoci prohlizece");
}


void handleRGB() {
  logEndpointMessage("/rgb");

  int httpRequestCode = 200; // OK
  int r = 0;
  int g = 0;
  int b = 0;
  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "r") {
      r = server.arg(i).toInt();
    }

    if (argumentName == "g") {
      g = server.arg(i).toInt();
    }

    if (argumentName == "b") {
      b = server.arg(i).toInt();
    }
  }
  pixels.setPixelColor(LED_INDEX, pixels.Color(r, g, b));
  pixels.show();
  server.send(httpRequestCode, "text/plain", "prave si aktivoval RGB LED pres wifi pomoci prohlizece !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
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
  Serial.println(getClientIp() + " vola " + enpointName);
}


String getClientIp() {
  return server.client().remoteIP().toString();
}


String textToAhref(String text) {
  String ahref = String("<a href=\"") + text + String("\">") + ip2Str(WiFi.localIP()) + text + String("</a>");
  Serial.println(ahref);
  return ahref;
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

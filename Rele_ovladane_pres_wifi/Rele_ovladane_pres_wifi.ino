#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RELAY           D1
#define PIN_LED             D2
#define PIN_BUTTON          D3
#define LED_INDEX           0
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"

//192.168.2.xxx port 80
ESP8266WebServer server(80);


//RGB LED knihovna
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN_LED, NEO_GRB + NEO_KHZ800);

boolean isRelayOn = false;

int buttonState1 = LOW;
int buttonState2 = LOW;

void handleRoot() {
  server.send(200, "text/html", "<html><head><title>My first web page</title></head><body><h1>HTTP server funguje!</h1><br>Muzete volat nasledujici <b>endpointy:</b> <p>/zapni</p> <p>/vypni</p> <p>/relay</p> <p>/rgb?r=0&g=0&b=0</p> </body></html>");
  Serial.println("");
  Serial.print("nekdo vola root");
}

void handleTurnOn() {
  isRelayOn = HIGH;
  digitalWrite(PIN_RELAY, isRelayOn);
  digitalWrite(LED_BUILTIN, LOW);
  server.send(200, "text/plain", "prave si aktivoval LED pres wifi pomoci prohlizece !!!!");
  Serial.println("");
  Serial.print("nekdo zapnul LED");
}

void handleTurnOff() {
  isRelayOn = LOW;
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(PIN_RELAY, isRelayOn);
  pixels.setPixelColor(LED_INDEX, pixels.Color(0, 0, 0));
  pixels.show();
  server.send(200, "text/plain", "prave si deaktivoval LED pres wifi pomoci prohlizece");
  Serial.println("");
  Serial.print("nekdo vypnul LED");
}

void handleRelay() {
  int relayNumber = 0;

  for (int i = 0; i < server.args(); i = i + 1) {
    String argumentName = String(server.argName(i));
    String argumentValue = String(server.arg(i));
    Serial.print(String(i) + " ");  //print id
    Serial.print("\"" + argumentName + "\" ");  //print name
    Serial.println("\"" + argumentValue + "\"");  //print value

    if (argumentName == "relayNumber") {
      relayNumber = server.arg(i).toInt();
    }
    if (argumentName == "isOn") {
      isRelayOn = server.arg(i) == "true";
    }
  }

  digitalWrite(PIN_RELAY, isRelayOn ? HIGH : LOW);
  digitalWrite(LED_BUILTIN, isRelayOn ? LOW : HIGH);
  String rele = "prave si rele cislo: " + String(relayNumber);
  String stav = isRelayOn ? " zapnuto" : " vypnuto";
  String zprava =  rele + stav;
  server.send(200, "text/plain", zprava);

  Serial.println("");
  Serial.print("nekdo aktivoval RGB LED");
}

void handleRGB() {
  int r = 0;
  int g = 0;
  int b = 0;
  for (int i = 0; i < server.args(); i=i+1) {
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
  server.send(200, "text/plain", "prave si aktivoval RGB LED pres wifi pomoci prohlizece !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
 
  Serial.println("");
  Serial.print("nekdo aktivoval RGB LED");
}

void handleNotFound() {
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
  server.send(404, "text/plain", message);
}

void setup(void) {
  // zapnuti debug konzole
  Serial.begin(115200);

  // nastaveni LED aby se sni dalo blikat
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
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
  server.on("/relay", handleRelay);
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
    if (digitalRead(PIN_RELAY) == HIGH) {
      isRelayOn = LOW ;

      Serial.println("vypni rele");
    } else {
      isRelayOn = HIGH;

      Serial.println("zapni rele");
    }

    // zapis hodnotu do rele
    digitalWrite(PIN_RELAY , isRelayOn);
  }

  // uloz starou hodnotu na dalsi loop
  buttonState1 = buttonState2;

  // pauza 100ms
  delay(100);
}

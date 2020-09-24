// HTTP server co bezi na svabu za dolar

// include pouzitych knihoven
#include <ESP8266WebServer.h>
#include <WEMOS_SHT3X.h>    // teplomer neni implementovany


// definice pinu
#define PIN_RELAY_1         D8
#define PIN_RELAY_2         D7
#define PIN_RELAY_3         D6
#define PIN_LED             D2
#define PIN_BUTTON          D3
#define WIFI_SSID           "IoT"
#define WIFI_PASSWORD       "qSUpFC3XyLSLabgQ"


//192.168.2.xxx port 80
ESP8266WebServer server(80);


//knihovna na teplomer
SHT3X sht30(0x45);


// stavove promene relatek a tlacitka
boolean isButtonOn  = false;
boolean isRelay1On = false;
boolean isRelay2On = false;
boolean isRelay3On = false;

// stavy tlacitka
int buttonState1 = LOW;
int buttonState2 = LOW;


void setup(void) {
  // zapnuti debug konzole
  Serial.begin(115200);

  // nastaveni LED aby se sni dalo blikat
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  pinMode(PIN_RELAY_3, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // nastavi vsechny rele do vychoziho stavu == vypnuto
  switchAllRelays();

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
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  // zapiname http server
  server.begin();
  Serial.println("HTTP server started");
}


void loop(void) {
  server.handleClient();
  handleButton();

  // pauza 100ms
  delay(100);
}

boolean isRelayOn(int outputPin, String relayId) {
  boolean relayOn = false;

  // tlacitko zmenilo svuj stav ;
  if (isButtonOn == HIGH) {
    relayOn = false;
    Serial.println(String("vypni rele ") + relayId);
  } else {
    relayOn = true;
    Serial.println(String("zapni rele ") + relayId);
  }

  // zapis hodnotu do rele
  digitalWrite(outputPin , relayOn ? HIGH : LOW);

  return relayOn;
}

void handleButton() {
  // nacti novou hodnotu pro tento loop
  buttonState2 = digitalRead(PIN_BUTTON);

  if (buttonState1 == LOW && buttonState2 == HIGH) {

    isRelay1On = isRelayOn(PIN_RELAY_1, "1");
    // uncomment this code if you want to controll all relays by buttons
//    isRelay2On = isRelayOn(PIN_RELAY_2, "2");
//    isRelay3On = isRelayOn(PIN_RELAY_3, "3");

    isButtonOn = !isButtonOn;
    digitalWrite(LED_BUILTIN, isButtonOn ?  LOW : HIGH);
  }

  // uloz starou hodnotu na dalsi loop
  buttonState1 = buttonState2;

}


void handleRoot() {
  Serial.println("");
  Serial.println("nekdo vola root");
  int httpRequestCode = 200; // OK
  String webTitle = String("Muj ESP8266 web server");
  String body = String("<h1>HTTP server funguje!</h1><br>Muzete volat nasledujici <b>endpointy:</b>");
  String zapni = textToAhref("/zapni");
  String vypni = textToAhref("/vypni");
  String relay = textToAhref("/relay?relayNumber=1&isOn=false");
  String dalsi = String("</p> <p>");
  String rootContent = String("<html><head><title>") + webTitle + String("</title></head><body>") + body + String(" <p>") + zapni + dalsi + vypni + dalsi + relay + String("</p> </body></html>");

  server.send(httpRequestCode, "text/html", rootContent);
}


void handleTurnOn() {
  Serial.println("");
  Serial.println("nekdo zapnul LED");
  int httpRequestCode = 200; // OK
  isRelay1On = HIGH;
  isRelay2On = HIGH;
  isRelay3On = HIGH;
  isButtonOn = true;
  switchAllRelays();
  digitalWrite(LED_BUILTIN, LOW);
  server.send(httpRequestCode, "text/plain", "prave si aktivoval LED pres wifi pomoci prohlizece !!!!");
}

void switchAllRelays() {
  digitalWrite(PIN_RELAY_1, isRelay1On);
  digitalWrite(PIN_RELAY_2, isRelay2On);
  digitalWrite(PIN_RELAY_3, isRelay3On);
}


void handleTurnOff() {
  Serial.println("");
  Serial.println("nekdo vypnul LED");
  int httpRequestCode = 200; // OK
  isRelay1On = LOW;
  isRelay2On = LOW;
  isRelay3On = LOW;
  isButtonOn = false;
  switchAllRelays();
  digitalWrite(LED_BUILTIN, HIGH);
  server.send(httpRequestCode, "text/plain", "prave si deaktivoval LED pres wifi pomoci prohlizece");
}


void handleRelay() {
  Serial.println("");
  Serial.println("nekdo aktivoval rele");

  int httpRequestCode = 200; // OK
  int relayNumber = 0;

  boolean isRelayOn = false;

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

  switch (relayNumber) {
    case 1:
      isRelay1On = isRelayOn;
      digitalWrite(PIN_RELAY_1, isRelayOn);
      break;
    case 2:
      isRelay2On = isRelayOn;
      digitalWrite(PIN_RELAY_2, isRelayOn);
      break;
    case 3:
      isRelay3On = isRelayOn;
      digitalWrite(PIN_RELAY_3, isRelayOn);
      break;
  }

  if (isRelay1On || isRelay2On || isRelay3On) {
    isButtonOn = true;
  } else {
    isButtonOn = false;
  }

  digitalWrite(LED_BUILTIN, isButtonOn ? LOW : HIGH);
  String rele = "prave si rele cislo: " + String(relayNumber);
  String stav = isRelayOn ? " zapnuto" : " vypnuto";
  String zprava =  rele + stav;
  server.send(httpRequestCode, "text/plain", zprava);
}


void handleNotFound() {
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


String textToAhref(String text) {
  String endpointText = ip2Str(WiFi.localIP()) + text;
  String ahref = String("<a href=\"") + text + String("\">") + endpointText + String("</a>");
  Serial.println(String("Endpoint: ") + endpointText);
  return ahref;
}


String ip2Str(IPAddress ip) {
  String s = "";
  for (int i = 0; i < 4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

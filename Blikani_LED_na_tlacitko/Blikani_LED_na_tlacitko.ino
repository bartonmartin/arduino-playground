//Blikame s LED

// pevne definovane a nemene konstaty pouzite v programu
// LED_BUILTIN je vlastne konstanta, kterou clovek nemusi definovat a uz je "predprogramovana" v cipu
#define PIN_LED        LED_BUILTIN  // pouzivame vestavenou LED diodu na desce, cislo pinu RGB LED (D2)
#define PIN_BUTTON     D3          // cislo pinu tlacitka (D3)


// globalni promene, ktere jsou videt ze vsech funkci
// int = cislo
// LOW a HIGH jsou podobne jako LED_BUILTIN preddefinovane konstanty (LOW == 0 a HIGH == 1)
int buttonStateLastLoop = HIGH;    // hodnota jakou melo tlacitko v predchozim loop (HIGH = nahore, LOW = dole)
int buttonStateNow = HIGH;         // hodnota jakou ma tlacitko tento loop
int ledState = LOW ;             // hodnota jaka je nastavena pro LED (HIGH = sviti, LOW = nesviti)


// setup se vola jenom jednou kdyz se cip zapina
void setup()
{
  // pinMode(cisloPortu, vstupNeboVystup) se musi volat pro kazdy port ktery chceme pouzit
  pinMode(PIN_LED, OUTPUT);       // PIN_LED je vystup ktery nastavujeme na 0 nebo 1 (zapnuto, vypnuto)
  pinMode(PIN_BUTTON, INPUT);     // PIN_BUTTON je vstup, ktery je nastaveny na 1 kdyz je tlacitko nahore, a na 0 kdyz je dole

  // zapnuti serial logu
  Serial.begin(9600);             // stejne cislo se musi zadat do Serial Monitoru
  Serial.println("");             // vypise prazdny radek
  Serial.println("");             // vypise prazdny radek
  Serial.print("start");          // vypise "start"
  Serial.print(" programu");      // vypise " programu" na stejny radek jako "start"
  Serial.println("");             // vypise prazdny radek
  Serial.println("");             // vypise prazdny radek
  // v serial monitoru by ted melo byt "start programu"
}


// loop se vola porad dokola, dokud je cip zaply
void loop()
{
  // vypiseme si do logu start loopu
  Serial.println("start loop");


  // nacti novou hodnotu pro tento loop
  buttonStateNow = digitalRead(PIN_BUTTON);


  // budeme sledovat zmenu kdy tlacitko bylo predchozi loop zmackle a tento loop uz je pustene (click)
  if (buttonStateLastLoop == LOW && buttonStateNow == HIGH) {

    // tlacitko zmenilo svuj stav a chceme zmenit hodnotu ulozenou v promene ledState
    if (digitalRead(PIN_LED) == HIGH) {
      ledState = LOW ;
      Serial.println("********************************");
      Serial.println("LED je vypla a chceme ji zapnout");
      Serial.println("********************************");
    } else {
      ledState = HIGH;
      Serial.println("================================");
      Serial.println("LED je zapla a chceme ji vypnout");
      Serial.println("================================");
    }

    // zapise hodnotu promene "ledState" na PIN_LED
    digitalWrite(PIN_LED , ledState);
  }
  

  // uloz starou hodnotu na dalsi loop
  buttonStateLastLoop = buttonStateNow;


  // pauza 100ms a zacina dalsi loop
  delay(100);


  // konec loopu vypiseme do logu pomoci nasi vlastni funkce
  writeTextToSerialMonitor("konec loop");
}


// nase funkce ma jeden parametr typu String, ve kterem muzeme predat text z loopu do nasi funkce
void writeTextToSerialMonitor(String text) {

  // uvnitr funkce text vypiseme do logu
  Serial.println(text);
}

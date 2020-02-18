#include <SPI.h>
#include <MFRC522.h>
//#include <Wire.h>
#include <LiquidCrystal_I2C.h>



//MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x3F, 20, 4); // set the LCD address to 0x3F for a 16 chars and 2 line display

int ButtonPin = 2; //Eventuell mit Interrupt

int LED_Yellow    = 4;
int LED_Red       = 5;
int LED_Green     = 6;

int BuzzerPin     = 7;
int LockPin       = 8;

#define     RST_PIN 9
#define     SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);

long   CardID      = 0;
bool   LockState   = 0;

String lastState   = "";

int minTime    = 1500; //Minimale Wartezeit zwischen ver- und entriegeln des Schranks in Millisekunden
int unlocktime = 5000; //Zeit die das Relais nach dem Öffnen anzieht

long long Time1;


long Whitelist[]  = {331260, 2312360};
String KnownUsers[] = {".Mandorf", "2234520.Stoll", ".Fecke", "572260.Keßler", "331260.Hugger"};

void setup()
{

  //Hier wird die Startbotschaft kurz auf dem Display angezeigt
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Herzlich Willkommen!");
  lcd.setCursor(5, 2);
  lcd.print("SILOCK 1.3");
  delay(1000);
  //lcd.clear();

  //Die Kommunikation mit dem RIFD Sensor wird aufgebaut
  Serial.begin(9600); //Die Baudrate von 9600 Einheiten pro Sekunde wird eingestellt 
  SPI.begin();
  mfrc522.PCD_Init(); //Der RIFD Sensor wird initialisiert

  //Die Ausgangspins werden deklariert
  pinMode (LED_Red, OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Green, OUTPUT);        // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Yellow, OUTPUT);       // Der LEDPin ist jetzt ein Ausgang
  pinMode (BuzzerPin, OUTPUT);        // Der BuzzerPin ist jetzt ein Ausgang
  pinMode (LockPin, OUTPUT);          // Der LockPin ist jetzt ein Ausgang
  pinMode (ButtonPin, INPUT);         // Der ButtonPin ist jetzt ein Eingang

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SILOCK 1.3");
  lcd.setCursor(0, 2);
  lcd.print("Schrank offen");

  LockState = true;
}

void loop()
{

  if ( ! mfrc522.PICC_IsNewCardPresent()) { //Überprüft, ob eine Karte vor auf dem Scanner liegt
    lastState = "noCard";
    //Serial.println("keine Karte vorhanden");
    return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Überprüft, ob eine lesbare Karte vor auf dem scanner liegt
    Serial.println("Karte kk");
    return;
  }

  long CardActual = 0;

  if (!CardID)
  {
    lcd.clear();
    CardID = scan();
    Serial.println("Karte eingespeichert");
    lastState = "CardSaved";
    Time1 = millis();

    //Buzzer
    tone(BuzzerPin, 2000, 500);
    
    //LED Indikatoren
    tone(BuzzerPin, 2000, 500);
    digitalWrite(LED_Yellow, HIGH);
    delay(750);
    digitalWrite(LED_Red, HIGH);
    delay(250);
    digitalWrite(LED_Yellow, LOW);
    digitalWrite(LED_Red, LOW);

    //LCD Ausgabe
    lcd.setCursor(0, 0);
    lcd.print("SILOCK 1.3");

    lcd.setCursor(0, 1);
    lcd.print("Schrank verschlossen");
    LockState = false;

    lcd.setCursor(0, 2);
    lcd.print("ID:");

    //String Name = Namecheck();

    lcd.setCursor(0, 3);
    //lcd.print(""+Name);

  }

  else
  {
    CardActual = scan();
    long long passed = millis() - Time1;

    if (CardActual == CardID && lastState == "noCard" && minTime <= passed )
    {
      unlock();
      Serial.println("Korrekte Karte");
      CardID = 0;
      Serial.println("Karte gelöscht");
      delay(1000);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.setCursor(10, 0);
      lcd.print("SILOCK 1.3");
      lcd.setCursor(0, 2);
      lcd.print("Schrank offen");
    }

    else if (CardActual != CardID)
    {
      audioFeedback(false);
    }
  }

  long code = 0;
  
  while(digitalRead(ButtonPin))
    digitalWrite(LockPin, HIGH);

  digitalWrite(LockPin, LOW);
} //Main Loop beendet

String Namecheck()
{
  
}

void unlock()
{
  //bool signal = in;
  digitalWrite (LockPin, HIGH);
  audioFeedback(true);
  delay(1000);
  digitalWrite (LockPin, LOW);
}

void OpenButton()
{
  
}

long scan()
{
  long k = 0;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    k = ((k + mfrc522.uid.uidByte[i]) * 10);
  }
  Serial.print("Die eingelesene Kartennummer lautet:");
  Serial.println(k);
  return k;
}

void audiovisualFeedback(String status)
{
   String currentstate=status;
   
}

void audioFeedback(bool status)
{
  bool code = status;
  if (code) //Wenn die richtige Karte aufgelegt ist
  {
    tone(BuzzerPin, 2000, 100);
    digitalWrite (LED_Green, HIGH);
    delay (200);
    tone(BuzzerPin, 2000, 100);
    digitalWrite (LED_Green, LOW);
    delay (1000);
  }

  else //Wenn die falsche Karte aufgelegt ist
  {
    digitalWrite (LED_Red, HIGH);
    //delay(500);
    tone(BuzzerPin, 3000, 100);
    delay(150);
    tone(BuzzerPin, 3000, 100);
    delay(150);
    tone(BuzzerPin, 3000, 100);
    delay(500);
    digitalWrite (LED_Red, LOW);
    Serial.println("Falsche Karte");
  }
}

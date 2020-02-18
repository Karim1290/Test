#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
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
String text = "";

int minTime    = 1500; //Minimale Wartezeit zwischen ver- und entriegeln des Schranks in Millisekunden
int unlocktime = 5000; //Zeit die das Relais nach dem Öffnen anzieht

long long TimeA;

long CardActual = 0;
long code = 0;

long Whitelist[]  = {331260, 2312360};
String KnownUsers[] = {".Mandorf", "2234520.Stoll", ".Fecke", "572260.Keßler", "331260.Hugger"};
char Trennzeichen = ".";


void setup()
{

  //Hier wird die Startbotschaft kurz auf dem Display angezeigt
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Herzlich Willkommen!");
  lcd.setCursor(5, 2);
  lcd.print("SILOCK 1.4");
  delay(1000);
  //lcd.clear();

  //Die Kommunikation mit dem RIFD Sensor wird aufgebaut
  Serial.begin(9600); //Die Baudrate von 9600 Einheiten pro Sekunde wird eingestellt
  SPI.begin();
  mfrc522.PCD_Init(); //Der RIFD Sensor wird initialisiert

  //Die Ausgangspins werden deklariert
  pinMode (LED_Red,     OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Green,   OUTPUT);        // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Yellow,  OUTPUT);       // Der LEDPin ist jetzt ein Ausgang
  pinMode (BuzzerPin,   OUTPUT);        // Der BuzzerPin ist jetzt ein Ausgang
  pinMode (LockPin,     OUTPUT);          // Der LockPin ist jetzt ein Ausgang
  pinMode (ButtonPin,   INPUT);         // Der ButtonPin ist jetzt ein Eingang

  VisualFeedback("entsperrt");

  LockState = true;
}

void loop()
{

  if( ! mfrc522.PICC_IsNewCardPresent()) { //Überprüft, ob eine Karte vor auf dem Scanner liegt
    lastState = "noCard";
    return;
  }

  if( ! mfrc522.PICC_ReadCardSerial()) {   //Überprüft, ob eine lesbare Karte vor auf dem Scanner liegt
    Serial.println("Karte kk");
    return;
  }

  if (!CardID)
  {
    lcd.clear();
    CardID = scan();
    Serial.println("Karte eingespeichert");
    lastState = "CardSaved";
    TimeA = millis();

    //Buzzer
    tone(BuzzerPin, 2000, 1000);

    LockState = false;
    VisualFeedback("versperrt");
  }

  else{
    CardActual = scan();
    long long passed = millis() - TimeA;

    if (CardActual == CardID && lastState == "noCard" && minTime <= passed )
    {
      audioFeedback(true);
      unlock();
      Serial.println("Korrekte Karte");
      CardID = 0;
      Serial.println("Karte gelöscht");
      delay(1000);
      
      VisualFeedback("entsperrt");
    }

    else if (CardActual != CardID){
      audioFeedback(false);
    }
  }

  while (digitalRead(ButtonPin))
    digitalWrite(LockPin, HIGH);

  digitalWrite(LockPin, LOW);
} //Main Loop beendet

String Namecheck(long CardID)
{
  
  String Name;

  for (int i = 0; i < 5 ; i++) 
  {
    text = KnownUsers[i];
    int a = text.indexOf(".", 0);
    
    String PredefinedName = text.substring(a + 1);
    String PredefinedCardID = text.substring(0,a);
  
    if(PredefinedCardID==String(CardID))
    {
      Name=PredefinedName;
      Serial.println("User recognised");
    }
      
    else
    {
      Name="Gast";
      Serial.println("Guest recognised");
    }
  }
  
  return Name;  
}

void unlock()
{
  //while(ButtonPin){
  digitalWrite (LockPin, HIGH);
  digitalWrite(LED_Green, HIGH);
  //}
  //audioFeedback(true);
  digitalWrite(LockPin, LOW);
  digitalWrite(LED_Green, LOW);
}

long scan()
{
  long Kartennummer = 0;
  for (byte i = 0; i < mfrc522.uid.size; i++){
    Kartennummer = ((Kartennummer + mfrc522.uid.uidByte[i]) * 10);
  }
  Serial.print("Die eingelesene Kartennummer lautet:");
  Serial.print(Kartennummer);
  Serial.println(".");
  return Kartennummer;
}

void VisualFeedback(String Operation)
{
  //Serial.println("Feedback Methode reached");
  if(Operation=="versperrt"){

    //LED Ausgabe
    digitalWrite(LED_Red,HIGH);
    digitalWrite(LED_Yellow,LOW);
    digitalWrite(LED_Green,LOW);

    //LCD Ausgabe
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SILOCK 1.4");

    lcd.setCursor(0, 1);
    lcd.print("-Schrank verriegelt-");

    lcd.setCursor(0, 2);
    lcd.print("User ID: ");
    
    lcd.setCursor(0, 3);
    lcd.print(Namecheck(CardID));
    
  }
  else if(Operation=="entsperrt"){
    //LED Ausgabe
    digitalWrite(LED_Yellow,HIGH);
    digitalWrite(LED_Red,LOW);
    digitalWrite(LED_Green,LOW);

    //LCD Ausgabe
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("-Schrank entriegelt-");

    lcd.setCursor(0, 2);
    lcd.print("Druecken Sie den");

    lcd.setCursor(0, 3);
    lcd.print("Taster zum oeffnen");
    
  }
  else if(Operation=="geöffnet"){
    //LED Ausgabe
    digitalWrite(LED_Green,HIGH);
    digitalWrite(LED_Yellow,LOW);
    digitalWrite(LED_Red,LOW);

  }
  else{
  Serial.println("Error 1337");  
  lcd.clear();
  lcd.print("Error 1337");
  }
}

void Feedback(String status)
{
  String currentstate = status;

}

void audioFeedback(bool status)
{
  bool code = status;
  if (code) //Wenn die richtige Karte aufgelegt ist
  {
    tone(BuzzerPin, 2000, 100);
    delay (200);
    tone(BuzzerPin, 2000, 100);
    delay (1000);  
  }

  else //Wenn die falsche Karte aufgelegt ist
  {
    tone(BuzzerPin, 3000, 100);
    delay(150);
    tone(BuzzerPin, 3000, 100);
    delay(150);
    tone(BuzzerPin, 3000, 100);
    delay(500);
    
    Serial.println("Falsche Karte");
  }
}

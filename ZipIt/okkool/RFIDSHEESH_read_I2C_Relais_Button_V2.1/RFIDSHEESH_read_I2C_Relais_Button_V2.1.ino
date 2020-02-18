#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x3F, 20, 4); //Legt die Adresse für ein LCD mit 20 Zeichen und 4 Zeilen fest 0x3F fest 
// set the LCD address to 0x3F for a 20 chars and 4 line display

int ButtonPin     = 2;    

int LED_Yellow    = 4;
int LED_Red       = 5;
int LED_Green     = 6;

int BuzzerPin     = 7;
int RelaisPin     = 8;

#define     RST_PIN 9
#define     SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);

long CardID       = 0;
long CardActual   = 0;
long code         = 0;

long long TimeA;

bool LockState    = 0;
bool AdminState   = 0;

String lastState = "";
String text      = "";

int minTime    = 1500;  //Minimale Wartezeit zwischen ver- und entriegeln des Schranks in Millisekunden
int unlocktime = 5000; //Zeit die das Relais nach dem Öffnen anzieht in Millisekunden
int Number_Admins  = 2; //Anzahl der registrierten Admins
int Number_Users   = 4; //Anzahl der namentlich bekannten User


long Adminlist[]    = {331260, 2312360, 2234520};
String KnownUsers[] = {"2461320.Mandorf", "2234520.Stoll", "572260.Keßler", "331260.Hugger"};

//String KnownUsers[] = {"2461320.Mandorf.Admin", "2234520.Stoll.Admin", "572260.Keßler", "331260.Hugger.Admin"};

char Trennzeichen   = '.';


void setup()
{
  //Hier wird die Startbotschaft für eine Sekunde auf dem Display angezeigt
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Herzlich Willkommen!");
  lcd.setCursor(5, 2);
  lcd.print("SILOCK 2.1");            //Produktnamen und aktuellen Version
  delay(1000);

  //Nun werden einige Infos zum aktuellen Programmablauf eingeblendet:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Entsperrzeitraum: ");
  lcd.setCursor(0, 1);
  lcd.print(unlocktime / 1000);

  /*
   * In der folgenden If-Abfrage wird erkannt, ob die Sperrzeit ein oder zweistellig ist.
   * Abhängig davon wird die "Sekunde" Displayausgabe um eine Stelle auf dem Display verschoben
   */
  if ((unlocktime / 1000) < 10)
    lcd.setCursor(1, 1);
  else
    lcd.setCursor(2, 1);
    
  lcd.print(" Sekunden");

  delay(2000);

  //Die Kommunikation mit dem RIFD Sensor wird aufgebaut
  Serial.begin(9600); //Die Baudrate von 9600 Einheiten pro Sekunde wird eingestellt
  SPI.begin();
  mfrc522.PCD_Init(); //Der RIFD Sensor wird initialisiert

  //Die Ausgangspins werden deklariert
  pinMode (LED_Red,     OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Green,   OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Yellow,  OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (BuzzerPin,   OUTPUT);          // Der BuzzerPin ist jetzt ein Ausgang
  pinMode (RelaisPin,   OUTPUT);          // Der RelaisPin ist jetzt ein Ausgang

  //Der Pin an dem der Button angeschlossen wird wird als Eingang definiert
  pinMode (ButtonPin,  INPUT);

  //Nach dem Neustart wird der Schrank entsperrt
  VisualFeedback("entsperrt");
  LockState = true;
}

void loop()
{
  AdminState = AdminlistCheck();

  if (!mfrc522.PICC_IsNewCardPresent()) { //Überprüft, ob eine Karte vor auf dem Scanner liegt
    lastState = "noCard";

    if (LockState && digitalRead(ButtonPin))
    {
      Serial.println("Tür kann geöffnet werden");
      unlock();
    }
    else if (!LockState && digitalRead(ButtonPin) && AdminState)
    {
      Serial.println("Admin-Unlock erfolgreich");

      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Adminrecht eingesetzt");
      
      delay(1000);
      unlock();

      !AdminState;
      CardID = 0;
      LockState = true;

      //Zum debuggen:
      Serial.println("Die Tür wurde per Admin-Recht entsperrt!");
    }
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {   //Überprüft, ob eine lesbare Karte vor auf dem scanner liegt
    Serial.println("Karte lesbar");
    return;
  }

  if (!CardID)
  {
    CardID = scan();
    lastState = "CardSaved";
    TimeA = millis();
    LockState = false;

    //Schalten der Anzeigeelemente
    VisualFeedback("versperrt");
    Serial.println("Tür gesperrt!");
  }
  else
  {
    CardActual = scan();
    long long passed = millis() - TimeA;

    if (CardActual == CardID && lastState == "noCard" && minTime <= passed )
    {
      CardID = 0;
      LockState = true;

      audioFeedback(true);
      VisualFeedback("entsperrt");

      delay(1000);

      //Zum debuggen:
      Serial.println("Tür entsperrt");
    }

    else if (CardActual != CardID)
    {
      audioFeedback(false);
    }
  }
} //Main Loop beendet

/*  In dieser Methode wird überprüft, ob es sich bei der Vorgehaltenen karte um einen Admin handelt.
    Wenn dies der Fall ist kann der Nutzer die auch dann öffnen wenn sie durch einen anderen Nutzer
    gesperrt wurde.
*/
bool AdminlistCheck()
{
  long scanValue = scan();              //Die ID der aktuell vorgehaltenen Karte wird in eine Variable gespeichert
  
  bool value = 0;                       //Der Rückgabewert ist von Beginn an asl false
  for (int i = 0; i < Number_Admins ; i++)
  {
    if (Adminlist[i] == scanValue)
      value = 1;
  }
  return value;

  //Zum debuggen
  Serial.println("AdminlistCheck erreicht");
}

/*  Dieser Methode wird überprüft, ob mit der CardID ein Name verknüpft ist.
    Falls dies der Fall ist wird dieser Name im gesperrten Zustand auf dem Display angezeigt,
    wenn nicht wird der aktuelle Nutzer als "Gast" angezeigt
*/
String Namecheck(long CardID)
{
  String Name;

  for (int i = 0; i < Number_Users ; i++)
  {
    text = KnownUsers[i];
    int a = text.indexOf(".", 0);

    String PredefinedName   = text.substring(a +1);
    String PredefinedCardID = text.substring(0, a);

    if (PredefinedCardID == String(CardID)) {
      Name = PredefinedName;

      //Debugging:
      Serial.println("Name:"+Name);
      Serial.println("CardID"+PredefinedCardID);
      
    }
    else {
      Name = "Gast";
      //Serial.println("Guest recognised");
    }
  }

  Serial.println(Name);
  return Name;
}

//Diese Methode übernimmt den Teil des eigentlichen öffnens.
void unlock()
{
  VisualFeedback("geöffnet");     //Die LCD-Anzeige und die LEDs werden besteuert

  digitalWrite(RelaisPin, HIGH);  //Das Relais wird geschaltet

  //Zeitanimation für Relais
  int Schrittweite = unlocktime / 20;
  int k = 0;

  while (k < 20)
  {
    lcd.setCursor(k, 2);
    lcd.print("*");
    lcd.setCursor(k, 3);
    lcd.print("*");
    delay(Schrittweite);
    k++;
  }

  digitalWrite(RelaisPin, LOW);

  VisualFeedback("entsperrt");    //Die LCD-Anzeige und die LEDs werden besteuert
}

/*  In der folgenden Methode wird die Zahl, welcher auf der Karte hinterlegt ist,
    ausgelesen und für die weitere Nutzung durch das Programm weitergegeben
*/
long scan()
{
  long Kartennummer = 0;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Kartennummer = ((Kartennummer + mfrc522.uid.uidByte[i]) * 10);
  }
  Serial.print(Kartennummer);
  Serial.println(".");

  return Kartennummer;
}

void VisualFeedback(String Operation)
{
  if      (Operation == "versperrt")
  {
    //LED Ausgabe
    digitalWrite(LED_Red, HIGH);
    digitalWrite(LED_Yellow, LOW);
    digitalWrite(LED_Green, LOW);

    //LCD Ausgabe
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("");

    lcd.setCursor(0, 0);
    lcd.print("-Schrank verriegelt-");

    lcd.setCursor(0, 2);
    lcd.print("User ID: ");

    lcd.setCursor(0, 3);
    lcd.print(Namecheck(CardID));

    //Buzzer
    tone(BuzzerPin, 2000, 1000);
  }
  else if (Operation == "entsperrt")
  {
    //LED Ausgabe
    digitalWrite(LED_Yellow, HIGH);
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, LOW);

    //LCD Ausgabe
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("-Schrank entriegelt-");

    lcd.setCursor(0, 2);
    lcd.print("Druecken Sie den");

    lcd.setCursor(0, 3);
    lcd.print("Taster zum oeffnen");

    //Buzzer
    //audioFeedback(true);
  }
  else if (Operation == "geöffnet" )
  {
    //LED Ausgabe
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Yellow, LOW);
    digitalWrite(LED_Red, LOW);

    //LCD Ausgabe
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("-Schrank geoeffnet-");
  }
}

void audioFeedback(bool status)
{
  if (status) //Wenn die richtige Karte aufgelegt ist
  {
    tone(BuzzerPin, 2000, 100);
    delay (200);
    tone(BuzzerPin, 2000, 100);
    delay (1000);

    Serial.println("Richtige Karte");
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

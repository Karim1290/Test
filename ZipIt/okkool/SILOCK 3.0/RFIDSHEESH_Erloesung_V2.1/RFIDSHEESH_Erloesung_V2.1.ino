//Alle über die Serielle Schnittstelle ausgegebenen Informationen sind nur yum debuggen und erweitern des Programms gedacht.
//Sie werden nicht für den Endnutzer zur Verfügung stehen.

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 20, 4); //Legt die Adresse für ein LCD mit 20 Zeichen und 4 Zeilen auf 0x3F//0x27 fest

int ButtonPin     = 2;
int LED_Yellow    = 4;
int LED_Red       = 5;
int LED_Green     = 6;

int BuzzerPin     = 7;
int RelaisPin     = 8;

#define     RST_PIN 9
#define     SS_PIN 10

MFRC522 RFID(SS_PIN, RST_PIN);

long CardID       = 0;
long CardActual   = 0;
long code         = 0;

long long TimeA;

bool LockState    = 0;
bool AdminState   = 0;

String lastState = "";
String text      = "";

int minTime    = 1500; //Minimale Wartezeit zwischen ver- und entriegeln des Schranks in Millisekunden
int unlocktime = 5000; //Zeit die das Relais nach dem Öffnen anzieht in Millisekunden
int Number_Admins  =4; //Anzahl der registrierten Admins
int Number_Users   =7; //Anzahl der namentlich bekannten User

long Adminlist[]    = {331260, 2312360, 2234520, 188619590};
String KnownUsers[] = {"2461320.Mandorf", "2234520.Stoll", "79310780.Rammstein" , "188619590.Admincard" , "331260.Hugger", "572260.Kessler" , "1040850.van der Will"};
char Trennzeichen   = '.';

//Hier werden zwei Icons, ein offenes und ein verschlossenes Schloss, zur Verwendung im weiteren Programmablauf per Bitmuster generiert
byte openLock[8] = {   
                0b01110, 
                0b10001, 
                0b10001, 
                0b10000, 
                0b11111, 
                0b11111, 
                0b11111, 
                0b11111    };

byte closedLock[8] = {   
                0b01110, 
                0b10001, 
                0b10001, 
                0b10001, 
                0b11111, 
                0b11111, 
                0b11111, 
                0b11111    };



void setup()
{
  //lcd.createChar(0, openLock);   //das Byte-Array für das verschlossene Schloss in den LCD-Speicher 0 schreiben
  //lcd.createChar(1, closedLock); //das Byte-Array für das offene Schloss in den LCD-Speicher 1 schreiben

  
  //Die Kommunikation mit dem RIFD Sensor wird aufgebaut
  Serial.begin(9600);  //Die Baudrate von 9600 Einheiten pro Sekunde wird eingestellt
  SPI.begin();         //Die zur Kommunikation mit dem RFID Sensor nötige SPI Kommunikation wird initialisiert.
  RFID.PCD_Init();      //Der RFID Sensor wird initialisiert

  //Die Ausgangspins werden deklariert
  pinMode (LED_Red,     OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Green,   OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (LED_Yellow,  OUTPUT);          // Der LEDPin ist jetzt ein Ausgang
  pinMode (BuzzerPin,   OUTPUT);          // Der BuzzerPin ist jetzt ein Ausgang
  pinMode (RelaisPin,   OUTPUT);          // Der RelaisPin ist jetzt ein Ausgang

  //Der Pin an dem der Button angeschlossen wird wird als Eingang definiert
  pinMode (ButtonPin,  INPUT);
  
  //Hier wird die Startbotschaft für eine Sekunde auf dem Display angezeigt
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Herzlich Willkommen!");
  lcd.setCursor(5, 2);
  lcd.print("SILOCK 3.0");            //Produktnamen und aktuellen Version
  delay(1000);

  //Nun werden einige Infos zum aktuellen Programmablauf eingeblendet:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Entsperrzeitraum: ");
  lcd.setCursor(0, 1);
  lcd.print(unlocktime / 1000); //Da die delay() Methode mit Werten im ms-Bereich bekommt müssen diese Werte für die Anzeige umgerechnet werden
  /*
     In der folgenden If-Abfrage wird erkannt, ob die Sperrzeit ein oder zweistellig ist.
     Abhängig davon wird die "Sekunde" Displayausgabe um eine Stelle auf dem Display verschoben
  */
  if ((unlocktime / 1000) < 10)
    lcd.setCursor(1, 1);
  else
    lcd.setCursor(2, 1);

  lcd.print(" Sekunden");

  delay(2000);

  lcd.createChar(0, openLock);   //das Byte-Array für das verschlossene Schloss in den LCD-Speicher 0 schreiben
  lcd.createChar(1, closedLock); //das Byte-Array für das offene Schloss in den LCD-Speicher 1 schreiben


  //Nach dem Neustart wird der Schrank entsperrt
  VisualFeedback("entsperrt");
  LockState = true;

  //Kurzes Tutorial zum nutzen des Schranks

  
}

void loop()
{
  long newCardscan = scan();
  
  AdminState = AdminlistCheck(newCardscan);
  if (!RFID.PICC_IsNewCardPresent()){     //Überprüft, ob eine Karte vor auf dem Scanner liegt
    lastState = "noCard";                 //Hinterlegt die Information, das während dem letzten durchlaufs keine neue Karte aufgelegt war

    //zum debuggen
    Serial.println("keine neue Karte vor Scanner");

    if (LockState && digitalRead(ButtonPin))
    {
      Serial.println("Tür kann geöffnet werden");
      unlock();
    }
    else if (!LockState && digitalRead(ButtonPin) && newCardscan!=CardID && AdminState)
    {

      Serial.println("Admin-Unlock erfolgreich");

      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Adminrecht aktiviert");

      delay(1000);
      unlock();

      AdminState = !AdminState;
      CardID = 0;
      LockState = true;

      //Zum debuggen:
      Serial.println("Die Tür wurde per Admin-Recht entsperrt!");
    }
    //Um die Luafzeit des Programms kurz zu halten wird die Hauptschleife an dieser Stelle neugestartet.
    return;
  }

  if (!RFID.PICC_ReadCardSerial()) {   //Überprüft, ob eine lesbare Karte vor auf dem Scanner liegt
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
    //CardID=0;
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
      unlock();
      
      //Zum debuggen:
      Serial.println("Tür entsperrt");
    }

    else if (CardActual != CardID)
    {
      audioFeedback(false);     
    }
  }
} //Main Loop beendet

/*  In dieser Methode wird überprüft, ob die vorgewhaltene Karte mit einem Admin-Nutzer verknüpft ist.
    Wenn dies der Fall ist kann der Nutzer die auch dann öffnen wenn sie durch einen anderen Nutzer
    gesperrt wurde.
*/
bool AdminlistCheck(long i)
{
  long scanValue = i;                       //Die ID der aktuell vorgehaltenen Karte wird in eine Variable gespeichert
  bool value = false;                       //Der Rückgabewert ist zu Beginn als nicht wahr festgelegt
  
  for (int i = 0; i < Number_Admins ; i++)  //Das Array in dem die Admins hinterlegt sind wird Eintrag für Eintrag auf die eingescannte ID0000000 überprüft 
  {
    if (Adminlist[i] == scanValue)          //Wenn die ID mit einerm Admin-Nutzer verknüpft ist, wird von dieser Methode ein "true" zurückgegeben
      value = true;
  }

  return value;
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
    int a = text.indexOf(Trennzeichen, 0);

    String PredefinedName   = text.substring(a + 1);
    String PredefinedCardID = text.substring(0, a);

    if (PredefinedCardID == String(CardID)) {
      Name = PredefinedName;

      //Debugging:
      Serial.println("Name:"  + Name);
      Serial.println("CardID" + PredefinedCardID);
      break;
    }
    else {
      Name = "Gast";
      //Serial.println("Guest recognised");
    }
  }
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

  //Anhand der fgolgenden Schleife wird ein Ladebalken angezeigt, welcher die
  while (k < 20)
  {
    lcd.setCursor(k, 2);
    lcd.print("\377");
    lcd.setCursor(k, 3);
    lcd.print("\377");
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
  for (byte i = 0; i < RFID.uid.size; i++) {
    Kartennummer = ((Kartennummer + RFID.uid.uidByte[i]) * 10);
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
    lcd.write(((uint8_t)1));
    
    lcd.setCursor(1, 0);
    lcd.print("Schrank verriegelt");

    lcd.setCursor(19, 0);
    lcd.write(((uint8_t)1));
    
    lcd.setCursor(0, 2);
    lcd.print("Aktueller Nutzer:");

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
    lcd.write(((uint8_t)0));
    lcd.setCursor(1, 0);
    lcd.print("Schrank entriegelt");
    lcd.setCursor(19, 0);
    lcd.write((uint8_t)0);

    lcd.setCursor(0, 2);
    lcd.print("  Dr\365cken Sie den");

    lcd.setCursor(0, 3);
    lcd.print(" Taster zum \357ffnen");

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
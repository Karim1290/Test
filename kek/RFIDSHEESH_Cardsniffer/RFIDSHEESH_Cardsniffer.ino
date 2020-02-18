//Alle über die Serielle Schnittstelle ausgegebenen Informationen sind lediglich zum Debuggen und Erweitern des Programms gedacht.
//Sie werden nicht für den Endnutzer zur Verfügung stehen.

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); //Legt die Adresse für ein LCD mit 20 Zeichen und 4 Zeilen auf 0x3F//0x27 fest

#define     RST_PIN 9
#define     SS_PIN 10

MFRC522 RFID(SS_PIN, RST_PIN);

long CardID       = 0;

void setup()
{
  //lcd.createChar(0, openLock);   //das Byte-Array für das verschlossene Schloss in den LCD-Speicher 0 schreiben
  //lcd.createChar(1, closedLock); //das Byte-Array für das offene Schloss in den LCD-Speicher 1 schreiben


  //Die Kommunikation mit dem RIFD Sensor wird aufgebaut
  Serial.begin(9600);  //Die Baudrate von 9600 Einheiten pro Sekunde wird eingestellt
  SPI.begin();         //Die zur Kommunikation mit dem RFID Sensor nötige SPI Kommunikation wird initialisiert.
  RFID.PCD_Init();     //Der RFID Sensor wird initialisiert

  //Hier wird die Startbotschaft für eine Sekunde auf dem Display angezeigt
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
}

void loop()
{
  long newCardscan = scan();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Seriennummer:");
      lcd.setCursor(0, 1);
      lcd.print(scan());
} 

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

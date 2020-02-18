String KnownUsers[] = {".Mandorf", "2234520.Stoll", ".Fecke", "572260.Keßler", "331260.Hugger"};

String text = "";

void setup() {
  Serial.begin(9600);

  int x = sizeof(KnownUsers);

  //for (int counter = 0; counter < x && text.indexOf(".") >= 0; counter++) {
  for (int i = 0; i < 5 ; i++) {

    text = KnownUsers[i];
    int a = text.indexOf(".", 0);
    
    String Name = text.substring(a + 1);
    String CardID = text.substring(0,a);

    Serial.println("Array_Name:"+Name+".");
    Serial.println("Array_Card ID:"+CardID+".");
    Serial.println("");

    Serial.print("Verglichene CardID:");
    Serial.println(CardID);
      
    if(PredefinedCardID.equals(CardID)){
      Name=PredefinedName;
      Serial.println("User recognised");
    }
      
    else{
      Name="Gast";
      Serial.println("Guest recognised");
    }
  }
}
void loop() {}

void setup() {
  // put your setup code here, to run once:
  pinMode(2, INPUT);
  pinMode(8, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
    while(digitalRead(2))
    digitalWrite(8, HIGH);

  digitalWrite(8, LOW);
}

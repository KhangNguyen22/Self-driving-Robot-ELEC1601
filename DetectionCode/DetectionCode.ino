const int leyePin = A0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(leyePin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int x = analogRead(leyePin);
  if(x == 0){
    Serial.println("YES");
  }
  else{
    Serial.println("NO");
  }
}

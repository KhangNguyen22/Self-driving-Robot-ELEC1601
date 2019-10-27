#include <LiquidCrystal.h>
int contrast = 20;
int contrastPin = 8;
//One for each button
int analogOne = A2;
int analogTwo = A3;
int analogThree = A4;
int analogFour = A5;
// ###                
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  
  Serial.begin(9600); 
  
  // ## OUR OWN SETUP ##
  analogWrite(contrastPin, contrast);
  lcd.begin(16, 2);
  pinMode(analogOne, INPUT);
  pinMode(analogTwo, INPUT);
  pinMode(analogThree, INPUT);
  pinMode(analogFour, INPUT);
  
}

void loop() {
  
    float buttonOne = convert(analogRead(analogOne));
    float buttonTwo = convert(analogRead(analogTwo));
    float buttonThree = convert(analogRead(analogThree));
    float buttonFour = convert(analogRead(analogFour));

        if (isButtonPressed(buttonOne)) {
          Serial.println("buttonOne Is Pressed");
        }
        else if (isButtonPressed(buttonTwo)) {
          Serial.println(1);
        } 
        else if (isButtonPressed(buttonThree)) {
          Serial.println(2);
        } 
        else if (isButtonPressed(buttonFour)) {
          Serial.println(3);
        }
}

float convert(int analogNum) {
  float voltage = analogNum * (5.0/1023.0); //analog -> digital
  return voltage;
}

// ### THIS FUNCTION NEEDS TESTING ASAP! ####
boolean isButtonPressed(float convertedVoltage) {
  if(convertedVoltage >= 4.95) {  //this i am unsure of, needs to be tested
    return true;   
  }
  return false;
}

void show(String lineOne, String lineTwo) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(lineOne);
  lcd.setCursor(0,1);
  lcd.print(lineTwo);
}

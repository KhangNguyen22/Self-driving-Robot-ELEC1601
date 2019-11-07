/*
GROUP T15-13
Slave code
OVERVIEW:
  The circuit consists of an LCD and 4 buttons. Each button is 
  connected to its own analog pin, which is constantly reading 
  for 5V (when the button is pressed). If the reading of an 
  analog pin does reach this threshold, it sends a bluetooth 
  signal (it's different depending on which button was pressed). 
  The program then "waits", to ensure only one signal is sent 
  from the button pressed.
  ---
  The master will also send a signal back when:
  1. It recieves a signal
  2. It has finished moving to its destination
  Depending on the signal receieved, the LCD will display a 
  different message.
  
ASSUMPTIONS:
  1. No simultaneous button pressing.
  2. A button won't be pressed until the the master has finished
  running the code for the last signal sent.
  
CIRCUIT:
  ANALOG PINS:
    Used 2 - 5.
    Each assigned to their own button.
  DIGITAL PINS:
    Contrast pin:
      Determines contrast of LCD.
    Data bus:
      Handles data for LCD.
      
FUNCTIONS:
  boolean isButtonPressed(float convertedVoltage)
    Determines if button was pressed (if the respective pin reads
    a certain threshold). If it does, return true, else return false.
  void sendSignal(char command)
    Send "command" to the bluetooth serial. Then it calls the wait()
    function.
  void wait()
    Makes program wait for a second. This is to ensure when a button
    is pressed, it does not send more than one signal.
    
Version official
Date 03/11/19
Authors: GROUP T15-13   
*/

#include <SoftwareSerial.h>     // Software Serial Port
#include <LiquidCrystal.h>      // Include LCD library
/*
  SET CONTRAST FOR LCD
*/
int contrast = 20;
int contrastPin = 8;
/*
  ASSIGN EACH BUTTON ITS OWN ANALOG PIN
*/
int analogOne = A2;
int analogTwo = A3;
int analogThree = A4;
int analogFour = A5;
/*
  SETUP LCD
*/
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int period = 1000;                  //setup variables for wait() function
unsigned long timeNow = 0;          //called after a button press to ensure multiple signals aren't set from one press

#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

int shieldPairNumber = 13;

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);
/*
void setup()
  Set up what's needed for the bluetooth component
  Set the analog pins for the buttons to INPUT
  Begin the LCD
  Write the contrast to the contrastPin for the LCD
*/
void setup() {
  
  Serial.begin(9600);
  blueToothSerial.begin(38400);  // Set Bluetooth module to default baud rate 38400
    
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  pinMode(ConnStatus, INPUT);
  
  //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
  //  This prevents running the full connection setup routine if not necessary.

  if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

  if(ConnStatusSupported && digitalRead(ConnStatus)==1)
  {
    Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
  }
  else
  {
    Serial.println("Not connected to Master.");
        
    setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

    delay(1000);                  // Wait one second and flush the serial buffers
    Serial.flush();
    blueToothSerial.flush();
  }  
 
  analogWrite(contrastPin, contrast);
  lcd.begin(16, 2);
  //analogPins will receive 5V for it's respective button when pressed
  pinMode(analogOne, INPUT);            
  pinMode(analogTwo, INPUT);
  pinMode(analogThree, INPUT);
  pinMode(analogFour, INPUT);
  
}
/*
void loop()
  Constantly checking if a bluetooth signal is received from master.
    -  If a signal is receieved, show the corresponding message on
       the LCD
  Constantly checking if a button has been pressed
    -  If a button was pressed, send a bluetooth signal to the 
       master (signal unique to which button was pressed)
*/
void loop() {
  
  char recvChar;
  
  while(1)
  {
    if(blueToothSerial.available())   // Check if there's any data sent from the remote Bluetooth shield
    {
      recvChar = blueToothSerial.read();
      if (recvChar == 'a') {
        show("IDLE AT", "KITCHEN");         // Different status to print on LCD depending on signal received
      }                                     // Signal will be received when:
      else if (recvChar == 'b') {           //    1. A button is pressed
        show("IDLE AT", "TABLE 1");         //      -   Sends bluetooth signal to robot to move to a specific location             
            }                               //      -   Recieve bluetooth signal back to show one of the lower 3 status (uppercase)
      else if (recvChar == 'c') {           //    2. Robot reaches destination
        show("IDLE AT", "TABLE 2");         //      -   Robot sends signal back       
      }                                     //      -   Recieve bluetooth signal to show one of the upper 3 status depending where it was moving to, determined by button press before (lowercase)
      else if (recvChar == 'A') {
        show("MOVING TO", "KITCHEN");      
      }
      else if (recvChar == 'B') {
        show("MOVING TO", "TABLE 1");    
      }
      else if (recvChar == 'C') {
        show("MOVING TO", "TABLE 2");    
      }
    }  
    
    //read the analog pins
    int buttonOne = analogRead(analogOne);
    int buttonTwo = analogRead(analogTwo);
    int buttonThree = analogRead(analogThree);
    int buttonFour = analogRead(analogFour);
    
    //check if any button was pressed
    if (isButtonPressed(buttonOne)) {
          sendSignal('1');
        }
    else if (isButtonPressed(buttonTwo)) {
          sendSignal('2');
        } 
    else if (isButtonPressed(buttonThree)) {
          sendSignal('3');
        } 
    else if (isButtonPressed(buttonFour)) {
          sendSignal('4');
        }
    }
}
/*
boolean isButtonPressed(float convertedVoltage)
  Function checks if button was pressed. If a button
  was pressed, its respective analog pin will
  receive 5V. If this occurs, return true, else false
*/
boolean isButtonPressed(float convertedVoltage) {
  if(convertedVoltage >= 500) {        
    return true;                        // If button isn't pressed, convertedVoltage is 0
  }                                     // If button is pressed, convertedVoltage is ~1000
  return false;                         
}

/*
void sendSignal(char command)
  Sends the command to the bluetooth serial which is 
  to read by the master.
  WHICH BUTTON SENDS WHICH SIGNAL
    1: Move to kitchen from table 1
    2: Move to kitchen from table 2
    3: Move to table 1 from kitchen
    4: Move to table 2 from kitchen
*/
void sendSignal(char command) {
  blueToothSerial.println(command);     // Send the signal 
  wait();                               // Call wait() function
}

/*
void show(String lineOne, String lineTwo)
  Displays message on LCD screen.
  The first line displays lineOne.
  Similarly, second line for lineTwo.
  Initially clears message so it does not
  overlap with the previous message.
*/
void show(String lineOne, String lineTwo) {
  lcd.clear();              
  lcd.setCursor(0,0);       // Cursor begins at first line, first index
  lcd.print(lineOne);       // Displays given text
  lcd.setCursor(0,1);
  lcd.print(lineTwo);
}

/*
void wait()
  PURPOSE: Called when a button is pressed to ensure
  only one signal is sent on a button pressed. 
  It makes the program wait until a second has passed 
  (period is 1000 milliseconds, and will remain in loop
  until current time beat the time when it entered loop
  + a second).
*/
void wait() {
  timeNow = millis();
  while(millis() < timeNow + period) {
    //do nothing i.e. do not send a signal if one was already sent for a second
  }
}

/*
	The below code sets up the bluetooth module. Written by the ELEC1601 staff team.
*/


void setupBlueToothConnection() {
  
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}

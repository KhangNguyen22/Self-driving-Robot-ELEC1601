/*
GROUP T15-13
Master code
OVERVIEW:
  At any other given time, the robot is waiting to receive a signal
  (initially it will be at kitchen). When the robot is in its "idle" 
  state, it remains halted. When it receives a signal, depending on
  the signal received, it will choose its destination and path, and
  begin to move. When it reaches an intersection, it will perform 
  certain moves determined by the path it took. It can go left, right,
  forward, or halt (indicates it has reached its destination). Once it
  reaches its destination, it will return back to its idle state, 
  waiting for the next signal.
  
ASSUMPTIONS:
  1. It will not receive a signal while moving to a destination.
  2. It will move to and from its respective locations i.e. it won't
  be told to move to table 2 from table 1, it must move from table 1
  to kitchen first before moving to table 2.
  
CIRCUIT:
  ANALOG PINS:
    Eyes:
    Uses pins 2-3.
    Left photoresistor: 2
    Right photoresistor: 3
    Used for detecting when it's on the black track or not.
    These are set to INPUT.
  DIGITAL PINS:
    Servos:
    Uses pins 12-13.
    Left servo: 12
    Right servo: 13
    Attach servos to allow Boe-Bot to move.
      
FUNCTIONS:
  void sendStatus()
    Each time the robot recieves a signal or is currently halting, it
    will send a signal to the slave. This signal will be different 
    depending on where it's moving or where it has stopped. 
  void halt()
    Detach both servos to halt the Boe-Bot.
  void forward()
    Make the Boe-Bot go forward.
  void left()
    Make the Boe-Bot go left.
  void right()
    Make the Boe-Bot go right.
  void leftTurn()
    Make the Boe-Bot turn left then stop for 1.5 seconds.
  void rightTurn()
    Make the Boe-Bot turn right then stop for 1.5 seconds.
  void continueOn()
    Make the Boe-Bot go forward then stop for 1.5 seconds.
  void intersection()
    Sends the next instruction of the path array to the path decoder
    when it detects an intserction, where it will determine what to do.
  void pathDecoder()
    Executes the next instruction of the path array e.g. turn left, continue 
    on, etc. and iterates the iterator.
   
Version official
Date 03/11/19
Authors: GROUP T15-13   
*/

#include <SoftwareSerial.h>   // Software Serial Port
#include <Servo.h>            // Include servo library

#define RxD 7
#define TxD 6
#define ConnStatus A1         // Connection status on the SeeedStudio v1 shield is available on pin A1
                              // See also ConnStatusSupported boolean below
#define DEBUG_ENABLED  1

int shieldPairNumber = 13;

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// The following four string variable are used to simplify adaptation of code to different shield pairs

String slaveName = "Slave";                  // This is concatenated with shieldPairNumber later
String masterNameCmd = "\r\n+STNA=Master";   // This is concatenated with shieldPairNumber later
String connectCmd = "\r\n+CONN=";            // This is concatenated with slaveAddr later

int nameIndex = 0;
int addrIndex = 0;

String recvBuf;
String slaveAddr;
String retSymb = "+RTINQ=";   // Start symble when there's any return

SoftwareSerial blueToothSerial(RxD,TxD);

/* 
START SERVO DECLARATIONS
*/
Servo servoLeft;                        // Declare left and right servos
Servo servoRight;

const int rSpeed = 1450;               // Declare the speeds for the wheels
const int lSpeed = 1550;
/* 
START EYE DECLARATIONS
*/
const int reyePin = A2;
const int leyePin = A3;
/* 
DEFINE PATHS
  PATH DEFINTITIONS:
    F: Move forward
    L: Turn left
    R: Turn right
    S: Halt 
*/
char table1[] = {'F','L','S'};
char table2[] = {'F','R','S'};
char kitchen1[] = {'R','R','L','L','S'};
char kitchen2[] = {'L','L','R','L','S'};

//variables used for manipulating the paths
int currentPath;
int iterator = 0;
int currentDest = 0;

char instruction;
unsigned long currentMillis;

//true when it's in "idle state", false when it's moving
boolean homeState = true;

/*
void setup()
  Setup what's needed for the bluetooth component
  Setup servos
  Setup the pins connected  to the photoresistors to INPUT
*/
void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);   // Set Bluetooth module to default baud rate 38400
        
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Master-Slave connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Slave - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Slave.");
        
        setupBlueToothConnection();     // Set up the local (master) Bluetooth module
        getSlaveAddress();              // Search for (MAC) address of slave
        makeBlueToothConnection();      // Execute the connection to the slave

        delay(1000);                    // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }

    /* 
    ATTACH SERVOS
    */
    servoLeft.attach(12);               
    servoRight.attach(13);
    /* 
    SET PINMODES
    */ 
    pinMode(leyePin, INPUT);
    pinMode(reyePin, INPUT);
}
/*
void loop()
  Constantly checking if a bluetooth signal is received from slave
    - If a signal is received, set the destination and path and
      send a signal back to master to display where it's going.
    - Make homeState false, so it will begin to move (stays still
      when it is true).
  When homeState is false:
    - It will begin to move and follow the black tape until it hits
      an instersection.
      When it hits an intersection:
        - It will execute the next instruction in the path array
          defined by the signal it received.
        - This determines where it goes e.g. right turn, stop, etc.
      Once it stops (last instruction in each path array), it returns
      back to its idle state, that is, homeState is true (so now it 
      will continue halting) and will continue to send a signal where
      it is to the slave.
*/
void loop()
{
    char recvChar;


    while(1)
    {
        if(blueToothSerial.available())         // Check if there's any data sent from the remote Bluetooth shield
        {
            recvChar = blueToothSerial.read();
            if (homeState) {                    // in each case, the currentPath is set and homeState is set to false 
              if(recvChar == '1'){
                Serial.println("From Table 1 To Kitchen");
                currentPath = 1;
                currentDest = 0;
                homeState = false;
                sendStatus();
              }
              else if (recvChar == '2') {
                Serial.println("From Table2 to Kitchen");
                currentDest = 0;
                currentPath = 2;
                homeState = false;
                sendStatus();
              }
              else if (recvChar == '3') {
                Serial.println("To Table 1");
                currentDest = 1;
                currentPath = 3;
                homeState = false;
                sendStatus();
              }
              else if (recvChar == '4') {
                Serial.println("To Table 2");
                currentDest = 2;
                currentPath = 4;
                homeState = false;
                sendStatus();
              }
            }
          }
          sendStatus();
      
        /*
         * THIS IS THE MOTOR FUNCTION
         */
        int lState = analogRead(leyePin);
        int rState = analogRead(reyePin);
        servoLeft.attach(12);                 // has to be attached each time as the halt() function detaches the servos
        servoRight.attach(13);
        if(homeState == true){                // halt if it's in "idle" state
          halt();
        }
        else if(lState < 40 && rState < 40){  // both photoresistors have detected black tape, thus it is an intersection
          intersection();
        }
        else if (lState < 40){                // the left photoresistor has detected black tape, move left to stay on track
          left();
        }
        else if (rState < 40){                // the right photoresistor has detected black tape, move right to stay on track
          right();
        }
        else{
          forward();                          // neither photoresistor has detected black tape, meaning its on track
        }                                     // keep moving forward                            
    }
}
/*
void wait()
  PURPOSE: Called when a signal is sent to ensure
  only one signal is sent. It makes the program wait 
  until a tenth of a second has passed (will remain 
  in loop until current time beat the time when it 
  entered loop + 100 milliseconds).
*/
void wait() {
   currentMillis = millis();
   while (millis() < currentMillis + 100) {
      // do nothing i.e. ensure no signal sent after a signal was just sent
   }
}
/*
void sendStatus()
  Send a bluetooth signal to the slave dependent on it's destination and whether
  if homeState is true (destination reached) or homeState is false (moving to 
  destination).
*/
void sendStatus() {
  if (homeState) {                     // If it's currently in "idle" state (destination reached, homeState true),
    if (currentDest == 0) {            // it will send a signal which will make the slave display an "IDLE" message,
      blueToothSerial.println('a');    // following the destination on the LCD
    }
    else if (currentDest == 1) {
      blueToothSerial.println('b');
    }
    else if (currentDest == 2) {
      blueToothSerial.println('c');
    }
  }
  else {                                // If it's currently in "moving" state (moving to destination, homeState false),
    if (currentDest == 0) {             // it will send a signal which will make the slave display a "MOVING TO" message,
      blueToothSerial.println('A');     // following the destination on the LCD
    }
    else if (currentDest == 1) {
      blueToothSerial.println('B');
    }
    else if (currentDest == 2) {
      blueToothSerial.println('C');
    }
  }
  wait(); //wait() called to ensure this is not sent more than once.
}
void setupBlueToothConnection()
{
    Serial.println("Setting up the local (master) Bluetooth module.");

    masterNameCmd += shieldPairNumber;
    masterNameCmd += "\r\n";
 
    blueToothSerial.print("\r\n+STWMOD=1\r\n");      // Set the Bluetooth to work in master mode
    blueToothSerial.print(masterNameCmd);            // Set the bluetooth name using masterNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection is forbidden here

    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the master Bluetooth inquire
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("Master is inquiring!");
}


void getSlaveAddress()
{
    slaveName += shieldPairNumber;
    
    Serial.print("Searching for address of slave: ");
    Serial.println(slaveName);

    slaveName = ";" + slaveName;   // The ';' must be included for the search that follows
    
    char recvChar;

    // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
    //    +STWMOD=1 followed by a blank line
    //    +STNA=MasterTest (followed by a blank line)
    //    +S
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    WORK:
    //
    // It will then, character-by-character, add the result of the first device that responds to the +INQ request:
    //    +RTINQ=64,A2,F9,xx,xx,xx;OnePlus 6 (xx substituted for anonymity)
    //
    // If this does not contain slaveName, the loop will continue. If nothing else responds to the +INQ request, the
    // process will appear to have frozen BUT IT HAS NOT. Be patient. Ask yourself why your slave has not been detected.
    // Eventually your slave will respond and the loop will add:
    //    +RTINQ=0,6A,8E,16,C4,1B;SlaveTest
    //
    // nameIndex will identify "SlaveTest", return a non -1 value and progress to the if() statement.
    // This will strip 0,6A,8E,16,C4,1B from the buffer, assign it to slaveAddr, and break from the loop. 
    
    while(1)
    {
        if(blueToothSerial.available())
        {  
            recvChar = blueToothSerial.read();
            recvBuf += recvChar;
                        
            nameIndex = recvBuf.indexOf(slaveName);   // Get the position of slave name
            
            if ( nameIndex != -1 )   // ie. if slaveName was found
            {
                addrIndex = (recvBuf.indexOf(retSymb,(nameIndex - retSymb.length()- 18) ) + retSymb.length());   // Get the start position of slave address
                slaveAddr = recvBuf.substring(addrIndex, nameIndex);   // Get the string of slave address

                Serial.print("Slave address found: ");
                Serial.println(slaveAddr);
                
                break;  // Only breaks from while loop if slaveName is found
            }
        }
    }
}


void makeBlueToothConnection()
{
    Serial.println("Initiating connection with slave.");

    char recvChar;

    // Having found the target slave address, now form the full connection command
    
    connectCmd += slaveAddr;
    connectCmd += "\r\n";
  
    int connectOK = 0;       // Flag used to indicate succesful connection
    int connectAttempt = 0;  // Counter to track number of connect attempts

    // Keep trying to connect to the slave until it is connected (using a do-while loop)

    do
    {
        Serial.print("Connect attempt: ");
        Serial.println(++connectAttempt);
        
        blueToothSerial.print(connectCmd);   // Send connection command

        // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
        //    OK (followed by a blank line)
        //    +BTSTATE:3 (followed by a blank line)(BTSTATE:3 = Connecting)
        //
        // It will then, character-by-character, add the result of the connection request.
        // If that result is "CONNECT:OK", the while() loop will break and the do() loop will exit.
        // If that result is "CONNECT:FAIL", the while() loop will break with an appropriate "FAIL" message
        // and a new connection command will be issued for the same slave address.

        recvBuf = "";
        
        while(1)
        {
            if(blueToothSerial.available())
            {
                recvChar = blueToothSerial.read();
                recvBuf += recvChar;
        
                if(recvBuf.indexOf("CONNECT:OK") != -1)
                {
                    connectOK = 1;
                    Serial.println("Connected to slave!");
                    blueToothSerial.print("Master-Slave connection established!");
                    break;
                }
                else if(recvBuf.indexOf("CONNECT:FAIL") != -1)
                {
                    Serial.println("Connection FAIL, try again!");
                    break;
                }
            }
        }
    } while (0 == connectOK);

}

/*
void forward()
  Move the robot forward.
  Set the speed of left servo to lSpeed (counterclockwise)
  Set the speed of right servo to rSpeed (clockwise)
*/
void forward(){
  servoLeft.writeMicroseconds(lSpeed);
  servoRight.writeMicroseconds(rSpeed); 
  }
/*
void left()
  Move the robot left.
  Set the speed of left servo to 1500 (stopped)
  Set the speed of right servo to rSpeed (clockwise)
*/
void left(){
  servoLeft.writeMicroseconds(1500); 
  servoRight.writeMicroseconds(rSpeed);
  }
/*
void right()
  Move the robot right.
  Set the speed of left servo to lSpeed (counterclockwise)
  Set the speed of right servo to 1500 (stopped)
*/
void right(){
  servoLeft.writeMicroseconds(lSpeed); 
  servoRight.writeMicroseconds(1500); 
  }
/*
void halt()
  Stop the robot.
  Detatch both servos.
*/
void halt(){
  servoLeft.detach();
  servoRight.detach();
}
/*
void intersection()
  Called when an intersection is hit (both photoresistors detect the black tape).
  Path array is chosen dependent on the current destination (chosen by signal sent from slave)
  Instruction is chosen from the path array and sent to another function to determine what to do.
*/
void intersection(){
  Serial.println("intersection");
  switch(currentPath){            // There are four path arrays. The signal sent from slave determines which one to use
    case 1:
      instruction = kitchen1[iterator];  // For this example, this sets the instruction to the next instruction of getting from
      pathDecoder(instruction);          // table 1 to the kitchen.
      break;                             // The instruction is sent to pathDecoder() to determine what to do.
    case 2:
      instruction = kitchen2[iterator];
      pathDecoder(instruction);
      break;
    case 3:
      instruction = table1[iterator];
      pathDecoder(instruction);
      break;
    case 4:
      instruction = table2[iterator];
      pathDecoder(instruction);
      break;
  }
}
/*
void pathDecoder(char instruction)
    Called in the intserction() function to determine what the robot should do.
    It will execute the following instruction as defined in the switch case e.g. 'L'
    does a left turn.
    A POSSIBLE QUESTION YOU MIGHT HAVE:
      What's the point of 'F' when each intersection it seems to turn? And how does
      it know to do a specific instruction when it already stops at an intersection 
      (might lead to error)?
    ANSWER:
      When the robot is at the kitchen, and needs to move to a table, 'F' will be
      the next instruction, so it gets off the intersection, stops for 1.5 seconds 
      then continues to move forward. Similarly, when it reaches a table, the first
      instruction will be a turn ('R' in both cases) when it moves back to the
      kitchen.
*/
void pathDecoder(char instruction){
  Serial.println(instruction);
  switch(instruction){  // execute instruction dependent on what it is
    case 'L':           // example is 'L'
      leftTurn();       // perform a left turn
      iterator++;       // iterate iterator so next instruction is the next character in the path array
      break;
    case 'R':
      rightTurn();
      iterator++;
      break;
    case 'F':
      continueOn();
      iterator++;
      break;
    case 'S':
      homeState = true; // if it stops, then it has reached the destination (homeState true)
      iterator=0; // set back the iterator, as the next instruction will be in a new path array
      break;
  }
}
/*
A POSSIBLE QUESTION YOU MIGHT HAVE:
  Why do I need the functions below when I already have left(), right() and forward()?
ANSWER:
  When dealing with an intersection, we want the robot to ignore the values retreived 
  from the photo resistors. We want to ensure when it hits an intersection, it doesn't
  accidentally detect the intersection again which would completely ruin the pathing.
*/

/*
void leftTurn()
  Same principles as left(), but is followed by a delay of 1.5 seconds.
  This ensures the intersection is not detected again, and can continue
  moving forward after this event-driven case.
*/
void leftTurn(){
  servoLeft.writeMicroseconds(1500);
  servoRight.writeMicroseconds(rSpeed);
  delay(1500);
}
/*
void righttTurn()
  Same principles as right(), with the same principles why a delay is
  followed in the leftTurn() comment.
*/
void rightTurn(){
  servoLeft.writeMicroseconds(lSpeed);
  servoRight.writeMicroseconds(1500);
  delay(1500);
}
/*
void continueOn()
  Same principles as forward(), with the same principles why a delay is
  followed in the leftTurn() comment.
*/
void continueOn(){
  servoLeft.writeMicroseconds(lSpeed);
  servoRight.writeMicroseconds(rSpeed);
  delay(1500);
}

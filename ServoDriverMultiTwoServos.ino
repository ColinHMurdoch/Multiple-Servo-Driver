/* 
ServoDriverMulti

This sketch allows an Arduino Nano to operate a number of servos 
for railway point control.

Operating paramaters are programmable and stored in the EEPROM

*/


#include <VarSpeedServo.h>
#include <Button.h>
#include <EEPROM.h>

int NumberOfServos = 2; // Set this value to the number of servos in use.

# define LEVER1 14 // the operating switch/lever1 A0
# define LEVER2 15 // the operating switch/lever2 A1
# define LEVER3 16 // the operating switch/lever3 A2
# define LEVER4 17 // the operating switch/lever4 A3
# define LEVER5 18 // the operating switch/lever5 A4
# define LEVER6 19 // the operating switch/lever6 A5



struct ServoLimits {
  int servonum;
  int lowvalue;
  int highvalue;
  bool reverseflag;
} servodata[5]; //Array to store custom data read from EEPROM.

Button LOWBUTTON(7);
Button PROGBUTTON(6);
Button HIGHBUTTON(5);
Button MODEBUTTON(4);

VarSpeedServo myservo0;  // create servo object to control a servo
VarSpeedServo myservo1;  // create servo object to control a servo
//VarSpeedServo myservo2;  // create servo object to control a servo
//VarSpeedServo myservo3;  // create servo object to control a servo
//VarSpeedServo myservo4;  // create servo object to control a servo
//VarSpeedServo myservo5;  // create servo object to control a servo
// twelve servo objects can be created on most boards

// General variable to control the logic
int pos = 0;    // variable to store the servo position
int lowpos = 0; // variable to store to off position
int midpos = 0; // variable to store the mod position
int highpos = 0; // variable to store the on position
bool revflag = false; // flag to indicate if action is reversed
int eeAddress = 0; // varable for EEPROM address
int LeverState[5]; // array to read the current lever state
int LastLeverState[5]; // array to detect if lever state changed
bool SetupMode = false; // Indicates the current setup state
int currentpos = 0; // a counter for setupmode
int direction = LOW; // an indicator for down (LOW) or up (HIGH)
int thisservonumber = 0; // which one are we dealing with
bool SomethingsChanged = false; // a flag to ensure we only write back changed data

// A few more variables to control the LED flash
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated
int LEDSTATE = LOW;   // Used to check if the LED is on or off
int LEDFLASHES = 0;
// constants won't change:
const long interval = 10000;           // interval at which to blink (milliseconds)


void setup() {

  Serial.begin(9600);

  LOWBUTTON.begin();
  PROGBUTTON.begin();
  HIGHBUTTON.begin();
  MODEBUTTON.begin();

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW

  pinMode(LEVER1, INPUT_PULLUP);
  pinMode(LEVER2, INPUT_PULLUP);
  pinMode(LEVER3, INPUT_PULLUP);
  pinMode(LEVER4, INPUT_PULLUP);
  pinMode(LEVER5, INPUT_PULLUP);
  pinMode(LEVER6, INPUT_PULLUP);
  
  Serial.println("Read data from EEPROM: ");

  GetEEPROMData();
 
  // Servos should be on PWM pins - 3, 5, 6, 9, 10,11
  // not sure if this is necessary - 8 & 9 in use for test
  // they seem to work otherwise with the servo library to use more convenient pins

  // Uncomment the ones actually in use.
  myservo0.attach(8);  // attaches the servo on pin 8 to the servo objec
  myservo1.attach(9);  // attaches the servo on pin 9 to the servo objec
  //myservo2.attach(10);  // attaches the servo on pin 10 to the servo objec
  //myservo3.attach(11);  // attaches the servo on pin 11 to the servo objec
  //myservo4.attach(12);  // attaches the servo on pin 12 to the servo objec
  //myservo5.attach(3);  // attaches the servo on pin 3 to the servo objec

  ClearLeverStates();

}

void loop() {

  bool buttonloop = true; // a flag to control held buttons
  
    if (SetupMode == false){ // dont do this section if we are in setupmode

      // We only have two servos.  Uncomment if using more than 2
      // second field = array element
      CheckLever(LEVER1, 0);
      CheckLever(LEVER2, 1);
      //CheckLever(LEVER3, 2);
      //CheckLever(LEVER4, 3);
      //CheckLever(LEVER5, 4);
      //CheckLever(LEVER6, 5); 
      
    }

    if (SetupMode == true){
      CheckFlash();   // Check if the LED needs flashing
    }
    
    if (PROGBUTTON.pressed()) {  // Go into program mode
        if (SetupMode == false) {
            SetupMode = true;
            thisservonumber = 0;
            SetProgramMode();
        }
        else {
            UpdateLimitData();
            thisservonumber = thisservonumber + 1;
            SetProgramMode();
            if (thisservonumber >= NumberOfServos){
              ReleaseProgramMode();
              SetupMode = false;
            }
            
        }
    }

    if (LOWBUTTON.pressed()) {
      Serial.println("The LOWBUTTON is pressed");
      if (SetupMode == true) {
        buttonloop = true;
        SetLow();
        while (buttonloop == true) {
         delay(150);
          if (LOWBUTTON.released()) {
            buttonloop = false;
            Serial.println("Button 1 released");
          }
          else {
            Serial.println("LOWBUTTON HELD");
            SetLow();
          }
        }
      }
    }

    if (HIGHBUTTON.pressed()) {
      Serial.println("The HIGHBUTTON is pressed");
      if (SetupMode == true) {
        buttonloop = true;
        SetHigh();
        while (buttonloop == true) {
         delay(150);
          if (HIGHBUTTON.released()) {
            buttonloop = false;
            Serial.println("HIGHBUTTON released");
          }
          else {
            Serial.println("HIGHBUTTON HELD");
            SetHigh();
          }
        }
      }
    }

    if (MODEBUTTON.pressed()) {
      if (SetupMode == true) {
        ChangeMode();
      }
    }
}

void CheckFlash() {
  // check to see if it's time to blink the LED; that is, if the difference
  // between the current time and last time you blinked the LED is bigger than
  // the interval at which you want to blink the LED.
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    Serial.print("Flashing the LED at servo number ");
    Serial.println(thisservonumber);
    previousMillis = currentMillis;

    for (int x = 1; x <= (thisservonumber + 1); x++) {
      Serial.println(x);
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(100);                       // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(200);   
    }                  
  }
}

void CheckLever(int LeverIn, int LeverVal) {

    LeverState[LeverVal] = digitalRead(LeverIn);
    if (LeverState[LeverVal] != LastLeverState[LeverVal]) {
      MoveServo(LeverIn, LeverVal);
    }
}

void MoveServo(int LeverIn, int LeverVal){

  Serial.println("Moving Servo ");
// Get the servo data from the struct array

    if (servodata[LeverVal].reverseflag == true) {
      lowpos = servodata[LeverVal].highvalue;
      highpos = servodata[LeverVal].lowvalue;
      midpos = ((lowpos - highpos) / 2) + lowpos;
    }
    else {
      lowpos = servodata[LeverVal].lowvalue;
      highpos = servodata[LeverVal].highvalue;
      midpos = ((highpos - lowpos) / 2) + lowpos;
    }
    revflag = servodata[LeverVal].reverseflag; 
   
     if (LeverState[LeverVal] == LOW) {  // The lever has been pulled so put servo ON
      
             Serial.print("Lever ");
             Serial.print(LeverVal);
             Serial.print(" On - moving to ");
             Serial.println(lowpos);  
             WriteToServo(LeverVal, lowpos);
             //myservo.write(lowpos, 50, true);              // tell servo to go to position in variable 'pos'
             delay(100);                       // waits 15ms for the servo to reach the position
     }
    
      if (LeverState[LeverVal] == HIGH) { // Lever off so go to OFF position
       Serial.print("Lever ");
       Serial.print(LeverVal);
       Serial.print(" Off - moving to ");
        Serial.print(highpos);
       WriteToServo(LeverVal, highpos);
       //myservo.write(highpos, 50, true);              // tell servo to go to position in variable 'pos'
       delay(100);                       // waits 15ms for the servo to reach the position
      }
    
      LastLeverState[LeverVal] = LeverState[LeverVal];
          
}

void WriteToServo(int LeverVal, int thisposition) {

  Serial.println("Write to Servo ");

  switch (LeverVal) {
    case 0:
      myservo0.write(thisposition, 50, true);
    break;
    case 1:
      myservo1.write(thisposition, 50, true);
    break;
    case 2:
      myservo2.write(thisposition, 50, true);
    break;
    case 3:
      myservo3.write(thisposition, 50, true);
    break;
    case 4:
      myservo4.write(thisposition, 50, true);
    break;
    case 5:
      myservo5.write(thisposition, 50, true);
    break;

    delay(15);
  }

}

void GetEEPROMData() {
  
  Serial.println("Read data from EEPROM: ");

  eeAddress = 0; //Position at start of EEPROM.
  
  for (int x = 0; x < 6; x++) { 

    EEPROM.get(eeAddress, servodata[x]);
 
    Serial.print("Servo Number  ");
    Serial.print(servodata[x].servonum);
    Serial.print("  ");
    Serial.println(servodata[x].lowvalue);
    Serial.print("  ");
    Serial.println(servodata[x].highvalue);
    Serial.print("  ");
    Serial.println(servodata[x].reverseflag);
    eeAddress += 7;
  }
  
}

void SetProgramMode() {
      
  Serial.println("Setting Program Mode");
  midpos = 90; // variable to store the mod position
  SomethingsChanged = false; // a flag to check for changes
  currentpos = midpos;
  Serial.print("Current Position - ");
  Serial.println(currentpos);
  WriteToServo(thisservonumber, currentpos);
 
}

void SetLow(){

  Serial.println("Setting Low Routine ");
  if (direction == HIGH) { // we are changing cirection so save the High
    highpos = currentpos;
  }
    currentpos = currentpos -1;
    direction = LOW;
    Serial.print("Current Position - ");
    Serial.println(currentpos);
    WriteToServo(thisservonumber, currentpos);
    SomethingsChanged = true; // changeshave been made
    lowpos = currentpos;
  
  
}

void SetHigh(){

  Serial.println("Set High Routine ");
   if (direction == LOW) { // we are changing cirection so save the Low
    lowpos = currentpos;
  }
  
    currentpos = currentpos + 1;
    direction = HIGH;
    Serial.print("Current Position - ");
    Serial.println(currentpos);
    WriteToServo(thisservonumber, currentpos);
    SomethingsChanged = true; // changes have been made
    highpos = currentpos;
  
}

void ChangeMode() {

    Serial.println("Setting the Reverse Flag ");
   // we need to set the reverse flag - toggle it.
   //

   SomethingsChanged = true; // changeshave been made
   if (revflag == false) {
     revflag = true;
   }
   else {
      revflag = false;
   }

}

void UpdateLimitData(){

   if (SomethingsChanged == true) { // Only update if the user has changed something
    
  
  // Update the data and write back to the EEprom

    servodata[thisservonumber].lowvalue = lowpos;
    servodata[thisservonumber].highvalue = highpos;
    servodata[thisservonumber].reverseflag = revflag;

    Serial.println("Writing new values to EEPROM: ");
    Serial.print("For Servo Number : ");
    Serial.println(thisservonumber);
    Serial.println(servodata[thisservonumber].lowvalue);
    Serial.println(servodata[thisservonumber].highvalue);
    Serial.println(servodata[thisservonumber].reverseflag);

    eeAddress = 0; // the array starts at position zero
    eeAddress += (thisservonumber * 7);
    Serial.print("At address : ");
    Serial.println(eeAddress);
    EEPROM.put(eeAddress, servodata[thisservonumber]);

    SomethingsChanged = false;
   }
}

void ReleaseProgramMode(){

  Serial.println("Releasing Program Mode");
  // Force the logic to reset the servo when we exit Program mode
  // by making it look as if the lever has been moved.

  if (LeverState[thisservonumber] == LOW) {
    LastLeverState[thisservonumber] = HIGH;
  }
  else {
    LastLeverState[thisservonumber]= LOW;
  }
    
}

void ClearLeverStates() {
  Serial.println("Clearing Lever States ");
  for (int x = 0; x < 6; x++) {
    LeverState[x] = LOW;
    LastLeverState[x] = LOW;
  }
  
}

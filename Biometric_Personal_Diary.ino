/* Biometric Personal Diary - Arduino Pro Mini April 2018 TechKiwiGradgets
V1 -  Revise after adding more fingerprints -If valid fingerprint then unlock door Ten possible Prints


*/

//Libraries
#include <Servo.h>
#include <EEPROM.h>

// WS2182 LED Driver Library Setup
#include "FastLED.h"
// How many leds in your strip?
#define NUM_LEDS 10 // Note: First LED is address 0
#define DATA_PIN 11 // Note: D11 used to control LED chain

// Define the array of leds
CRGB leds[NUM_LEDS];

#include <Adafruit_Fingerprint.h>
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// comment these two lines if using hardware serial
#include <SoftwareSerial.h>
SoftwareSerial mySerial(2, 3);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
int fingerprintID = 0;

//Constants

//Variables
int servoPin = 9;
int pulse = 1500;
int lockdelay = 500; // Duration in mS to delay activity until Servo changes state from Open to Closed
int slockp = 160; // Servo position in Degrees for locked position
int sopenp = 40; // Servo position in degrees for the open position
int doorsensor = 100; // Measures the InfraRed signal on the IR Transistor. Indicates the door is open or closed.
int dst = 750;  // Testing identified that doorsensor was approx 640 when door open and approx 1000 when closed. Therefore 750 used as a threshold for open/closed.

int t1 = 20;// LED Powerup Sequence delay

Servo lockServo;  // create servo object
byte lockposition; //Byte variable for servo position to be stored in EEPROM where unlock = 0 and lock = 1

/*
 *  to write use    EEPROM.write(100, lockposition);
 *  to read use     lockposition = EEPROM.read(100);
 */



void setup() {
  
  Serial.begin(9600); // Setupserial interface for test data outputs
  // set the data rate for the sensor serial port
  finger.begin(57600);

// WS2182 LED Driver Setup
  LEDS.addLeds<WS2812,DATA_PIN,GRB>(leds,NUM_LEDS); // Default if RGB for this however may vary dependent on LED manufacturer
  LEDS.setBrightness(2); //Set brightness of LEDs here
  // limit my draw to 1A at 5v of power draw
  FastLED.setMaxPowerInVoltsAndMilliamps(5,100);

  FastLED.setDither(0); // Turns off Auto Dithering function to remove flicker


  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } 
  else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  // Initialize Analog Pin 7 as input to read the output of the IR Transistor
  pinMode(A7, INPUT); 
  // initialize the test button pin 
   pinMode(8, INPUT_PULLUP); // Use pullup feature to emulate an open close command

  lockposition = EEPROM.read(100); // Read in the Lock Position from EEPROM where unlock = 0 and lock = 1
  if (lockposition > 1 ){ // If run for the first time set the lock position to open
    EEPROM.write(100, 0); 
    lockposition = 0; 
  }

  ledtest(); // startup led test
  
}


void loop() {


  
  // Only Read the Fingerprint sensor if the door is locked and the door is closed
  if ((lockposition == 1)&&(doorsensor >= dst)){ // Door Locked and Door Closed
    fingerprintID = getFingerprintIDez();
    delay(100);// This delay impacts Servo stability

    if(fingerprintID >= 1 && fingerprintID <= 10){ 
        // If valid fingerprint then unlock door Ten possible Prints
        confidencebarchart(); // Draw the outcome on LED bar graph
          unlockdoor(); // Routine for opening lock
          fingerprintID = 1000;  // Block any further false positives        
        } else if(finger.confidence > 0) {
            failbarchart();
            finger.confidence = 0; // Block any further false positives   
        }
    }



  // Test if push of a button D8 to lock or unlock door
  if(digitalRead(8) == LOW ){ // Check that button pushed
    delay(150); // Debounce by waiting then checking again
      if(digitalRead(8) == LOW){

        if ((lockposition == 0)&&(doorsensor >= dst)) {  // If door closed properly then Close lock 

          lockdoor(); // Routine for closing lock 
          finger.confidence = 0; // Block any further false positives           
        } 
       }     
    }

  // Read the Door Sensor
  doorsensor = analogRead(A7);

/*
    Serial.print(finger.confidence);
    Serial.print("      ");  
    Serial.print(doorsensor);
    Serial.print("      ");    
    Serial.print(EEPROM.read(100));
    Serial.print("      ");    
    Serial.println(lockposition);      // Display status of the lockposition variable           
*/


}

// Routine for loccking the Personal Diary Door and storing the state of the lock
void lockdoor(){

    lockServo.attach(servoPin);  // Temporarily Activate Servo
    lockServo.write(slockp);   // Issue command to close the lock
    lockposition = 1; // Setflag indicating the lock is closed
    EEPROM.write(100, lockposition); // Store the position of the lock in EEPROM so it is not affecte by power cycling     
    delay(lockdelay);
    lockServo.detach();  // To avoid jitter deactivate servo  
}


// Routine for loccking the Personal Diary Door and storing the state of the lock
void unlockdoor(){

    lockServo.attach(servoPin);  // Temporarily Activate Servo
    lockServo.write(sopenp);   // Issue command to open the lock
    lockposition = 0; // Setflag indicating the lock is open
    EEPROM.write(100, lockposition); // Store the position of the lock in EEPROM so it is not affecte by power cycling     
    delay(lockdelay);
    lockServo.detach();  // To avoid jitter deactivate servo  
      
}


// Adafruit getFingerprintIDez returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID; 
}

void ledtest() { // Powerup sequence for unit

  // Flash LED to show calibrate done

      for (int x = 0; x < 10; x++) {
        leds[x] = CRGB::Blue;
        FastLED.show();
        delay(t1);     
      }
      for (int x = 0; x < 10; x++) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(t1); 
      }

      for (int x = 9; x > -1; x--) {
        leds[x] = CRGB::Blue;
        FastLED.show();
        delay(t1);     
      }
      for (int x = 9; x > -1; x--) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(t1); 
      }
}



void confidencebarchart(){ // Display the confidence LED Level based on fingerprint read

      for (int x = 0; x < 10; x++) {
        leds[x] = CRGB::Green;
        FastLED.show();
        delay(t1);     
      }
      for (int x = 0; x < 10; x++) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(t1); 
      }

}

void failbarchart(){ // Display the confidence LED Level based on fingerprint read

      for (int x = 0; x < 10; x++) {
        leds[x] = CRGB::Red;
        FastLED.show();
        delay(t1);     
      }
      for (int x = 0; x < 10; x++) {

        leds[x] = CRGB::Black;
        FastLED.show();      
        delay(t1); 
      }

}



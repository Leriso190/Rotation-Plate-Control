#include "BluetoothSerial.h"
#include <ESP32Servo.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define LED_PIN    5
#define LED_COUNT 8

// touch variables
int touchMinus;
int touchPausePlay;
int touchCounterClockwise;
int touchPlus;
int touchClockwise;

//step motor io constants
const int DIR = 12;
const int STEP = 14;
const int MS1 = 25;
const int MS2 = 26;
const int MS3 = 27;
const int ENA = 33;

//logic variables
int pauseBetweenSteps;
int pauseBetweenStepsUpperLimit=1900;
int pauseBetweenStepsLowerLimit=500;
int velocity=0;
bool paused=true;
bool pauseIsPressed=false;
bool minusIsPressed=false;
bool plusIsPressed=false;
bool rotatesClockwise=true;

//neo pixel stripe variables
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t colorStripe;
uint32_t colorRed=strip.Color(255,0,0);
uint32_t colorBlue=strip.Color(0,0,255);
uint32_t colorGreen=strip.Color(0,255,0);

//Bluetooth variables
BluetoothSerial SerialBT;
char recMsg;

void setup() {
  setUpLED();
  setUpMotor();
  Serial.begin(57600);
  setUpBluetooth();
}

void loop() {
  readTouches();
  readBluetoothInputs();
  checkIfButtonsReleased();
  moveMotor();
}

//moves the motor in 25 steps
void moveMotor() {
  if(!paused){
    for(int i=0; i<25;i++){
      digitalWrite(STEP, HIGH);
      delayMicroseconds(pauseBetweenSteps);
      digitalWrite(STEP, LOW);
      delayMicroseconds(pauseBetweenSteps);
    }  
  }
}

//reads all touch inputs from the control panel
void readTouches() {
  //read touch pins
  touchMinus = touchRead(T0);
  touchPausePlay = touchRead(T9);
  touchCounterClockwise = touchRead(T2);
  touchPlus = touchRead(T3);
  touchClockwise = touchRead(T4);

  //print all informations
  Serial.print("Minus: ");
  Serial.print(touchMinus);
  Serial.print("\tPlus: ");
  Serial.print(touchPlus);
  Serial.print("\tPlay: ");
  Serial.print(touchPausePlay);
  Serial.print("\tCounterClockwise: ");
  Serial.print(touchCounterClockwise);
  Serial.print("\tClockwise: ");
  Serial.print(touchClockwise);
  Serial.print("\tPause Between Steps: ");
  Serial.print(pauseBetweenSteps);
  Serial.print("\tVelocity: ");
  Serial.println(velocity);

  //performs actions when a capacitive button is pressed
  if (touchMinus <= 50) {
    if(!minusIsPressed){
      decreaseVelocity();
    }
  }
  else if (touchPausePlay <= 58) {
    if(!pauseIsPressed){
      playAndPause();
    }
  }
  else if (touchCounterClockwise <= 40) {
    digitalWrite(DIR,LOW);
    rotatesClockwise=false;
    if(!paused){
      setLEDColor(colorBlue);
    }
  }
  else if (touchPlus <= 48) {
    if(!plusIsPressed){
      increaseVelocity();
    }
  }
  else if (touchClockwise <= 60) {
    digitalWrite(DIR,HIGH);
    rotatesClockwise=true;
    if(!paused){
      setLEDColor(colorGreen);
    }
  }
}

//reads all sent bluetooth messages
void readBluetoothInputs(){
  if(SerialBT.available()){
    char recMsg=SerialBT.read();
    Serial.println(recMsg);
    
    if(recMsg=='a'){
        decreaseVelocity();
    }
    else if(recMsg=='b'){
        increaseVelocity();
    }
    else if(recMsg=='c'){
        playAndPause();
    }
    else if(recMsg=='d'){
      digitalWrite(DIR,LOW);
      rotatesClockwise=false;
      if(!paused){
        setLEDColor(colorBlue);
      }  
    }
    else if(recMsg=='e'){ 
      digitalWrite(DIR,HIGH);
      rotatesClockwise=true;
      if(!paused){
        setLEDColor(colorGreen);
      }
    }
  }
}

//checks if buttons where released to set the flags that they can be pressed again
void checkIfButtonsReleased(){
  if(touchPausePlay>60){
    pauseIsPressed=false;
  }
  if(touchMinus > 50){
    minusIsPressed=false;
  }
  if(touchPlus>50){
    plusIsPressed=false;
  }
}

//decreases velocity if used
void decreaseVelocity(){
  minusIsPressed=true;
  if(pauseBetweenSteps>=pauseBetweenStepsUpperLimit){
    pauseBetweenSteps=pauseBetweenStepsUpperLimit;
  }else{
    pauseBetweenSteps+=200;
    velocity--;
    strip.setPixelColor(7-velocity-1,strip.Color(0,   0,   0));
    strip.show();
  }
}

//increases velocity if used
void increaseVelocity(){
  plusIsPressed=true;
  if(pauseBetweenSteps<=pauseBetweenStepsLowerLimit){
    pauseBetweenSteps=pauseBetweenStepsLowerLimit;
  }
  else{
    pauseBetweenSteps-=200;
    velocity++;
    strip.setPixelColor(7-velocity, colorStripe);
    strip.show();
  }
}

//performs actions when play buttons are pressed
void playAndPause(){
  pauseIsPressed=true;
  if(paused){
    paused=false;
    digitalWrite(ENA,LOW);
    if(rotatesClockwise){
      setLEDColor(colorGreen);
    }else{
      setLEDColor(colorBlue);
    }
  }else{
    paused=true;
    digitalWrite(ENA,HIGH);
    setLEDColor(colorRed);
  }
}

//sets LED colors and turns on specific number of pixels
void setLEDColor(uint32_t color){
  colorStripe=color;
  for (int i = 0; i  <=velocity; i++) { // For each pixel in strip...
    strip.setPixelColor((7 - i), color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
  }
}

//sets up LEDs initially
void setUpLED() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS to about 1/5 (max = 255)
  setLEDColor(colorRed);
}

//sets up Motor initially
void setUpMotor() {
  pinMode(STEP, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(MS3, OUTPUT);
  pinMode(ENA, OUTPUT);

  digitalWrite(MS1, HIGH);
  digitalWrite(MS2, HIGH);
  digitalWrite(MS3, HIGH);
  digitalWrite(DIR,HIGH);
  digitalWrite(ENA,HIGH);

  pauseBetweenSteps=pauseBetweenStepsUpperLimit;
}
void setUpBluetooth(){
  if(!SerialBT.begin("RotationPlateControl")){
    Serial.println("An error occured initializing Bluetooth");
  }
}

//
// Lego Robot - based on Pulse Sensor Amped 1.2 app
//
// Created by Dale Low.
// Copyright (c) 2014 gumbypp consulting. All rights reserved.
//

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// - The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

/*
>> Pulse Sensor Amped 1.2 <<
This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
    www.pulsesensor.com 
    >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
The following variables are automatically updated:
Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
IBI  :      int that holds the time interval between beats. 2mS resolution.
BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.

This code is designed with output serial data to Processing sketch "PulseSensorAmped_Processing-xx"
The Processing sketch is a simple data visualizer. 
All the work to find the heartbeat and determine the heartrate happens in the code below.
Pin 13 LED will blink with heartbeat.
If you want to use pin 13 for something else, adjust the interrupt handler
It will also fade an LED on pin fadePin with every beat. Put an LED and series resistor from fadePin to GND.
Check here for detailed code walkthrough:
http://pulsesensor.myshopify.com/pages/pulse-sensor-amped-arduino-v1dot1

Code Version 1.2 by Joel Murphy & Yury Gitman  Spring 2013
This update fixes the firstBeat and secondBeat flag usage so that realistic BPM is reported.

*/

#include <LiquidCrystal.h>
#include <TimerOne.h>

#define LEGO_ROBOT 

//  VARIABLES
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
#ifndef LEGO_ROBOT
int fadePin = 3;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
#endif

#ifdef LEGO_ROBOT
int forwardPin = 4;               // pins to turn the motor forward or backwards when used in conjunction with 
int reversePin = 5;               // the seeed relay shield (or relays connected to digital pins 4 and 5)

#define kOpenCountMin          1
#define kOpenCountMax          5
#define kClosedCount           1

#define kGlyphEyeUL            0
#define kGlyphEyeTop           1
#define kGlyphEyeUR            2
#define kGlyphEyeLL            3
#define kGlyphEyeBottom        4
#define kGlyphEyeLR            5

#define kGlyphBirdTail         0
#define kGlyphBirdTopWing      1
#define kGlyphBirdMidTop       2
#define kGlyphBirdMidBottom    3
#define kGlyphBirdBottomWing   4
#define kGlyphBirdMidRight     5
#define kGlyphBirdBeak         6

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 3, 2, 9, 8);

byte eye_ul[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00001,
  B00010,
  B00100,
};

byte eye_top[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000,
};

byte eye_ur[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B10000,
  B01000,
  B00100,
};

byte eye_ll[8] = {
  B00000,
  B00001,
  B00010,
  B00100,
  B00010,
  B00001,
  B00000,
  B00000,
};

byte eye_bot[8] = {
  B11111,
  B01110,
  B11011,
  B11111,
  B11110,
  B01100,
  B11111,
  B00000,
};

byte eye_lr[8] = {
  B00000,
  B10000,
  B01000,
  B00100,
  B01000,
  B10000,
  B00000,
  B00000,
};

byte eye_ll_closed[8] = {
  B00000,
  B00000,
  B00000,
  B00010,
  B00001,
  B00000,
  B00000,
  B00000,
};

byte eye_bot_closed[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B00000,
  B00000,
  B00000,
};

byte eye_lr_closed[8] = {
  B00000,
  B00000,
  B00000,
  B01000,
  B10000,
  B00000,
  B00000,
  B00000,
};

/////////////////////////////////////

byte bird_tail[8] = {
  B00000,
  B00001,
  B01110,
  B10000,
  B01111,
  B00000,
  B00000,
  B00000,
};

byte bird_top_wing[8] = {
  B00000,
  B11000,
  B10100,
  B10110,
  B01011,
  B01011,
  B01011,
  B01101,
};

byte bird_mid_with_top[8] = {
  B01101,
  B11101,
  B00000,
  B00000,
  B00000,
  B10000,
  B10111,
  B00000,
};

byte bird_mid_with_bottom[8] = {
  B00000,
  B11101,
  B00000,
  B00000,
  B00000,
  B10000,
  B10111,
  B00100,
};

byte bird_bottom_wing[8] = {
  B10100,
  B10100,
  B01000,
  B01000,
  B00000,
  B00000,
  B00000,
  B00000,
};

byte bird_mid_right[8] = {
  B00011,
  B11100,
  B00101,
  B00100,
  B01111,
  B10000,
  B00000,
  B00000,
};

byte bird_beak[8] = {
  B11111,
  B10001,
  B11110,
  B10010,
  B11110,
  B00000,
  B00000,
  B00000,
};

typedef enum {
  kAppStateIdle,
  kAppStateMovingBack,
  kAppStateMovingForwards
} AppState;

volatile bool g_show_eyes_flag = false;
volatile bool g_toggle_eyes_flag = false;
volatile int g_valid_bpm_count = 0;
volatile bool g_eye_closed = false;

AppState g_last_state = kAppStateIdle;

#endif

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

void setup()
{
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
#ifdef LEGO_ROBOT
  randomSeed(analogRead(1));
  Serial.begin(57600);
  Serial.print("\n*** PulseLego starting\n");
  pinMode(forwardPin, OUTPUT);
  pinMode(reversePin, OUTPUT);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);

  create_and_show_open_eyes();

  Timer1.initialize(500000);             // initialize timer1, and set a 1/2 second period
  Timer1.attachInterrupt(timerCallback); // attaches callback() as a timer interrupt  
#else
  pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
#endif  

  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
   // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE, 
   // AND APPLY THAT VOLTAGE TO THE A-REF PIN
   //analogReference(EXTERNAL);   
}

#ifdef LEGO_ROBOT

void create_and_show_open_eyes()
{
  lcd.createChar(kGlyphEyeUL, eye_ul);
  lcd.createChar(kGlyphEyeTop, eye_top);
  lcd.createChar(kGlyphEyeUR, eye_ur);

  lcd.createChar(kGlyphEyeLL, eye_ll);
  lcd.createChar(kGlyphEyeBottom, eye_bot);
  lcd.createChar(kGlyphEyeLR, eye_lr);
  
  // left eye
  lcd.setCursor(4,0);
  lcd.write((uint8_t)kGlyphEyeUL);
  lcd.write((uint8_t)kGlyphEyeTop);
  lcd.write((uint8_t)kGlyphEyeUR);

  lcd.setCursor(4,1);
  lcd.write((uint8_t)kGlyphEyeLL);
  lcd.write((uint8_t)kGlyphEyeBottom);
  lcd.write((uint8_t)kGlyphEyeLR);

  // right eye
  lcd.setCursor(9,0);
  lcd.write((uint8_t)kGlyphEyeUL);
  lcd.write((uint8_t)kGlyphEyeTop);
  lcd.write((uint8_t)kGlyphEyeUR);

  lcd.setCursor(9,1);
  lcd.write((uint8_t)kGlyphEyeLL);
  lcd.write((uint8_t)kGlyphEyeBottom);
  lcd.write((uint8_t)kGlyphEyeLR);
}

void create_bird()
{
  lcd.createChar(kGlyphBirdTail, bird_tail);
  lcd.createChar(kGlyphBirdTopWing, bird_top_wing);
  lcd.createChar(kGlyphBirdMidTop, bird_mid_with_top);
  lcd.createChar(kGlyphBirdMidBottom, bird_mid_with_bottom);
  lcd.createChar(kGlyphBirdBottomWing, bird_bottom_wing);
  lcd.createChar(kGlyphBirdMidRight, bird_mid_right);
  lcd.createChar(kGlyphBirdBeak, bird_beak);
}

void show_bird_with_heart_rate(int col, int hr)
{    
  bool show_top_wing = !(col & 1);  // even column

  lcd.clear();
  lcd.setCursor(col, 1);
  
  char hr1 = (hr/100) ? (hr/100 + 0x30) : ' ';
  char hr2 = ((hr/10)%10) + 0x30;
  char hr3 = (hr%10) + 0x30;
  
  lcd.setCursor(col, show_top_wing ? 1 : 0);
  lcd.print(hr1);
  lcd.print(hr2);
  lcd.print(hr3);
  
  if (show_top_wing) {
    lcd.setCursor(col + 3, 1);
    lcd.write((uint8_t)kGlyphBirdTail);
    lcd.setCursor(col + 4, 0);
    lcd.write((uint8_t)kGlyphBirdTopWing);    
    lcd.setCursor(col + 4, 1);
    lcd.write((uint8_t)kGlyphBirdMidTop);    
  } else {
    lcd.setCursor(col + 3, 0);
    lcd.write((uint8_t)kGlyphBirdTail);
    lcd.setCursor(col + 4, 0);
    lcd.write((uint8_t)kGlyphBirdMidBottom);    
    lcd.setCursor(col + 4, 1);
    lcd.write((uint8_t)kGlyphBirdBottomWing);    
  }

  lcd.setCursor(col + 5, show_top_wing ? 1 : 0);
  lcd.write((uint8_t)kGlyphBirdMidRight);
  lcd.write((uint8_t)kGlyphBirdBeak);
}

void handle_heart_rate(bool reset, int data)
{
  static int bird_position = 0;
  
  if (reset) {
    create_bird();
  }

  Serial.print("\nBPM: ");
  Serial.print(data);
  
  show_bird_with_heart_rate(bird_position, data);
  if (++bird_position >= 10) {
    bird_position = 0;
  }

  if (data > 90) {
    Serial.print(" - FWD");
    digitalWrite(forwardPin, HIGH);
    digitalWrite(reversePin, LOW);
    g_last_state = kAppStateMovingForwards;
  } else if (data > 80) {        
    Serial.print(" - REV");
    digitalWrite(forwardPin, LOW);
    digitalWrite(reversePin, HIGH);
    g_last_state = kAppStateMovingBack;
  } else {
    Serial.print(" - OFF");
    digitalWrite(forwardPin, LOW);
    digitalWrite(reversePin, LOW);
    g_last_state = kAppStateIdle;
  }
}

void timerCallback()
{
  static int eye_count = kOpenCountMax;

  if (!g_valid_bpm_count) {
    if (!--eye_count) {
      g_eye_closed = !g_eye_closed;
      if (g_eye_closed) {
        eye_count = kClosedCount;
      } else {
        eye_count = random(kOpenCountMin, kOpenCountMax+1);
      }
  
      g_toggle_eyes_flag = true;
    }
  } else if (!--g_valid_bpm_count) {
    g_show_eyes_flag = true;    
  }
}

void loop()
{
  if (QS == true) {    
    QS = false;

    // (re)load the bird glyphs if we were showing the eyes
    bool reload_bird_glyphs = (0 == g_valid_bpm_count);    
    handle_heart_rate(reload_bird_glyphs, BPM);

    // wait N timer ticks before going back to eyes
    g_valid_bpm_count = 5;
    
    // reset eye display in case these got set since the last loop just before a HR was detected
    g_show_eyes_flag = false;
    g_toggle_eyes_flag = false;      
  } else if (g_show_eyes_flag) {
    g_show_eyes_flag = false;
    
    lcd.clear();
    create_and_show_open_eyes();
    
    if (g_last_state != kAppStateIdle) {
      digitalWrite(forwardPin, LOW);
      digitalWrite(reversePin, LOW);
      g_last_state = kAppStateIdle;
    }
  } else if (g_toggle_eyes_flag) {
    g_toggle_eyes_flag = false;
    
    if (g_eye_closed) {
      lcd.createChar(3, eye_ll_closed);
      lcd.createChar(4, eye_bot_closed);
      lcd.createChar(5, eye_lr_closed);
    } else {
      lcd.createChar(3, eye_ll);
      lcd.createChar(4, eye_bot);
      lcd.createChar(5, eye_lr);
    }
    
    lcd.setCursor(4,1);
    lcd.write((uint8_t)kGlyphEyeLL);
    lcd.write((uint8_t)kGlyphEyeBottom);
    lcd.write((uint8_t)kGlyphEyeLR);
  
    lcd.setCursor(9,1);
    lcd.write((uint8_t)kGlyphEyeLL);
    lcd.write((uint8_t)kGlyphEyeBottom);
    lcd.write((uint8_t)kGlyphEyeLR);
  }
  
  delay(20);                             //  take a break
}

#else

void ledFadeToBeat()
{
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
}
  
void sendDataToProcessing(char symbol, int data )
{
    Serial.print(symbol);                // symbol prefix tells Processing what type of data is coming
    Serial.println(data);                // the data to send culminating in a carriage return
}

void loop()
{
  sendDataToProcessing('S', Signal);   // send Processing the raw Pulse Sensor data

  if (QS == true) {                    // Quantified Self flag is true when arduino finds a heartbeat
      fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
      sendDataToProcessing('B',BPM);   // send heart rate with a 'B' prefix
      sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
      QS = false;                      // reset the Quantified Self flag for next time          
  } 

  ledFadeToBeat();
  
  delay(20);                             //  take a break
}

#endif


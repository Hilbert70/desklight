

// Include Libraries
#include "Arduino.h"
#include "Encoder.h"
#include "Button.h"
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

// Pin Definitions
#define ROTARYENCI_PIN_CLK	3
#define ROTARYENCI_PIN_D	2
#define ROTARYENCI_PIN_S1	4

// number of elements in the bar, for testing just 3
#define MAXBAR 3
#define MAXMENU 3
//#define MAXLIGHT 175
#define MAXLIGHT 2048

// object initialization
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Encoder rotaryEncI(ROTARYENCI_PIN_D,ROTARYENCI_PIN_CLK);
Button rotaryEncIButton(ROTARYENCI_PIN_S1);

// 0 dimmer status
// 1 bar length, minimum = 1
// 2 bar position
long status[3] = {10,1,1};

// define vars for testing menu
const int timeout = 5000;       //define timeout of 5 sec
int menu = 0;
long time0;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup() 
{
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages
    Serial.begin(9600);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("start");
    
    rotaryEncIButton.init();
    pinMode(ROTARYENCI_PIN_S1, INPUT_PULLUP);

    pwm.begin();
    // In theory the internal oscillator is 25MHz but it really isn't
    // that precise. You can 'calibrate' by tweaking this number till
    // you get the frequency you're expecting!
    pwm.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
    pwm.setPWMFreq(1600);  // This is the maximum PWM frequency

    // if you want to really speed stuff up, you can go into 'fast 400khz I2C' mode
    // some i2c devices dont like this so much so if you're sharing the bus, watch
    // out for this!
    //Wire.setClock(400000);

    rotaryEncI.write(status[0]*4);
}
long rotaryEncOldPosition = 0;

// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop() 
{
  long rotaryEncINewPosition = rotaryEncI.read();
  long rotaryEncNewPosition = rotaryEncINewPosition/4;  // save the origial one 
  bool rotaryEncIButtonVal = rotaryEncIButton.onPress();

  
  if (rotaryEncIButtonVal) {
        menu = (menu+1) % MAXMENU;
        time0 = millis();
        rotaryEncI.write(status[menu]*4);
        rotaryEncNewPosition = status[menu];
  }
  
  switch (menu){
    case 0: // just dim
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
            rotaryEncI.write(0);
        }
        if (rotaryEncNewPosition > MAXLIGHT) {
            rotaryEncNewPosition = MAXLIGHT;
            rotaryEncI.write(4*MAXLIGHT);
        }
        break;
    case 1: // start
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
            rotaryEncI.write(0);
        }
        if (rotaryEncNewPosition > MAXBAR-1) {
            rotaryEncNewPosition = MAXBAR-1;
            rotaryEncI.write(4*(MAXBAR-1));
        }
        break;
    case 2: //length
        if (rotaryEncNewPosition < 1) {
            rotaryEncNewPosition = 1;
            rotaryEncI.write(1*4);
        }
        if (rotaryEncNewPosition > MAXBAR) {
            rotaryEncNewPosition = MAXBAR;
            rotaryEncI.write(MAXBAR*4);
        }
        break;
  }
  
  if (rotaryEncNewPosition != status[menu]) {
        status[menu] = rotaryEncNewPosition;
        time0 = millis(); // don't go out of the menu
  //}
  //if (rotaryEncINewPosition != status[menu] || rotaryEncIButtonVal) {
        Serial.print(F("Pos: "));
        Serial.print(rotaryEncNewPosition);
        Serial.print(F("\tButton1: "));
        Serial.print(rotaryEncIButtonVal);
        //Serial.print(F("\tButton2: "));
        //Serial.print(rotaryEncIButtonVal);
        Serial.print(F("\tmenu: "));
        Serial.print(menu);
        Serial.print(F("\tstatus[0] (dim): "));
        Serial.print(status[0]);
        Serial.print(F("\tstatus[1] (start): "));
        Serial.print(status[1]);
        Serial.print(F("\tstatus[2] (lengtg): "));
        Serial.print(status[2]);

        Serial.print(F("\tMenus: "));
        Serial.println(menu);
    }
    if (millis() - time0 > timeout) {
        menu = 0; // just dim
        rotaryEncI.write(status[0]*4);
        rotaryEncNewPosition = status[0];
    }

    // Drive each PWM in a 'wave'
    for (int i =0 ; i<MAXBAR ; i++ ){
        if (i<status[1] || i>=status[1]+status[2]) {
            // before the start or after start + length efferyting is off
            pwm.setPWM(i, 0, 0 );
        } else {
//            pwm.setPWM(i, 0, long(square(status[0])/8) % 4096 );
            pwm.setPWM(i, 0, abs(status[0]) % 4096 );
        }
    }
}

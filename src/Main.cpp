// Include Libraries
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include "Button.h"
#include <ErriezRotaryFullStep.h>

#define PIN_A   D5 //ky-040 clk pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIN_B   D6 //ky-040 dt  pin,             add 100nF/0.1uF capacitors between pin & ground!!!
#define BUTTON  D7 //ky-040 sw  pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!

int16_t position = 0;

RotaryFullStep encoder(PIN_A, PIN_B);
Button rotaryButton(BUTTON);

// number of elements in the bar, for testing just 3
#define MAXBAR 3
#define MAXMENU 3
//#define MAXLIGHT 175
#define MAXLIGHT 2048

// object initialization
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// 0 dimmer status
// 1 bar length, minimum = 1
// 2 bar position
long status[3] = {50,1,1};

// define vars for testing menu
const int timeout = 5000;       //define timeout of 5 sec
int menu = 0;
long time0;

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup() 
{
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages

    encoder.setSensitivity(125); 
    rotaryButton.init();

    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("start");
   
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

    //encoder.setPosition(status[0]);
}
long rotaryEncOldPosition = status[0];
long rotaryEncNewPosition = status[0];
// Main logic of your circuit. It defines the interaction between the components you selected. After setup, it runs over and over again, in an eternal loop.
void loop() 
{
  long rotaryEncINewPosition = encoder.read(); //rotaryEncI.read();
  //long rotaryEncNewPosition; // = rotaryEncINewPosition;  // save the origial one 
  bool rotaryEncIButtonVal = rotaryButton.onPress();

  //Serial.print("Position: ");
  //Serial.println(rotaryEncINewPosition);
  if (rotaryEncINewPosition == 0) {
      // no change
  } else if (abs(rotaryEncINewPosition) >= 2) {
    rotaryEncNewPosition += rotaryEncINewPosition * 2;
  } else {
    rotaryEncNewPosition += rotaryEncINewPosition;
  }


  if (rotaryEncIButtonVal) {
        menu = (menu+1) % MAXMENU;
        time0 = millis();
        //rotaryEncI.setPosition(status[menu]*4);
        rotaryEncNewPosition = status[menu];
        Serial.print(F("\tmenu: "));
        Serial.println(menu);
  }
  
  switch (menu){
    case 0: // just dim
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
            //encoder.setPosition(0);
        }
        if (rotaryEncNewPosition > MAXLIGHT) {
            rotaryEncNewPosition = MAXLIGHT;
            //encoder.setPosition(MAXLIGHT);
        }
        break;
    case 1: // start
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
            //encoder.setPosition(0);
        }
        if (rotaryEncNewPosition > MAXBAR-1) {
            rotaryEncNewPosition = MAXBAR-1;
            //encoder.setPosition((MAXBAR-1));
        }
        break;
    case 2: //length
        if (rotaryEncNewPosition < 1) {
            rotaryEncNewPosition = 1;
            //encoder.setPosition(1);
        }
        if (rotaryEncNewPosition > MAXBAR) {
            rotaryEncNewPosition = MAXBAR;
            //encoder.setPosition(MAXBAR);
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
        //encoder.setPosition(status[0]);
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

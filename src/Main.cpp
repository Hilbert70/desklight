// Include Libraries
#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include "Button.h"
#include "Menu.h"
#include <ErriezRotaryFullStep.h>

#include "ssidandpassword.h"

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

#define PIN_A   D5 //ky-040 clk pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIN_B   D6 //ky-040 dt  pin,             add 100nF/0.1uF capacitors between pin & ground!!!
#define BUTTON  D7 //ky-040 sw  pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIXEL_PIN    2 // D4 

// number of elements in the bar, for testing just 3
#define MAXBAR 16
#define MAXMENU 4
#define MAXMODES 6
#define MAXLIGHT 4095

#define ST_DIM 0
#define ST_START 1
#define ST_LENGTH 2
#define ST_LEDS 3

uint32_t menuColours[] ={0x000000,0x002200,0x220022,0x000022,};

Adafruit_NeoPixel pixels(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
RotaryFullStep encoder(PIN_A, PIN_B);
Button rotaryButton(BUTTON);
Menu barMenu(MAXMENU,pixels,menuColours);


// object initialization
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x41);

// 0 dimmer status; ST_DIM
// 1 bar length, minimum = 1; ST_LENGTH
// 2 bar position; ST_START
long status[4] = {50,1,1,1};

// define vars for testing menu
const int timeout = 5000;       //define timeout of 5 sec
long time0;
long rotaryEncOldPosition = status[0];
long rotaryEncNewPosition = status[0];

void updateLED(long what[])
{
    
    // only update when we have a change
    // Drive each PWM in a 'wave'
    for (int i =0 ; i<MAXBAR ; i++ ){
        if (i<what[ST_START] || i>=what[ST_START]+what[ST_LENGTH]) {
            // before the start or after start + length efferyting is off
            pwm.setPWM(i, 0, 0 );
        } else {
            pwm.setPWM(i, 0, 4095); // testing pwm1
        }
    }
    // Should become colour temperature
    switch (what[ST_LEDS]) {
    case 0: // warm white is on (1)
        pwm1.setPWM(0, 0,  0  );
        pwm1.setPWM(1, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(2, 0,  0  );
        break;
    case 1: // warm white and white is on (0 + 1)
        pwm1.setPWM(0, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(1, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(2, 0,  0  );
        break;
    case 2: //  white is on (0)
        pwm1.setPWM(0, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(1, 0,  0  );
        pwm1.setPWM(2, 0,  0  );
        break;
    case 3: // white and cool whilte s on (0 2)
        pwm1.setPWM(0, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(1, 0,  0  );
        pwm1.setPWM(2, 0,  abs(what[0]) % 4096  );
        break;
    case 4: // warm white is on (2)
        pwm1.setPWM(0, 0,  0  );
        pwm1.setPWM(1, 0,  0  );
        pwm1.setPWM(2, 0,  abs(what[0]) % 4096  );
        break;
    case 5: // all ar on
        pwm1.setPWM(0, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(1, 0,  abs(what[0]) % 4096  );
        pwm1.setPWM(2, 0,  abs(what[0]) % 4096  );
        break;
    }
}




// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup() 
{
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages

    encoder.setSensitivity(125); 
    rotaryButton.init();

    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("Booting deskLight 0.3");
    // Hostname defaults to esp8266-[ChipID]
    // later the hostname comes from the web page
    WiFi.hostname("desklight");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    
    

    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else { // U_FS
            type = "filesystem";
        }

        // NOTE: if updating FS this would be the place to unmount FS using FS.end()
        Serial.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            Serial.println("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) {
            Serial.println("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            Serial.println("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            Serial.println("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            Serial.println("End Failed");
        }
    });

    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    pwm.begin();
    pwm1.begin();
    // In theory the internal oscillator is 25MHz but it really isn't
    // that precise. You can 'calibrate' by tweaking this number till
    // you get the frequency you're expecting!
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(800);  // This is the maximum PWM frequency
    pwm1.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
    pwm1.setPWMFreq(800);  // This is the maximum PWM frequency
    
    
    barMenu.setup();
    updateLED(status);
}    

void loop() 
{
    long rotaryEncINewPosition = encoder.read(); //rotaryEncI.read();
    bool rotaryEncIButtonVal = rotaryButton.onPress();
    int  menu;
    int  sign;

    if (rotaryEncINewPosition == 0) {
        // no change
    //} else if (abs(rotaryEncINewPosition) >= 2) {
    //    rotaryEncNewPosition += rotaryEncINewPosition * 2;
    } else {
        menu = barMenu.getState();
        
        if (menu == ST_DIM) {
            sign = 1;
            if (rotaryEncINewPosition <0) {
                sign = -1;
            }
            if (rotaryEncNewPosition < 10) {
                rotaryEncINewPosition = 1;
            } else if (rotaryEncNewPosition >= 10 && rotaryEncNewPosition < 100 ) {
                rotaryEncINewPosition = 5;
            } else if (rotaryEncNewPosition >= 100 && rotaryEncNewPosition < 1000 ) {
                rotaryEncINewPosition = 10;
            } else {
                // larger that 1000
                rotaryEncINewPosition = 100;
            }
            rotaryEncINewPosition *= sign;
        }
        rotaryEncNewPosition += rotaryEncINewPosition;
    }

    if (rotaryEncIButtonVal) {
        barMenu.setState(barMenu.getState() +1);
        time0 = millis();
        menu = barMenu.getState();
        rotaryEncNewPosition = status[menu];
        Serial.print(F("\tmenu: "));
        Serial.println(menu);
    }
  
    switch (barMenu.getState()){
    case ST_DIM: // just dim
        if (rotaryEncNewPosition < 1) {
            rotaryEncNewPosition = 1;
        }
        if (rotaryEncNewPosition > MAXLIGHT) {
            rotaryEncNewPosition = MAXLIGHT;
        }
        break;
    case ST_START: // start
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
        }
        if (rotaryEncNewPosition > MAXBAR-1) {
            rotaryEncNewPosition = MAXBAR-1;
        }
        break;
    case ST_LENGTH: //length
        // the decrement/increment is rotaryEncINewPosition
        // length can not be lower than 1 or higher than maxbar
        if (rotaryEncNewPosition < 1) {
            rotaryEncNewPosition = 1;
        }
        if (rotaryEncNewPosition > MAXBAR) {
            rotaryEncNewPosition = MAXBAR;
            // start should be 0
            status[ST_START] = 0;
        }
        // and now for the smart stuff
        if (rotaryEncNewPosition % 2 == 1) {
            // decrease start by rotaryEncINewPosition
            status[ST_START] -= rotaryEncINewPosition;
        }
        if (status[ST_START] < 0) {
            // if start is reached lenght should be increased
            status[ST_START] = 0;
            rotaryEncNewPosition ++;
            if (rotaryEncNewPosition > MAXBAR) {
                rotaryEncNewPosition = MAXBAR;
            }
        }
        if (status[ST_START] + rotaryEncNewPosition > MAXBAR) {
            // move start back if end is reached
            status[ST_START] = MAXBAR -rotaryEncNewPosition -1;
        }
        break;
    case ST_LEDS: // bar while mode
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = MAXMODES-1;
        }
        if (rotaryEncNewPosition >= MAXMODES) {
            rotaryEncNewPosition = 0;
        }
        break;
    }
  
    menu = barMenu.getState();
    if (rotaryEncNewPosition != status[menu]) {
        status[menu] = rotaryEncNewPosition;
        time0 = millis(); // don't go out of the menu
        Serial.print(F("Pos: "));
        Serial.print(rotaryEncNewPosition);
        Serial.print(F("\tButton1: "));
        Serial.print(rotaryEncIButtonVal);
        Serial.print(F("\tmenu: "));
        Serial.print(menu);
        Serial.print(F("\tstatus[ST_DIM] (dim): "));
        Serial.print(status[ST_DIM]);
        Serial.print(F("\tstatus[ST_START] (start): "));
        Serial.print(status[ST_START]);
        Serial.print(F("\tstatus[ST_LENGTH] (length): "));
        Serial.print(status[ST_LENGTH]);
        Serial.print(F("\tstatus[ST_LEDS] (white temp): "));
        Serial.print(status[ST_LEDS]);

        Serial.print(F("\tMenus: "));
        Serial.println(menu);

        updateLED(status);
    }

    if (millis() - time0 > timeout) {
        barMenu.setState( 0 ); // just dim
        rotaryEncNewPosition = status[0];
    }

    
    ArduinoOTA.handle();
}

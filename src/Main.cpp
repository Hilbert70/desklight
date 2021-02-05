// Include Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>

#include <WiFi.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include "Button.h"
#include "Menu.h"
#include <ErriezRotaryFullStep.h>
#include <ESPASyncWebServer.h>

#include "sweeprom.h"
#include "functions.h"

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

// for esp 32 use gpio25, 26,27 and 14
//                       original esp8266 pins
#define PIN_A      25 // D5 yellow    clk pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!
#define PIN_B      26 // D6 blue      dt  pin,             add 100nF/0.1uF capacitors between pin & ground!!!
#define BUTTON     27 // D7 green     sw  pin, interrupt & add 100nF/0.1uF capacitors between pin & ground!!!

#define PIXEL_PIN  14 // D4 Blue      datapin of the neo pixel 

#define DESKLIGHT_VERSION "3.0"

//#define DEBUG
#undef DEBUG

const char* PARAM_HOSTNAME = "hostname";
const char* PARAM_SSID     = "ssid";
const char* PARAM_PSK      = "psk";

const char* PARAM_START    = "start";
const char* PARAM_LENGTH   = "length";
const char* PARAM_DIM      = "dim";
const char* PARAM_COLOUR   = "colour";

uint32_t menuColours[] ={0x000000,0x002200,0x220022,0x000022,};

Adafruit_NeoPixel pixels(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
RotaryFullStep encoder(PIN_A, PIN_B);
Button rotaryButton(BUTTON);
Menu barMenu(MAXMENU,pixels,menuColours);


// object initialization
// called this way, it uses the default address 0x40
// gpio22 i2c scl D1 blue
// gpio21 i2c sda D2 white
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x41);

// define vars for testing menu
const int timeout = 5000;       //define timeout of 5 sec
long time0;
boolean doneTimeout = false;
boolean doneTimeout0 = false;
long lastMenuUpdate;
long timeButton;
long rotaryEncOldPosition;
long rotaryEncNewPosition;


long     breath_time;
uint16_t breath_delay = 10;
uint16_t breath_lum = 14;
bool     breath_inc = true;
bool     breath_start = false;

SWEeprom eepromdata;

String apList;
AsyncWebServer server(80);

void handleRoot(AsyncWebServerRequest *request);
void handleRootAP(AsyncWebServerRequest *request);
void handleOff(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
// upd stuff
void setupAPmode();

void updateLED(long what[])
{
    if(what[ST_LEDS] != -1) {
        // back to on state
        if (breath_start) barMenu.bypassMenu(255,0);
        breath_start = false;
    }

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
    case -1: // all ar off
        pwm1.setPWM(0, 0,  0  );
        pwm1.setPWM(1, 0,  0  );
        pwm1.setPWM(2, 0,  0  );
        break;    
    }
}

// dirty, very dirty!!!
void updateGlobals(long start,long length, long brightness, long colour)
{
    switch (barMenu.getState()){
    case ST_DIM: // just dim
        rotaryEncNewPosition = brightness;      
        break;
    case ST_START: // start'
        rotaryEncNewPosition = start; 
        break;
    case ST_LENGTH: //length
        rotaryEncNewPosition = length; 
        break;
    case ST_LEDS: // bar while mode
        rotaryEncNewPosition = colour; 
        break;
    }
}

// Setup the essentials for your circuit to work. It runs first every time your circuit is powered with electricity.
void setup() 
{
    int i;
    long * status;
    char * hostname;
    char * ssid;
    char * psk;
    bool inSTAmode;

    uint32_t chipID = 0;
    for(int i=0; i<17; i=i+8) {
        chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    
    // Setup Serial which is useful for debugging
    // Use the Serial Monitor to view printed messages

    encoder.setSensitivity(125); 
    rotaryButton.init();

    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("Booting deskLight " DESKLIGHT_VERSION);
    // Hostname defaults to esp8266-[ChipID]
    // later the hostname comes from the web page
    Serial.printf(" ESP32 Chip id = %08X\n", chipID);
    
    status = eepromdata.init();
    if (eepromdata.errorCode != ESP_OK) {
        Serial.println(eepromdata.errorMessage);
    }
    hostname = eepromdata.getHostname();
    ssid     = eepromdata.getSSID();
    psk      = eepromdata.getPSK();

    if (strlen(hostname) == 0) {
        sprintf(hostname,"ESP_%08X",chipID);
        eepromdata.write();
    }
    
    // debug stuff
    if (strlen(hostname) !=0 ){
        Serial.printf(" Hostname '%s'\n", hostname);
    }
    if (strlen(ssid) !=0 ){
        Serial.printf(" SSID '%s'\n",ssid); 
    }
    if (strlen(psk) !=0 ){
        Serial.printf(" PSK '***'\n");// not going to show ;-)
    }

    Serial.printf(" Hostname length = %d\n", strlen(eepromdata.getHostname()));
    Serial.printf(" ssid length = %d\n", strlen(eepromdata.getSSID()));
    Serial.printf(" psk length = %d\n", strlen(eepromdata.getPSK()));

    Serial.print("Eeprom data: ");
    for (i=0; i<4; i++) {
        Serial.print(status[i]);
        Serial.print(" ");
    }
    Serial.println();

    inSTAmode = true;
    
    if (strlen(ssid)!=0 && strlen(psk) != 0) {
        WiFi.mode(WIFI_STA);
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(hostname);
        WiFi.begin(ssid, psk);
        Serial.println("Waiting for Wifi to connect"); 
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
            delay(500);
            inSTAmode = false;
            break;
        }
    } else {
        inSTAmode = false;
    }

    if (inSTAmode) {
        server.on("/",  handleRoot);
        server.on("/settings", HTTP_POST, handleRootAP);
        server.on("/off",  handleOff);
    } else {
        setupAPmode();
        server.on("/",  HTTP_POST, handleRootAP);
    }
        
    server.onNotFound(handleNotFound);
    server.begin();

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
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(1600);  // This is the maximum PWM frequency
    pwm1.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
    pwm1.setPWMFreq(1600);  // This is the maximum PWM frequency
    
    rotaryEncOldPosition = status[0];
    rotaryEncNewPosition = status[0];

    barMenu.setup();
    updateLED(status);
}    

void loop() 
{
    long rotaryEncINewPosition = encoder.read(); //rotaryEncI.read();
    bool rotaryEncIButtonVal = rotaryButton.onChange();
    int  menu;
    int  sign;
    long * status;
    
    status = eepromdata.getStatus();

    if (rotaryEncINewPosition != 0) {
        // back to on state
        if (breath_start) barMenu.bypassMenu(255,0);
        breath_start = false;

        lastMenuUpdate = millis();
        menu = barMenu.getState();
        sign = 1;
        if (rotaryEncINewPosition <0) {
            sign = -1;
        }
        if (menu == ST_DIM) {
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
        } else {
            // Else do only one decrease or increase, high is not needed
            rotaryEncINewPosition = sign;
        }
        rotaryEncNewPosition += rotaryEncINewPosition;
    }
    if (rotaryEncIButtonVal && rotaryButton.getState() ==HIGH) {
        Serial.print(F("\ttime: "));
        Serial.println(millis()-timeButton);
    }

    if (rotaryEncIButtonVal && rotaryButton.getState() ==LOW) {
        // back to on state
        if (breath_start) barMenu.bypassMenu(255,0);
        breath_start = false;

        lastMenuUpdate = millis();
        barMenu.setState(barMenu.getState() +1);
        time0 = millis();
        doneTimeout = false;
        doneTimeout0 = false;        
        timeButton = millis();
        menu = barMenu.getState();
        rotaryEncNewPosition = status[menu];
        Serial.print(F("\ttime: "));
        Serial.print(millis()-timeButton);
        Serial.print(F("\tmenu: "));
        Serial.println(menu);
    }

    //Serial.println(rotaryButton.onChange());

    menu = barMenu.getState();
    switch (menu){
    case ST_DIM: // just dim
        if (rotaryEncNewPosition < 1) {
            rotaryEncNewPosition = 1;
        }
        if (rotaryEncNewPosition > MAXLIGHT) {
            rotaryEncNewPosition = MAXLIGHT;
        }
        break;
    case ST_START: // start
        // start should do something with length
        if (rotaryEncNewPosition < 0) {
            rotaryEncNewPosition = 0;
        }
        if (rotaryEncNewPosition > MAXBAR-status[ST_LENGTH]) {
            rotaryEncNewPosition = MAXBAR-status[ST_LENGTH];
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
            eepromdata.setStatus(ST_START,0);
        }
        // and now for the smart stuff
        if (rotaryEncNewPosition % 2 == 1) {
            // decrease start by rotaryEncINewPosition
            eepromdata.decStatus(ST_START, rotaryEncINewPosition);
        }
        if (status[ST_START] < 0) {
            // if start is reached lenght should be increased
            eepromdata.setStatus(ST_START,  0);
            rotaryEncNewPosition ++;
            if (rotaryEncNewPosition > MAXBAR) {
                rotaryEncNewPosition = MAXBAR;
            }
        }
        if (status[ST_START] + rotaryEncNewPosition > MAXBAR) {
            // move start back if end is reached
            eepromdata.setStatus(ST_START , MAXBAR -rotaryEncNewPosition);
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
  
    
    if (rotaryEncNewPosition != status[menu]) {
        // back to on state
        if (breath_start) barMenu.bypassMenu(255,0);
        breath_start = false;

        eepromdata.setStatus(menu , rotaryEncNewPosition);
        time0 = millis(); // don't go out of the menu
        doneTimeout = false;
#ifdef DEBUG        
        Serial.print(F("Pos: "));
        Serial.print(rotaryEncNewPosition);
        Serial.print(F(" Button1: "));
        Serial.print(rotaryEncIButtonVal);
        Serial.print(F(" menu: "));
        Serial.print(menu);
        Serial.print(F(" (dim): "));
        Serial.print(status[ST_DIM]);
        Serial.print(F(" (start): "));
        Serial.print(status[ST_START]);
        Serial.print(F(" (length): "));
        Serial.print(status[ST_LENGTH]);
        Serial.print(F(" (white temp): "));
        Serial.print(status[ST_LEDS]);

        Serial.print(F(" Menus: "));
        Serial.println(menu);
#endif
        updateLED(status);
    }

    if (millis() - time0 > timeout && !doneTimeout0) {
        barMenu.setState( 0 ); // just dim
        rotaryEncNewPosition = status[0];
        doneTimeout0= true;
    }
    if (millis() - lastMenuUpdate > timeout && !eepromdata.written() && !doneTimeout) {
        // after timeout no actions (5 sec), update the eeprom.
        // The eeprom only writes the changes
        Serial.println("Write eeprom");
        eepromdata.write();
        doneTimeout= true;
    }
    // breathe test

    if (breath_start && ( millis()- breath_time) > (breath_delay/2)) {
        if (breath_inc) {
            breath_lum+=1;
        } else {
            breath_lum-=1;
        }

        barMenu.bypassMenu(breath_lum,0x0f0f4f);
        breath_time = millis();

        if (breath_inc && breath_lum >=255) {
            breath_inc = false;
        }
        if (!breath_inc && breath_lum <=15) {
            breath_inc = true;
        }
        if(breath_lum <= 15) breath_delay = 400; // 970 pause before each breath
        else if(breath_lum <=  25) breath_delay = 38; // 19
        else if(breath_lum <=  50) breath_delay = 36; // 18
        else if(breath_lum <=  75) breath_delay = 28; // 14
        else if(breath_lum <= 100) breath_delay = 20; // 10
        else if(breath_lum <= 125) breath_delay = 14; // 7
        else if(breath_lum <= 150) breath_delay = 11; // 5
        else breath_delay = 10; // 4

    }
    ArduinoOTA.handle();
}

void setupAPmode()
{
    char * hostname;
    // start in AP mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    apList = "<datalist id=\"apList\">";
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        for (int i=0; i<n; i++) {
            Serial.print("SSID ");
            Serial.println(WiFi.SSID(i));
            apList += "<option value=\"" + WiFi.SSID(i) +"\">"+ WiFi.SSID(i) +"</option>";
        }
    }
    apList += "</datalist>";
    delay(100);
    hostname = eepromdata.getHostname();
    Serial.printf("SSID '%s'\n",hostname);
    WiFi.setHostname(hostname);
    WiFi.softAP(hostname, "123456789",6); // passfrase must be six or larger!
    Serial.println("softap");
} 

void handleRootAP(AsyncWebServerRequest *request)
{
    String message;
    char * hostname = eepromdata.getHostname();
    bool hadPSK      = false;
    String error = "";

    if (request->hasParam(PARAM_HOSTNAME) &&
        request->hasParam(PARAM_SSID)     &&
        request->hasParam(PARAM_PSK))
    {
        eepromdata.setHostname(request->getParam(PARAM_HOSTNAME)->value());
        eepromdata.setSSID(request->getParam(PARAM_SSID)->value());
        if (request->getParam(PARAM_PSK)->value().length()>7 ) {
            hadPSK= true;
            eepromdata.setPSK(request->getParam(PARAM_PSK)->value());
        } else {
                error += "<p style=\"color:red;\">password has to be longer than 7 characters</p>";
        }

    }
    if (hadPSK) {
        Serial.println("Writing eeprom.");
        eepromdata.write();
        Serial.print("Written ");
        Serial.print(eepromdata.errorMessage);
        delay(500);
        Serial.println("Rebooting into new wifi!");
        ESP.restart();

    }
    message  = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    message += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
    message += ".container { display: flex; justify-content: center;}";
    message += "input {font-size: 20px; width: 100px ;align-self: flex-end;}";
    message += "form {text-align: right;}";
    message += "</style></head><body>";
            
    message += "<h1>ESP32 Web Server</h1>";
    message += "<body><h3>Alter " + String(hostname) +"</h3>";
    message += error;
    message += "<div class=\"container\">";
    message += "<form method='post'>";
    message += "<label>Hostname: </label><input name='hostname' type='text' length='32' value='"+String(hostname)+"'/><br />";
    message += "<label>SSID: </label><input name='ssid' length='32' value='"+String(eepromdata.getSSID())+"' list=\"apList\"/><br />";
    message += apList;
    message += "<label>Password: </label><input name='psk' type='password' length='64'/><br />";
    message += "<input type='submit'>";
    message += "</form></div>";
    message += "</body></html>";
    request->send(200, "text/html", message);
}

void handleRoot(AsyncWebServerRequest *request)
{
    String message;
    long * status = eepromdata.getStatus();
    char * hostname = eepromdata.getHostname();

    long start =0, length=0, dim=0, colour=0;

    int i;
    int params  = request->params();
    Serial.println("Request");
    for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            Serial.println(" _POST[" + p->name() +"]: " +  p->value());
        } else {
            Serial.println( " _GET[" + p->name() + "]: " + p->value());
        }
    }
    
    if (request->hasParam(PARAM_START, true)  &&
        request->hasParam(PARAM_LENGTH, true) &&
        request->hasParam(PARAM_DIM, true)    &&
        request->hasParam(PARAM_COLOUR, true))
    {
        Serial.println("Handle it");
        start = StrtoLong( request->getParam(PARAM_START, true)->value() );
        handleStart(&start,length);
        length = StrtoLong( request->getParam(PARAM_LENGTH, true)->value() );
        handleLength(&start,&length,0);
        dim = StrtoLong( request->getParam(PARAM_DIM, true)->value() );
        handleBrightness(&dim);
        colour    = StrtoLong( request->getParam(PARAM_COLOUR, true)->value() );
        handleColour(&colour);
        Serial.print("Start: ");
        Serial.println(start);
        Serial.print("Length: ");
        Serial.println(length);
        Serial.print("dim: ");
        Serial.println(dim);
        Serial.print("colour: ");
        Serial.println(colour);
        // back to on state
        if (breath_start) barMenu.bypassMenu(255,0);
        breath_start = false;

        eepromdata.setStatus(ST_START,start);
        eepromdata.setStatus(ST_LENGTH,length);
        eepromdata.setStatus(ST_DIM,dim);
        eepromdata.setStatus(ST_LEDS,colour);
        eepromdata.write();
        updateLED(status);
        
        // AUCH, update the global variable!!!
        updateGlobals(start,length,dim,colour);
    }

    message  = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    message += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
    message += ".container { display: flex; justify-content: center;}";
    message += "input {font-size: 20px; width: 100px ;align-self: flex-end;}";
    message += "form {text-align: right;}";
    message += "</style></head><body>";
            
    message += "<h1>ESP32 Web Server</h1>";
    message += "<h3>" + String(hostname) + "<br />Version: " + DESKLIGHT_VERSION +"</h3>";
    message += "<a href='settings'>Settings</a><br /><br />";
    message += "<a href='off'>Soft off</a><br /><br />";
    message += "<div class=\"container\">";
    message += "<form method='post'>";
    message += "<label>Start: </label><input name='start' length='2' type='number' min='0' max='"+String(MAXBAR-1)+"' value='"+String(status[ST_START])+"'/><br />";
    message += "<label>Length: </label><input name='length' length='2' type='number' min='1' max='"+String(MAXBAR)+"' value='"+String(status[ST_LENGTH])+"'/><br />";
    message += "<label>Brightness: </label><input name='dim' length='4' type='number' min='1' max='"+String(MAXLIGHT)+"' value='"+String(status[ST_DIM])+"'/><br />";
    message += "<label>Colour: </label><input name='colour' length='2' type='number' min='0' max='"+String(MAXMODES-1)+"' value='"+String(status[ST_LEDS])+"'/><br />";
    message += "<input type='submit'>";
    message += "</form></div></body></html>";
    request->send(200, "text/html", message);
}

void handleNotFound(AsyncWebServerRequest *request)
{
    String message = "File Not Found\n\n";
    int i;
    int params = request->params();
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += params;
    message += "\n";
    for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
            message += " _POST[" + p->name() +"]: " +  p->value() + "\n";
        } else {
            message += " _GET[" + p->name() + "]: " + p->value() + "\n";
        }
    }
    request->send(404, "text/plain", message);
}

void handleOff(AsyncWebServerRequest *request)
{
    long offStatus[4];
    int i;
    for (i=0; i<4 ; i++){
        offStatus[i] = 0;
    }
    offStatus[ST_LEDS]= -1;
    updateLED(offStatus);
    breath_start = true;
    request->redirect("/");
}
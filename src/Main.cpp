// Include Libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>

#include <WiFi.h>
#include <AsyncUDP.h>

#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <Wire.h>
#include "Button.h"
#include "Menu.h"
#include <ErriezRotaryFullStep.h>
#include <WebServer.h>

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

#define DESKLIGHT_VERSION "2.1"

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
long lastMenuUpdate;
long timeButton;
long rotaryEncOldPosition;
long rotaryEncNewPosition;

SWEeprom eepromdata;

String apList;
WebServer server(80);
AsyncUDP udp;

void handleRoot();
void handleRootAP();
void handleNotFound();
// Rest interface
//  api/v1/light
void handlepost();  // post
void handleget();  // get
void handleStartIncPatch();
void handleStartDecPatch();
void handleLengthIncPatch();
void handleLengthDecPatch();
void handleBrightnessIncPatch();
void handleBrightnessDecPatch();
void handleColourIncPatch();
void handleColourDecPatch();
// delete, never going to be supported

void setupAPmode();

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

    if(udp.listen( 54321)) {
        Serial.println("UDP connected");
        udp.onPacket([](AsyncUDPPacket packet) {
            char * hostname = eepromdata.getHostname();
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            
            String response;
            StaticJsonDocument<300> doc;

            doc["hostname"] = hostname;
            doc["apiversion"] = "v1";
            serializeJson(doc, response);
            // check for message size
            // friendly name must be returned and api version
            // create select lamp page, in this page a lamp is selected and stored 
            //reply to the client
            packet.printf(response.c_str());
        });
    }
    if (inSTAmode) {
        server.on("/", handleRoot);
        server.on("/settings", handleRootAP);
        // api functions
        server.on("/api/v1/state", HTTP_GET,  handleget);
        server.on("/api/v1/state", HTTP_POST, handlepost);
        server.on("/api/v1/start/inc",   HTTP_PATCH, handleStartIncPatch);
        server.on("/api/v1/start/dec",   HTTP_PATCH, handleStartDecPatch);
        server.on("/api/v1/length/inc",   HTTP_PATCH, handleLengthIncPatch);
        server.on("/api/v1/length/dec",   HTTP_PATCH, handleLengthDecPatch);
        server.on("/api/v1/brightness/inc",   HTTP_PATCH, handleBrightnessIncPatch);
        server.on("/api/v1/brightness/dec",   HTTP_PATCH, handleBrightnessDecPatch);
        server.on("/api/v1/colour/inc",   HTTP_PATCH, handleColourIncPatch);
        server.on("/api/v1/colour/dec",   HTTP_PATCH, handleColourDecPatch);
    } else {
        setupAPmode();
        server.on("/", handleRootAP);
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
    pwm.setPWMFreq(800);  // This is the maximum PWM frequency
    pwm1.setOscillatorFrequency(27000000);  // The int.osc. is closer to 27MHz
    pwm1.setPWMFreq(800);  // This is the maximum PWM frequency
    
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
    
    server.handleClient();

    status = eepromdata.getStatus();

    if (rotaryEncINewPosition == 0) {
        // no change
    //} else if (abs(rotaryEncINewPosition) >= 2) {
    //    rotaryEncNewPosition += rotaryEncINewPosition * 2;
    } else {
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
        lastMenuUpdate = millis();
        barMenu.setState(barMenu.getState() +1);
        time0 = millis();
        timeButton = millis();
        menu = barMenu.getState();
        rotaryEncNewPosition = status[menu];
        Serial.print(F("\ttime: "));
        Serial.print(millis()-timeButton);
        Serial.print(F("\tmenu: "));
        Serial.println(menu);
    }

    //Serial.println(rotaryButton.onChange());

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
  
    menu = barMenu.getState();
    if (rotaryEncNewPosition != status[menu]) {
        eepromdata.setStatus(menu , rotaryEncNewPosition);
        time0 = millis(); // don't go out of the menu
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

        updateLED(status);
    }

    if (millis() - time0 > timeout) {
        barMenu.setState( 0 ); // just dim
        rotaryEncNewPosition = status[0];
    }
    if (millis() - lastMenuUpdate > timeout && !eepromdata.written()) {
        // after timeout no actions (5 sec), update the eeprom.
        // The eeprom only writes the changes
        Serial.println("Write eeprom");
        eepromdata.write();
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

void handleRootAP()
{
    String message;
    char * hostname = eepromdata.getHostname();
    int  i;
    bool hadHostname = false;
    bool hadSSID     = false;
    bool hadPSK      = false;
    String error = "";

    for (i=0;i<server.args(); i++){
        if (server.argName(i) == "hostname")  {
            hadHostname= true;
            eepromdata.setHostname(server.arg(i));
        }
        if  (server.argName(i) == "ssid"){
            hadSSID= true;
            eepromdata.setSSID(server.arg(i));
        } 
        if (server.argName(i) == "psk") {
            if (server.arg(i).length()>7 ) {
                hadPSK= true;
                eepromdata.setPSK(server.arg(i));
            } else {
                error += "<p style=\"color:red;\">password has to be longer than 7 characters</p>";
            }
        }   
    }
    if (hadHostname && hadSSID && hadPSK) {
        Serial.println("Writing eeprom.");
        eepromdata.write();
        Serial.print("Written ");
        Serial.print(eepromdata.errorMessage);
        delay(500);
        Serial.println("Rebooting into new wifi!");
        ESP.restart();

    }
    message  = "<!DOCTYPE HTML>\r\n<html>";
    message += "<style>body { font: 16px Arial, sans-serif; }</style>";
    message += "<body><h3>Alter " + String(hostname) +"</h3>";
    message += error;
    message += "<form method='post'>";
    message += "<label>Hostname: </label><input name='hostname' type='text' length='32' value='"+String(hostname)+"'/><br />";
    message += "<label>SSID: </label><input name='ssid' length='32' value='"+String(eepromdata.getSSID())+"' list=\"apList\"/><br />";
    message += apList;
    message += "<label>Password: </label><input name='psk' type='password' length='64'/><br />";
    message += "<input type='submit'>";
    message += "</form>";
    message += "</body></html>";
    server.send(200, "text/html", message);
}

void handleRoot()
{
    String message;
    long * status = eepromdata.getStatus();
    char * hostname = eepromdata.getHostname();

    long start =0, length=0, dim=0, colour=0;
    int i;
    bool hadStart  = false;
    bool hadLength = false;
    bool hadDim    = false;
    bool hadColour = false;

    for (i=0;i<server.args(); i++){
        if (server.argName(i) == "start")  {
            hadStart= true;
            start = StrtoLong( server.arg(i) );
            handleStart(&start);
        }
        if  (server.argName(i) == "length"){
            hadLength= true;
            length = StrtoLong( server.arg(i) );
            handleLength(&length);
        } 
        if (server.argName(i) == "dim") {
            hadDim= true;
            dim    = StrtoLong( server.arg(i) );
            handleBrightness(&dim);
        }   
        if (server.argName(i) == "colour") {
            hadColour= true;
            colour    = StrtoLong( server.arg(i) );
            handleColour(&colour);
        }   
    }
    if (hadStart && hadLength && hadDim && hadColour) {
        limitStart(&start, length);
        eepromdata.setStatus(ST_START,start);
        eepromdata.setStatus(ST_LENGTH,length);
        eepromdata.setStatus(ST_DIM,dim);
        eepromdata.setStatus(ST_LEDS,colour);
        eepromdata.write();
        updateLED(status);
        
        // AUCH, update the global variable!!!
        updateGlobals(start,length,dim,colour);
    }


    message  = "<!DOCTYPE HTML>\r\n<html>";
    message += "<style>body { font: 16pt Arial, sans-serif; }</style>";
    message += "<body><h3>Alter " + String(hostname) + "<br />Version: " + DESKLIGHT_VERSION +"</h3>";
    message += "<a href='settings'>Settings</a><br /><br />";
    message += "<form method='post'>";
    message += "<label>Start: </label><input name='start' length='2' type='number' min='0' max='"+String(MAXBAR-1)+"' value='"+String(status[ST_START])+"'/><br />";
    message += "<label>Length: </label><input name='length' length='2' type='number' min='1' max='"+String(MAXBAR)+"' value='"+String(status[ST_LENGTH])+"'/><br />";
    message += "<label>Brightness: </label><input name='dim' length='4' type='number' min='1' max='"+String(MAXLIGHT)+"' value='"+String(status[ST_DIM])+"'/><br />";
    message += "<label>Colour: </label><input name='colour' length='2' type='number' min='0' max='"+String(MAXMODES-1)+"' value='"+String(status[ST_LEDS])+"'/><br />";
    message += "<input type='submit'>";
    message += "</form></body<</html>";
    server.send(200, "text/html", message);
  //}
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// rest api
void handlepost()
{
    StaticJsonDocument<300> doc;
    long * status = eepromdata.getStatus();
    long start =0, length=0, brightness=0, colour=0;
    bool hadStart      = false;
    bool hadLength     = false;
    bool hadBrightness = false;
    bool hadColour     = false;
    String message = "";

    if (server.hasArg("plain") == false) {
    //handle error here
        server.send(400, "application/json", "{}");
        return;
    }
    
    DeserializationError err= deserializeJson(doc, server.arg("plain"));
    if (err) {
        // json error
        message =  "{ \"error\": " + String(err.c_str()) +", \"message\": \"json parse error\"}";
        server.send(400, "application/json", message); 
        return;
    }
    hadStart      = doc.containsKey("start");
    hadLength     = doc.containsKey("length");
    hadBrightness = doc.containsKey("brightness");
    hadColour     = doc.containsKey("colour");
    if (hadStart) {
        start =doc["start"];
        handleStart(&start);
    } else {
        message = "Missing 'start'. ";
    }
    if (hadLength) {
        length = doc["length"];
        handleLength(&length);
    } else {
        message += "Missing 'length'. ";
    }
    if (hadBrightness) {
        brightness = doc["brightness"];
        handleBrightness(&brightness);
    } else {
        message += "Missing 'brightness'. ";
    }
    if (hadColour) {
        colour = doc["colour"];
        handleColour(&colour);
    } else {
        message += "Missing 'colour'. ";
    }
    if (hadStart && hadLength && hadBrightness && hadColour) {
        limitStart(&start, length);
        eepromdata.setStatus(ST_START,start);
        eepromdata.setStatus(ST_LENGTH,length);
        eepromdata.setStatus(ST_DIM,brightness);
        eepromdata.setStatus(ST_LEDS,colour);
        eepromdata.write();
        updateLED(status);
        // AUCH, update the global variable!!!
        updateGlobals(start,length,brightness,colour);
    } else {
        // payload incomplete
        message =  "{ \"error\":  112, \"message\": \"" + message + "\" }";
        server.send(400, "application/json", message); 
        return; 
    }
    server.send(200, "application/json", "{}"); 
}

void handleget()
{
    long * status = eepromdata.getStatus();
    String response;
    StaticJsonDocument<300> doc;

    Serial.println("Get light state");
    doc["brightness"] = status[ST_DIM];
    doc["colour"] = status[ST_LEDS];
    doc["start"] = status[ST_START];
    doc["length"] = status[ST_LENGTH];
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void responseStartLenth(long start, long length)
{
    String response;
    StaticJsonDocument<30> doc;
    doc["start"] = start;
    doc["length"] = length;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
void responseVar(String variable, long value)
{
    String response;
    StaticJsonDocument<30> doc;
    doc[variable] = value;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleStartIncPatch()
{
    long * status = eepromdata.getStatus();
    long start  = status[ST_START];
    long length = status[ST_LENGTH];
    start++;
    handleStart(&start);
    limitStart(&start, length);
    eepromdata.setStatus(ST_START,start);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(start,length,status[ST_DIM],status[ST_LEDS]);
    responseStartLenth(start, length);
}
void handleStartDecPatch()
{
    long * status = eepromdata.getStatus();
    long start  = status[ST_START];
    long length = status[ST_LENGTH];
    start--;
    handleStart(&start);
    limitStart(&start, length);
    eepromdata.setStatus(ST_START,start);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(start,length,status[ST_DIM],status[ST_LEDS]);

    responseStartLenth(start, length);
}
void handleLengthIncPatch()
{
    long * status = eepromdata.getStatus();
    long start  = status[ST_START];
    long length = status[ST_LENGTH];
    length++;
    handleLength(&length);
    limitStart(&start, length);
    eepromdata.setStatus(ST_START,start);
    eepromdata.setStatus(ST_LENGTH,length);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(start,length,status[ST_DIM],status[ST_LEDS]);
    responseStartLenth(start, length);
}
void handleLengthDecPatch()
{
    long * status = eepromdata.getStatus();
    long start  = status[ST_START];
    long length = status[ST_LENGTH];
    length--;
    handleLength(&length);
    limitStart(&start, length);
    eepromdata.setStatus(ST_START,start);
    eepromdata.setStatus(ST_LENGTH,length);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(start,length,status[ST_DIM],status[ST_LEDS]);
    responseStartLenth(start, length);
}
void handleBrightnessIncPatch()
{   
    long * status = eepromdata.getStatus();
    long brightness  = status[ST_DIM];
    //brightness--;
    if (brightness < 10) {
        brightness++;
    } else if (brightness >= 10 && brightness < 100 ) {
        brightness+=5;
    } else if (brightness >= 100 && brightness < 1000 ) {
        brightness+=10;
    } else {
        // larger that 1000
        brightness+= 100;
    }
    handleBrightness(&brightness);
    eepromdata.setStatus(ST_DIM,brightness);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(status[ST_START],status[ST_LENGTH],brightness,status[ST_LEDS]);
    responseVar("brightness", brightness);
}
void handleBrightnessDecPatch()
{
    long * status = eepromdata.getStatus();
    long brightness  = status[ST_DIM];
    //brightness--;
    if (brightness < 10) {
        brightness--;
    } else if (brightness >= 10 && brightness < 100 ) {
        brightness-=5;
    } else if (brightness >= 100 && brightness < 1000 ) {
        brightness-=10;
    } else {
        // larger that 1000
        brightness-= 100;
    }
    handleBrightness(&brightness);
    eepromdata.setStatus(ST_DIM,brightness);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(status[ST_START],status[ST_LENGTH],brightness,status[ST_LEDS]);
    responseVar("brightness", brightness);    
}
void handleColourIncPatch()
{
    long * status = eepromdata.getStatus();
    long colour  = status[ST_LEDS];
    colour++;
    handleColour(&colour);
    eepromdata.setStatus(ST_LEDS,colour);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(status[ST_START],status[ST_LENGTH],status[ST_DIM],colour);
    responseVar("colour", colour);
}
void handleColourDecPatch()
{
    long * status = eepromdata.getStatus();
    long colour  = status[ST_LEDS];
    colour--;
    handleColour(&colour);
    eepromdata.setStatus(ST_LEDS,colour);
    eepromdata.write();
    updateLED(status);
    // AUCH, update the global variable!!!
    updateGlobals(status[ST_START],status[ST_LENGTH],status[ST_DIM],colour);
    responseVar("colour", colour);

}

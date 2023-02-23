#ifndef _MENU_H_
#define _MENU_H_

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
/* 
Handles menu plus pixel colors
*/
class Menu {
    public:
        Menu(const int maxStates, Adafruit_NeoPixel menupixel, uint32_t colours[]);
        void setup();
        int getState();
        void setState(int state);
        void bypassMenu(uint8_t br, uint64_t col);
    private:
        Adafruit_NeoPixel m_menupixel;
        uint32_t m_colours[10];
        int m_maxStates;
        int m_menuState =0;
};


#endif //_MENU_H_
#include "Menu.h"



// Declare our NeoPixel strip object:

// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


Menu::Menu(const int maxStates,Adafruit_NeoPixel menupixel, uint32_t colours[])
{   
    // just copy the colours to local to the class and thats it
    m_menupixel =menupixel;
    m_maxStates = maxStates;
    for (int i=0 ; i<m_maxStates; i++)
        m_colours[i] = colours[i];
}

void Menu::setup()
{
    // called in the arduino setup function
    m_menupixel.begin(); // Initialize NeoPixel strip object (REQUIRED)
    m_menupixel.setPixelColor(0, m_colours[0]);
    m_menupixel.show();  // Initialize all pixels to 'off'
}

int Menu::getState(){
    // yes evil just return the value
    return m_menuState;
}

void Menu::setState(int state)
{
    m_menuState = state % m_maxStates;
    m_menupixel.setPixelColor(0, m_colours[m_menuState]);
    m_menupixel.show();
    // and change the menu colour
}

void Menu::bypassMenu(uint8_t br, uint64_t col)
{
    m_menupixel.setPixelColor(0,col);
    m_menupixel.setBrightness(br);
    m_menupixel.show();
}
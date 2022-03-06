#ifndef LedState_h
#define LedState_h

#include <Arduino.h>


class LedState {
  
  private:
    byte pin;
    
  public:
    // Setup pin LED and call init()
    LedState(byte pin);

    // Setup the pin led as OUTPUT and power off the LED - default state
    void init();
    
    // Power on the LED
    void setOn();

    // Power off the LED
    void setOff();
    
    void setPulsing();
    
    void setBlinking();

    void loop();
};

#endif

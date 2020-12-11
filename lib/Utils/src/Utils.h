#include <Arduino.h>
#include <ArduinoUniqueID.h>
#include "U8glib.h" 

#ifndef _Utils_H
#define _Utils_H


enum statusSound {
  sound_ok,
  sound_error
};

class Som {
  public:
    Som (int pinBuzzer);
    void somCerto(int pinLed, int interval);
    void somErrado(int pinLed, int firstInterval, int lastInterval); 

  private:
    int _pinLed;
    int _pinBuzzer;
    int _time;
    int _interval;
    int _firstInterval;
    int _lastInterval;
};

class UID {
  public:
    UID ();
    String getUID();
  private:
    String _uid;
};

enum ScreenName{
  SCREEN_DRAW_LOGO,
  SCREEN_INIT,
  SCREEN_VERIFY_DATA_LOGGER,
};

class DrawScreen {
  public:
    DrawScreen ();
    void begin();
    void draw(ScreenName screen, uint8_t status, bool c1, bool c2, bool c3);
  private:
    ScreenName _screen;
    uint8_t _status;
    bool _c1, _c2, _c3;
};


#endif 

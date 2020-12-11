
#include <Utils.h>

U8GLIB_ST7920_128X64_1X display(11, 12, 13);

Som::Som(int pinBuzzer){
	_pinBuzzer = pinBuzzer;
	_time = 3;
	_interval = 100;
}

void Som::somCerto(int pinLed, int interval){

	_pinLed = pinLed;
	_time = 2;
	_interval = interval;
	

	digitalWrite(_pinBuzzer, HIGH);
  for (int j = 0; j < _time; j++)
  {
    tone(_pinBuzzer, 900);
    digitalWrite(_pinBuzzer, HIGH);
    digitalWrite(_pinLed, HIGH);
    delay(_interval);
    noTone(_pinBuzzer);
    digitalWrite(_pinLed, LOW);
    delay(_interval);
  }
  digitalWrite(_pinBuzzer, LOW);
}

void Som::somErrado(int pinLed, int firstInterval, int lastInterval){

	_pinLed = pinLed;
	_time = 3;
	_firstInterval = firstInterval;
	_lastInterval = lastInterval;

	digitalWrite(_pinBuzzer, HIGH);
  for (int j = 0; j < _time; j++)
  {
    tone(_pinBuzzer, 900);
    digitalWrite(_pinBuzzer, HIGH);
    digitalWrite(_pinLed, HIGH);
    delay(_firstInterval);
    noTone(_pinBuzzer);
    digitalWrite(_pinLed, LOW);
    delay(_lastInterval);
  }
  digitalWrite(_pinBuzzer, LOW);
}

UID::UID(){
}

String UID::getUID(){
  _uid = "";
  for (size_t i = 0; i < UniqueIDsize; i++)
  {
    _uid.concat(String(UniqueID[i] < 0x10 ? "0" : ""));
    _uid.concat(String(UniqueID[i], HEX));
  }
  _uid.toUpperCase();
  return _uid;
}

DrawScreen::DrawScreen(){
}

void DrawScreen::begin(){
  if ( display.getMode() == U8G_MODE_R3G3B2 )
    display.setColorIndex(255);   // white
  else if ( display.getMode() == U8G_MODE_GRAY2BIT )
    display.setColorIndex(1);     // max intensity
  else if ( display.getMode() == U8G_MODE_BW )
    display.setColorIndex(1);     // pixel on
  display.setContrast(0x30);
}

void DrawScreen::draw(ScreenName screen, uint8_t status, bool c1, bool c2, bool c3){
  _screen = screen;
  _status = status;
  _c1 = c1;
  _c2 = c2;
  _c3 = c3;

  switch (_screen)
  {
  case SCREEN_DRAW_LOGO:
    Serial.println("entrou no logo");  
  break;
  
  case SCREEN_INIT:
    Serial.println("entrou na INICIALIZACAO");  
  break;

  case SCREEN_VERIFY_DATA_LOGGER:
    Serial.println("entrou na VERIFICAÇAÕ DO DATA LOGGER");  
  break;

  default:
    break;
  }
}
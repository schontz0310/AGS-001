
#include <Utils.h>

U8GLIB_ST7920_128X64_1X display(11, 12, 13);
MFRC522 rfid(SS_SDA_PIN, RST_PIN);
UID uniqueNumber; 

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

void DrawScreen::drawSetup(ScreenName screen, int interval, uint8_t status, bool state[]){
  _screen = screen;
  _status = status;
  _interval = interval;

  switch (_screen)
  {
  case SCREEN_DRAW_LOGO:
    display.firstPage();
    do {
      display.drawXBMP( 0, 0, 128, 64, img_bitmap);
    } while (display.nextPage());
    delay(_interval);  
  break;
  
  case SCREEN_INIT:
    display.firstPage();
    do {
      (uniqueNumber.getUID()).toCharArray(_buffer, 24);
      display.setFont(u8g_font_unifont);
      display.drawStr( 25, 12, F("SISTEMA DE"));
      display.drawStr( 26, 12, F("SISTEMA DE"));
      display.drawStr( 12, 27, F("ABASTECIMENTO"));
      display.drawStr( 13, 27, F("ABASTECIMENTO"));
      display.drawStr( 14, 42, F("AGS-001 V1.0"));
      display.drawStr( 15, 42, F("AGS-001 V1.0"));
      display.setFont(u8g_font_6x10);
      display.drawStr( 0, 57, F("UID"));
      display.drawStr( 21, 57, _buffer);
    } while (display.nextPage());
    delay(_interval);  
  break;

  case SCREEN_VERIFY_DATA_LOGGER_SD:
    display.firstPage();
    do {
      if (_status == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = OK"));
      }

      if (_status == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = XX"));
      }
    } while (display.nextPage());
     delay(_interval);  
  break;

  case SCREEN_VERIFY_DATA_LOGGER_RTC:
    display.firstPage();
    do {
      if (_state[0] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = OK"));
      }

      if (_state[0] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = XX"));
      }

      if (_status == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 20, F("DATALOOGER RTC = OK"));
      }

      if (_status == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 20, F("DATALOOGER RTC = XX"));
      }

    } while (display.nextPage());
     delay(_interval);  
  break;

  case SCREEN_VERIFY_RFID:
    display.firstPage();
    do {
      if (_state[0] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = OK"));
      }

      if (_state[0] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 10, F("DATALOOGER SD  = XX"));
      }

      if (_state[1] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 20, F("DATALOOGER RTC = OK"));
      }

      if (_state[1] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 20, F("DATALOOGER RTC = XX"));
      }

      if (_status == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = OK"));
      }

      if (_status == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = XX"));
      }

    } while (display.nextPage());
    delay(_interval);  
  break;

  default:
    break;
  }
}

RFIDReader::RFIDReader(){
}

RFIDStatus RFIDReader::begin(){
   rfid.PCD_Init();
  if ((_result = rfid.PCD_PerformSelfTest()) > 0) {
    Serial.println(F("RFID OK"));
    return RFID_OK;
  }  else {
    Serial.println(F("RFID ERROR"));
    return RFID_ERROR;
  }
}

DatalLogger::DatalLogger(){

}

tmElements_t tm;
String timestamp;
const char *monthNameDatalogger[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

DataLoggerStatus DatalLogger::begin(uint8_t pin_ss_datalogger){
  _pin_ss_datalogger = pin_ss_datalogger;

  if (SD.begin(_pin_ss_datalogger)) {             // Inicializaçção do cartao SD
    delay(100);
    return DATALOGGER_SD_OK;
  } else {
    delay(100);
    return DATALOGGER_SD_ERROR;
  }  
}

DataLoggerStatus DatalLogger::setSystemTimestamp(){
  if (getDate(__DATE__) && getTime(__TIME__)) 
  {
    RTC.write(tm);
    getTimestamp();
    return DATALOGGER_TIME_OK;
  }else{
    return DATALOGGER_TIME_ERROR;
  }
}

bool DatalLogger::getDate(const char *str) {
  
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthNameDatalogger[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

bool DatalLogger::getTime(const char *str) {
  int Hour, Min, Sec;
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

String DatalLogger::getTimestamp() {
  
  timestamp = "-erro-"; //Mensagem em caso de erro na leitura da data/hora
  if (RTC.read(tm)) { //Função para lder hora
    Serial.println("Entrou na Leitura de hora do rtc");
    timestamp = "";
    timestamp += get2digits(tm.Day); //Dia
    timestamp += "/";
    timestamp += get2digits(tm.Month); //Mes
    timestamp += "/";
    timestamp += get2digits(tmYearToCalendar(tm.Year)); //Ano
    timestamp += "_";
    timestamp += get2digits(tm.Hour); //Hora
    timestamp += ":";
    timestamp += get2digits(tm.Minute); //Minuto
    timestamp += ":";
    timestamp += get2digits(tm.Second); //Segundo
  }
  delay(500);
  Serial.println(timestamp);
  return timestamp;
}

String DatalLogger::get2digits(int number) {
  String val = "";
  if (number >= 0 && number < 10) {
    val += "0";
  }
  val += String(number);
  return val;
}

DataLoggerStatus DatalLogger::getDateHour(){
  if(RTC.read(tm)){
    return DATALOGGER_TIME_OK;
  }else{
    return DATALOGGER_TIME_ERROR;
  }
}
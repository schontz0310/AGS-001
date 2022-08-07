
#include <Utils.h>

const char MQTT_SERVER[]        =       "mqtt.datamills.com.br";                //"mqtt://things.ubidots.com"
const int MQTT_PORT             =       1883;                                   // Porta para comunicação com Broker MQTT
const char USER[]               =       "supply-admin";                         // "USUARIO"
const char PASS[]               =       "supply@19!";                           // "SENHA"

String SENHA_AGS                =       "380130";                             // Senha ROOT, apenas pessoal autorizado da AGS possue
String jsonPayload              =       "";                             
const uint8_t _buzzer           =       27;
const uint8_t _pinDatalogger    =       46;   
const byte lines = 4;     //NUMERO DE LINHAS DO TECLADO
const byte columns = 4;  //NUMERO DE COLUNAS DO TECLADO

//MATRIZ DO TECLADO DEFINA PELAS LINHAS E COLUNAS 
char matriz[lines][columns] =
{
  { '1', '2', '3', 'A'},
  { '4', '5', '6', 'B'},
  { '7', '8', '9', 'C'},
  { '*', '0', '#', 'D'},
};
// byte linesPines[lines] = {49, 47, 45, 43};        //PINOS CONECTADOS AS LINHAS DO TECLADO
// byte columnsPines[columns] = {41, 39, 37, 35};    //PINOS CONECTADOS AS COLUNAS DO TECLADO

byte linesPines[lines] = {41, 39, 37, 35};        //PINOS CONECTADOS AS LINHAS DO TECLADO
byte columnsPines[columns] = {49, 47, 45, 43};    //PINOS CONECTADOS AS COLUNAS DO TECLADO

char tecla_presionada;

const uint8_t RELE_02           =       A13;                                  // Pino Rele_02
const uint8_t RELE_01           =       A0;                                   // Pino Rele_01
const uint8_t BOTAO             =       8;                                    // Botao de uso geral

volatile unsigned long counter;
float fuelQuantity;
String _COMPANY;


U8GLIB_ST7920_128X64_1X display(11, 12, 13);
MFRC522 rfid(SS_SDA_PIN, RST_PIN);
UID uniqueNumber;
TinyGsm modemGSM(Serial2);
TinyGsmClient gsmClient(modemGSM);
PubSubClient client(gsmClient);
Keypad keyboard = Keypad( makeKeymap(matriz), linesPines, columnsPines, lines, columns);

DrawScreen screen;
RFIDReader rfidReader;
Menu menu;
Access access;
Keyboard key;
DataLogger sd;
MQTTConnection mqtt;
ModemGPRS internet;
Som buzzer(_buzzer);
File fileName;
Json json;

const uint8_t ledErrado      =       29;
const uint8_t ledCerto       =       31;



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

void DrawScreen::readOperator(ScreenName screen, String name, String cardID){
  _screen = screen;
  _name = name;
  _cardID = cardID;  
  memset(_buffer, 0, sizeof(_buffer));
  switch (_screen)
  {
    case SCREEN_OPERATOR_READ:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "PASSE O CARTAO");
        display.drawStr( 22, 25, " DO  OPERADOR ");
        display.drawStr( 3, 60, "APERTE 'A' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, _buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    case SCREEN_OPERATOR_SEARCH:
      _cardID.toCharArray(_buffer, 24);
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "PROCURANDO POR");
        display.drawStr( 22, 25, "   OPERADOR   ");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, _buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    case SCREEN_OPERATOR_FOUND:
      _name.toCharArray(_buffer, 24);
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "   OPERADOR   ");
        display.drawStr( 22, 25, "  ENCONTRADO  ");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, _buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    case SCREEN_OPERATOR_NOT_FOUND:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, " OPERADOR NAO ");
        display.drawStr( 22, 25, "  ENCONTRADO  ");
        display.drawStr( 3, 60, "APERTE 'A' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
  }
}

void DrawScreen::drawScreen(ScreenName targetScreen, String value){
  _screen = targetScreen;
  _name = value;
  switch (_screen)
  {
    case SCREEN_PUMP_CHARGE_FUEL:
      _name.toCharArray(_buffer, 24);
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15,"    LITROS    ");
        display.drawStr( 22, 25," ABASTECIDOS  ");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, _buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());
    break;
  }
}  

void DrawScreen::drawMenu(ScreenName targetScreen){
  _screen = targetScreen;

  switch (_screen)
  {
    case SCREEN_MENU_PRINCIPAL:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 10, 12, "MENU PRINCIPAL");
        display.drawStr( 11, 12, "MENU PRINCIPAL");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "CADASTROS");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "CONFIGORACOES");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
        display.drawStr( 27, 60, "ENTRADAS");
        display.drawStr( 6, 60, "3");
        display.drawRFrame(1, 49, 15, 15, 3);
      } while (display.nextPage());  
    break;
    case SCREEN_MENU_CADASTRO:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 27, 12, "CADASTROS");
        display.drawStr( 28, 12, "CADASTROS");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "OPERADORES");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "VEICULOS");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
        display.drawStr( 27, 60, "PERMISSOES");
        display.drawStr( 6, 60, "3");
        display.drawRFrame(1, 49, 15, 15, 3);
      } while (display.nextPage());  
    break;

    case SCREEN_MENU_CONFIGURACAO:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 27, 12, "CONFIGURACAO");
        display.drawStr( 28, 12, "CONFIGURACAO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "REGISTRO");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
      } while (display.nextPage());  
    break;
    
    case SCREEN_ACCCESSES:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 7, 12, "TELA DE ACESSO");
        display.drawStr( 8, 12, "TELA DE ACESSO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "ACESSO COM SENHA");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "ACESSO COM CARTAO");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
        display.drawStr( 27, 60, "CANCELAR");
        display.drawStr( 6, 60, "*");
        display.drawRFrame(1, 49, 15, 15, 3);
      } while (display.nextPage());  
    break;
    
    case SCREEN_ACCCESSES_PASSWORD:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "DIGITE A SENHA");
        display.drawStr( 22, 25, "  DE  ACESSO  ");
        display.drawStr( 3, 60, "APERTE '*' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, access._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;

    case SCREEN_ACCCESSES_CARD:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "PASSE O CARTAO");
        display.drawStr( 22, 25, "  DE  ACESSO  ");
        display.drawStr( 3, 60, "APERTE '*' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, access._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    
    case SCREEN_MENU_CADASTRO_OPERADOR_CHOICE:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA A OPCAO");
        display.drawStr( 2, 12, "ESCOLHA A OPCAO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "INCLUIR OPERADOR");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "EXCLUIR OPERADOR");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
      } while (display.nextPage());  
    break;
    case SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "APROXIME A TAG");
        display.drawStr( 22, 25, " DO  OPERADOR ");
        display.drawStr( 3, 60, "APERTE '*' PARA CANCELAR");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, menu._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    case SCREEN_MENU_CADASTRO_OPERADOR_READ_NAME:
      
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "DIGITE O NOME ");
        display.drawStr( 22, 25, " DO OPERADOR  ");
        display.drawStr( 3, 60, "APERTE '*' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, key._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());
    break;

    case SCREEN_MENU_CADASTRO_OPERADOR_READ_LEVEL:
      
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA O NIVEL");
        display.drawStr( 2, 12, "ESCOLHA O NIVEL");

        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "ADMINISTRADOR");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);

        display.drawStr( 27, 44, "FRENTISTA");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);

        display.drawStr( 27, 60, "MOTORISTA");
        display.drawStr( 6, 60, "3");
        display.drawRFrame(1, 49, 15, 15, 3);
      } while (display.nextPage());
    break;

    case SCREEN_MENU_CADASTRO_VEHICLE_CHOICE:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA A OPCAO");
        display.drawStr( 2, 12, "ESCOLHA A OPCAO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "INCLUIR VEICULO");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "EXCLUIR VEICULO");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
      } while (display.nextPage());  
    break;
    case SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "APROXIME A TAG");
        display.drawStr( 22, 25, " DO   VEICULO ");
        display.drawStr( 3, 60, "APERTE '*' PARA CANCELAR");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, menu._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;
    case SCREEN_MENU_CADASTRO_VEHICLE_READ_NAME:
      
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, "DIGITE O NOME ");
        display.drawStr( 22, 25, "  DO VEICULO  ");
        display.drawStr( 3, 60, "APERTE '*' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, key._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());
    break;
    case SCREEN_MENU_CADASTRO_VEHICLE_READ_LEVEL:
      
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA O TIPO ");
        display.drawStr( 2, 12, "ESCOLHA O TIPO ");
        display.drawStr( 1, 28, "DE COMBUSTIVEL ");
        display.drawStr( 2, 28, "DE COMBUSTIVEL ");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 44, "S10");
        display.drawStr( 6, 44, "1");
        display.drawRFrame(1, 33, 15, 15, 3);

        display.drawStr( 27, 60, "S500");
        display.drawStr( 6, 60, "2");
        display.drawRFrame(1, 49, 15, 15, 3);
      } while (display.nextPage());
    break;
    case SCREEN_MENU_CADASTRO_PERMISSION_CHOICE:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA A OPCAO");
        display.drawStr( 2, 12, "ESCOLHA A OPCAO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "INCLUIR PERMISSAO");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "EXCLUIR PERMISSAO");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
      } while (display.nextPage());  
    break;
    case SCREEN_CONFIGURACAO_REGISTRO_CHOICE:
      display.firstPage();
      do {
        display.setFont(u8g_font_unifont);
        display.drawStr( 1, 12, "ESCOLHA A OPCAO");
        display.drawStr( 2, 12, "ESCOLHA A OPCAO");
        display.setFont(u8g_font_6x10);
        display.drawStr( 27, 28, "INCLUIR REGISTRO");
        display.drawStr( 6, 28, "1");
        display.drawRFrame(1, 17, 15, 15, 3);
        display.drawStr( 27, 44, "EXCLUIR REGISTRO");
        display.drawStr( 6, 44, "2");
        display.drawRFrame(1, 33, 15, 15, 3);
      } while (display.nextPage());  
    break;
    case SCREEN_CONFIGURACAO_REGISTRO:
      display.firstPage();
      do {
        display.setFont(u8g_font_6x10);
        display.drawStr( 22, 15, " INSIRA O CNPJ ");
        display.drawStr( 22, 25, "  DA EMPRESA  ");
        display.drawStr( 3, 60, "APERTE '*' PARA MENU");
        display.setFont(u8g_font_unifont);
        display.drawStr( 4, 45, key._buffer);
        display.drawRFrame(1, 31, 126, 18, 5);
      } while (display.nextPage());  
    break;

    case SCREEN_PROGRESS:
        _status  = _status + 10;       
        if (_status >= 100 ){
          _status = 0;
        };
      display.firstPage();
      do {
        _status++;   
        display.setFont(u8g_font_unifont);
        display.drawStr( 20, 27, F("PROCESSANDO"));
        display.drawStr( 21, 27, F("PROCESSANDO"));
        display.drawFrame(12,40,100,10);
        display.drawBox(12,40,_status,10);
      } while (display.nextPage());
    break;

    case SCREEN_ERROR:
      display.firstPage();
      do {
        display.drawXBMP((128 - error_width)/2, 0, error_width, error_height, error);
      } while (display.nextPage());
    break;

    case SCREEN_SUCCESS:
      display.firstPage();
      do {
        display.drawXBMP((128 - success_width)/2, 0, success_width, success_height, success);
      } while (display.nextPage());
    break;
  }
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
      display.drawXBMP( 8, 0, 128, 64, img_bitmap);
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

  case SCREEN_VERIFY_MODEM:
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

      if (_state[2] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = OK"));
      }

      if (_state[2] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = XX"));
      }

      if (_status == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 40, F("MODEM GPRS     = OK"));
      }

      if (_status == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 40, F("MODEM GPRS     = XX"));
      }

    } while (display.nextPage());
    delay(_interval);  
  break;

  case SCREEN_VERIFY_MQTT:
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

      if (_state[2] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = OK"));
      }

      if (_state[2] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 30, F("RFID           = XX"));
      }

      if (_state[3] == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 40, F("MODEM GPRS     = OK"));
      }

      if (_state[3] == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 40, F("MODEM GPRS     = XX"));
      }

      if (_status == 0) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 50, F("CONEXAO MQTT   = OK"));
      }

      if (_status == 1) {
        display.setFont(u8g_font_6x10);
        display.drawStr( 0, 50, F("CONEXAO MQTT   = XX"));
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

bool RFIDReader::getID(){
  _value = "";
  if ( ! rfid.PICC_IsNewCardPresent()) {
    return 0;
  }
  if ( ! rfid.PICC_ReadCardSerial()) {
    return 0;
  }

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    _value.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    _value.concat(String(rfid.uid.uidByte[i], HEX));
  }
  _value.toUpperCase();
  IDValue = _value;
  rfid.PICC_HaltA();
  return 1;
}

tmElements_t tm;
String timestamp;
const char *monthNameDatalogger[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

DataLogger::DataLogger(){

}

DataLoggerStatus DataLogger::begin(uint8_t pin_ss_datalogger){
  _pin_ss_datalogger = pin_ss_datalogger;

  if (SD.begin(_pin_ss_datalogger)) {
    delay(100);
    return DATALOGGER_SD_OK;
  } else {
    delay(100);
    return DATALOGGER_SD_ERROR;
  }  
}

bool DataLogger::checkOperatorExist(String uuid){
  _uuidToCheck = uuid;
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-OPE.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _uuidRead = fileName.readStringUntil(13);
      Serial.println(_uuidRead);
      _uuidRead.trim();
      _uuidRead.remove(0,43);
      int length = _uuidRead.indexOf(";");
      _uuidRead.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_uuidRead);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _uuidRead){
        return true;
      }
    }
    return false;
  }
}

bool DataLogger::checkOperatorIsAdmin(String uuid){
  _uuidToCheck = uuid;
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-OPE.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _uuidRead = fileName.readStringUntil(13);
      _operatorLevel = _uuidRead.substring(_uuidRead.lastIndexOf(";") + 1);
      Serial.println(_uuidRead);
      _uuidRead.trim();
      _uuidRead.remove(0,43);
      int length = _uuidRead.indexOf(";");
      _uuidRead.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_uuidRead);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _uuidRead){
        if(_operatorLevel == "1"){
          return true;
        }else{
          return false;
        }
      }
    }
    return false;
  }
}

bool DataLogger::checkPermissionExist(String uuid){
  _uuidToCheck = uuid;
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-PER.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _uuidRead = fileName.readStringUntil(13);
      Serial.println(_uuidRead);
      _uuidRead.trim();
      _uuidRead.remove(0,43);
      int length = _uuidRead.indexOf(";");
      _uuidRead.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_uuidRead);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _uuidRead){
          return true;
      }else{
          return false;
      }
    }
    return false;
  }
}

bool DataLogger::checkCompanyExist(String uuid){
  _uuidToCheck = uuid;
  fileName.close();
  Serial.println("entrou na funcao WriteCompanyRegister");
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-COM.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD");
      delay(1500);
      loop();
    }
  }
  _uuidToCheck = uuid;
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-COM.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _uuidRead = fileName.readStringUntil(13);
      Serial.println(_uuidRead);
      _uuidRead.trim();
      _uuidRead.remove(0,43);
      int length = _uuidRead.indexOf(";");
      _uuidRead.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_uuidRead);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _uuidRead){
        return true;
      }
    }
    return false;
  }
}

String DataLogger::getCompany(){
  Serial.println("entrou na funcao getCompany");
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-COM.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD");
      delay(1500);
      loop();
    }
  }
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-COM.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _operatorUuid = fileName.readStringUntil(13);
      _operatorLevel = _operatorUuid.substring(_operatorUuid.lastIndexOf(";") + 1);
      if (_operatorLevel.length() > 10){
        _COMPANY = _operatorLevel;
        return _operatorLevel;
      }
    }
    return "NO_COMPANY";
  }
}

String DataLogger::getOperator(String uuid){
  _uuidToCheck = uuid;
  _operatorUuid = "";
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-OPE.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _operatorUuid = fileName.readStringUntil(13);
      _operatorLevel = _operatorUuid.substring(_operatorUuid.lastIndexOf(";") + 1);
      _operatorUuid.remove(_operatorUuid.lastIndexOf(";"));
      _operatorName = _operatorUuid.substring(_operatorUuid.lastIndexOf(";") + 1);
      Serial.println(_operatorUuid);
      _operatorUuid.trim();
      _operatorUuid.remove(0,43);
      int length = _operatorUuid.indexOf(";");
      _operatorUuid.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_operatorUuid);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _operatorUuid){
        return _operatorUuid + "#" + _operatorName + "#" + _operatorLevel;
      }
    }
    return "NO_OPERATOR";
  }
}

String DataLogger::getVehicle(String uuid){
  _uuidToCheck = uuid;
  _vehicleUuid = "";
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-VEH.txt");
  if (fileName) {
    while (fileName.available())
    {
      screen.drawMenu(SCREEN_PROGRESS);
      _vehicleUuid = fileName.readStringUntil(13);
      _vehicleFuel = _vehicleUuid.substring(_vehicleUuid.lastIndexOf(";") + 1);
      _vehicleUuid.remove(_vehicleUuid.lastIndexOf(";"));
      _vehicleName = _vehicleUuid.substring(_vehicleUuid.lastIndexOf(";") + 1);
      Serial.println(_vehicleUuid);
      _vehicleUuid.trim();
      _vehicleUuid.remove(0,43);
      int length = _vehicleUuid.indexOf(";");
      _vehicleUuid.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_vehicleUuid);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _vehicleUuid){
        return _vehicleUuid + "#" + _vehicleName + "#" + _vehicleFuel;
      }
    }
    return "NO_VEHICLE";
  }
}

bool DataLogger::checkVehicleExist(String uuid){
  _uuidToCheck = uuid;
  fileName.close();
  delay(50);
  fileName = SD.open("CAD-VEH.txt");
  if (fileName) {
    while (fileName.available())
    { 
      screen.drawMenu(SCREEN_PROGRESS);
      _uuidRead = fileName.readStringUntil(13);
      Serial.println(_uuidRead);
      _uuidRead.trim();
      _uuidRead.remove(0,43);
      int length = _uuidRead.indexOf(";");
      _uuidRead.remove(length);
      Serial.print(F("Verificação de UUID = "));
      Serial.print(_uuidRead);
      Serial.print (F(" Sistema <---> Nova Tag "));
      Serial.println (_uuidToCheck);
      if (_uuidToCheck == _uuidRead){
        return true;
      }
    }
    return false;
  }
}

void DataLogger::WriteOperatorInDatalogger(){
  Serial.println("entrou na funcao WirteOperator");
  if (!SD.exists("CAD-OPE.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-OPE.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-OPE.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 736]");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-OPE.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.print(sd.getTimestamp());                  // DATA E HORA
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(uniqueNumber.getUID());              // NUMERO UNICO DO EQUIPAMENTO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print("001");                              // CODIGO DA FUNÇÃO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(menu._UUIDCard);                     // UID TAG RFID
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(menu._operatorName);                 // NOME DO OPERADOR
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.println(menu._operatorlevel);              // NIVEL DE PERMISSAO DO OPERADOR
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 669]");
    delay(1500);
    loop();
  }
}

void DataLogger::WriteVehicleInDatalogger(){
  Serial.println("entrou na funcao WirteVehicle");
  if (!SD.exists("CAD-VEH.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-VEH.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-VEH.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 770]");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-VEH.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.print(sd.getTimestamp());                  // DATA E HORA
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(uniqueNumber.getUID());              // NUMERO UNICO DO EQUIPAMENTO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print("002");                              // CODIGO DA FUNÇÃO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(menu._UUIDCard);                     // UID TAG RFID
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(menu._vehicleName);                  // NOME DO VEICULO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.println(menu._vehicleFuel);
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 789]");
    delay(1500);
    loop();
  }
}

void DataLogger::WritePermissionInDatalogger(){
  Serial.println("entrou na funcao WritePermission");
  if (!SD.exists("CAD-PER.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-PER.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-PER.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 882]");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-PER.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.print(sd.getTimestamp());                  // DATA E HORA
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(uniqueNumber.getUID());              // NUMERO UNICO DO EQUIPAMENTO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print("003");                              // CODIGO DA FUNÇÃO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.println(menu._UUIDPermission);             // UID TAG RFID
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 892]");
    delay(1500);
    loop();
  }
}

void DataLogger::WriteFuelChargeInDatalogger(){
  Serial.println("entrou na funcao WriteFuelCharger");
  if (!SD.exists("CAD-REG.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-REG.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-REG.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-REG.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.print(sd.getTimestamp());                  // DATA E HORA
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(uniqueNumber.getUID());              // NUMERO UNICO DO EQUIPAMENTO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print("099");                              // CODIGO DA FUNÇÃO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._operatorUuid);                   // UID TAG RFID
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._operatorName);                   // NOME DO VEICULO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._operatorLevel);
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._vehicleUuid);
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._vehicleName);
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(sd._vehicleFuel);
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(fuelQuantity);
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.println(_COMPANY);
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro na abertura do cartao SD");
    delay(1500);
    loop();
  }
}

void DataLogger::WriteCompanyRegisterInDatalogger(){
  Serial.println("entrou na funcao WriteCompanyRegister");
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-COM.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-COM.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-COM.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.print(sd.getTimestamp());                  // DATA E HORA
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print(uniqueNumber.getUID());              // NUMERO UNICO DO EQUIPAMENTO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.print("080");                              // CODIGO DA FUNÇÃO
    fileName.print(";");                                // SEPARADOR CONDICIONAL
    fileName.println(menu._companyNumber);             // UID TAG RFID
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 892]");
    delay(1500);
    loop();
  }
}

void DataLogger::WriteFailMqttLog(String payload){
  Serial.println("entrou na funcao WriteFailMqttLog");
  if (!SD.exists("CAD-ERR.txt")){
    Serial.println("tentou criar arquivo");
    SD.open("CAD-ERR.txt", FILE_WRITE);
  } 
  if (!SD.exists("CAD-ERR.txt")){
    Serial.println("cria;áo do arquivo deu errado");
    if(!SD.begin(_pin_ss_datalogger)){
      screen.drawMenu(SCREEN_ERROR);
      Serial.println("Erro na abertura do cartao SD");
      delay(1500);
      loop();
    }
  }
  Serial.println(fileName);
  fileName.close();
  Serial.println(fileName);
  fileName = SD.open("CAD-ERR.txt", FILE_WRITE);
  Serial.println(fileName);
  if (fileName) {
    Serial.println(F("GRAVANDO DADOS NO CARTÃO SD"));
    fileName.println(payload);         
    fileName.close();
    delay(500);
  } else {
    Serial.println(F("FALHA AO GRAVAR DADOS NO CARTÃO SD"));
    Serial.println("Erro na abertura do cartao SD, [Utils.cpp - 706]");
    delay(1500);
  }
}

DataLoggerStatus DataLogger::setSystemTimestamp(){
  if (getDate(__DATE__) && getTime(__TIME__)) 
  {
    Serial.println("Guardando Timestamp do sistema");
    Serial.println(getDate(__DATE__));
    Serial.println(getTime(__TIME__));
    delay(200);
    RTC.write(tm);
    getTimestamp();
    return DATALOGGER_TIME_OK;
  }else{
    return DATALOGGER_TIME_ERROR;
  }
}

bool DataLogger::getDate(const char *str) {
  Serial.println(__DATE__);
  Serial.println(__TIME__);
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

bool DataLogger::getTime(const char *str) {
  int Hour, Min, Sec;
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

String DataLogger::getTimestamp() {
  
  timestamp = "-erro-"; //Mensagem em caso de erro na leitura da data/hora
  if (RTC.read(tm)) { //Função para lder hora
    Serial.println("Entrou na Leitura de hora do rtc");
    timestamp = "";
    timestamp += get2digits(tm.Day); //Dia
    timestamp += "/";
    timestamp += get2digits(tm.Month); //Mes
    timestamp += "/";
    timestamp += get2digits(tmYearToCalendar(tm.Year)); //Ano
    timestamp += ";";
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

String DataLogger::get2digits(int number) {
  String val = "";
  if (number >= 0 && number < 10) {
    val += "0";
  }
  val += String(number);
  return val;
}

DataLoggerStatus DataLogger::getDateHour(){
  if(RTC.read(tm)){
    return DATALOGGER_TIME_OK;
  }else{
    return DATALOGGER_TIME_ERROR;
  }
}

ModemGPRS::ModemGPRS(){

}

ModemGPRSStatus ModemGPRS::setup(){
  Serial.println(F("Setup GSM..."));
  //Inicializamos a serial onde está o modem
  Serial2.begin(9600);
  delay(100);
  //Mostra informação sobre o modem
  AGAIN_GPRS:
  if (!modemGSM.restart())
  {
    Serial.println(F("FALHOU REINICIALIZACAO DO MODEM GSM"));
    return MODEM_ERROR_RESTART;
  }else{
    Serial.println(modemGSM.getModemInfo());
  }
  int connectionCount = 0;
  AGAIN:
  if (!modemGSM.waitForNetwork())
  {
    Serial.println(F("FALHA DE CONEXÃO COM A REDE"));
    if (connectionCount >= 1 ){
      return MODEM_ERROR_NETWORK;
    }else{
      connectionCount ++;
      Serial.print("TENTATIVA = ");
      Serial.println(connectionCount);
      goto AGAIN;
    }
  }
  if (modemGSM. isNetworkConnected ()) {
    Serial.println(F("CONECTADO A REDE COM SUCESSO"));
  } 
  uint8_t gprs_count = 0;

  AGAIN_GPRS_CON:
  if (!modemGSM.gprsConnect("m2mprepago.br", "Arqia", "Arqia")) {
  // if (!modemGSM.gprsConnect("zap.vivo.com.br", "vivo", "vivo")) {  
    delay(2000);
    Serial.print(F("TENTATIVA = "));
    Serial.println(gprs_count);
    Serial.println(F("CONEXÃO DE DADOS FALHOU"));
    gprs_count++;
    if (gprs_count <= 0) {
      goto AGAIN_GPRS_CON;
    }
    return MODEM_ERROR_GPRS;
  } else {
    Serial.println(F("SETUP GSM BEM SUCEDIDO"));
    return MODEM_READY;
  }
}

ModemGPRSStatus ModemGPRS::reconnect(){
AGAIN_GPRS:
  if (!modemGSM.restart())
  {
    Serial.println(F("FALHOU REINICIALIZACAO DO MODEM GSM"));
  }
  if (!modemGSM.waitForNetwork())
  {
    Serial.println(F("FALHA DE CONEXÃO COM A REDE"));
    delay(3000);
  } 
  if (modemGSM. isNetworkConnected ()) {
    Serial.println(F("CONECTADO A REDE COM SUCESSO"));
  }else{
    Serial.println(F("NÃO FOI POSSIVEL CONECTAR A REDE")); 
  }
  String imei = modemGSM.getIMEI();
  Serial.print(F("IMEI:"));
  Serial.println(imei);
  int gprs_count = 0;
  AGAIN_GPRS_CON:
  //Conecta à rede gprs (APN, usuário, senha)
  if (!modemGSM.gprsConnect("m2mprepago.br", "Arqia", "Arqia")) {
  //if (!modemGSM.gprsConnect("zap.vivo.com.br", "vivo", "vivo")) {
  //if (!modemGSM.gprsConnect("gprs.oi.com.br", "oi", "oi")) {
    delay(2000);
    Serial.print(F("TENTATIVA = "));
    Serial.println(gprs_count);
    Serial.println(F("CONEXÃO DE DADOS FALHOU"));
    gprs_count++;
    if (gprs_count <= 1) {
      goto AGAIN_GPRS_CON;
    }else{
      Serial.println(F("IMPOSSIVEL ESTABELECER CONEXÃO DE DADOS"));
      return;
    }
  }else{
    Serial.println(F("SETUP GSM BEM SUCEDIDO"));
    delay(1500);
  }
  bool res = modemGSM.isGprsConnected();
  Serial.print(F("GPRS status:"));
  Serial.println(res);
  if (res){
    return MODEM_OK_RESTART;
  }else{
    return MODEM_ERROR_RESTART;
  }
}

MQTTConnection::MQTTConnection(){

}

MQTTStatus MQTTConnection::setup(const char * domain, uint16_t port, const char * user, const char * password)
{
  Serial.println(F("Connecting to MQTT Server..."));
  client.setServer(domain, port);
  bool conn = client.connect("çfdkf", user, password);
  Serial.println("teste" + conn);
  client.disconnect();
  if (client.connect(_buffer, user, password)) {
    return MQTT_READY;
    delay(2000);
  } else {
    Serial.print(F("error = not connected"));
    Serial.print(F("error = "));
    Serial.println(client.state());
    delay(5000);
    client.disconnect();
    return MQTT_FAILED;
  }
}

MQTTStatus MQTTConnection::reconnect(const char * user, const char * password){
  MQTTStatus result;
  if(client.state() == 0){
    Serial.println(F("MQTT CONECTADO 1"));
    return MQTT_READY;
  }else{
    (uniqueNumber.getUID()).toCharArray(_buffer, 24);
    if(client.connect("teste", user, password)){
      Serial.println(F("MQTT CONECTADO 2"));
      delay(2000);
      return MQTT_READY;
    }else{
      result = mqtt.setup(MQTT_SERVER, MQTT_PORT, USER, PASS);
      if (result == MQTT_READY){
        return result;
      }else{
        client.disconnect();
        Serial.println(F("MQTT ERROR"));
        return MQTT_FAILED;
      }
    }
  }
}

boolean MQTTConnection::send(String topic, String payload){
  // check if connection is ok
  bool flag = true;
  if(!modemGSM.isGprsConnected()){
    Serial.println(F("Step 1"));
    flag = false;
  };
  if(!modemGSM.isNetworkConnected()){
    Serial.println(F("Step 2"));
    flag = false;
  };
  if(flag == false){
    Serial.println(F("step 3"));
    if(internet.reconnect() == MODEM_OK_RESTART){
      flag = true;
    }else{
      return false;
    } 
  }
  _payload = payload;
  _topic = topic;
  int steps = 0;
  AGAIN_MQTT:
  Serial.println(F("Step 5"));
  mqtt.reconnect(USER, PASS);
  if (client.publish(_topic.c_str(), _payload.c_str())){
    return true;
  }else{
    if(steps < 1){
      Serial.print(F("tentativa = "));
      Serial.println(steps);
      steps++;
      goto AGAIN_MQTT;
    }else{
      return false;
    }
  }
}

Menu::Menu(){
}

void Menu::menuConfiguracao(){
  Serial.println(F(" ENTROU MENU CONFIGURACAO"));
  screen.drawMenu(SCREEN_MENU_CONFIGURACAO);
  do {
    tecla_presionada = keyboard.getKey();
  } while (!tecla_presionada);
  switch (tecla_presionada)
  {
    case '1':
      menu.menuAccesses(SCREEN_CONFIGURACAO_REGISTRO_CHOICE);
      Serial.println(F("BOTAO 1 - REGISTRO DE DISPOSITIVO"));
      break;
    case '2':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
    case '*':
    case '#':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      Serial.println(F("SAIU DO MENU"));
      loop();
      break;
  }
}

void Menu::menuPrincipal(){
  Serial.println(F(" ENTROU MENU PRINCIPAL"));
  screen.drawMenu(SCREEN_MENU_PRINCIPAL);
  do {
    tecla_presionada = keyboard.getKey();
  }
  while (!tecla_presionada);   

  switch (tecla_presionada)       
  {
    case '1':
      menu.menuCadastro();
      Serial.println(F("BOTAO 1"));
      break;
    case '2':
      menu.menuConfiguracao();
      Serial.println(F("BOTAO 2"));
      break;
    case '3':
      //Menu_Entradas();
      Serial.println(F("BOTAO 3"));
      break;
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
    case '*':
    case '#':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      loop();
      Serial.println(F("SAIU DO MENU"));
      break;
  }
}

void Menu::menuCadastro(){
  Serial.println(F("ENTROU MENU CADASTRO"));
  screen.drawMenu(SCREEN_MENU_CADASTRO);
  do {
    tecla_presionada = keyboard.getKey();
  } while (!tecla_presionada);
  switch (tecla_presionada)
  {
    case '1':
      menu.menuAccesses(SCREEN_MENU_CADASTRO_OPERADOR_CHOICE);
      Serial.println(F("BOTAO 1 - CADASTRO DE OPERADOR"));
      break;
    case '2':
      menu.menuAccesses(SCREEN_MENU_CADASTRO_VEHICLE_CHOICE);
      Serial.println(F("BOTAO 2 - CADASTRO DE VEICULO"));
      break;
    case '3':
      menu.menuAccesses(SCREEN_MENU_CADASTRO_PERMISSION_CHOICE);
      Serial.println(F("BOTAO 3 - CADASTRO DE PERMISSAO"));
      break;
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
    case '*':
    case '#':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      Serial.println(F("SAIU DO MENU"));
      loop();
      break;
  }
}

void Menu::menuAccesses(ScreenName nextScreen){
  _nextScreen = nextScreen;
  Serial.println(F("ENTROU NO MENU DE AUTORIZACAO DE ACESSO"));
  screen.drawMenu(SCREEN_ACCCESSES);
  do {
    tecla_presionada = keyboard.getKey();
  } while (!tecla_presionada);

  switch (tecla_presionada)
  {
    case '1':
      Serial.println(F("SENHA"));
      menu.menuAccesses(PASSWORD, _nextScreen);
      break;
    case '2':
      menu.menuAccesses(CARD, _nextScreen);
      Serial.println(F("CARTAO"));
      break;

    case '*':
      loop();
      Serial.println(F("CANCELAR"));
      break;
  }
}

void Menu::menuAccesses(MetodeAccesses metode, ScreenName nextScreen){
  _nextScreen = nextScreen;
  _metode = metode;
  _flag = false;

  switch (_metode)
  {
    case CARD:
      memset(access._buffer, 0, sizeof(access._buffer));
      screen.drawMenu(SCREEN_ACCCESSES_CARD);
      if(access.accessValidate(_metode)){
        Serial.println("Senha valida");
        _flag = true; 
      }else{
        Serial.println("Acesso negado");
        _flag = false;
      };
      break;
    case PASSWORD:
      memset(access._buffer, 0, sizeof(access._buffer));
      screen.drawMenu(SCREEN_ACCCESSES_PASSWORD);
      if(access.accessValidate(_metode)){
        Serial.println("Senha valida");
        _flag = true;        
      }else{
        Serial.println("Acesso negado");
        _flag = false;
      }
      break;
    default:
      Serial.println(F("CANCELAR"));
      loop();    
      break;
  }
  Serial.println(F("teste"));
  if (_flag == true)
  {  
    switch (_nextScreen)
    {
      case SCREEN_MENU_CADASTRO_OPERADOR_CHOICE:
        screen.drawMenu(_nextScreen);
        do {
          tecla_presionada = keyboard.getKey();
        } while (!tecla_presionada);
        Serial.print("Tecla pressionada = ");
        Serial.println(tecla_presionada);
        switch (tecla_presionada)
        {
          case '1':
            menu.menuCadastroOperador();
            break;
          case '2':
            //[ ] implement metode to delete operator
            break;
          default:
            Serial.println(F("CANCELAR"));
            loop();
            break;
        }
        break;
      case SCREEN_MENU_CADASTRO_VEHICLE_CHOICE:
        screen.drawMenu(_nextScreen);
        do {
          tecla_presionada = keyboard.getKey();
        } while (!tecla_presionada);
        Serial.print("Tecla pressionada = ");
        Serial.println(tecla_presionada);
        switch (tecla_presionada)
        {
          case '1':
            menu.menuCadastroVeiculo();
            break;
          case '2':
            //[ ] implement metode to delete veichle
            break;
          default:
            Serial.println(F("CANCELAR"));
            loop();
            break;
        }
        break;
      case SCREEN_MENU_CADASTRO_PERMISSION_CHOICE:
        screen.drawMenu(_nextScreen);
        do {
          tecla_presionada = keyboard.getKey();
        } while (!tecla_presionada);
        Serial.print("Tecla pressionada = ");
        Serial.println(tecla_presionada);
        switch (tecla_presionada)
        {
          case '1':
            menu.permissionRegistrationMenu();
            break;
          default:
            Serial.println(F("CANCELAR"));
            loop();
            break;
        }
        break;
      case SCREEN_CONFIGURACAO_REGISTRO_CHOICE:
        screen.drawMenu(_nextScreen);
        do {
          tecla_presionada = keyboard.getKey();
        } while (!tecla_presionada);
        Serial.print("Tecla pressionada = ");
        Serial.println(tecla_presionada);
        switch (tecla_presionada)
        {
          case '1':
            menu.assignDeviceRegister(); // [ ] TODO
            break;
          default:
            Serial.println(F("CANCELAR"));
            loop();
            break;
        }
        break;  
      default:
        loop();
        break;
    }
  }else{
    screen.drawMenu(SCREEN_ERROR);
    buzzer.somErrado(ledErrado, 250, 50);
    delay(1500);
    loop();
  }
}

void Menu::menuCadastroOperador(){
  memset(_buffer, 0, sizeof(_buffer));  
  screen.drawMenu(SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD);
  do {
    _successRead =  rfidReader.getID();
    tecla_presionada = keyboard.getKey();
    if ( tecla_presionada == '*') {
      Serial.println(F("CANCELADO"));
      loop();
    }
  } while (!_successRead);
  _UUIDCard = rfidReader.IDValue;
  _UUIDCard.toCharArray(_buffer, 24);
  screen.drawMenu(SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD);
  memset(_buffer, 0, sizeof(_buffer));
  delay(1200);
 //check if card exist
  screen.drawMenu(SCREEN_PROGRESS);
  if (!sd.begin(_pinDatalogger)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro no sistema para gravar informacoes");
    delay(1500);
    loop();
  }
  if(sd.checkOperatorExist(_UUIDCard)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Cartao com cadastro existente");
    delay(1500);
    loop();
  }
  _operatorName = key.keyboardGetKeyAlfanumeric(SCREEN_MENU_CADASTRO_OPERADOR_READ_NAME);
  _operatorName.trim();
  Serial.println("Nome = " + _operatorName);
 //enter operator level
  screen.drawMenu(SCREEN_MENU_CADASTRO_OPERADOR_READ_LEVEL);
  do {
    tecla_presionada = keyboard.getKey();  
  } while (!tecla_presionada);
  switch (tecla_presionada)
  {
    case '1':
      Serial.println(F("ADMINISTRADOR"));
      _operatorlevel = 1;
      break;
    case '2':
      Serial.println(F("FRENTISTA"));
      _operatorlevel = 2;
      break;
    case '3':
      Serial.println(F("MOTORISTA"));
      _operatorlevel = 3;
      break;
    default:
      loop();
    break;
  }
  Serial.println(_operatorlevel);
  // gravar novo operador no cartão SD
  screen.drawMenu(SCREEN_PROGRESS);
  sd.WriteOperatorInDatalogger();
  // Monta o JSON para enviar para o Broker
    screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = json.jsonOperatorMount();
  Serial.println(jsonPayload);
  // enviar novo operador por mqtt para broker
    screen.drawMenu(SCREEN_PROGRESS);
  if(mqtt.send(TOPIC_REGISTER, jsonPayload)){
    Serial.println("Eviou MQTT");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }else{
    sd.WriteFailMqttLog(jsonPayload);
    Serial.println("Erro MQTT final");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }
  loop();
}

void Menu::menuCadastroVeiculo(){
  memset(_buffer, 0, sizeof(_buffer));  
  screen.drawMenu(SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD);
  do {
    _successRead =  rfidReader.getID();
    tecla_presionada = keyboard.getKey();
    if ( tecla_presionada == '*') {
      Serial.println(F("CANCELADO"));
      loop();
    }
  } while (!_successRead);
  _UUIDCard = rfidReader.IDValue;
  _UUIDCard.toCharArray(_buffer, 24);
  screen.drawMenu(SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD);
  memset(_buffer, 0, sizeof(_buffer));
  delay(1200);
 //check if card exist
  screen.drawMenu(SCREEN_PROGRESS);
  if (!sd.begin(_pinDatalogger)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro no sistema para gravar informacoes");
    delay(1500);
    loop();
  }
  if(sd.checkVehicleExist(_UUIDCard)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Cartao com cadastro existente");
    delay(1500);
    loop();
  }
  _vehicleName = key.keyboardGetKeyAlfanumeric(SCREEN_MENU_CADASTRO_VEHICLE_READ_NAME);
  _vehicleName.trim();
  Serial.println("Veiculo = " + _vehicleName);
 //enter vehicle fuel type
  screen.drawMenu(SCREEN_MENU_CADASTRO_VEHICLE_READ_LEVEL);
  do {
    tecla_presionada = keyboard.getKey();  
  } while (!tecla_presionada);
  switch (tecla_presionada)
  {
    case '1':
      Serial.println(F("ADMINISTRADOR"));
      _vehicleFuel = DIESEL_S10;
      break;
    case '2':
      Serial.println(F("FRENTISTA"));
      _vehicleFuel = DIESEL_S500;
      break;
    default:
      loop();
    break;
  }
  Serial.println(_vehicleFuel);
  screen.drawMenu(SCREEN_PROGRESS);
  sd.WriteVehicleInDatalogger();
  // Monta o JSON para enviar para o Broker
  screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = json.jsonVehicleMount();
  Serial.println(jsonPayload);
  // enviar novo operador por mqtt para broker
    screen.drawMenu(SCREEN_PROGRESS);
  if(mqtt.send(TOPIC_REGISTER, jsonPayload)){
    Serial.println("Eviou MQTT");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }else{
    sd.WriteFailMqttLog(jsonPayload);
    Serial.println("Erro MQTT final");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }
  delay(5000);
}

void Menu::permissionRegistrationMenu(){
  memset(_buffer, 0, sizeof(_buffer));  
  screen.drawMenu(SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD);
  do {
    _successRead =  rfidReader.getID();
    tecla_presionada = keyboard.getKey();
    if ( tecla_presionada == '*') {
      Serial.println(F("CANCELADO"));
      loop();
    }
  } while (!_successRead);
  _UUIDCard = rfidReader.IDValue;
  _UUIDCard.toCharArray(_buffer, 24);
  screen.drawMenu(SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD);
  memset(_buffer, 0, sizeof(_buffer));
  delay(1200);
 //check if card exist
  screen.drawMenu(SCREEN_PROGRESS);
  if (!sd.begin(_pinDatalogger)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro no sistema para ler informacoes");
    delay(1500);
    loop();
  }
  if(!sd.checkOperatorExist(_UUIDCard)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Cartao sem cadastro existente");
    delay(1500);
    loop();
  }
  if(sd.checkOperatorIsAdmin(_UUIDCard)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Administrador");
    delay(1500);
    loop();
  }

  _UUIDPermission = _UUIDCard;

  memset(_buffer, 0, sizeof(_buffer));  
  screen.drawMenu(SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD);
  do {
    _successRead =  rfidReader.getID();
    tecla_presionada = keyboard.getKey();
    if ( tecla_presionada == '*') {
      Serial.println(F("CANCELADO"));
      loop();
    }
  } while (!_successRead);
  _UUIDCard = rfidReader.IDValue;
  _UUIDCard.toCharArray(_buffer, 24);
  screen.drawMenu(SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD);
  memset(_buffer, 0, sizeof(_buffer));
  delay(1200);
  screen.drawMenu(SCREEN_PROGRESS);
  if (!sd.begin(_pinDatalogger)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Erro no sistema para ler informacoes");
    delay(1500);
    loop();
  }
  if(!sd.checkVehicleExist(_UUIDCard)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Cartao sem cadastro existente");
    delay(1500);
    loop();
  }
  _UUIDPermission.concat(_UUIDCard);
  Serial.println(_UUIDPermission);

  sd.WritePermissionInDatalogger();
  // Monta o JSON para enviar para o Broker
  screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = json.jsonPermissionMount();
  Serial.println(jsonPayload);
  // enviar novo operador por mqtt para broker
    screen.drawMenu(SCREEN_PROGRESS);
  if(mqtt.send(TOPIC_REGISTER, jsonPayload)){
    Serial.println("Eviou MQTT");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }else{
    sd.WriteFailMqttLog(jsonPayload);
    Serial.println("Erro MQTT final");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }
  delay(5000);
}

void Menu::assignDeviceRegister(){
  _companyNumber = key.keyboardGetKeyAlfanumeric(SCREEN_CONFIGURACAO_REGISTRO);
  _companyNumber.trim();
  Serial.println("compainha = " + _companyNumber);
  if(sd.checkCompanyExist(_companyNumber)){
    buzzer.somErrado(ledErrado, 250, 50);
    screen.drawMenu(SCREEN_ERROR);
    Serial.println("Cadastro existente");
    delay(1500);
    loop();
  }
  sd.WriteCompanyRegisterInDatalogger();
  screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = json.jsonAssignMount();
  Serial.println(jsonPayload);
    screen.drawMenu(SCREEN_PROGRESS);
  if(mqtt.send(TOPIC_REGISTER, jsonPayload)){
    Serial.println("Eviou MQTT");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }else{
    sd.WriteFailMqttLog(jsonPayload);
    Serial.println("Erro MQTT final");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }
  loop();
}

Access::Access(){
}

bool Access::accessValidate(MetodeAccesses metode){
  memset(_buffer, 0, sizeof(_buffer));
  _metode = metode;
  _position = 0;

  switch (_metode)
  {
    case PASSWORD:
      do {
        _keyPressed = key.keyboardGetKeyNumeric();
        switch (_keyPressed)
        {
          case 'C':
            memset(_buffer, 0, sizeof(_buffer));
            if (_position > 0){
            _position = 0;
            }
          break;
          case 'B':
            if (_position > 0 && _position != 14) {
                _position--;
              }
              if (_position == 14 && _buffer[_position] == ' ') {
                _position--;
              }
              if (_position == 0 && _buffer[_position] == ' ') {
                _position = 0;
              }
              _buffer[_position] = ' ';    
          break;
          case 'A':
            _secret = "";
            for (byte i = 0; i < sizeof(_buffer); i++) {
              _secret.concat(String(_buffer[i]));
            }
            Serial.print("Senha digitada = ");
            Serial.println(_secret);
            if (_secret == SENHA_AGS) {
              Serial.println(F("Senha correta"));
              return true;
            } else {
              Serial.println(F("Senha incorreta"));
              return false;
            }
          break;
          case '*':
            memset(_buffer, 0, sizeof(_buffer));
            if (_position > 0){
            _position = 0;
            }
            loop();
          break;
          case '0':
          if (_position < 15){
            Serial.println("entrou no case 0");
            _buffer[_position] = '0';
            _position++;  
          }
          break;
          
          case '1':
          if (_position < 15){
            Serial.println("entrou no case 1");
            _buffer[_position] = '1';
            _position++;  
          }
          break;
          
          case '2':
          if (_position < 15){
            Serial.println("entrou no case 2");
            _buffer[_position] = '2';
            _position++;  
          }
          break;
          
          case '3':
          if (_position < 15){
            Serial.println("entrou no case 3");
            _buffer[_position] = '3';
            _position++;  
          }
          break;
          
          case '4':
          if (_position < 15){
            Serial.println("entrou no case 4");
            _buffer[_position] = '4';
            _position++;  
          }
          break;
          
          case '5':
          if (_position < 15){
            Serial.println("entrou no case 5");
            _buffer[_position] = '5';
            _position++;  
          }
          break;
          
          case '6':
          if (_position < 15){
            Serial.println("entrou no case 6");
            _buffer[_position] = '6';
            _position++;  
          }
          break;
          
          case '7':
          if (_position < 15){
            Serial.println("entrou no case 7");
            _buffer[_position] = '7';
            _position++;  
          }
          break;
          
          case '8':
          if (_position < 15){
            Serial.println("entrou no case 8");
            _buffer[_position] = '8';
            _position++;  
          }
          break;
          
          case '9':
          if (_position < 15){
            Serial.println("entrou no case 9");
            _buffer[_position] = '9';
            _position++;  
          }
          break;
        }
        Serial.println(_keyPressed);
        screen.drawMenu(SCREEN_ACCCESSES_PASSWORD);
      } while (_keyPressed != '.');
    break;
    case CARD:
    rfid.PCD_Init(SS_SDA_PIN, RST_PIN);
    Serial.println(F("PASSE A TAG DO VEICULO"));
    do {
      cardIsRead =  rfidReader.getID();
      tecla_presionada = keyboard.getKey();
    } while (!cardIsRead);    delay(500);
    Serial.println(rfidReader.IDValue);
    screen.readOperator(SCREEN_OPERATOR_SEARCH, "", rfidReader.IDValue);
    delay(500);
    _operatorLevel = sd.getOperator(rfidReader.IDValue);
    Serial.println(_operatorLevel);
    if(_operatorLevel != "NO_OPERATOR"){
      if(sd.checkOperatorIsAdmin(rfidReader.IDValue)){
        return true;
      }else{
        return false;
      }
    }else{
      screen.readOperator(SCREEN_OPERATOR_NOT_FOUND, "", rfidReader.IDValue);
      screen.drawMenu(SCREEN_ERROR);
      buzzer.somErrado(ledErrado, 250, 50);
      delay(1500);
      loop();
    }
    break;  
    default:
    break;
  }
}

Keyboard::Keyboard(){
}

char Keyboard::keyboardGetKeyNumeric(){
  do{
  _pressedKey = keyboard.getKey();
  } while (!_pressedKey);
  return _pressedKey;
}

String Keyboard::keyboardGetKeyAlfanumeric(ScreenName targetScreen){
  _offset = 250;
    memset(_buffer, 0, sizeof(_buffer) -1);
    _cursorPossition = 0;
  _screen = targetScreen;
  screen.drawMenu(targetScreen);
  _timesPressed = 0;
  _counter = 0;
  _elapsedTime = millis();
  do{
    _pressedKey = keyboard.getKey();
    if (millis() > _lastTime + _offset)
    {
      _lastTime = millis();
      if (_timesPressed > 0)
      {
        _counter++;
        if (_counter == 3)
        {
          if (_cursorPossition < 14) {
            _cursorPossition++;
          }
          _timesPressed = 0;
        }
      }
    }
    switch (_pressedKey)
    {

      case 'D':
      case '#':
      case '*':
        loop();
      break;

      case 'A':
        if (_lastPressedKey != 'A' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = 'A';
        if (_timesPressed > 1)
        {
          _timesPressed = 1;
          break;
        }
        
        if (_timesPressed == 1)
        { 
          NameValue = ' ';
          for (byte i = 0; i < sizeof(_buffer); i++)
            {
              NameValue.concat(String(_buffer[i]));
              Serial.println(NameValue);
            }
            NameValue.toUpperCase();
            return NameValue;
        }
      break;

      case 'B':
        if (_lastPressedKey != 'B' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = 'B';

        if (_timesPressed > 1)
        {
          _timesPressed = 1;
          break;
        }

        if (_timesPressed == 1)
        {
          if (_cursorPossition > 0 && _cursorPossition != 14){
            _cursorPossition--;
          }
          if (_cursorPossition == 14 && _buffer[_cursorPossition] == ' '){
            _cursorPossition--;
          }
          _buffer[_cursorPossition] = ' ';
          screen.drawMenu(targetScreen); 
        }
        if (_cursorPossition > 0 && _cursorPossition != 14) {
          _cursorPossition--;
        }
        if (_cursorPossition == 14 && _buffer[_cursorPossition] == ' ') {
          _cursorPossition--;
        }
        break;
      break;

      case 'C':
        if (_lastPressedKey != 'C' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = 'C';
        if (_timesPressed > 1)
        {
          _timesPressed = 1;
          break;
        }
        
        if (_timesPressed == 1)
        {
          memset(_buffer, 0, sizeof(_buffer));
          _cursorPossition = 0;
          screen.drawMenu(targetScreen); 
        }
        break;

      case '0':
        if (_lastPressedKey != '0' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '0';
        if (_timesPressed > 5)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = '+';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = '-';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = '*';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '/';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 5)
        {
          _buffer[_cursorPossition] = '0';
          screen.drawMenu(targetScreen);
        }
      break;

      case '1':
        if (_lastPressedKey != '1' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '1';
        if (_timesPressed > 5)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = '(';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = ')';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = '.';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = ',';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 5)
        {
          _buffer[_cursorPossition] = '1';
          screen.drawMenu(targetScreen);
        }
      break;

      case '2':
        if (_lastPressedKey != '2' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '2';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'A';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'B';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'C';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '2';
          screen.drawMenu(targetScreen);
        }
      break;

      case '3':
        if (_lastPressedKey != '3' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '3';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'D';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'E';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'F';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '3';
          screen.drawMenu(targetScreen);
        }
      break;  

      case '4':
        if (_lastPressedKey != '4' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '4';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'G';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'H';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'I';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '4';
          screen.drawMenu(targetScreen);
        }
      break;

      case '5':
        if (_lastPressedKey != '5' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '5';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'J';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'K';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'L';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '5';
          screen.drawMenu(targetScreen);
        }
      break;

      case '6':
        if (_lastPressedKey != '6' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '6';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'M';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'N';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'O';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '6';
          screen.drawMenu(targetScreen);
        }
      break;

      case '7':
        if (_lastPressedKey != '7' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '7';
        if (_timesPressed > 5)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'P';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'Q';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'R';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = 'S';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 5)
        {
          _buffer[_cursorPossition] = '7';
          screen.drawMenu(targetScreen);
        }
      break;

      case '8':
        if (_lastPressedKey != '8' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '8';
        if (_timesPressed > 4)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'T';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'U';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'V';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = '8';
          screen.drawMenu(targetScreen);
        }
      break;

      case '9':
        if (_lastPressedKey != '9' && _timesPressed > 0)
        {
          _timesPressed = 0;
        }
        _elapsedTime = millis();
        _timesPressed++;
        _counter = 0;
        _lastPressedKey = '9';
        if (_timesPressed > 5)
        {
          _timesPressed = 1;
        }
        if (_timesPressed == 1)
        {
          _buffer[_cursorPossition] = 'W';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 2)
        {
          _buffer[_cursorPossition] = 'X';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 3)
        {
          _buffer[_cursorPossition] = 'Y';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 4)
        {
          _buffer[_cursorPossition] = 'Z';
          screen.drawMenu(targetScreen);
        }
        if (_timesPressed == 5)
        {
          _buffer[_cursorPossition] = '9';
          screen.drawMenu(targetScreen);
        }
      break;

      default:
      break;
    }
  } while (_pressedKey != '?');
  Serial.println("Saiu da funcao do teclado alfa");
}

Operator::Operator(){
}

String Operator::Read(){
  Serial.println(F("==== FUNCAO OPERADOR =========================================="));
  _operator = "";
  do {
    screen.readOperator(SCREEN_OPERATOR_READ, "", "");
    rfid.PCD_Init(SS_SDA_PIN, RST_PIN);
    Serial.println(F("PASSE O CARTÃO DO OPERADOR"));
    do {
      successRead =  rfidReader.getID();
      tecla_presionada = keyboard.getKey();
      if ( tecla_presionada == 'A') {
        Serial.println(F("ACESSANDO MENU"));
        menu.menuPrincipal();
      }
    } while (!successRead);
    Serial.println(rfidReader.IDValue);
    screen.readOperator(SCREEN_OPERATOR_SEARCH, "", rfidReader.IDValue);
    _operator = sd.getOperator(rfidReader.IDValue);
    Serial.println(_operator);
    if(_operator != "NO_OPERATOR"){
    screen.readOperator(SCREEN_OPERATOR_FOUND,
    _operator.substring(_operator.indexOf("#") + 1, _operator.lastIndexOf("#")), 
    rfidReader.IDValue);
    buzzer.somCerto(ledCerto, 50);
    return _operator;
    }else{
      screen.readOperator(SCREEN_OPERATOR_NOT_FOUND, "", rfidReader.IDValue);
      screen.drawMenu(SCREEN_ERROR);
      buzzer.somErrado(ledErrado, 250, 50);
      delay(1500);
      loop();
    }
    delay(100);
  } while (status != 1);
}


Vehicle::Vehicle(){
}

String Vehicle::Read(){
  Serial.println(F("==== FUNCAO VEICULO =========================================="));
  _vehicle = "";
  do {
    screen.readOperator(SCREEN_OPERATOR_READ, "", "");
    rfid.PCD_Init(SS_SDA_PIN, RST_PIN);
    Serial.println(F("PASSE A TAG DO VEICULO"));
    do {
      successRead =  rfidReader.getID();
      tecla_presionada = keyboard.getKey();
    } while (!successRead);
    Serial.println(rfidReader.IDValue);
    screen.readOperator(SCREEN_OPERATOR_SEARCH, "", rfidReader.IDValue);
    _vehicle = sd.getVehicle(rfidReader.IDValue);
    Serial.println(_vehicle);
    if(_vehicle != "NO_VEHICLE"){
    screen.readOperator(SCREEN_OPERATOR_FOUND,
    _vehicle.substring(_vehicle.indexOf("#") + 1, _vehicle.lastIndexOf("#")), 
    rfidReader.IDValue);
    buzzer.somCerto(ledCerto, 50);
    return _vehicle;
    }else{
      screen.readOperator(SCREEN_OPERATOR_NOT_FOUND, "", rfidReader.IDValue);
      screen.drawMenu(SCREEN_ERROR);
      buzzer.somErrado(ledErrado, 250, 50);
      delay(1500);
      loop();
    }
    return "error";
    delay(100);
  } while (status != 1);
  return "Error";
}

Permission::Permission(){
}

bool Permission::check(String uuid){
  return (sd.checkPermissionExist(uuid));
}

void CANAL_A() {
  // CANAL_A é ativado se o pino digital 2 sai de nivel baixo(LOW) para nivel alto(HIGH).
  // Faz a chcagem do nivel logico do pino digital 3 para determinar a direção
  if (digitalRead(3) == LOW) {
    counter++;
  } else {
    counter--;
  }
}

void CANAL_B() {
  // CANAL_B é ativado se o pino digital 3 sai de nivel baixo(LOW) para nivel alto(HIGH).
  // Faz a chcagem do nivel logico do pino digital 2 para determinar a direção
  if (digitalRead(2) == LOW) {
    counter--;
  } else {
    counter++;
  }
}
void CANAL_C() {
  // CANAL_C é ativado se o pino digital 19 sai de nivel baixo(LOW) para nivel alto(HIGH).
  // Faz a chcagem do nivel logico do pino digital 18 para determinar a direção
  if (digitalRead(18) == LOW) {
    counter++;
  } else {
    counter--;
  }
}

void CANAL_D() {
  // CANAL_D é ativado se o pino digital 18 sai de nivel baixo(LOW) para nivel alto(HIGH).
  // Faz a chcagem do nivel logico do pino digital 19 para determinar a direção
  if (digitalRead(19) == LOW) {
    counter--;
  } else {
    counter++;
  }
}

Pump::Pump(){
  pinMode(RELE_01, OUTPUT);
  pinMode(RELE_02, OUTPUT);
  pinMode(BOTAO, INPUT);
  digitalWrite(BOTAO, LOW);
}

float Pump::fuelLoad(Fuel fuel){
  counter = 0;
  _buttonStatus = 0;
  fuelQuantity = 0;
  screen.drawScreen(SCREEN_PUMP_CHARGE_FUEL, String(fuelQuantity));
  switch (fuel)
  {
  case S500:
    _pump = RELE_02;
    digitalWrite(_pump, HIGH);
    do {
      attachInterrupt(0, CANAL_A, RISING); //A rampa de subida do da interrupção 4 ativa a função CANAL_D(). AttachInterrupt 4 esta ligado no pino 19 do Arduino Mega.
      attachInterrupt(1, CANAL_B, RISING); //A rampa de subida do da interrupção 5 ativa a função CANAL_D(). AttachInterrupt 4 esta ligado no pino 19 do Arduino Mega.
      _buttonStatus = digitalRead(_pump);
      delay(10);
      if (_buttonStatus == 1) {
        fuelQuantity = (float)counter * 0.0025;
        if ((counter < 20) || (counter > 4000000)) {
          fuelQuantity = 0;
        } else {
          // tela de abastecimento
          screen.drawScreen(SCREEN_PUMP_CHARGE_FUEL, String(fuelQuantity));
        }
        delay(100);
      }
      if (digitalRead(BOTAO) == HIGH) { // Condicao que interrompe o abastecimento
        digitalWrite(_pump, LOW);
      }
    }
    while (_buttonStatus == 1);
    return fuelQuantity;
    break;
  
  case S10:
    _pump = RELE_01;
    digitalWrite(_pump, HIGH);
    do {
      attachInterrupt(4, CANAL_C, RISING); //A rampa de subida do da interrupção 4 ativa a função CANAL_D(). AttachInterrupt 4 esta ligado no pino 19 do Arduino Mega.
      attachInterrupt(5, CANAL_D, RISING); //A rampa de subida do da interrupção 5 ativa a função CANAL_D(). AttachInterrupt 4 esta ligado no pino 19 do Arduino Mega.
      _buttonStatus = digitalRead(_pump);
      delay(10);
      if (_buttonStatus == 1) {
        fuelQuantity = (float)counter * 0.0025;
        if ((counter < 20) || (counter > 4000000)) {
          fuelQuantity = 0;
        } else {
          // tela de abastecimento
          screen.drawScreen(SCREEN_PUMP_CHARGE_FUEL, String(fuelQuantity));
        }
        delay(100);
      }
      if (digitalRead(BOTAO) == HIGH) { // Condicao que interrompe o abastecimento
        digitalWrite(_pump, LOW);
      }
    }
    while (_buttonStatus == 1);
    return fuelQuantity;
    break;
  
  default:
    break;
  }
}

boolean Pump::registerFueLCharger() {
  screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = "";
  sd.WriteFuelChargeInDatalogger();
  screen.drawMenu(SCREEN_PROGRESS);
  jsonPayload = json.jsonFuelChargeMount();
  Serial.println(jsonPayload);
  // enviar novo abastecimento por mqtt para broker
  screen.drawMenu(SCREEN_PROGRESS);
  if(mqtt.send(TOPIC_REGISTER, jsonPayload)){
    Serial.println("Eviou MQTT");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }else{
    sd.WriteFailMqttLog(jsonPayload);
    Serial.println("Erro MQTT final");
    screen.drawMenu(SCREEN_SUCCESS);
    buzzer.somCerto(ledCerto, 50);
    delay(1500);
    loop();
  }

}

Json::Json(){
}

String Json::jsonOperatorMount(){
  _payload = "";
  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(10, 9);
  //_temp.remove(2, 1);
  //_temp.remove(4, 1);

  _payload = "{";
  _payload += "\"d\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(0, 11);

  _payload += "\"h\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _payload += "\"e\":";
  _payload += "\"";
  _payload += String(uniqueNumber.getUID());
  _payload += "\"";
  _payload += ",";

  _payload += "\"c\":";
  _payload += "\"";
  _payload += String("001");
  _payload += "\"";
  _payload += ",";

  _payload += "\"ot\":";
  _payload += "\"";
  _payload += String(menu._UUIDCard);
  _payload += "\"";
  _payload += ",";

  _payload += "\"on\":";
  _payload += "\"";
  _payload += String(menu._operatorName);
  _payload += "\"";
  _payload += ",";

  _payload += "\"ol\":";
  _payload += "\"";
  _payload += String(menu._operatorlevel);
  _payload += "\"";

  _payload += "}";

  return _payload;
}

String Json::jsonVehicleMount(){
  _payload = "";
  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(10, 9);
  
  _payload = "{";
  _payload += "\"d\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(0, 11);

  _payload += "\"h\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _payload += "\"e\":";
  _payload += "\"";
  _payload += String(uniqueNumber.getUID());
  _payload += "\"";
  _payload += ",";

  _payload += "\"c\":";
  _payload += "\"";
  _payload += String("002");
  _payload += "\"";
  _payload += ",";

  _payload += "\"vt\":";
  _payload += "\"";
  _payload += String(menu._UUIDCard);
  _payload += "\"";
  _payload += ",";

  _payload += "\"vn\":";
  _payload += "\"";
  _payload += String(menu._vehicleName);
  _payload += "\"";
  _payload += ",";

  _payload += "\"vf\":";
  _payload += "\"";
  _payload += String(menu._vehicleFuel);
  _payload += "\"";

  _payload += "}";

  return _payload;
}

String Json::jsonPermissionMount(){
  _payload = "";
  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(10, 9);
  //_temp.remove(2, 1);
  //_temp.remove(4, 1);

  _payload = "{";
  _payload += "\"d\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(0, 11);

  _payload += "\"h\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _payload += "\"e\":";
  _payload += "\"";
  _payload += String(uniqueNumber.getUID());
  _payload += "\"";
  _payload += ",";

  _payload += "\"c\":";
  _payload += "\"";
  _payload += String("003");
  _payload += "\"";
  _payload += ",";

  _payload += "\"pt\":";
  _payload += "\"";
  _payload += String(menu._UUIDPermission);
  _payload += "\"";

  _payload += "}";

  return _payload;

}

String Json::jsonFuelChargeMount(){
  _payload = "";
  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(10, 9);

  _payload = "{";
  _payload += "\"d\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(0, 11);

  _payload += "\"h\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _payload += "\"e\":";
  _payload += "\"";
  _payload += String(uniqueNumber.getUID());
  _payload += "\"";
  _payload += ",";

  _payload += "\"c\":";
  _payload += "\"";
  _payload += String("099");
  _payload += "\"";
  _payload += ",";

  _payload += "\"ot\":";
  _payload += "\"";
  _payload += String(sd._operatorUuid);
  _payload += "\"";
  _payload += ",";

  _payload += "\"on\":";
  _payload += "\"";
  _payload += String(sd._operatorName);
  _payload += "\"";
  _payload += ",";

  _payload += "\"ol\":";
  _payload += "\"";
  _payload += String(sd._operatorLevel);
  _payload += "\"";
  _payload += ",";

  _payload += "\"vt\":";
  _payload += "\"";
  _payload += String(sd._vehicleUuid);
  _payload += "\"";
  _payload += ",";

  _payload += "\"vn\":";
  _payload += "\"";
  _payload += String(sd._vehicleName);
  _payload += "\"";
  _payload += ",";

  _payload += "\"vf\":";
  _payload += "\"";
  _payload += String(sd._vehicleFuel);
  _payload += "\"";
  _payload += ",";

  _payload += "\"fq\":";
  _payload += "\"";
  _payload += String(fuelQuantity);
  _payload += "\"";
  _payload += ",";

  _payload += "\"cy\":";
  _payload += "\"";
  _payload += String(_COMPANY);
  _payload += "\"";

  _payload += "}";

  return _payload;
}

String Json::jsonAssignMount(){
  _payload = "";
  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(10, 9);

  _payload = "{";
  _payload += "\"d\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _temp = "";
  _temp = String(sd.getTimestamp());
  _temp.remove(0, 11);

  _payload += "\"h\":";
  _payload += "\"";
  _payload += String(_temp);
  _payload += "\"";
  _payload += ",";

  _payload += "\"e\":";
  _payload += "\"";
  _payload += String(uniqueNumber.getUID());
  _payload += "\"";
  _payload += ",";

  _payload += "\"c\":";
  _payload += "\"";
  _payload += String("080");
  _payload += "\"";
  _payload += ",";

  _payload += "\"cy\":";
  _payload += "\"";
  _payload += String(menu._companyNumber);
  _payload += "\"";

  _payload += "}";

  return _payload;  
}


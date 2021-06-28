////////////////////////////////////////////////////////////////////////////////////
//                                                                            //////
// Controle de bomba de abastecimento de Diesel com Bloco Medidor Analogico   //////
// Hardware - Arduino Mega                                                    //////
// Criado por Eliverto Schontz Moraes                                         //////
// 06/01/2020 - V2.0                                                          //////
// 06/08/2020 - V2.1 incluido telas de espera                                 //////
// 02/11/2020 - V3.1 Refatoração completa do cogido                           //////
// 10/12/2020 - V4.0 Refatoração completa usando plataformIO/VScode/GitHub    //////
//  @Commit xxxxxxxxx                                                         //////
//  [x] -> Correção da conectividade Gprs/Mqtt com problema de timeout        //////
//  [x] -> Refatoração dos codigos de tela                                    //////
//  [ ] -> Apresentar codigo de erro com opção de continuar                   //////
//  [ ] -> Incluir liberação de funções com apresentação de cartão MASTER     //////
//  [ ] -> Ocultar caracteres de senha ao digitar                             //////
//  [x] -> Criado lista de codigos de erro                                    //////
//                                                                            //////
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//                              CODIGOS DE ERROS                              //////
//  ------------------------------------------------------------------------  //////
// 1  - SD                                                                    //////
// 3  - RTC                                                                   //////
// 4  - SD E RTC                                                              //////
// 5  - RFID                                                                  //////
// 6  - SD E RFID                                                             //////
// 8  - RTC E RFID                                                            //////
// 9  - SD, RTC, RFID                                                         //////
// 11 - MODEM GPRS                                                            //////
// 12 - SD E MODEM GPRS                                                       //////
// 14 - RTC E MODEM GPRS                                                      //////
// 15 - SD, RTC E MODEM GPRS                                                  //////
// 16 - RFID E MODEM GPRS                                                     //////
// 17 - SD, RFID E MODEM GPRS                                                 //////
// 19 - RTC, RFID E MODEM GPRS                                                //////
// 20 - CONEXÃO MQTT                                                          //////
// 21 - SD E CONEXAO MQTT                                                     //////
// 23 - RTC E CONEXAO MQTT                                                    //////
// 24 - SD, RTC E CONEXAO MQTT                                                //////
// 25 - RFID E CONEXAO MQTT                                                   //////
// 26 - SD, RFID E CONEXAO MQTT                                               //////
// 28 - RTC, RFID E CONEXAO MQTT                                              //////
// 31 - MODEM GPRS E CONEXAO MQTT                                             //////
// 32 - SD, MODEM GPRS E CONEXAO MQTT                                         //////
// 34 - RTC, MODEM GPRS E CONEXAO MQTT                                        //////
// 36 - RFID, MODEM GPRS E CONEXAO MQTT                                       //////
//                                                                            //////
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//                            CODIGOS DE FUNCOES                              //////
//  ------------------------------------------------------------------------  //////
// [x]001  - CADASTRO DE OPERADOR                                             //////
// [ ]002  - CADASTRO DE VEICULO                                              //////
// [ ]003  - CADASTRO DE PERMISSOES                                           //////
// [ ]004  - CADASTRO DE ABASTECIMENTO                                        //////
////////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>                  // Biblioteca de comunicação SPI
#include <Keypad.h>               // Biblioteca para controle do teclado matricial 4x4  
#include <Utils.h>                // Biblioteca com as classes e funções para funcionamento do sistema 

// Declare of constants and variables
const uint8_t PIN_SS_DATA_LOG   =       46;                                   // Configuravel - Pino Slave Select/Chip Select modulo DATALOGGER
const bool GET_SYSTEM_TIMESTAMP =       true;                                // Variavel para setar quando o programa deve pegar a hora do sistema

const uint8_t SOM               =       27;                                   // Pino de ligação do buzzer
const uint8_t LED_VERMELHO      =       29;                                   // Pino de ligação do led vermelho
const uint8_t LED_VERDE         =       31;                                   // Pino de ligação do led verde

const uint8_t CS_PIN            =       10;                                   // Chip Select (Slave Select do protocolo SPI) do modulo Lora
const uint8_t RESET_LORA        =       5;                                    // Reset do modulo LoRa

const uint8_t RELE_02           =       A13;                                  // Pino Rele_02
const uint8_t RELE_01           =       A0;                                   // Pino Rele_01
const uint8_t BOTAO             =       8;                                    // Botao de uso geral

const String TOPIC              =       "Supply/";                            // TOPICO MQTT para publicação

#define MQTT_SERVER                     "mqtt.datamills.com.br"         //"mqtt://things.ubidots.com"
#define MQTT_PORT                       1883                                  // Porta para comunicação com Broker MQTT
#define USER                            "supply-admin"                     // "USUARIO"
#define PASS                            "supply@19!"                        // "SENHA"

uint8_t statusCheck             =       0;

bool stateCheck[8]              =       {0,0,0,0,0,0,0,0};                    // 0=SD, 1=RTC, 2=RFID, 3=MODEN           

const uint8_t errorCode[6]      =       {1,3,5,11,20,0};                       

// Initialize instancies 

Som alert(SOM);                                                               
UID uniqueID;
DrawScreen visor;
RFIDReader leitorRfid;
DataLogger datalogger;
ModemGPRS SIM800l;
MQTTConnection BrokerMQTT;
Operator Operador;

// VARIAVEIS DO SISTEMA //

String OPERADOR_REGISTER;
String OPERADOR_NAME;
String OPERADOR_PERMISSION;
String VEICULO_REG;
String VEICULO_TAG;


void setup() {


  // Initialize and setup pins
  
  pinMode(PIN_SS_DATA_LOG, OUTPUT);
  pinMode(SOM, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(RELE_01, OUTPUT);
  pinMode(RELE_02, OUTPUT);
  pinMode(BOTAO, INPUT);
  digitalWrite(BOTAO, LOW);
  
  // Initialize Serial 
  Serial.begin(9600);

  // Initialize SPI protocol
  SPI.begin();

  // Initialize LCD monitor
  visor.begin();

  // show first screen with logo 
  visor.drawSetup(SCREEN_DRAW_LOGO, 2000, 0, 0);

  // show screen with UID
  visor.drawSetup(SCREEN_INIT, 3000, 0, 0);
  
  // Initialize SD card reader
  if (datalogger.begin(PIN_SS_DATA_LOG) == DATALOGGER_SD_OK){
    Serial.println(F("SD PASSOU"));
    statusCheck = 0;
    stateCheck[0] = 0;
    alert.somCerto(LED_VERDE, 50);
  }else {
    Serial.println(F("SD NAO PASSOU"));
    statusCheck = 1;
    stateCheck[0] = 1;
    alert.somErrado(LED_VERMELHO, 250, 50);
  }
  visor.drawSetup(SCREEN_VERIFY_DATA_LOGGER_SD, 2000, statusCheck, stateCheck);

  // Initialize RTC 
  if (GET_SYSTEM_TIMESTAMP){
    if (datalogger.setSystemTimestamp() == DATALOGGER_TIME_OK){
      statusCheck = 0;
      stateCheck[1] = 0;
      alert.somCerto(LED_VERDE, 50);
    }else{
      statusCheck = 1;
      stateCheck[1] = 1;
      alert.somErrado(LED_VERMELHO, 250, 50);
    }
  }else{
    if (datalogger.getDateHour() == DATALOGGER_TIME_OK){
      statusCheck = 0;
      stateCheck[1] = 0;
      alert.somCerto(LED_VERDE, 50);
    }else{
      statusCheck = 1;
      stateCheck[1] = 1;
      alert.somErrado(LED_VERMELHO, 250, 50);
    }  
  }
  visor.drawSetup(SCREEN_VERIFY_DATA_LOGGER_RTC, 2000, statusCheck, stateCheck);

  // Initialize RFID reader
  if(leitorRfid.begin() == RFID_OK){
    statusCheck = 0;
    stateCheck[2] = 0;
    alert.somCerto(LED_VERDE, 50);
  }else{
    statusCheck = 1;
    stateCheck[2] = 1;
    alert.somErrado(LED_VERMELHO, 250, 50);
  }
  visor.drawSetup(SCREEN_VERIFY_RFID, 2000, statusCheck, stateCheck);
  //goto JUMP;
  // Initialize modem GPRS
  if (SIM800l.setup() == MODEM_READY){
    statusCheck = 0;
    stateCheck[3] = 0;
    alert.somCerto(LED_VERDE, 50);
  }else{
    statusCheck = 1;
    stateCheck[3] = 1;
    alert.somErrado(LED_VERMELHO, 250, 50);
  }
  visor.drawSetup(SCREEN_VERIFY_MODEM, 2000, statusCheck, stateCheck);

  // Initialize MQTT connection
  statusCheck = 0;
  MQTT_AGAIN:
  if (BrokerMQTT.setup(MQTT_SERVER, MQTT_PORT, USER, PASS) == MQTT_READY){
    statusCheck = 0;
    stateCheck[4] = 0;
    alert.somCerto(LED_VERDE, 50);
  }else{
    if (statusCheck < 2){
      goto MQTT_AGAIN;
      statusCheck++;
    }else{
      statusCheck = 1;
      stateCheck[4] = 1;
      alert.somErrado(LED_VERMELHO, 250, 50);
    }
  }
  visor.drawSetup(SCREEN_VERIFY_MQTT, 2000, statusCheck, stateCheck);
  JUMP:
  // Check for errors
  Serial.println("===========================");
  uint8_t errorValue  = 0;
  for (size_t i = 0; i <= 5; i++)
  {
    Serial.print("== error value = ");
    Serial.println(errorValue);
    Serial.print("== posição = ");
    Serial.println(i);
    Serial.print("== stateCheck = ");
    Serial.println(stateCheck[i]);
    Serial.print("== errorCode = ");
    Serial.println(errorCode[i]);
    if (stateCheck[i] == 1){
      errorValue = errorValue + errorCode[i];
    }
  }
  Serial.print(F("Final error Code = "));
  Serial.println(errorValue);
  Serial.println("===========================\n\n") ;
}

void loop() {
  OPERADOR_REGISTER = "";
  OPERADOR_NAME = "";
  OPERADOR_PERMISSION = "";
  VEICULO_REG = "";
  VEICULO_TAG = "";
  Serial.println(F("==============================================================="));
  Serial.println(F("====================== INICIO DO SISTEMA ======================"));
  Serial.println(F("==============================================================="));
  OPERADOR_REGISTER = Operador.Read();
  OPERADOR_NAME = OPERADOR_REGISTER.substring(OPERADOR_REGISTER.indexOf("#") + 1, OPERADOR_REGISTER.lastIndexOf("#"));
  OPERADOR_PERMISSION = OPERADOR_REGISTER.substring(OPERADOR_REGISTER.lastIndexOf("#") + 1);
  OPERADOR_REGISTER = OPERADOR_REGISTER.substring(0, OPERADOR_REGISTER.indexOf("#"));
  Serial.println(OPERADOR_NAME);
  Serial.println(OPERADOR_PERMISSION);
  Serial.println(OPERADOR_REGISTER);
  delay(10000);
}
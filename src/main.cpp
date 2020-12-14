////////////////////////////////////////////////////////////////////////////////////
//                                                                            //////
// Controle de bomba de abastecimento de Diesel com Bloco Medidor Analogico   //////
// Hardware - Arduino Mega                                                    //////
// Criado por Eliverto Schontz Moraes                                         //////
// 06/01/2020 - V2.0                                                          //////
// 06/08/2020 - V2.1 incluido telas de espera                                 //////
// 02/11/2020 - V3.1 Refatoração completa do cogido                           //////
// 10/12/2020 - V4.0 Refatoração completa usando plataformIO/VScode/GitHub    //////
//                                                                            //////
//  [x] -> Correção da conectividade Gprs/Mqtt com problema de timeout        //////
//  [ ] -> Refatoração dos codigos de tela                                    //////
//  [ ] -> Apresentar codigo de erro com opção de continuar                   //////
//  [ ] -> Incluir liberação de funções com apresentação de cartão MASTER     //////
//  [ ] -> Ocultar caracteres de senha ao digitar                             //////
//                                                                            //////
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
//                              CODIGOS DE ERROS                              //////
//  ------------------------------------------------------------------------  //////
// 1  - SD                                                                    //////
// 3  - RFID                                                                  //////
// 5  - LORA                                                                  //////
// 11 - MODEM GSM                                                             //////
// 4  - SD E RFID                                                             //////
// 6  - SD E LORA                                                             //////
// 9  - SD E RFID E LORA                                                      //////
// 8  - RFID E LORA                                                           //////
// 12 - SD E MODEM GSM                                                        //////
// 15 - SD , RFID E MODEM GSM                                                 //////
// 20 - SD ,RFID ,LORA E MODEM GSM                                            //////
// 14 - RFID E MODEM GSM                                                      //////
// 19 - RFID, LORA E MODEM GSM                                                //////
// 16 - LORA E MODEM GSM                                                      //////
//                                                                            //////
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//                            CODIGOS DE FUNCOES                              //////
//  ------------------------------------------------------------------------  //////
// 001  - CADASTRO DE OPERADOR                                                //////
// 002  - CADASTRO DE VEICULO                                                 //////
// 003  - CADASTRO DE PERMISSOES                                              //////
// 004  - CADASTRO DE ABASTECIMENTO                                           //////
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////
//                                CODIGOS DE MENUS                            //////
//  ------------------------------------------------------------------------  //////
// COD_MENU = 11  ---> Menu de Cadastro                                       //////
// COD_MENU = 111  |-> Menu Cadasro Operadores                                //////
// COD_MENU = 112  |-> Menu Cadasro placas                                    //////
// COD_MENU = 113  |-> Menu Cadasro Permissoes                                //////
// COD_MENU = 12  ---> Menu de Configurações                                  //////
// COD_MENU = 13  ---> Menu de Entradas                                       //////
////////////////////////////////////////////////////////////////////////////////////


//#include <Arduino.h>

#define TINY_GSM_MODEM_SIM808     // Tipo de modem que estamos usando (emboora usemos o modem sim800l os parametros do sim808 apresentaram maior estabilidade).
#include <TinyGsmClient.h>        // Biblioteca de comunicação com o Modem
#include <PubSubClient.h>
#include <SPI.h>                  // Biblioteca de comunicação SPI
#include <LoRa.h>                 // Biblioteca de integração com o modulo LORA - NiceRF v2.0 - 915 Mhz ou RFM95W
#include <Keypad.h>               // Biblioteca para controle do teclado matricial 4x4  
#include <Utils.h>

// Declaração de constantes e variaveis

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

const String USER               =       "supply-admin";                        // "USUARIO"
const String PASS               =       "supply@19!";                          // "SENHA"
const String TOPIC              =       "Supply/";                             // TOPICO MQTT para publicação
const String MQTT_SERVER        =       "mqtt.datamills.com.br";               //"mqtt://things.ubidots.com"
const int MQTT_PORT             =       1883;                                  // Porta para comunicação com Broker MQTT

String SENHA_AGS                =       "380130";                              // Senha ROOT, apenas pessoal autorizado da AGS possue

uint8_t statusCheck             =       0;
bool stateCheck[8]              =       {0,0,0,0,0,0,0,0};                     // 0=SD, 1=RTC, 2=RFID           

// Criação de instancias

Som alerta(SOM);                                                               // Instancia para classe de sinais audio visuais
UID uniqueID;
DrawScreen visor;
RFIDReader leitorRfid;
DatalLogger datalogger;

void setup() {

  // Inicializa estado e modo dos pinos
  pinMode(PIN_SS_DATA_LOG, OUTPUT);
  pinMode(SOM, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(RELE_01, OUTPUT);
  pinMode(RELE_02, OUTPUT);
  pinMode(BOTAO, INPUT);
  digitalWrite(BOTAO, LOW);
  
  // Inicia Serial
  Serial.begin(9600);

  // Incia a comunicação SPI
  SPI.begin();

  // Inicia Monitor LCD
  visor.begin();

  // monta tela com Logo Marca 
  visor.drawSetup(SCREEN_DRAW_LOGO, 2000, 0, 0);

  // monta tela com apresentação e UIID
  visor.drawSetup(SCREEN_INIT, 3000, 0, 0);
  
  // inicializa o Datalogger
  if (datalogger.begin(PIN_SS_DATA_LOG) == DATALOGGER_SD_OK){
    Serial.println(F("SD PASSOU"));
    statusCheck = 0;
    stateCheck[0] = 0;

  }else {
    Serial.println(F("SD NAO PASSOU"));
    statusCheck = 1;
    stateCheck[0] = 1;
  }
  visor.drawSetup(SCREEN_VERIFY_DATA_LOGGER_SD, 2000, statusCheck, stateCheck);

  // Se a codição for verdadeira tenta pegar a hora do sistema
  if (GET_SYSTEM_TIMESTAMP){
    if (datalogger.setSystemTimestamp() == DATALOGGER_TIME_OK){
    statusCheck = 0;
    stateCheck[1] = 0;
    }
  }else{
    if (datalogger.getDateHour() == DATALOGGER_TIME_OK){
      statusCheck = 0;
      stateCheck[1] = 0;
    }else{
      statusCheck = 1;
      stateCheck[1] = 0;
    }
    
  }
  visor.drawSetup(SCREEN_VERIFY_DATA_LOGGER_RTC, 2000, statusCheck, stateCheck);

  // Inicializa o RFID  
  if(leitorRfid.begin() == RFID_OK){
    statusCheck = 0;
    stateCheck[2] = 0;
  }else{
    statusCheck = 1;
    stateCheck[2] = 1;
  }
  visor.drawSetup(SCREEN_VERIFY_RFID, 2000, statusCheck, stateCheck);


}

void loop() {

  Serial.println(F("INCIO DO LOOP"));
  alerta.somCerto(LED_VERDE, 50);
  delay(2000);
  alerta.somErrado(LED_VERMELHO, 250, 50);
  delay(2000);
  Serial.println(uniqueID.getUID());

}
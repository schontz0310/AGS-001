// Controle de bomba de abastecimento de Diesel com Bloco Medidor Analogico   //////
// Hardware - Arduino Mega                                                    //////
// Criado por Eliverto Schontz Moraes                                         //////
// 06/01/2020 - V2.0                                                          //////
// 06/08/2020 - V2.1 incluido telas de espera                                 //////
// 02/11/2020 - V3.1 Refatoração completa do cogido                           //////
//  [x] -> Correção da conectividade Gprs/Mqtt com problema de timeout        //////
//  [ ] -> Refatoração dos codigos de tela                                    //////
//  [ ] -> Apresentar codigo de erro com opção de continuar                   //////
//  [ ] -> Incluir liberação de funções com apresentação de cartão MASTER     //////
//  [ ] -> Ocultar caracteres de senha ao digitar                             //////
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


#include <Arduino.h>

#define TINY_GSM_MODEM_SIM808     // Tipo de modem que estamos usando (emboora usemos o modem sim800l os parametros do sim808 apresentaram maior estabilidade).
#include <TinyGsmClient.h>        // Biblioteca de comunicação com o Modem
#include <PubSubClient.h>
#include <SD.h>                   // SD Card Biblioteca
#include <SPI.h>                  // Biblioteca de comunicação SPI
#include <Wire.h>                 // Biblioteca de comunicação I2C
#include <TimeLib.h>              // Biclioteca de configuração de data e hota 
#include <DS1307RTC.h>            // Biblioteca de integração com o modulo RTC
#include <MFRC522.h>              // Biblioteca de integração com o modulo RFID MFRC522
#include <LoRa.h>                 // Biblioteca de integração com o modulo LORA - NiceRF v2.0 - 915 Mhz ou RFM95W
#include <Keypad.h>               // Biblioteca para controle do teclado matricial 4x4  
#include <Utils.h>

// Declaração de constantes e variaveis

const uint8_t SOM               =       27;                                   // Pino de ligação do buzzer
const uint8_t LED_VERMELHO      =       29;                                   // Pino de ligação do led vermelho
const uint8_t LED_VERDE         =       31;                                   // Pino de ligação do led verde

const uint8_t RST_PIN           =       10;                                   // Configuravel - Pino Reset modulo RFID
const uint8_t SS_SDA_PIN        =       4;                                    // Configuravel - Pino Slave Select (SDA) modulo RFID

const uint8_t PIN_SS_DATA_LOG   =       46;                                   // Configuravel - Pino Slave Select/Chip Select modulo DATALOGGER

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




// Criação de instancias

Som alerta(SOM);                                                               // Instancia para classe de sinais audio visuais
UID uniqueID;
DrawScreen visor;

void setup() {

  pinMode(SOM, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  Serial.begin(9600);
  visor.begin();
  visor.draw(SCREEN_DRAW_LOGO, 0, 0, 0, 0);
  visor.draw(SCREEN_INIT, 0, 0, 0, 0);
  visor.draw(SCREEN_VERIFY_DATA_LOGGER, 0, 0, 0, 0);
}

void loop() {

  alerta.somCerto(LED_VERDE, 50);
  delay(2000);
  alerta.somErrado(LED_VERMELHO, 250, 50);
  delay(2000);
  Serial.println(uniqueID.getUID());

}
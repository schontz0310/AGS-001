#include <Arduino.h>
#include <ArduinoUniqueID.h>
#include <MFRC522.h>   
#include "U8glib.h"
#include <Wire.h>                 // Biblioteca de comunicação I2C
#include <TimeLib.h>              // Biclioteca de configuração de data e hota 
#include <DS1307RTC.h>            // Biblioteca de integração com o modulo RTC
#include <SD.h>                   // SD Card Biblioteca
#define TINY_GSM_MODEM_SIM800     // Tipo de modem que estamos usando (emboora usemos o modem sim800l os parametros do sim808 apresentaram maior estabilidade).
#include <TinyGsmClient.h>        // Biblioteca de comunicação com o Modem
#include <PubSubClient.h>
#include <Keypad.h>               // Biblioteca para controle do teclado matricial 4x4  
#include <SPI.h> 

#ifndef _Utils_H
#define _Utils_H

const uint8_t RST_PIN           =       10;                                   // Configuravel - Pino Reset modulo RFID
const uint8_t SS_SDA_PIN        =       4;                                    // Configuravel - Pino Slave Select (SDA) modulo RFID

#define success_width 74
#define success_height 60
const uint8_t success[] U8G_PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff,
   0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x03, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x07, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x7f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf8, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xf8, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe,
   0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x03,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x01, 0x00, 0x00,
   0x00, 0x80, 0x00, 0x00, 0x80, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xc0,
   0x01, 0x00, 0xe0, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
   0xe0, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x07, 0x00, 0xf0, 0xff,
   0x1f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0xf8, 0xff, 0x0f, 0x00,
   0x00, 0x00, 0x00, 0xfc, 0x1f, 0x00, 0xfe, 0xff, 0x07, 0x00, 0x00, 0x00,
   0x00, 0xfe, 0x3f, 0x00, 0xfe, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0xff,
   0x7f, 0x00, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0x80,
   0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xc1, 0xff, 0x7f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xe3, 0xff, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xfc, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff,
   0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x01,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xfc, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff,
   0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x0f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x03, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


#define error_width 44
#define error_height 64
const uint8_t error[] U8G_PROGMEM = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xe0, 0x01, 0x00, 0x00, 0x78, 0x00, 0xf0, 0x03, 0x00, 0x00, 0xfc, 0x00,
   0xf8, 0x07, 0x00, 0x00, 0xfe, 0x01, 0xfc, 0x0f, 0x00, 0x00, 0xff, 0x03,
   0xfe, 0x1f, 0x00, 0x80, 0xff, 0x07, 0xff, 0x3f, 0x00, 0xc0, 0xff, 0x0f,
   0xff, 0x7f, 0x00, 0xe0, 0xff, 0x0f, 0xff, 0xff, 0x00, 0xf0, 0xff, 0x0f,
   0xff, 0xff, 0x01, 0xf8, 0xff, 0x0f, 0xfe, 0xff, 0x03, 0xfc, 0xff, 0x07,
   0xfc, 0xff, 0x07, 0xfe, 0xff, 0x03, 0xf8, 0xff, 0x0f, 0xff, 0xff, 0x01,
   0xf0, 0xff, 0x9f, 0xff, 0xff, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x7f, 0x00,
   0xc0, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x80, 0xff, 0xff, 0xff, 0x1f, 0x00,
   0x00, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x07, 0x00,
   0x00, 0xfc, 0xff, 0xff, 0x03, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x01, 0x00,
   0x00, 0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x7f, 0x00, 0x00,
   0x00, 0xe0, 0xff, 0x7f, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x00, 0x00,
   0x00, 0xf8, 0xff, 0xff, 0x01, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x03, 0x00,
   0x00, 0xfe, 0xff, 0xff, 0x07, 0x00, 0x00, 0xff, 0xff, 0xff, 0x0f, 0x00,
   0x80, 0xff, 0xff, 0xff, 0x1f, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x3f, 0x00,
   0xe0, 0xff, 0xff, 0xff, 0x7f, 0x00, 0xf0, 0xff, 0x9f, 0xff, 0xff, 0x00,
   0xf8, 0xff, 0x0f, 0xff, 0xff, 0x01, 0xfc, 0xff, 0x07, 0xfe, 0xff, 0x03,
   0xfe, 0xff, 0x03, 0xfc, 0xff, 0x07, 0xff, 0xff, 0x01, 0xf8, 0xff, 0x0f,
   0xff, 0xff, 0x00, 0xf0, 0xff, 0x0f, 0xff, 0x7f, 0x00, 0xe0, 0xff, 0x0f,
   0xff, 0x3f, 0x00, 0xc0, 0xff, 0x0f, 0xfe, 0x1f, 0x00, 0x80, 0xff, 0x07,
   0xfc, 0x0f, 0x00, 0x00, 0xff, 0x03, 0xf8, 0x07, 0x00, 0x00, 0xfe, 0x01,
   0xf0, 0x03, 0x00, 0x00, 0xfc, 0x00, 0xe0, 0x01, 0x00, 0x00, 0x78, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const uint8_t img_bitmap[] U8G_PROGMEM = 
{

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x1f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0,
  0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xf0, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xf8, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf8, 0xff,
  0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff,
  0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0x3f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0xf8, 0xff,
  0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff,
  0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0x0f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff,
  0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff,
  0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xe0, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x03, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x03,
  0x00, 0x80, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0x3f, 0x00, 0x00, 0x00,
  0x00, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff,
  0x7f, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xfc, 0xff, 0x00, 0x00,
  0x00, 0x00, 0xe0, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x7f,
  0xf8, 0xff, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x3f, 0x00, 0x00, 0x00,
  0x80, 0xff, 0xff, 0x3f, 0xf0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe,
  0x1f, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xff, 0x1f, 0xe0, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0x00, 0xe0, 0xff, 0xff, 0x0f,
  0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00,
  0xf0, 0xff, 0xff, 0x07, 0xc0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
  0x03, 0x00, 0x00, 0x00, 0xf8, 0xff, 0xff, 0x03, 0xfc, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x01,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfc, 0xff, 0xff, 0xc0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x7f, 0xe0, 0x07, 0xfe, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff, 0x3f, 0xe0,
  0x01, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xe1, 0xff, 0x1f, 0xf0, 0x00, 0xf8, 0x00, 0xff, 0x0f, 0xe0, 0x3f, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xc3, 0xff, 0x0f, 0x70, 0x00, 0xfc, 0x00, 0xff,
  0x1f, 0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0xff, 0x07, 0x78,
  0x00, 0x7e, 0x00, 0xff, 0x3f, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0f, 0xff, 0x03, 0x38, 0x00, 0x3f, 0x00, 0xff, 0x3f, 0xfe, 0xff, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x1f, 0xfe, 0x01, 0x3c, 0x80, 0x1f, 0x00, 0xff,
  0x3f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xfc, 0x00, 0x3c,
  0xc0, 0x0f, 0x00, 0xff, 0x1f, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x7f, 0xf8, 0x00, 0x1e, 0xe0, 0x07, 0x00, 0x3f, 0x80, 0x3f, 0x30, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x01, 0x1e, 0xf0, 0x03, 0x00, 0x3f,
  0x80, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xe0, 0x03, 0x0e,
  0xf8, 0x01, 0x00, 0x3f, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1f, 0xc0, 0x07, 0x0e, 0xfc, 0x00, 0x00, 0x3f, 0xc0, 0x0f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x0f, 0x06, 0x7e, 0x00, 0x00, 0x3f,
  0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x1f, 0x00,
  0x3f, 0x00, 0x00, 0x3f, 0xc0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x3e, 0x80, 0x1f, 0x00, 0x00, 0x1f, 0xc0, 0x0f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x7c, 0xc0, 0x0f, 0x00, 0x00, 0x00,
  0xc0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xe0,
  0x07, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xf0, 0xff, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x63, 0x3c, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x01, 0x00, 0x00, 0x00,
  0xfe, 0xf3, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x7f,
  0x00, 0x00, 0x00, 0x00, 0xfe, 0xf9, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xf9, 0xff, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xfe, 0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x3e, 0xf0, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x80, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00

};

enum Fuel{
  S500,
  S10,
};

enum statusSound 
{
  sound_ok,
  sound_error
};

class Som 
{
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

class UID 
{
  public:
    UID ();
    String getUID();
  private:
    String _uid;

};

enum ScreenName
{
  // GENERAL
  SCREEN_PROGRESS,
  SCREEN_ERROR,
  SCREEN_SUCCESS,
  // SETUP
  SCREEN_DRAW_LOGO,
  SCREEN_INIT,
  SCREEN_VERIFY_DATA_LOGGER_SD,
  SCREEN_VERIFY_DATA_LOGGER_RTC,
  SCREEN_VERIFY_RFID,
  SCREEN_VERIFY_MODEM,
  SCREEN_VERIFY_MQTT,
  // OPERATOR
  SCREEN_OPERATOR_READ,
  SCREEN_OPERATOR_SEARCH,
  SCREEN_OPERATOR_FOUND,
  SCREEN_OPERATOR_NOT_FOUND,
  // MENUS
  SCREEN_MENU_PRINCIPAL,
  SCREEN_MENU_CADASTRO,  
  SCREEN_MENU_CADASTRO_OPERADOR_CHOICE,
  SCREEN_MENU_CADASTRO_OPERADOR_READ_CARD,
  SCREEN_MENU_CADASTRO_OPERADOR_READ_NAME,
  SCREEN_MENU_CADASTRO_OPERADOR_READ_LEVEL,
  SCREEN_MENU_CADASTRO_VEHICLE_CHOICE,
  SCREEN_MENU_CADASTRO_VEHICLE_READ_CARD,
  SCREEN_MENU_CADASTRO_VEHICLE_READ_NAME,
  SCREEN_MENU_CADASTRO_VEHICLE_READ_LEVEL,
  SCREEN_MENU_CADASTRO_PERMISSION_CHOICE,
  SCREEN_PUMP_CHARGE_FUEL,

  // ACCESSES SCREEN
  SCREEN_ACCCESSES,
  SCREEN_ACCCESSES_CHOICE,
  SCREEN_ACCCESSES_CARD,
  SCREEN_ACCCESSES_PASSWORD,
};



class DrawScreen 
{
  public:
    DrawScreen ();
    void begin();
    void drawSetup(ScreenName screen, int interval, uint8_t status, bool estado[]);
    void readOperator(ScreenName screen, String name, String cardID);
    void drawMenu(ScreenName screen);
    void drawScreen(ScreenName scren, String value);
    uint8_t _status;
    bool _state[8];
    int _interval;

  private:
    ScreenName _screen;
    char _buffer[24];
    String _name;
    String _cardID;
};

enum RFIDStatus
{
  RFID_OK,
  RFID_ERROR,
};

class RFIDReader 
{
  public:
    RFIDReader ();
    RFIDStatus begin();
    bool getID();
    String IDValue;

  private:
    bool _result;
    String _value;
};

enum DataLoggerStatus
{
  DATALOGGER_SD_OK,
  DATALOGGER_SD_ERROR,
  DATALOGGER_TIME_OK,
  DATALOGGER_TIME_ERROR,
  DATALOGGER_READY,
};

class DataLogger
{
  public:
    DataLogger();
    DataLoggerStatus begin(uint8_t pin_ss_datalogger);
    DataLoggerStatus setSystemTimestamp();
    DataLoggerStatus getDateHour();
    bool checkOperatorExist(String uuid);
    bool checkOperatorIsAdmin(String uuid);
    bool checkPermissionExist(String uuid);
    String getOperator(String uuid);
    String getVehicle(String uuid);
    bool checkVehicleExist(String uuid);
    void WriteOperatorInDatalogger();
    void WriteVehicleInDatalogger();
    void WritePermissionInDatalogger();
    void WriteFuelChargeInDatalogger();
    void WriteFailMqttLog(String payload);
    String getTimestamp();
    String _operatorUuid;
    String _vehicleUuid;
    String _operatorLevel;
    String _vehicleFuel;
    String _operatorName;
    String _vehicleName;

  private:
    bool getDate(const char *str);
    bool getTime(const char *str);
    String get2digits(int number);
    uint8_t _pin_ss_datalogger;
    String _uuidToCheck;
    String _uuidRead;
    char _readCharacther;
};

enum ModemGPRSStatus{
  MODEM_READY,
  MODEM_ERROR_RESTART,
  MODEM_OK_RESTART,
  MODEM_ERROR_NETWORK,
  MODEM_ERROR_GPRS,
};

class ModemGPRS
{
  private:
    
  public:
    ModemGPRS();
    ModemGPRSStatus setup();
    ModemGPRSStatus reconnect();
};

enum MQTTStatus{
  MQTT_READY,
  MQTT_FAILED,
  MQTT_SEND,
};

const String TOPIC_REGISTER = "registers/";

class MQTTConnection
{
  private:
    char _buffer[24];
    char _user[24];
    char _password[24];
    String _payload;
    String _topic;
  public:
    MQTTConnection();
    MQTTStatus setup(const char * domain, uint16_t port, const char * user, const char * password);
    MQTTStatus reconnect(const char * user, const char * password);
    boolean send(String topic, String payload);
};


class Operator
{
  private:
    byte status = 0;
    bool successRead;
    String _operator;
  public:
    Operator();
    String Read();

};

class Vehicle
{
  private:
    byte status = 0;
    bool successRead;
    String _vehicle;
  public:
    Vehicle();
    String Read();

};

class Permission
{
  private:
  public:
    Permission();
    bool check(String uuid);
};

void CANAL_A();
void CANAL_B();
void CANAL_C();
void CANAL_D();

class Pump
{
  private:
  uint8_t _buttonStatus;
  uint8_t _pump;
  public:
    Pump();
    float fuelLoad(Fuel fuel);
    boolean registerFueLCharger();
};



enum MetodeAccesses{
  CARD,
  PASSWORD,
};

enum VehicleFuel{
  DIESEL_S10,
  DIESEL_S500,
};

class Menu
{
private:
  ScreenName _nextScreen;
  MetodeAccesses _metode;
  bool _successRead;
  bool _flag;
public:
  String _UUIDCard;
  String _UUIDPermission;
  String _operatorName;
  uint8_t _operatorlevel; //1= administrador 2= Frentista 3= Motorista
  String _vehicleName;
  VehicleFuel _vehicleFuel;
  char _buffer[24];
  Menu();
  void menuPrincipal();
  void menuCadastro();
  void menuCadastroOperador();
  void menuCadastroVeiculo();
  void permissionRegistrationMenu();
  void menuAccesses(ScreenName nextScreen);
  void menuAccesses(MetodeAccesses metode, ScreenName nextScreen);
};


class Keyboard 
{
  private:
    char _pressedKey;
  public:
    Keyboard();
    char keyboardGetKeyNumeric();
    String keyboardGetKeyAlfanumeric(ScreenName screen);
    //variables
    String NameValue;
    unsigned long _elapsedTime;
    unsigned long _lastTime = 0;
    ScreenName _screen;
    char _buffer[16];
    char _lastPressedKey;
    int _offset;
    int _timesPressed;
    int _counter = 0;
    int _cursorPossition = 0;
};


class Access
{
  private:
  MetodeAccesses _metode;
  int _position;
  char _keyPressed;
  String _secret;
  
  public:
  char _buffer[16];
  Access();
  bool accessValidate(MetodeAccesses metode);
};

class Json{
  
  private:
  
  public:

    String _payload;
    String _temp;

    Json();
    String jsonOperatorMount();
    String jsonVehicleMount();
    String jsonPermissionMount();
    String jsonFuelChargeMount();
};

#endif 

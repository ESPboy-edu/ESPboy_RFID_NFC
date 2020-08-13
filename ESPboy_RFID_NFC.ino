/*
ESPboy RFID-NFC module
for www.ESPboy.com project by RomanS
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
*/



#include <Adafruit_MCP23017.h>
#include <TFT_eSPI.h>
#include "U8g2_for_TFT_eSPI.h"
#include <Adafruit_PN532.h>
#include <ESP8266WiFi.h>
#include "lib/ESPboyLogo.h"
#include "ESPboyGUI.h"
#include "ESPboyOTA.h"

#define PN532_IRQ   (15)
#define PN532_RESET (-1)  

#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff

#define MCP23017address 0  
#define LEDPIN D4
#define SOUNDPIN D3
#define CSTFTPIN 8  // CS MCP23017 PIN to TFT

Adafruit_MCP23017 mcp;
TFT_eSPI tft = TFT_eSPI();
U8g2_for_TFT_eSPI u8f;
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
ESPboyGUI* GUIobj = NULL;
ESPboyOTA* OTAobj = NULL;

uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
uint8_t uidLength; 
uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

enum commands{
  MiREAD=1,
  MiWRITEB,
  MiSTORE,
  MiEMULAT,
  MiFORMAT,
  MiMIMIC,
  MiWRITEC,
  NTGread,
  NTGwrite,
  NTGstore,
  NTGerase,
  NTGupdate
}cmd;

uint8_t commandMatrice[][2] = {
  {'r', MiREAD},
  {'w', MiWRITEB},
  {'c', MiWRITEC},
  {'s', MiSTORE},
  {'e', MiEMULAT},
  {'f', MiFORMAT},
  {'m', MiMIMIC},
  {'c', MiWRITEC},
  {'a', NTGread},
  {'t', NTGwrite},
  {'o', NTGstore},
  {'z', NTGerase},
  {'u', NTGupdate},  
  {0,0}
};


uint8_t getCommand(char cmd){
 if (cmd>=65 && cmd<=90) cmd+=32;
 for(uint8_t i=0;; i++){
   if (cmd == commandMatrice[i][0]) return(commandMatrice[i][1]);
   if (commandMatrice[i][0] == 0) return (0);
 }  
}


String capitaliseString(String str){
  for(uint8_t i = 0; i< str.length(); i++)
    if(str[i] >= 'a' && str[i] <= 'f') str.setCharAt(i,(char)((uint8_t)str[i]-32));
  return (str);
}


void setup() {
  
  WiFi.mode(WIFI_OFF);
  Serial.begin(115200);
  delay(100);

  // MCP23017 and buttons init, should preceed the TFT init
  mcp.begin(MCP23017address);
  delay(100);
  mcp.pinMode(CSTFTPIN, OUTPUT);
  for (int i = 0; i < 8; ++i) {
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }

  // Sound init and test
  pinMode(SOUNDPIN, OUTPUT);
  tone(SOUNDPIN, 200, 100);
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);

  // TFT init
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  //u8f init
  u8f.begin(tft);
  u8f.setFontMode(1);                 // use u8g2 none transparent mode
  u8f.setBackgroundColor(TFT_BLACK);
  u8f.setFontDirection(0);            // left to right
  u8f.setFont(u8g2_font_4x6_t_cyrillic); 

  // draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(F("RFID/NFC"), 42, 95);

  delay(1000);

//init rfid/nfc
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString(F("RFID module not found"), 2, 120);
    while(1) delay(1000);
  }

  GUIobj = new ESPboyGUI(&tft, &mcp, &u8f);
  if (GUIobj->getKeys()) OTAobj = new ESPboyOTA(&tft, &mcp, GUIobj);

  String ver = "RFID firmware ";
  ver += (String)((versiondata>>16) & 0xFF); 
  ver += "."; 
  ver += (String)((versiondata>>8) & 0xFF);
  GUIobj->printConsole(ver, TFT_MAGENTA, 1, 0);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();

  // clear screen
  tft.fillScreen(TFT_BLACK);
}


void readDataCard4(){
  uint8_t data[32];
  uint32_t cardid = uid[0];
  uint8_t currentblock;   
  bool authenticated = false;    
  
  cardid <<= 8;
  cardid |= uid[1];
  cardid <<= 8;
  cardid |= uid[2];  
  cardid <<= 8;
  cardid |= uid[3];

  String toPrint = "ID " + (String)cardid;
  GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);

  delay(1000);

 for (currentblock = 0; currentblock < 64; currentblock++){ 
   if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keya)){
     GUIobj->printConsole(F("Authentication fail"), TFT_RED, 1, 0);
     break;
   }
   else{ 
     toPrint = "S";
     if(currentblock/4 < 10) toPrint += "0";
     toPrint += (String)(currentblock/4);
     toPrint += "/B";
     if(currentblock < 10) toPrint += "0";
     toPrint += (String)currentblock;
     toPrint += "  ";
     GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
     if (!nfc.mifareclassic_ReadDataBlock(currentblock, data)){
       GUIobj->printConsole(F("Reading error"), TFT_RED, 1, 0);
       break;
     }
     else{
       toPrint = "";
       for (uint8_t i=0; i<8; i++) {
         if (data[i] < 16) toPrint += "0";
         toPrint += String(data[i], HEX);
         toPrint += " ";
       }
      
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       toPrint="";
       for (uint8_t i=8; i<16; i++) {
         if (data[i] < 16) toPrint += "0";
         toPrint += String(data[i], HEX);
         toPrint += " ";
       }
      
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       toPrint="";
       char conv[2]={0,0};
       for (uint8_t i=0; i<16; i++) {
         if (data[i]>0x19 && data[i]<0x7F) conv[0] = data[i];
         else conv[0]='.';
         toPrint += conv;
       }
      
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
     }          
   }
 }
}  


void readDataCard7(){
 /* for (uint8_t i = 0; i < 42; i++) {
    Serial.print("PAGE ");
    Serial.println(i);
    if (nfc.ntag2xx_ReadPage(i, data)) nfc.PrintHexChar(data, 4);
    else {
      Serial.println("Unable to read page!");
      break;
    }
  }*/
  GUIobj->printConsole(F("Card7 data read not suppoted"), TFT_RED, 1, 0);      
}



void writeBlock(){
  uint8_t blockToWrite = 0;
  uint8_t dta[16];
  uint8_t dataNo;
  
  GUIobj->printConsole(F("Write"), TFT_YELLOW, 1, 0);
  
  GUIobj->printConsole(F("What block No? DEC"), TFT_MAGENTA, 1, 0);
  while (!blockToWrite) blockToWrite = GUIobj->getUserInput().toInt();
  GUIobj->printConsole((String)blockToWrite, TFT_YELLOW, 1, 0);
  
  GUIobj->printConsole("Enter 16 bytes of data, DEC", TFT_MAGENTA, 1, 0);
  for (uint8_t i=0; i<16; i++){      
    GUIobj->printConsole("Byte " + (String)(i+1) + " of 16?", TFT_MAGENTA, 1, 0);
    dataNo=0;
    while (!dataNo) dataNo = GUIobj->getUserInput().toInt();
    dta[i] = dataNo;
    GUIobj->printConsole((String)(dta[i]), TFT_YELLOW, 1, 0);
  }
  String toPrint = F("Writing block: 0x");
  if(blockToWrite<16) toPrint+="0";
  GUIobj->printConsole(toPrint + (String (blockToWrite, HEX)), TFT_WHITE, 1, 0);
  GUIobj->printConsole("Data 0x:", TFT_WHITE, 1, 0);
  toPrint="";
  for (uint8_t i=0; i<8; i++) {
    if (dta[i] < 16) toPrint += "0";
    toPrint += String(dta[i], HEX);
    toPrint += " ";
  }
  GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
  toPrint="";
  for (uint8_t i=8; i<16; i++) {
    if (dta[i] < 16) toPrint += "0";
    toPrint += String(dta[i], HEX);
    toPrint += " ";
  }
  GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
  
  GUIobj->printConsole(F("Put RFID ISO14443A to write..."), TFT_MAGENTA, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    GUIobj->printConsole(F("Card error"), TFT_RED, 1, 0);
  else  
    if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, blockToWrite, 1, keya))
      GUIobj->printConsole(F("Autentification FAILED"), TFT_RED, 1, 0);
    else{
      GUIobj->printConsole(F("Card detected. Writing..."), TFT_MAGENTA, 1, 0); 
      if(nfc.mifareclassic_WriteDataBlock (blockToWrite, dta)) 
        GUIobj->printConsole(F("OK"), TFT_GREEN, 1, 0);
      else 
        GUIobj->printConsole(F("FAULT"), TFT_RED, 1, 0);
    }
}
    


void formatCard(){
  GUIobj->printConsole(F("Format"), TFT_YELLOW, 1, 0);
  GUIobj->printConsole(F("Waiting for RFID ISO14443A..."), TFT_MAGENTA, 1, 0);
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
    GUIobj->printConsole(F("ISO14443A RFID init error"), TFT_RED, 1, 0);
  else  
    if (uidLength != 4)
      GUIobj->printConsole(F("Not ISO14443A RFID"), TFT_RED, 1, 0);
    else
      if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 0, 0, keya))
        GUIobj->printConsole(F("Unable to authenticate block 0"), TFT_RED, 1, 0);
      else
        if(!nfc.mifareclassic_FormatNDEF())
          GUIobj->printConsole(F("Formatting error"), TFT_RED, 1, 0);
        else
          GUIobj->printConsole(F("Formatted for NDEF using MAD1"), TFT_GREEN, 1, 0);
}


void readCard(){    
  GUIobj->printConsole(F("Read"), TFT_YELLOW, 1, 0);
  GUIobj->printConsole(F("Waiting for RFID ISO14443A..."), TFT_MAGENTA, 1, 0);
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    String toPrint = "Len " + (String)uidLength;
    GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
    toPrint = "Val ";
    for (uint8_t i=0; i<uidLength; i++){
      if (uid[i] < 16) toPrint += "0";
      toPrint += String(uid[i], HEX);
      toPrint += " ";
    }  
    GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0); 
    if (uidLength == 4) readDataCard4(); 
    if (uidLength == 7) readDataCard7();
  }
  else 
    GUIobj->printConsole(F("Reading error"), TFT_RED, 1, 0);
}


void loop() {
  GUIobj->printConsole("", TFT_GREEN, 1, 0);      
  GUIobj->printConsole(F("[R]ead [W]rite [S]tore [F]ormat"), TFT_GREEN, 1, 0);
  GUIobj->printConsole(F("Command?"), TFT_MAGENTA, 1, 0);

  String rdCommand = "";
  while (rdCommand == "")
    rdCommand = GUIobj->getUserInput();
    
  switch (getCommand(rdCommand[0])){
    case MiREAD: 
      readCard();
      break;
    case MiWRITEB:
      writeBlock();
      break;
    case MiFORMAT:
      formatCard();
      break;
    default:
      GUIobj->printConsole(F("Wrong command"), TFT_RED, 1, 0);
      break;
  }
}

/*
ESPboy RFID-NFC module
for www.ESPboy.com project by RomanS
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
*/



#include <Adafruit_MCP23017.h>
#include <TFT_eSPI.h>
#include <Adafruit_PN532.h>
#include <ESP8266WiFi.h>
#include "lib/ESPboyLogo.h"
#include "ESPboyGUI.h"

#define PN532_IRQ   (15)
#define PN532_RESET (-1)  


#define NR_SHORTSECTOR          (32)    // Number of short sectors on Mifare 1K/4K
#define NR_LONGSECTOR           (8)     // Number of long sectors on Mifare 4K
#define NR_BLOCK_OF_SHORTSECTOR (4)     // Number of blocks in a short sector
#define NR_BLOCK_OF_LONGSECTOR  (16)    // Number of blocks in a long sector

// Determine the sector trailer block based on sector number
#define BLOCK_NUMBER_OF_SECTOR_TRAILER(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR + NR_BLOCK_OF_SHORTSECTOR-1):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR + NR_BLOCK_OF_LONGSECTOR-1))

// Determine the sector's first block based on the sector number
#define BLOCK_NUMBER_OF_SECTOR_1ST_BLOCK(sector) (((sector)<NR_SHORTSECTOR)? \
  ((sector)*NR_BLOCK_OF_SHORTSECTOR):\
  (NR_SHORTSECTOR*NR_BLOCK_OF_SHORTSECTOR + (sector-NR_SHORTSECTOR)*NR_BLOCK_OF_LONGSECTOR))
  
String url = "espboy.com";
uint8_t ndefprefix = NDEF_URIPREFIX_HTTP_WWWDOT;

// The default Mifare Classic key
static const uint8_t KEY_DEFAULT_KEYAB[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


const char* PROGMEM printInit = "RFID/NFC";
const char* PROGMEM printMifare = "Mifare Classic";
const char* PROGMEM printReadedStored = "Readed and stored";
const char* PROGMEM printFormatedNdef = "Formatted for NDEF using MAD1";

const char* PROGMEM printWritingBlock = "Writing block";
const char* PROGMEM printWriteReaded = "Write readed RFID";
const char* PROGMEM printUltralight = "Mifare Ultralight or Plus";
const char* PROGMEM printWhatBlockNo = "What block No? DEC";
const char* PROGMEM printStartBlockNo = "Start block No? DEC";
const char* PROGMEM printEndBlockNo = "End block No? DEC";

const char* PROGMEM printWaitingToWrite = "Waiting for RFID to write";
const char* PROGMEM printWaitingToRead = "Waiting for RFID to read";
const char* PROGMEM printWaitingToFormat = "Waiting for RFID to format";
const char* PROGMEM printMifareToFormat = "Mifare Classic to format...";
const char* PROGMEM printMifareClassic = "Mifare Classic to write";
const char* PROGMEM printDetectedWriting = "Card detected. Writing...";
const char* PROGMEM printShowStored = "Show stored card";
const char* PROGMEM printFormatingProgress = "Formating in progress...";
const char* PROGMEM printFormatingSucsesseful = "Formating sucsesseful";
const char* PROGMEM printNtagToErase = "Waiting NTAG to erase";
const char* PROGMEM printWaitingNtagToWrite = "Waiting NTAG to write";

const char* PROGMEM printOk = "OK";
const char* PROGMEM printFail = "FAIL";
const char* PROGMEM printFormat = "Format";
const char* PROGMEM printRead = "Read";
const char* PROGMEM printWrite = "Write";
const char* PROGMEM printBreak = "Break";

const char* PROGMEM printNotFound = "RFID module not found";
const char* PROGMEM printRfidInitError = "RFID init error";
const char* PROGMEM printCardError = "RFID error";
const char* PROGMEM printAuthFail = "Authentication fail";
const char* PROGMEM printReadingError = "Reading error";
const char* PROGMEM printNotIsoRfid = "Not ISO14443A RFID";
const char* PROGMEM printNotMifareClassic = "Not Mifare Classic";
const char* PROGMEM printFormatingError = "Formatting error";
const char* PROGMEM printWrongParameter = "Wrong parameter";
const char* PROGMEM printWrongCom ="Wrong command";
const char* PROGMEM printEnterUrl = "Enter URL to write";
const char* PROGMEM printErasingOld = "Erasing old...";
const char* PROGMEM printNtagStructureError = "NTAG structure error";
const char* PROGMEM printWrittingNtag = "Writting NTAG...";
const char* PROGMEM printErasingNtag = "Erasing NTAG...";
const char* PROGMEM printEraseError = "Erase error";
const char* PROGMEM printMifareToNtag = "Mifare Clas format to NTAG";
const char* PROGMEM printFormatingToNtag = "Formating to NTAG...";

const char* PROGMEM printWriteSectorError = "Write sector error";
const char* PROGMEM printWriteTrailerError = "Write trailer block error";
const char* PROGMEM printNtagToRead = "Waiting NTAG to read";
const char* PROGMEM printNotNtag = "Not NTAG203/NTAG213";
const char* PROGMEM printWrittingOk = "Writting OK";
const char* PROGMEM printWrittingError = "Writting Error";
const char* PROGMEM printWritting = "Writting...";
const char* PROGMEM printNtagDetected = "NTAG detected, bytes ";

const char* PROGMEM printMenuLine1 = "Mifare: R-read W-write";
const char* PROGMEM printMenuLine2 = "B-writeblock O-show F-format";
const char* PROGMEM printMenuLine3 = "NTAG: E-read I-write";
const char* PROGMEM printMenuLine4 = "H-show A-erase M-format";
const char* PROGMEM printMenuCommand = "Command?";


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
TFT_eSPI tft;
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
ESPboyGUI* GUIobj = NULL;

uint8_t data[64][16];
uint8_t ntgdata[42][4];

uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
uint8_t uidLength; 
uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

enum commands{
  MiREAD=1,
  MiWRITEB,
  MiWRITEM,
  MiSHOW,
  MiFORMAT,
  NTGread,
  NTGwrite,
  NTGshow,
  NTGerase,
  NTGformat,
}cmd;



uint8_t commandMatrice[][2] = {
  {'r', MiREAD},
  {'b', MiWRITEB},
  {'w', MiWRITEM},
  {'o', MiSHOW},
  {'f', MiFORMAT},
  {'e', NTGread},
  {'i', NTGwrite},
  {'h', NTGshow},
  {'a', NTGerase},
  {'m', NTGformat},
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
  

  // draw ESPboylogo
  tft.drawXBitmap(30, 20, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(printInit, 42, 95);

  delay(1000);

  GUIobj = new ESPboyGUI(&tft, &mcp);

//init rfid/nfc
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString(printNotFound, 2, 118);
    while(1) delay(1000);
  }

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
  uint32_t cardid = uid[0];
  uint8_t currentblock;   
  bool authenticated = false;    

  memset(&data,0,sizeof(data));
  
  cardid <<= 8;
  cardid |= uid[1];
  cardid <<= 8;
  cardid |= uid[2];  
  cardid <<= 8;
  cardid |= uid[3];


  GUIobj->printConsole(printMifare, TFT_GREEN, 1, 0);
  String toPrint = "ID " + (String)cardid;
  GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);

  delay(1000);

 for (currentblock = 0; currentblock < 64; currentblock++){ 
   if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keya)){
     GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
     return;
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
     if (!nfc.mifareclassic_ReadDataBlock(currentblock, &data[currentblock][0])){
       GUIobj->printConsole(printReadingError, TFT_RED, 1, 0);
       return;
     }
     else{
       toPrint = "";
       for (uint8_t i=0; i<8; i++) {
         if (data[currentblock][i] < 16) toPrint += "0";
         toPrint += String(data[currentblock][i], HEX);
         toPrint += " ";
       }
      
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       toPrint="";
       for (uint8_t i=8; i<16; i++) {
         if (data[currentblock][i] < 16) toPrint += "0";
         toPrint += String(data[currentblock][i], HEX);
         toPrint += " ";
       }
      
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       toPrint="";
       char conv[2]={0,0};
       for (uint8_t i=0; i<16; i++) {
         if (data[currentblock][i]>0x19 && data[currentblock][i]<0x7F) conv[0] = data[currentblock][i];
         else conv[0]='.';
         toPrint += conv;
       }
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
     }          
   }
 }
 GUIobj->printConsole(printReadedStored, TFT_GREEN, 1, 0);
}  


void readDataCard7(){
  memset(&data,0,sizeof(data));       
  GUIobj->printConsole(printUltralight, TFT_GREEN, 1, 0);
 
  for (uint8_t currentpage = 0; currentpage < 39; currentpage++){ 
     String toPrint = "P";
     if(currentpage < 10) toPrint += "0";
     toPrint += (String)(currentpage);
     GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
     if (!nfc.mifareultralight_ReadPage (currentpage, &data[currentpage][0])){
       GUIobj->printConsole(printReadingError, TFT_RED, 1, 0);
       return;
     }
     else{
       toPrint = "";
       for (uint8_t i=0; i<4; i++) {
         if (data[currentpage][i] < 16) toPrint += "0";
         toPrint += String(data[currentpage][i], HEX);
         toPrint += " ";
       }
      
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       
       toPrint="";
       char conv[2]={0,0};
       for (uint8_t i=0; i<4; i++) {
         if (data[currentpage][i]>0x19 && data[currentpage][i]<0x7F) conv[0] = data[currentpage][i];
         else conv[0]='.';
         toPrint += conv;
       }
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
     }          
   }
}



void writeBlock(){
  uint8_t blockToWrite = 0;
  uint8_t dta[16];
  uint8_t dataNo;
  
  GUIobj->printConsole(printWritingBlock, TFT_YELLOW, 1, 0);
  
  GUIobj->printConsole(printWhatBlockNo, TFT_MAGENTA, 1, 0);
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
  String toPrint = printWritingBlock;
  toPrint += ": 0x";
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
  
  GUIobj->printConsole(printWaitingToWrite, TFT_MAGENTA, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printCardError, TFT_RED, 1, 0);
    return;}
 
  if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, blockToWrite, 1, keya)){
    GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
    return;}
    
  GUIobj->printConsole(printWrite, TFT_MAGENTA, 1, 0); 
  
  if(nfc.mifareclassic_WriteDataBlock (blockToWrite, dta)) 
     GUIobj->printConsole(printOk, TFT_GREEN, 1, 0);
  else 
     GUIobj->printConsole(printFail, TFT_RED, 1, 0);
}
    


void formatCard(){
  GUIobj->printConsole(printFormat, TFT_YELLOW, 1, 0);
  GUIobj->printConsole(printWaitingToFormat, TFT_MAGENTA, 1, 0);
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printRfidInitError, TFT_RED, 1, 0);
    return;}
  if (uidLength != 4){
    GUIobj->printConsole(printNotIsoRfid, TFT_RED, 1, 0);
    return;}
  if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 0, 0, keya)){
    GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
    return;}
  if(!nfc.mifareclassic_FormatNDEF())
    {GUIobj->printConsole(printFormatingError, TFT_RED, 1, 0);
    return;}
  GUIobj->printConsole(printFormatedNdef, TFT_GREEN, 1, 0);
}


void readCard(){    
  GUIobj->printConsole(printRead, TFT_YELLOW, 1, 0);
  GUIobj->printConsole(printWaitingToRead, TFT_MAGENTA, 1, 0);
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
    GUIobj->printConsole(printReadingError, TFT_RED, 1, 0);
}



void writeMemory(){
 uint16_t writeFrom=0, writeTo=0;
  
 GUIobj->printConsole(printWriteReaded, TFT_YELLOW, 1, 0); 

 GUIobj->printConsole(printStartBlockNo, TFT_MAGENTA, 1, 0);
 while (!writeFrom) writeFrom = GUIobj->getUserInput().toInt();
 GUIobj->printConsole((String)writeFrom, TFT_YELLOW, 1, 0);

 GUIobj->printConsole(printEndBlockNo, TFT_MAGENTA, 1, 0);
 while (!writeTo) writeTo = GUIobj->getUserInput().toInt();
 GUIobj->printConsole((String)writeTo, TFT_YELLOW, 1, 0);

 if (writeTo < writeFrom || writeTo>64){
   GUIobj->printConsole(printWrongParameter, TFT_RED, 1, 0);
   return;
 }
 
 GUIobj->printConsole(printMifareClassic, TFT_MAGENTA, 0, 0);
 if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
   GUIobj->printConsole(printCardError, TFT_RED, 1, 0);
   return;}
 
 GUIobj->printConsole(printDetectedWriting, TFT_MAGENTA, 1, 0); 
  
   for (uint8_t currentblock = writeFrom; currentblock < writeTo; currentblock++){ 
     if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keya)){
       GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
       return;}
     String toPrint = "S";
     if(currentblock/4 < 10) toPrint += "0";
     toPrint += (String)(currentblock/4);
     toPrint += "/B";
     if(currentblock < 10) toPrint += "0";
     toPrint += (String)currentblock;
     toPrint += "  ";
     GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
      
     toPrint="";
     for (uint8_t i=0; i<8; i++) {
       if (data[currentblock][i] < 16) toPrint += "0";
       toPrint += String(data[currentblock][i], HEX);
       toPrint += " ";
     }
       
     GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       
     toPrint="";
     for (uint8_t i=8; i<16; i++) {
       if (data[currentblock][i] < 16) toPrint += "0";
       toPrint += String(data[currentblock][i], HEX);
       toPrint += " ";
     }
 
     GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
             
     if(nfc.mifareclassic_WriteDataBlock (currentblock, &data[currentblock][0])) 
        GUIobj->printConsole(printOk, TFT_GREEN, 1, 0);
     else {
        GUIobj->printConsole(printFail, TFT_RED, 1, 0);
        return;}
  }
}


void showMemory(){ 
 char conv[2]={0,0};
  GUIobj->printConsole(printShowStored, TFT_YELLOW, 0, 0);
  delay(1000);
  for (uint8_t currentblock = 0; currentblock < 64; currentblock++){
       delay(1);
       if(GUIobj->getKeys()){
          GUIobj->printConsole(printBreak, TFT_RED, 0, 0);
          return;}
       String toPrint = "S";
       if(currentblock/4 < 10) toPrint += "0";
       toPrint += (String)(currentblock/4);
       toPrint += "/B";
       if(currentblock < 10) toPrint += "0";
       toPrint += (String)currentblock;
       toPrint += "  ";
       GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
      
       toPrint="";
       for (uint8_t i=0; i<8; i++) {
         if (data[currentblock][i] < 16) toPrint += "0";
         toPrint += String(data[currentblock][i], HEX);
         toPrint += " ";
       }
       
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       
       toPrint="";
       for (uint8_t i=8; i<16; i++) {
         if (data[currentblock][i] < 16) toPrint += "0";
         toPrint += String(data[currentblock][i], HEX);
         toPrint += " ";
       }
 
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);

       toPrint="";
       conv[0] = 0;
       for (uint8_t i=0; i<16; i++) {
         if (ntgdata[currentblock][i]>0x19 && data[currentblock][i]<0x7F) conv[0] = data[currentblock][i];
         else conv[0]='.';
         toPrint += conv;}
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
  }
}

  

void formatMifare(){
  bool authenticated = false;
  uint8_t blockBuffer[16]; 
  uint8_t blankAccessBits[3] = { 0xff, 0x07, 0x80 };
  uint8_t numOfSector = 16;  

  GUIobj->printConsole(printMifareToFormat, TFT_YELLOW, 0, 0);
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
    return;}
  if (uidLength != 4){
    GUIobj->printConsole(printNotMifareClassic, TFT_RED, 0, 0);
    return;}
  GUIobj->printConsole(printFormatingProgress, TFT_MAGENTA, 0, 0);
    
    for (uint8_t idx = 0; idx < numOfSector; idx++){
      if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, BLOCK_NUMBER_OF_SECTOR_TRAILER(idx), 1, (uint8_t *)KEY_DEFAULT_KEYAB)){
        GUIobj->printConsole(printAuthFail, TFT_RED, 0, 0);
        return;}
      
      if (idx == 16){
        memset(blockBuffer, 0, sizeof(blockBuffer));
        if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 3, blockBuffer))){
            GUIobj->printConsole(printWriteSectorError, TFT_RED, 0, 0);
            return;}
        }
        if ((idx == 0) || (idx == 16)){
          memset(blockBuffer, 0, sizeof(blockBuffer));
          if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 2, blockBuffer))){
            GUIobj->printConsole(printWriteSectorError, TFT_RED, 0, 0);
            return;}}
        else{
          memset(blockBuffer, 0, sizeof(blockBuffer));
          if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 3, blockBuffer))){
            GUIobj->printConsole(printWriteSectorError, TFT_RED, 0, 0);
            return;}
    
          if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 2, blockBuffer))){
            GUIobj->printConsole(printWriteSectorError, TFT_RED, 0, 0);
            return;}
        }
      memset(blockBuffer, 0, sizeof(blockBuffer));
      if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)) - 1, blockBuffer))){
        GUIobj->printConsole(printWriteSectorError, TFT_RED, 0, 0);
        return;}
      
      memcpy(blockBuffer, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));
      memcpy(blockBuffer + 6, blankAccessBits, sizeof(blankAccessBits));
      blockBuffer[9] = 0x69;
      memcpy(blockBuffer + 10, KEY_DEFAULT_KEYAB, sizeof(KEY_DEFAULT_KEYAB));

      if (!(nfc.mifareclassic_WriteDataBlock((BLOCK_NUMBER_OF_SECTOR_TRAILER(idx)), blockBuffer))){
        GUIobj->printConsole(printWriteTrailerError, TFT_RED, 0, 0);
        return;}
  }
  
  GUIobj->printConsole(printFormatingSucsesseful, TFT_GREEN, 0, 0);
}


void ntgRead(){
  GUIobj->printConsole(printNtagToRead, TFT_YELLOW, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
    return;}
  
  if (uidLength!=7){
    GUIobj->printConsole(printNotNtag, TFT_RED, 0, 0);
    return;}
    
    for (uint8_t i = 0; i < 42; i++) {
      if (!nfc.ntag2xx_ReadPage(i, &ntgdata[i][0])){
        GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
        return;}
          
       String toPrint="Page ";
       if (i < 10) toPrint+="0";
       toPrint += (String)i; 
       GUIobj->printConsole(toPrint, TFT_YELLOW, 0, 0);

       toPrint = "";
       for (uint8_t j=0; j<4; j++) {
         if (ntgdata[i][j] < 16) toPrint += "0";
         toPrint += String(ntgdata[i][j], HEX);
         toPrint += " ";}
 
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);

       toPrint="";
       char conv[2]={0,0};
       for (uint8_t j=0; j<4; i++) {
         if (ntgdata[i][j]>0x19 && data[i][j]<0x7F) conv[0] = data[i][j];
         else conv[0]='.';
         toPrint += conv;}
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
  }
}


void ntgErase(){
  uint8_t erasedta[4]={0,0,0,0};
  GUIobj->printConsole(printNtagToErase, TFT_YELLOW, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
    return;}
  
  if (uidLength!=7){
    GUIobj->printConsole(printNotNtag, TFT_RED, 0, 0);
    return;}

  GUIobj->printConsole(printErasingNtag, TFT_MAGENTA, 0, 0);
  
  for (uint8_t i = 4; i < 39; i++) {
    if(!nfc.ntag2xx_WritePage(i, erasedta)){
      GUIobj->printConsole(printEraseError, TFT_RED, 0, 0);
      return;}

    String toPrint="Page ";
    if (i < 10) toPrint+="0";
    toPrint += (String)i; 
    toPrint += F(" erase OK");
    GUIobj->printConsole(toPrint, TFT_GREEN, 0, 0);
  }
}


void ntgFormat(){
  GUIobj->printConsole(printMifareToNtag, TFT_YELLOW, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
    return;}
  
  if (uidLength!=4){
    GUIobj->printConsole(printNotMifareClassic, TFT_RED, 0, 0);
    return;}
  GUIobj->printConsole(printFormatingToNtag, TFT_MAGENTA, 0, 0);
  
    if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 0, 0, keya)){
      GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
      return;}

    if (!nfc.mifareclassic_FormatNDEF()){
      GUIobj->printConsole(printFail, TFT_RED, 1, 0);
      return;}

    GUIobj->printConsole(printOk, TFT_GREEN, 0, 0);
    GUIobj->printConsole(printWrittingNtag, TFT_MAGENTA, 0, 0);

    if (!nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 0, keya)){
      GUIobj->printConsole(printAuthFail, TFT_RED, 1, 0);
      return;
    }

    
    if (url.length() > 38) url = url.substring(0,38);
    
    if (nfc.mifareclassic_WriteNDEFURI(1, ndefprefix, url.c_str()))
      GUIobj->printConsole(printOk, TFT_GREEN, 1, 0);
    else
      GUIobj->printConsole(printFail, TFT_RED, 1, 0);
}





void ntgShow(){
  GUIobj->printConsole(printShowStored, TFT_YELLOW, 0, 0);
  delay(1000);
  for (uint8_t currentblock = 0; currentblock < 42; currentblock++){
       delay(1);
       if(GUIobj->getKeys()){
          GUIobj->printConsole(printBreak, TFT_RED, 0, 0);
          return;}
       String toPrint = "Page: ";
       if(currentblock < 10) toPrint += "0";
       toPrint += (String)(currentblock);
       GUIobj->printConsole(toPrint, TFT_YELLOW, 1, 0);
      
       toPrint="";
       for (uint8_t i=0; i<4; i++) {
         if (data[currentblock][i] < 16) toPrint += "0";
         toPrint += String(data[currentblock][i], HEX);
         toPrint += " ";
       }
       
       GUIobj->printConsole(capitaliseString(toPrint), TFT_WHITE, 0, 0);
       
       toPrint="";
       char conv[2]={0,0};
       for (uint8_t i=0; i<4; i++) {
         if (ntgdata[currentblock][i]>0x19 && data[currentblock][i]<0x7F) conv[0] = data[currentblock][i];
         else conv[0]='.';
         toPrint += conv;}
       GUIobj->printConsole(toPrint, TFT_BLUE, 0, 0);
  }
}



void ntgWrite(){
 uint8_t ntgdta[32];
 String toPrint;
 String urlToWrite="";

 GUIobj->printConsole(printEnterUrl, TFT_MAGENTA, 0, 0);
 while (urlToWrite=="") urlToWrite = GUIobj->getUserInput();
 urlToWrite = urlToWrite.substring(0,38);
 GUIobj->printConsole(urlToWrite, TFT_YELLOW, 0, 0);
 
 GUIobj->printConsole(printWaitingNtagToWrite, TFT_YELLOW, 0, 0);
  
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)){
    GUIobj->printConsole(printWrittingError, TFT_RED, 0, 0);
    return;}
  
  if (uidLength!=7){
    GUIobj->printConsole(printNotNtag, TFT_RED, 0, 0);
    return;}

  GUIobj->printConsole(printWrittingNtag, TFT_MAGENTA, 0, 0);
     
  memset(ntgdta, 0, 4);
  if (!nfc.ntag2xx_ReadPage(3, ntgdta)){
    GUIobj->printConsole(printReadingError, TFT_RED, 0, 0);
    return;}
  
  if (!((ntgdta[0] == 0xE1) && (ntgdta[1] == 0x10))){
    GUIobj->printConsole(printNtagStructureError, TFT_RED, 0, 0);
    return;}

  toPrint = printNtagDetected;
  toPrint += (String)(ntgdta[2]*8);
  GUIobj->printConsole(toPrint, TFT_MAGENTA, 0, 0);
  GUIobj->printConsole(printErasingOld, TFT_MAGENTA, 0, 0);
          
  for (uint8_t i = 4; i < (ntgdta[2]*2)+4; i++) {
     memset(ntgdta, 0, 4);
     if (!nfc.ntag2xx_WritePage(i, ntgdta)){
       GUIobj->printConsole(printWrittingError, TFT_RED, 0, 0);
       return;}

  GUIobj->printConsole(printWritting, TFT_MAGENTA, 0, 0);

  if(!nfc.ntag2xx_WriteNDEFURI(ndefprefix, (char *)urlToWrite.c_str(), ntgdta[2]*8))
    GUIobj->printConsole(printWrittingError, TFT_RED, 0, 0);
  else 
    GUIobj->printConsole(printWrittingOk, TFT_GREEN, 0, 0);
  }
}




void loop() {
  GUIobj->printConsole("", TFT_GREEN, 1, 0);    
  GUIobj->printConsole(printMenuLine1, TFT_GREEN, 1, 0);
  GUIobj->printConsole(printMenuLine2, TFT_GREEN, 1, 0);
  GUIobj->printConsole(printMenuLine3, TFT_YELLOW, 1, 0);
  GUIobj->printConsole(printMenuLine4, TFT_YELLOW, 1, 0);
  GUIobj->printConsole(printMenuCommand, TFT_MAGENTA, 1, 0);

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
    case MiWRITEM:
      writeMemory();
      break;
    case MiSHOW:
      showMemory();
      break;
    case MiFORMAT:
      formatMifare();
      break;
    case NTGread:
      ntgRead();
      break;
    case NTGerase:
      ntgErase();
      break;
    case NTGformat:
      ntgFormat();
      break;
    case NTGshow:
      ntgShow();
      break;    
    case NTGwrite:
      ntgWrite();
      break;
    default:
      GUIobj->printConsole(printWrongCom, TFT_RED, 1, 0);
      break;
  }
}

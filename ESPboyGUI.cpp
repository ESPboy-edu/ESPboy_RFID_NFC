/*
ESPboyGUI class
for www.ESPboy.com project by RomanS
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
*/

#include "ESPboyGUI.h"
#define SOUNDPIN D3

const uint8_t ESPboyGUI::keybOnscr[2][3][21] PROGMEM = {
 {"+1234567890abcdefghi", "jklmnopqrstuvwxyz -=", "?!@$%&*()_[]\":;.,^<E",},
 {"+1234567890ABCDEFGHI", "JKLMNOPQRSTUVWXYZ -=", "?!@$%&*()_[]\":;.,^<E",}
};

String *ESPboyGUI::consoleStrings;
uint16_t *ESPboyGUI::consoleStringsColor;


ESPboyGUI::ESPboyGUI(TFT_eSPI* tftGUI, Adafruit_MCP23017* mcpGUI, U8g2_for_TFT_eSPI *u8fGUI) {
   consoleStrings = new String[GUI_MAX_CONSOLE_STRINGS+1];
   consoleStringsColor = new uint16_t[GUI_MAX_CONSOLE_STRINGS+1];
   keybParam.renderLine = 0;
   keybParam.displayMode = 0;
   keybParam.shiftOn = 0;
   keybParam.selX = 0;
   keybParam.selY = 0;
   keybParam.typing = "";
   
   tft = tftGUI;
   mcp = mcpGUI;
   u8f = u8fGUI;
   toggleDisplayMode(1);
}


uint8_t ESPboyGUI::keysAction() {
  uint8_t longActPress = 0;
  uint8_t keyState = getKeys();

  if (keyState) {
    tone(SOUNDPIN, 100, 10);
    if (!keybParam.displayMode) {
      if (keyState & GUI_PAD_LEFT && keyState & GUI_PAD_UP) {  // shift
        keybParam.shiftOn = !keybParam.shiftOn;
        drawKeyboard(keybParam.selX, keybParam.selY, 0);
        waitKeyUnpressed();
      } else {
        if ((keyState & GUI_PAD_RIGHT) && keybParam.selX < 20) keybParam.selX++;
        if ((keyState & GUI_PAD_LEFT) && keybParam.selX > -1) keybParam.selX--;
        if ((keyState & GUI_PAD_DOWN) && keybParam.selY < 3) keybParam.selY++;
        if ((keyState & GUI_PAD_UP) && keybParam.selY > -1) keybParam.selY--;
        if ((keyState & GUI_PAD_LEFT) && keybParam.selX == -1) keybParam.selX = 19;
        if ((keyState & GUI_PAD_RIGHT) && keybParam.selX == 20) keybParam.selX = 0;
        if ((keyState & GUI_PAD_UP) && keybParam.selY == -1) keybParam.selY = 2;
        if ((keyState & GUI_PAD_DOWN) && keybParam.selY == 3) keybParam.selY = 0;
      }

      if ((keyState & GUI_PAD_ACT && keyState & GUI_PAD_ESC) ||
          (keyState & GUI_PAD_RGT && keyState & GUI_PAD_LFT)) {
        if (keybParam.renderLine > GUI_MAX_CONSOLE_STRINGS - GUI_MAX_STRINGS_ONSCREEN_FULL)
          keybParam.renderLine = GUI_MAX_CONSOLE_STRINGS - GUI_MAX_STRINGS_ONSCREEN_FULL;
        toggleDisplayMode(1);
        waitKeyUnpressed();
      } else if (keyState & GUI_PAD_RGT && keybParam.renderLine) {
        keybParam.renderLine--;
        drawConsole(0);
      } else if (keyState & GUI_PAD_LFT &&
                 keybParam.renderLine <
                     GUI_MAX_CONSOLE_STRINGS - GUI_MAX_STRINGS_ONSCREEN_SMALL) {
        keybParam.renderLine++;
        drawConsole(0);
      }

      if ((((keyState & GUI_PAD_ACT) && (keybParam.selX == 19 && keybParam.selY == 2)) || (keyState & GUI_PAD_RGT && keyState & GUI_PAD_LFT))) {  // enter
        if (keybParam.typing.length() > 0) longActPress = 1;
      } else if ((keyState & GUI_PAD_ACT) && (keybParam.selX == 18 && keybParam.selY == 2)) {  // back space
        if (keybParam.typing.length() > 0) keybParam.typing.remove(keybParam.typing.length() - 1);
      } else if ((keyState & GUI_PAD_ACT) && (keybParam.selX == 17 && keybParam.selY == 1)) {  // SPACE
            if (keybParam.typing.length() < GUI_MAX_TYPING_CHARS) keybParam.typing += " ";
      } else if ((keyState & GUI_PAD_ACT) && (keybParam.selX == 17 && keybParam.selY == 2)) {
        keybParam.shiftOn = !keybParam.shiftOn;
        drawKeyboard(keybParam.selX, keybParam.selY, 0);
        waitKeyUnpressed();
      } else if (keyState & GUI_PAD_ACT){
        if (waitKeyUnpressed() > GUI_KEY_PRESSED_DELAY_TO_SEND)
          longActPress = 1;
        else if (keybParam.typing.length() < GUI_MAX_TYPING_CHARS)
          keybParam.typing += (char)pgm_read_byte(&keybOnscr[keybParam.shiftOn][keybParam.selY][keybParam.selX]);
      }

      if (keyState & GUI_PAD_ESC) {
        if (waitKeyUnpressed() > GUI_KEY_PRESSED_DELAY_TO_SEND)
          keybParam.typing = "";
        else if (keybParam.typing.length() > 0)
          keybParam.typing.remove(keybParam.typing.length() - 1);
      }
    }

    else {
      if ((keyState & GUI_PAD_ACT && keyState & GUI_PAD_ESC) ||
          (keyState & GUI_PAD_RGT && keyState & GUI_PAD_LFT)) {
        toggleDisplayMode(0);
        waitKeyUnpressed();
      } else

          if (((keyState & GUI_PAD_RGT || keyState & GUI_PAD_RIGHT ||
                keyState & GUI_PAD_DOWN)) &&
              keybParam.renderLine > 0) {
        keybParam.renderLine--;
        drawConsole(0);
      } else

          if (((keyState & GUI_PAD_LFT || keyState & GUI_PAD_LEFT ||
                keyState & GUI_PAD_UP)) &&
              keybParam.renderLine < GUI_MAX_CONSOLE_STRINGS - GUI_MAX_STRINGS_ONSCREEN_FULL) {
        keybParam.renderLine++;
        drawConsole(0);
      } else

          if (keyState & GUI_PAD_ESC)
        toggleDisplayMode(0);
    }
    if (!keybParam.displayMode) drawKeyboard(keybParam.selX, keybParam.selY, 1);
  }

  if (!keybParam.displayMode) drawBlinkingCursor();
  return (longActPress);
}

void ESPboyGUI::toggleDisplayMode(uint8_t mode) {
  keybParam.displayMode = mode;
  tft->fillScreen(TFT_BLACK);
  tft->drawRect(0, 0, 128, 128, TFT_NAVY);
  if (!keybParam.displayMode) {
    tft->drawRect(0, 128 - 3 * 8 - 5, 128, 3 * 8 + 5, TFT_NAVY);
    tft->drawRect(0, 0, 128, 86, TFT_NAVY);
  }
  if (!keybParam.displayMode) {
    drawKeyboard(keybParam.selX, keybParam.selY, 0);
  }
  drawConsole(0);
}

String ESPboyGUI::getUserInput() {
  String userInput;
  toggleDisplayMode(0);
  while (1) {
    while (!keysAction()) delay(GUI_KEYB_CALL_DELAY);
    if (keybParam.typing != "") break;
  }
  toggleDisplayMode(1);
  userInput = keybParam.typing;
  keybParam.typing = "";
  return (userInput);
}

void ESPboyGUI::printConsole(String bfrstr, uint16_t color, uint8_t ln, uint8_t noAddLine) {
  String toprint;

  keybParam.renderLine = 0;

  if(bfrstr == "") bfrstr = " ";
  
  if (!ln)
    if (bfrstr.length() > (128-2)/GUI_FONT_WIDTH) {
      bfrstr = bfrstr.substring(0, (128-2)/GUI_FONT_WIDTH);
      toprint = bfrstr;
    }

  for (uint8_t i = 0; i <= ((bfrstr.length()-1) / ((128-2)/GUI_FONT_WIDTH)); i++) {
    toprint = bfrstr.substring(i * (128-2)/GUI_FONT_WIDTH);
    toprint = toprint.substring(0, (128-2)/GUI_FONT_WIDTH);

    if (!noAddLine) {
      for (uint8_t j = 0; j < GUI_MAX_CONSOLE_STRINGS; j++) {
        consoleStrings[j] = consoleStrings[j + 1];
        consoleStringsColor[j] = consoleStringsColor[j + 1];
      }
    }

    consoleStrings[GUI_MAX_CONSOLE_STRINGS] = toprint;
    consoleStringsColor[GUI_MAX_CONSOLE_STRINGS] = color;
  }
  drawConsole(noAddLine);
}



void ESPboyGUI::drawConsole(uint8_t onlyLastLine) {
  uint8_t lines;
  uint8_t offsetY;
  
  if (keybParam.displayMode)
    lines = GUI_MAX_STRINGS_ONSCREEN_FULL;
  else
    lines = GUI_MAX_STRINGS_ONSCREEN_SMALL;

  if (!onlyLastLine)
    tft->fillRect(1, 1, 126, lines * GUI_FONT_HEIGHT, TFT_BLACK);
  else
    tft->fillRect(1, lines * GUI_FONT_HEIGHT - GUI_FONT_HEIGHT, 126, GUI_FONT_HEIGHT, TFT_BLACK);

  offsetY = GUI_FONT_HEIGHT;
  if (!onlyLastLine) {
    for (uint8_t i = GUI_MAX_CONSOLE_STRINGS - lines - keybParam.renderLine + 1; i < GUI_MAX_CONSOLE_STRINGS - keybParam.renderLine + 1; i++) {
      //tft->setTextColor(consoleStringsColor[i], TFT_BLACK);
      //tft->drawString(consoleStrings[i], 4, offsetY);
      u8f->setForegroundColor(consoleStringsColor[i]);
      u8f->drawStr(3, offsetY, consoleStrings[i].c_str());
      offsetY += GUI_FONT_HEIGHT;
    }
  } else {
    //tft->setTextColor(consoleStringsColor[GUI_MAX_CONSOLE_STRINGS], TFT_BLACK);
    //tft->drawString(consoleStrings[GUI_MAX_CONSOLE_STRINGS], 4, GUI_FONT_HEIGHT * (lines - 1) + 3);
    u8f->setForegroundColor(consoleStringsColor[GUI_MAX_CONSOLE_STRINGS]);
    u8f->drawStr(3, GUI_FONT_HEIGHT * lines, consoleStrings[GUI_MAX_CONSOLE_STRINGS].c_str());
  }
}


uint8_t ESPboyGUI::getKeys() { return (~mcp->readGPIOAB() & 255); }


uint32_t ESPboyGUI::waitKeyUnpressed() {
  uint32_t timerStamp = millis();
  while (getKeys() && (millis() - timerStamp) < GUI_KEY_UNPRESSED_TIMEOUT) delay(1);
  return (millis() - timerStamp);
}


void ESPboyGUI::drawKeyboard(uint8_t slX, uint8_t slY, uint8_t onlySelected) {
  static char chr[2]={0,0};
  static uint8_t prevX = 0, prevY = 0;

  if (!onlySelected) {
    tft->fillRect(1, 128 - 24, 126, 23, TFT_BLACK);
    tft->setTextColor(TFT_YELLOW, TFT_BLACK);
    for (uint8_t j = 0; j < 3; j++)
      for (uint8_t i = 0; i < 20; i++) {
        chr[0] = pgm_read_byte(&keybOnscr[keybParam.shiftOn][j][i]);
        tft->drawString(&chr[0], i * 6 + 4, 128 - 2 - 8 * (3 - j));
      }
  }

  tft->setTextColor(TFT_YELLOW, TFT_BLACK);
  chr[0] = pgm_read_byte(&keybOnscr[keybParam.shiftOn][prevY][prevX]);
  tft->drawString(&chr[0], prevX * 6 + 4, 128 - 24 + prevY * 8 - 2);

  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  tft->drawString("^<E", 6 * 17 + 4, 128 - 24 + 2 * 8 - 2);

  tft->setTextColor(TFT_YELLOW, TFT_RED);
  chr[0] = pgm_read_byte(&keybOnscr[keybParam.shiftOn][slY][slX]);
  tft->drawString(&chr[0], slX * 6 + 4, 128 - 24 + slY * 8 - 2);
  
  prevX = slX;
  prevY = slY;

  drawTyping(0);
}


void ESPboyGUI::drawTyping(uint8_t changeCursor) {
  static char cursorType[2] = {220, '_'};
  static uint8_t cursorTypeFlag=0;

  if(changeCursor) cursorTypeFlag=!cursorTypeFlag;
  tft->fillRect(1, 128 - 5 * 8, 126, 10, TFT_BLACK);
  if (keybParam.typing.length() < 20) {
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString(keybParam.typing + cursorType[cursorTypeFlag], 4, 128 - 5 * 8 + 1);
  } else {
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->drawString("<" + keybParam.typing.substring(keybParam.typing.length() - 18) +cursorType[cursorTypeFlag], 4, 128 - 5 * 8 + 1);
  }
}

void ESPboyGUI::drawBlinkingCursor() {
 static uint32_t cursorBlinkMillis = 0; 
  if (millis() > (cursorBlinkMillis + GUI_CURSOR_BLINKING_PERIOD)) {
    cursorBlinkMillis = millis();
    drawTyping(1);
  }
}


void ESPboyGUI::SetKeybParamTyping(String str){
  keybParam.typing = str;
}

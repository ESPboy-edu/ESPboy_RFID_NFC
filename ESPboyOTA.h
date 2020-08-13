/*
ESPboyOTA class -- ESPboy App Store client core
for www.ESPboy.com project 
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
thanks to DmitryL (Plague) for coding help,
Corax, AlRado, Torabora, MLXXXP for tests and advices.
*/

#ifndef ESPboy_OTA
#define ESPboy_OTA

#include <Adafruit_MCP23017.h>
#include <TFT_eSPI.h>
#include <FS.h> 
using fs::FS;
#include <HTTPSRedirect.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include "ESPboyGUI.h"

#define OTA_TIMEOUT_CONNECTION 10000

#define OTA_PAD_LEFT        0x01
#define OTA_PAD_UP          0x02
#define OTA_PAD_DOWN        0x04
#define OTA_PAD_RIGHT       0x08
#define OTA_PAD_ACT         0x10
#define OTA_PAD_ESC         0x20
#define OTA_PAD_LFT         0x40
#define OTA_PAD_RGT         0x80
#define OTA_PAD_ANY         0xff

struct firmware{
    String firmwareName;
    String firmwareLink;
};

struct wf {
    String ssid;
    uint8_t rssi;
    uint8_t encription;
};

struct fw {
    String fwName;
    String fwLink;
};

struct lessRssi{
    inline bool operator() (const wf& str1, const wf& str2) {return (str1.rssi > str2.rssi);}
};


class ESPboyOTA{

private:
  Adafruit_MCP23017 *mcp; 
  TFT_eSPI *tft;
  ESPboyGUI *GUIobj = NULL;

  std::vector<wf> wfList;  // WiFi list
  std::vector<fw> fwList;  // Firmware list

  struct wificlient{
    String ssid;
    String pass;
  HTTPSRedirect *clientD;
  }wificl;

  const static char PROGMEM *hostD;
  const static char PROGMEM *urlPost;
  const static uint16_t PROGMEM httpsPort;

  static String *consoleStrings;
  static uint16_t *consoleStringsColor;

	uint16_t scanWiFi();
	String getWiFiStatusName();
	boolean connectWifi();
	void OTAstarted();
	void OTAfinished();
	void OTAprogress(int cur, int total);
	void OTAerror(int err);
	void updateOTA(String otaLink);
	String fillPayload(String downloadID, String downloadName);
	void postLog(String downloadID, String downloadName);
	firmware getFirmware();
	void checkOTA();

public:
	ESPboyOTA(TFT_eSPI *tftOTA, Adafruit_MCP23017 *mcpOTA, ESPboyGUI* GUIobjOTA);
};

#endif

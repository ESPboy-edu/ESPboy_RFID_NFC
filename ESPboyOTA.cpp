/*
ESPboyOTA class -- ESPboy App Store client core
for www.ESPboy.com project 
https://hackaday.io/project/164830-espboy-games-iot-stem-for-education-fun
thanks to DmitryL (Plague) for coding help,
Corax, AlRado, Torabora, MLXXXP for tests and advices.
v2.1
*/

#define OTAv 16

#include "ESPboyOTA.h"
#define SOUNDPIN D3


//http://script.google.com/macros/s/AKfycbxIfj1Eqi1eupe9Vmkhk0liuNrhSvM1Sx65qxocbBsd4jl0e7yj/exec
const char PROGMEM* ESPboyOTA::hostD = "script.google.com";
const char PROGMEM* ESPboyOTA::urlPost = "/macros/s/AKfycbxIfj1Eqi1eupe9Vmkhk0liuNrhSvM1Sx65qxocbBsd4jl0e7yj/exec";
const uint16_t PROGMEM ESPboyOTA::httpsPort = 443;

ESPboyOTA::ESPboyOTA(ESPboyGUI* GUIobjOTA) {    
   GUIobj = GUIobjOTA;
   GUIobj->toggleDisplayMode(1);
   checkOTA();
}


uint16_t ESPboyOTA::scanWiFi() {
  GUIobj->printConsole(F("Scaning WiFi..."), TFT_MAGENTA, 1, 0);
  int16_t WifiQuantity = WiFi.scanNetworks();
  if (WifiQuantity != -1 && WifiQuantity != -2 && WifiQuantity != 0) {
    for (uint8_t i = 0; i < WifiQuantity; i++) wfList.push_back(wf());
    if (!WifiQuantity) {
      GUIobj->printConsole(F("WiFi not found"), TFT_RED, 1, 0);
      delay(3000);
      ESP.reset();
    } else
      for (uint8_t i = 0; i < wfList.size(); i++) {
        wfList[i].ssid = WiFi.SSID(i);
        wfList[i].rssi = WiFi.RSSI(i);
        wfList[i].encription = WiFi.encryptionType(i);
        delay(1);
      }
    sort(wfList.begin(), wfList.end(), lessRssi());
    return (WifiQuantity);
  } else
    return (0);
}

String ESPboyOTA::getWiFiStatusName() {
  String stat;
  switch (WiFi.status()) {
    case WL_IDLE_STATUS:
      stat = (F("Idle"));
      break;
    case WL_NO_SSID_AVAIL:
      stat = (F("No SSID available"));
      break;
    case WL_SCAN_COMPLETED:
      stat = (F("Scan completed"));
      break;
    case WL_CONNECTED:
      stat = (F("WiFi connected"));
      break;
    case WL_CONNECT_FAILED:
      stat = (F("Wrong passphrase"));
      break;
    case WL_CONNECTION_LOST:
      stat = (F("Connection lost"));
      break;
    case WL_DISCONNECTED:
      stat = (F("Wrong password"));
      break;
    default:
      stat = (F("Unknown"));
      break;
  };
  return stat;
}

boolean ESPboyOTA::connectWifi() {
  uint16_t wifiNo = 0;
  uint32_t timeOutTimer;

  if (wificl.ssid == "1" && wificl.pass == "1" && !(GUIobj->getKeys()&OTA_PAD_ESC) && (WiFi.SSID()!="")) {
    wificl.ssid = WiFi.SSID();
    wificl.pass = WiFi.psk();
    GUIobj->printConsole(F("Last network:"), TFT_GREEN, 0, 0);
    GUIobj->printConsole(wificl.ssid, TFT_MAGENTA, 0, 0);
  } else {
    if (WiFi.SSID()==""){
      wificl.ssid = "";
      wificl.pass = "";
    }
    if (scanWiFi())
      for (uint8_t i = wfList.size(); i > 0; i--) {
        String toPrint =
            (String)(i) + " " + wfList[i - 1].ssid + " [" + wfList[i - 1].rssi +
            "]" + ((wfList[i - 1].encription == ENC_TYPE_NONE) ? "" : "*");
        GUIobj->printConsole(toPrint, TFT_GREEN, 0, 0);
      }

    while (!wifiNo) {
      GUIobj->printConsole(F("Choose WiFi No:"), TFT_MAGENTA, 0, 0);
      wifiNo = GUIobj->getUserInput().toInt();
      if (wifiNo < 1 || wifiNo > wfList.size()) wifiNo = 0;
    }

    wificl.ssid = wfList[wifiNo - 1].ssid;
    GUIobj->printConsole(wificl.ssid, TFT_YELLOW, 1, 0);

    while (!wificl.pass.length()) {
      GUIobj->printConsole(F("Password:"), TFT_MAGENTA, 0, 0);
      wificl.pass = GUIobj->getUserInput();
    }
    GUIobj->printConsole(/*pass*/F("******"), TFT_YELLOW, 0, 0);
  }

  wfList.clear();

  WiFi.mode(WIFI_STA);
  WiFi.begin(wificl.ssid, wificl.pass);

  GUIobj->printConsole(F("Connection..."), TFT_MAGENTA, 0, 0);
  timeOutTimer = millis();
  String dots = "";
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - timeOutTimer < OTA_TIMEOUT_CONNECTION)) {
    delay(700);
    GUIobj->printConsole(dots, TFT_MAGENTA, 0, 1);
    dots += ".";
  }

  if (WiFi.status() != WL_CONNECTED) {
    wificl.ssid = "";
    wificl.pass = "";
    GUIobj->printConsole(getWiFiStatusName(), TFT_RED, 0, 1);
    return (false);
  } else {
    // Serial.println(WiFi.localIP());
    GUIobj->printConsole(getWiFiStatusName(), TFT_MAGENTA, 0, 1);
    return (true);
  }
}

void ESPboyOTA::OTAstarted() {
  GUIobj->printConsole(F("Starting download..."), TFT_MAGENTA, 0, 0);
  GUIobj->printConsole("", TFT_MAGENTA, 0, 0);
}

void ESPboyOTA::OTAfinished() {
  GUIobj->printConsole(F("Downloading OK"), TFT_GREEN, 0, 0);
  GUIobj->printConsole(F("Restarting..."), TFT_MAGENTA, 0, 0);
  GUIobj->printConsole(F("And then reset it again by yourself"), TFT_RED, 1, 0);
  ESP.reset();
}

void ESPboyOTA::OTAprogress(int cur, int total) {
  GUIobj->printConsole((String)(cur * 100 / total) + "%", TFT_GREEN, 0, 1);
}

void ESPboyOTA::OTAerror(int err) {
  // Serial.print(F("Error: ")); Serial.print(err);
  //printConsole("Error: "+String(err), TFT_RED, 1, 0);
  GUIobj->printConsole(ESPhttpUpdate.getLastErrorString(), TFT_RED, 1, 0);
  delay(3000);
  ESP.reset();
}

void ESPboyOTA::updateOTA(String otaLink) {
  BearSSL::WiFiClientSecure updateClient;
  updateClient.setInsecure();
  // ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  ESPhttpUpdate.onStart([this](){this->OTAstarted();});
  ESPhttpUpdate.onEnd([this](){this->OTAfinished();});
  ESPhttpUpdate.onProgress([this](int a, int b){this->OTAprogress(a,b);});
  ESPhttpUpdate.onError([this](int a){this->OTAerror(a);});
  ESPhttpUpdate.update(updateClient, otaLink);
}

String ESPboyOTA::fillPayload(String downloadID, String downloadName) {
  String payload = "{\"values\": \"";
  payload += WiFi.macAddress();   // MAC address
  payload += F(", ");  // date/time
  payload +=F(", "); 
  payload += WiFi.localIP().toString();
  payload += F(", "); 
  payload += downloadID;    // download ID
  payload += F(", "); 
  payload += downloadName;  // download name
  payload += F(", "); 
  payload += (String)ESP.getFreeHeap();
  payload += F(", "); 
  payload += (String)ESP.getFreeContStack();
  payload += F(", "); 
  payload += (String)ESP.getChipId();
  payload += F(", "); 
  payload += (String)ESP.getFlashChipId();
  payload += F(", "); 
  payload += (String)ESP.getCoreVersion();
  payload += F(", "); 
  payload += (String)ESP.getSdkVersion();
  payload += F(", "); 
  payload += (String)ESP.getCpuFreqMHz();
  payload += F(", "); 
  payload += (String)ESP.getSketchSize();
  payload += F(", "); 
  payload += (String)ESP.getFreeSketchSpace();
  payload += F(", "); 
  payload += (String)ESP.getSketchMD5();
  payload += F(", "); 
  payload += (String)ESP.getFlashChipSize();
  payload += F(", "); 
  payload += (String)ESP.getFlashChipRealSize();
  payload += F(", "); 
  payload += (String)ESP.getFlashChipSpeed();
  payload += F(", ");
  payload += (String)ESP.getCycleCount();
  payload += F(", "); 
  payload += WiFi.SSID();
  payload += F("\"}");

  return (payload);
}

void ESPboyOTA::postLog(String downloadID, String downloadName) {
  wificl.clientD = new HTTPSRedirect(httpsPort);
  wificl.clientD->setInsecure();
  wificl.clientD->setPrintResponseBody(false);
  wificl.clientD->setContentTypeHeader("application/json");
  wificl.clientD->setPrintResponseBody(true);

  int connectionAttempts = 0;
  while (wificl.clientD->connect(hostD, httpsPort) != 1 && connectionAttempts++ < 5)
    ;
  if (connectionAttempts > 4) {
    GUIobj->printConsole(F("Server failed"), TFT_RED, 0, 0);
    delay(5000);
    ESP.reset();
  }

  GUIobj->printConsole(F("Server OK"), TFT_MAGENTA, 0, 0);
  String payload = fillPayload(downloadID, downloadName);
  // Serial.println(payload);
  wificl.clientD->POST(urlPost, hostD, payload, false);
  // Serial.println(wificl.clientD->getResponseBody());

  wificl.clientD->stop();
  delete wificl.clientD;
}

firmware ESPboyOTA::getFirmware() {
  uint16_t firmwareNo;
  String readedData = "";
  firmware fmw;
  
  wificl.clientD = new HTTPSRedirect(httpsPort);
  wificl.clientD->setInsecure();
  wificl.clientD->setPrintResponseBody(false);

  int connectionAttempts = 0;
  while (wificl.clientD->connect(hostD, httpsPort) != 1 && connectionAttempts++ < 5);
  if (connectionAttempts > 4) {
    GUIobj->printConsole(F("Server faild"), TFT_RED, 0, 0);
    delay(5000);
    ESP.reset();
  }

  GUIobj->printConsole(F("Loading Apps..."), TFT_MAGENTA, 0, 0);

  if (wificl.clientD->GET((String)urlPost + F("?read"), hostD)) {
    readedData = wificl.clientD->getResponseBody();

    uint16_t countVector = 0;
    char* prs = strtok((char*)readedData.c_str(), ";\n");
    while (prs != NULL) {
      fwList.push_back(fw());
      fwList[countVector].fwName = prs;
      prs = strtok(NULL, ";\n");
      fwList[countVector].fwLink = prs;
      prs = strtok(NULL, ";\n");
      countVector++;
    }
  } else
    GUIobj->printConsole(F("Loading failed"), TFT_RED, 0, 0);

  for (uint8_t i = 0; i < fwList.size(); i++) {
    String toprint = (String)(i + 1) + " " + fwList[i].fwName;
    GUIobj->printConsole(toprint, TFT_GREEN, 0, 0);
    delay(1);
  }

  firmwareNo = 0;
  while (!firmwareNo) {
    GUIobj->printConsole(F("Choose App:"), TFT_MAGENTA, 0, 0);
    firmwareNo = GUIobj->getUserInput().toInt();
    if (firmwareNo < 1 || firmwareNo > fwList.size()) firmwareNo = 0;
  }

  fmw.firmwareName = fwList[firmwareNo - 1].fwName;
  fmw.firmwareLink = fwList[firmwareNo - 1].fwLink;

  GUIobj->printConsole(fmw.firmwareName, TFT_YELLOW, 0, 0);
  GUIobj->printConsole(F("Loading info..."), TFT_MAGENTA, 0, 0);

  if (wificl.clientD->GET((String)urlPost + F("?info=") + String(firmwareNo + 1),
                   hostD)) {
    readedData = wificl.clientD->getResponseBody();
    char* prs = strtok((char*)readedData.c_str(), ";\n");
    GUIobj->printConsole(F("App name:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
    prs = strtok(NULL, ";\n");
    GUIobj->printConsole(F("Type:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
    prs = strtok(NULL, ";\n");
    GUIobj->printConsole(F("Genre:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
    prs = strtok(NULL, ";\n");
    GUIobj->printConsole(F("Author:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
    prs = strtok(NULL, ";\n");
    GUIobj->printConsole(F("License:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
    prs = strtok(NULL, ";\n");
    GUIobj->printConsole(F("Info:"), TFT_MAGENTA, 0, 0);
    GUIobj->printConsole((String)prs, TFT_GREEN, 1, 0);
  } else
    GUIobj->printConsole(F("Failed"), TFT_RED, 0, 0);

  char approve = 0;
  while (approve == 0) {
    GUIobj->printConsole(F("Download App?  [y/n]"), TFT_MAGENTA, 0, 0);
    GUIobj->SetKeybParamTyping("y");
    approve = GUIobj->getUserInput()[0];
  }


  if (approve == 'y' || approve == 'Y') {
    GUIobj->printConsole(F("YES"), TFT_YELLOW, 0, 0);
  } else {
    GUIobj->printConsole(F("NO"), TFT_YELLOW, 0, 0);
    fmw.firmwareName="";
  }

  fwList.clear();
  wificl.clientD->stop();
  delete wificl.clientD;
  return (fmw);
}

void ESPboyOTA::checkOTA() {
  firmware fmw;
  fmw.firmwareName="";
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
  wifi_station_disconnect();
  String toprint = F("ESPboy App store v");
  toprint += (String)OTAv;
  GUIobj->printConsole(toprint, TFT_YELLOW, 1, 0);
  // Serial.println(F("\n\nWiFi init OK"));
  while (!connectWifi()) delay(1500);
  postLog("no", "no");
  while (fmw.firmwareName == "") {
    fmw = getFirmware();
    delay(500);
  }
  postLog("no", fmw.firmwareName);
  updateOTA(fmw.firmwareLink);
  wifi_station_disconnect();
  delay(5000);
}

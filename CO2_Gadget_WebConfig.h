#ifndef CO2_Gadget_WebConfig_h
#define CO2_Gadget_WebConfig_h

#ifdef SUPPORT_WEBCONFIG

#if !(defined(ESP8266) || defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

// Use from 0 to 4. Higher number, more debugging messages and memory usage.
#define _ESPASYNC_WIFIMGR_LOGLEVEL_ 3

#define ESP_getChipId() ((uint32_t)ESP.getEfuseMac())
const int TRIGGER_PIN = WEBCONFIG_TRIGGER_PIN;

// SSID and PW for Config Portal
String ssid = "CO2-GADGET_" + String(ESP_getChipId(), HEX);
const char *password = "emariete";

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

#define MIN_AP_PASSWORD_SIZE 8

#define SSID_MAX_LEN 32
// From v1.0.10, WPA2 passwords can be up to 63 characters long.
#define PASS_MAX_LEN 64

typedef struct {
  char wifi_ssid[SSID_MAX_LEN];
  char wifi_pw[PASS_MAX_LEN];
} WiFi_Credentials;

typedef struct {
  String wifi_ssid;
  String wifi_pw;
} WiFi_Credentials_String;

#define NUM_WIFI_CREDENTIALS 2

// Assuming max 49 chars
#define TZNAME_MAX_LEN 50
#define TIMEZONE_MAX_LEN 50

// Use USE_DHCP_IP == true for dynamic DHCP IP, false to use static IP which you
// have to change accordingly to your network
#if (defined(USE_STATIC_IP_CONFIG_IN_CP) && !USE_STATIC_IP_CONFIG_IN_CP)
// Force DHCP to be true
#if defined(USE_DHCP_IP)
#undef USE_DHCP_IP
#endif
#define USE_DHCP_IP true
#else
// You can select DHCP or Static IP here
#define USE_DHCP_IP true
//#define USE_DHCP_IP     false
#endif

#if (USE_DHCP_IP)
// Use DHCP
#warning Using DHCP IP
IPAddress stationIP = IPAddress(0, 0, 0, 0);
IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
IPAddress netMask = IPAddress(255, 255, 255, 0);
#else
// Use static IP
#warning Using static IP

#ifdef ESP32
IPAddress stationIP = IPAddress(192, 168, 2, 232);
#else
IPAddress stationIP = IPAddress(192, 168, 2, 186);
#endif

IPAddress gatewayIP = IPAddress(192, 168, 2, 1);
IPAddress netMask = IPAddress(255, 255, 255, 0);
#endif

#define USE_CONFIGURABLE_DNS true

IPAddress dns1IP = gatewayIP;
IPAddress dns2IP = IPAddress(8, 8, 8, 8);

#define USE_CUSTOM_AP_IP false

IPAddress APStaticIP = IPAddress(192, 168, 100, 1);
IPAddress APStaticGW = IPAddress(192, 168, 100, 1);
IPAddress APStaticSN = IPAddress(255, 255, 255, 0);

typedef struct {
  WiFi_Credentials WiFi_Creds[NUM_WIFI_CREDENTIALS];
  char TZ_Name[TZNAME_MAX_LEN];  // "America/Toronto"
  char TZ[TIMEZONE_MAX_LEN];     // "EST5EDT,M3.2.0,M11.1.0"
  uint16_t checksum;
} WM_Config;

WM_Config WM_config;

#define FileFS SPIFFS
#define CONFIG_FILENAME F("/wifi_cred.dat")

// Indicates whether ESP has WiFi credentials saved from previous session, or manual config
bool initialConfig = false;

//////

#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#include <ESPAsync_WiFiManager.h>  //https://github.com/khoih-prog/ESPAsync_WiFiManager

#define HTTP_PORT 80

#if !defined WIFI_SSID_CREDENTIALS || !defined WIFI_PW_CREDENTIALS
#include "credentials.h"
#endif

WiFiClient espClient;

AsyncWebServer server(HTTP_PORT);

#if (USING_ESP32_S2 || USING_ESP32_C3)
  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, NULL, "CO2-Gadget");
#else
  DNSServer dnsServer;
  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, &dnsServer, "CO2-Gadget");
#endif

// flag for saving data
bool shouldSaveConfig = false;

/******************************************
 * // Defined in ESPAsync_WiFiManager.h
typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
#if USE_CONFIGURABLE_DNS  
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
#endif
}  WiFi_STA_IPConfig;
******************************************/

WiFi_AP_IPConfig  WM_AP_IPconfig;
WiFi_STA_IPConfig WM_STA_IPconfig;

void initAPIPConfigStruct(WiFi_AP_IPConfig &in_WM_AP_IPconfig)
{
  in_WM_AP_IPconfig._ap_static_ip   = APStaticIP;
  in_WM_AP_IPconfig._ap_static_gw   = APStaticGW;
  in_WM_AP_IPconfig._ap_static_sn   = APStaticSN;
}

void initSTAIPConfigStruct(WiFi_STA_IPConfig &in_WM_STA_IPconfig)
{
  in_WM_STA_IPconfig._sta_static_ip   = stationIP;
  in_WM_STA_IPconfig._sta_static_gw   = gatewayIP;
  in_WM_STA_IPconfig._sta_static_sn   = netMask;
#if USE_CONFIGURABLE_DNS  
  in_WM_STA_IPconfig._sta_static_dns1 = dns1IP;
  in_WM_STA_IPconfig._sta_static_dns2 = dns2IP;
#endif
}

void displayIPConfigStruct(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  LOGERROR3(F("stationIP ="), in_WM_STA_IPconfig._sta_static_ip, ", gatewayIP =", in_WM_STA_IPconfig._sta_static_gw);
  LOGERROR1(F("netMask ="), in_WM_STA_IPconfig._sta_static_sn);
#if USE_CONFIGURABLE_DNS
  LOGERROR3(F("dns1IP ="), in_WM_STA_IPconfig._sta_static_dns1, ", dns2IP =", in_WM_STA_IPconfig._sta_static_dns2);
#endif
}

void configWiFi(WiFi_STA_IPConfig in_WM_STA_IPconfig)
{
  #if USE_CONFIGURABLE_DNS  
    // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn, in_WM_STA_IPconfig._sta_static_dns1, in_WM_STA_IPconfig._sta_static_dns2);  
  #else
    // Set static IP, Gateway, Subnetmask, Use auto DNS1 and DNS2.
    WiFi.config(in_WM_STA_IPconfig._sta_static_ip, in_WM_STA_IPconfig._sta_static_gw, in_WM_STA_IPconfig._sta_static_sn);
  #endif 
}

int calcChecksum(uint8_t *address, uint16_t sizeToCalc) {
  uint16_t checkSum = 0;

  for (uint16_t index = 0; index < sizeToCalc; index++) {
    checkSum += *(((byte *)address) + index);
  }

  return checkSum;
}

// callback notifying us of the need to save config
void saveConfigCallback() {
//   Serial.println("Should save config");
//   shouldSaveConfig = true;
}

bool loadConfigData() {
  File file = FileFS.open(CONFIG_FILENAME, "r");
  LOGERROR(F("LoadWiFiCfgFile "));

  memset((void *)&WM_config, 0, sizeof(WM_config));

  // New in v1.4.0
  memset((void *)&WM_STA_IPconfig, 0, sizeof(WM_STA_IPconfig));
  //////

  if (file) {
    file.readBytes((char *)&WM_config, sizeof(WM_config));

    // New in v1.4.0
    file.readBytes((char *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
    //////

    file.close();
    LOGERROR(F("OK"));

    if (WM_config.checksum !=
        calcChecksum((uint8_t *)&WM_config,
                     sizeof(WM_config) - sizeof(WM_config.checksum))) {
      LOGERROR(F("WM_config checksum wrong"));

      return false;
    }

    // New in v1.4.0
    displayIPConfigStruct(WM_STA_IPconfig);
    //////

    return true;
  } else {
    LOGERROR(F("failed"));

    return false;
  }
}

void saveConfigData() {
  // File file = FileFS.open(CONFIG_FILENAME, "w");
  // LOGERROR(F("SaveWiFiCfgFile "));

  // if (file) {
  //   WM_config.checksum = calcChecksum(
  //       (uint8_t *)&WM_config, sizeof(WM_config) - sizeof(WM_config.checksum));

  //   file.write((uint8_t *)&WM_config, sizeof(WM_config));

  //   displayIPConfigStruct(WM_STA_IPconfig);

  //   // New in v1.4.0
  //   file.write((uint8_t *)&WM_STA_IPconfig, sizeof(WM_STA_IPconfig));
  //   //////

  //   file.close();
  //   LOGERROR(F("OK"));
  // } else {
  //   LOGERROR(F("failed"));
  // }
}

#if USE_ESP_WIFIMANAGER_NTP

void printLocalTime() {
#if ESP8266
  static time_t now;

  now = time(nullptr);

  if (now > 1451602800) {
    Serial.print("Local Date/Time: ");
    Serial.print(ctime(&now));
  }
#else
  struct tm timeinfo;

  getLocalTime(&timeinfo);

  // Valid only if year > 2000.
  // You can get from timeinfo : tm_year, tm_mon, tm_mday, tm_hour, tm_min,
  // tm_sec
  if (timeinfo.tm_year > 100) {
    Serial.print("Local Date/Time: ");
    Serial.print(asctime(&timeinfo));
  }
#endif
}
#endif

void heartBeatPrint() {
#if USE_ESP_WIFIMANAGER_NTP
  printLocalTime();
#else
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H"));  // H means connected to WiFi
  else
    Serial.print(F("F"));  // F means not connected to WiFi

  if (num == 80) {
    Serial.println();
    num = 1;
  } else if (num++ % 10 == 0) {
    Serial.print(F(" "));
  }
#endif
}

uint8_t connectMultiWiFi() {
#if ESP32
// For ESP32, this better be 0 to shorten the connect time.
// For ESP32-S2/C3, must be > 500
#if (USING_ESP32_S2 || USING_ESP32_C3)
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 500L
#else
// For ESP32 core v1.0.6, must be >= 500
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 800L
#endif
#else
// For ESP8266, this better be 2200 to enable connect the 1st time
#define WIFI_MULTI_1ST_CONNECT_WAITING_MS 2200L
#endif

#define WIFI_MULTI_CONNECT_WAITING_MS 500L

  uint8_t status;

  // WiFi.mode(WIFI_STA);

  LOGERROR(F("ConnectMultiWiFi with :"));

  if ((Router_SSID != "") && (Router_Pass != "")) {
    LOGERROR3(F("* Flash-stored Router_SSID = "), Router_SSID,
              F(", Router_Pass = "), Router_Pass);
    LOGERROR3(F("* Add SSID = "), Router_SSID, F(", PW = "), Router_Pass);
    wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
  }

  for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++) {
    // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
    if ((String(WM_config.WiFi_Creds[i].wifi_ssid) != "") &&
        (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE)) {
      LOGERROR3(F("* Additional SSID = "), WM_config.WiFi_Creds[i].wifi_ssid,
                F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw);
    }
  }

  LOGERROR(F("Connecting MultiWifi..."));

  // WiFi.mode(WIFI_STA);

#if !USE_DHCP_IP
  // New in v1.4.0
  configWiFi(WM_STA_IPconfig);
  //////
#endif

  int i = 0;
  status = wifiMulti.run();
  delay(WIFI_MULTI_1ST_CONNECT_WAITING_MS);

  while ((i++ < 20) && (status != WL_CONNECTED)) {
    status = WiFi.status();

    if (status == WL_CONNECTED)
      break;
    else
      delay(WIFI_MULTI_CONNECT_WAITING_MS);
  }

  if (status == WL_CONNECTED) {
    LOGERROR1(F("WiFi connected after time: "), i);
    LOGERROR3(F("SSID:"), WiFi.SSID(), F(",RSSI="), WiFi.RSSI());
    LOGERROR3(F("Channel:"), WiFi.channel(), F(",IP address:"), WiFi.localIP());
  } else {
    LOGERROR(F("WiFi not connected"));

#if ESP8266
    ESP.reset();
#else
    ESP.restart();
#endif
  }

  return status;
}

void check_WiFi() {
  if ((WiFi.status() != WL_CONNECTED)) {
    Serial.println(F("\nWiFi lost. Call connectMultiWiFi in loop"));
    connectMultiWiFi();
  }
}

void check_status() {
  static ulong checkstatus_timeout = 0;
  static ulong checkwifi_timeout = 0;

  static ulong current_millis;

#define WIFICHECK_INTERVAL 1000L

#if USE_ESP_WIFIMANAGER_NTP
#define HEARTBEAT_INTERVAL 60000L
#else
#define HEARTBEAT_INTERVAL 10000L
#endif

#define LED_INTERVAL 2000L

  current_millis = millis();

  // Check WiFi every WIFICHECK_INTERVAL (1) seconds.
  if ((current_millis > checkwifi_timeout) || (checkwifi_timeout == 0)) {
    check_WiFi();
    checkwifi_timeout = current_millis + WIFICHECK_INTERVAL;
  }

  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0)) {
    heartBeatPrint();
    checkstatus_timeout = current_millis + HEARTBEAT_INTERVAL;
  }
}



void initWebConfig() {
  Serial.print(F("\n-->[PORT] Starting captive portal "));
// Serial.setDebugOutput(false);
initAPIPConfigStruct(WM_AP_IPconfig);
initSTAIPConfigStruct(WM_STA_IPconfig);

#if USE_CUSTOM_AP_IP
  // set custom ip for portal
  // New in v1.4.0
  ESPAsync_wifiManager.setAPStaticIPConfig(WM_AP_IPconfig);
  //////
#endif

  // ESPAsync_wifiManager.setMinimumSignalQuality(-1);

  // Set config portal channel, default = 1. Use 0 => random channel from 1-13
  // ESPAsync_wifiManager.setConfigPortalChannel(0);
  //////

#if !USE_DHCP_IP
  // Set (static IP, Gateway, Subnetmask, DNS1 and DNS2) or (IP, Gateway,
  // Subnetmask). New in v1.0.5 New in v1.4.0
  ESPAsync_wifiManager.setSTAStaticIPConfig(WM_STA_IPconfig);
  //////
#endif

  // ESPAsync_wifiManager.setSaveConfigCallback(saveConfigCallback); 

  Router_SSID = ESPAsync_wifiManager.WiFi_SSID();
  Router_Pass = ESPAsync_wifiManager.WiFi_Pass();

  #ifndef WIFI_PRIVACY
  Serial.println("ESP Self-Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);
  #endif

  // SSID to uppercase
  // ssid.toUpperCase();

  bool configDataLoaded = loadConfigData();
  // bool configDataLoaded = false;

  ESPAsync_wifiManager.resetSettings();

  if (!configDataLoaded)
  {
    // From v1.1.0, Don't permit NULL password
    if ( (Router_SSID == "") || (Router_Pass == "") )
    {
      Serial.println(F("We haven't got any access point credentials, so get them now"));
  
      initialConfig = true;

      ESPAsync_wifiManager.autoConnect("AutoConnectAP", "password");
  
      // Starts an access point
      // if (!ESPAsync_wifiManager.startConfigPortal((const char *) ssid.c_str(), password))
      //   Serial.println(F("Not connected to WiFi but continuing anyway."));
      // else
      //   Serial.println(F("WiFi connected...yeey :)"));
  
      // memset(&WM_config, 0, sizeof(WM_config));
      
      // for (uint8_t i = 0; i < NUM_WIFI_CREDENTIALS; i++)
      // {
      //   String tempSSID = ESPAsync_wifiManager.getSSID(i);
      //   String tempPW   = ESPAsync_wifiManager.getPW(i);
    
      //   if (strlen(tempSSID.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1)
      //     strcpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str());
      //   else
      //     strncpy(WM_config.WiFi_Creds[i].wifi_ssid, tempSSID.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_ssid) - 1);
  
      //   if (strlen(tempPW.c_str()) < sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1)
      //     strcpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str());
      //   else
      //     strncpy(WM_config.WiFi_Creds[i].wifi_pw, tempPW.c_str(), sizeof(WM_config.WiFi_Creds[i].wifi_pw) - 1);  
  
      //   // Don't permit NULL SSID and password len < MIN_AP_PASSWORD_SIZE (8)
      //   if ( (String(WM_config.WiFi_Creds[i].wifi_ssid) != "") && (strlen(WM_config.WiFi_Creds[i].wifi_pw) >= MIN_AP_PASSWORD_SIZE) )
      //   {
      //     LOGERROR3(F("* Add SSID = "), WM_config.WiFi_Creds[i].wifi_ssid, F(", PW = "), WM_config.WiFi_Creds[i].wifi_pw );
      //     wifiMulti.addAP(WM_config.WiFi_Creds[i].wifi_ssid, WM_config.WiFi_Creds[i].wifi_pw);
      //   }
      // }
  
      // New in v1.4.0
      // ESPAsync_wifiManager.getSTAStaticIPConfig(WM_STA_IPconfig);
      //////
      
      // saveConfigData();
    }
    else
    {
      wifiMulti.addAP(Router_SSID.c_str(), Router_Pass.c_str());
    }
  }
}

void WebConfigLoop() {
  ESPAsync_wifiManager.loop();
  check_status();
}

void disableWiFi() {
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off
  Serial.println("-->[WiFi] WiFi dissabled!");
}

#endif  // SUPPORT_WEBCONFIG
#endif  // CO2_Gadget_WebConfig_h
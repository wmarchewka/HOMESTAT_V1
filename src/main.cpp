#define _TASK_TIMECRITICAL
#define _TASK_WDT_IDS
#define _TASK_PRIORITY
#define _TASK_STATUS_REQUEST
#define HTTP_WEBSERVER_PORT 80
#define DATASERVER_PORT 1504
#define DEBUG_ESP_OTA
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */

//#include "sensitive.h"
#include "include.h"
#include "build_defs.h"
#include "Free_Fonts.h"
#include "homestat_FS.h"
#include "ModbusIP_ESP8266.h"
#include <Adafruit_GFX.h>
#include <Adafruit_MCP23017.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <Esp.h>
#include <ESP8266FtpServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <Modbus.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <RemoteDebug.h>
#include <rom/rtc.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <TaskScheduler.h>
#include <time.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <Wire.h>

#ifdef ESP32_WROVER
#include "WROVER_KIT_LCD.h"
#include "esp_wrover_pins.h"
#elif ESP32_DEVKIT
#include "TFT_eSPI.h" // Include the graphics library (this includes the sprite functions)
#include "esp_devkit_pins.h"
#endif

//classes
ModbusIP mb;           //ModbusIP object
DHT dht;               //temperature humidity
Adafruit_MCP23017 mcp; //io expander

#ifdef ESP32_WROVER
  WROVER_KIT_LCD tft; //lcd
#elif ESP32_DEVKIT
  TFT_eSPI tft = TFT_eSPI(); // Create object "tft"
#endif

Scheduler runner;                         //task schedule
WebServer webServer(HTTP_WEBSERVER_PORT); //webserver
WebServer DataServer(DATASERVER_PORT);    //wifi server
RemoteDebug Debug;                        //telnet debug
WiFiManager wifiManager;                  //wifi manager
WiFiClient espClient;                     // wifi client for use withMQTT
PubSubClient MQTT_Client(espClient);      // MQTT client

String errorCodes[12] = {"0", "WIFI NO SSID AVAIL", "WIFI SCAN COMPLETED", "WIFI CONNECTED",
                         "WIFI CONNECT FAILED", "WIFI CONNECTION_LOST", "WIFI DISCONNECTED", "DHT TIMEOUT", "DHT CHECKSUM",
                         "TSTAT DETECT ERR", "WIFI IDLE STATUS"};

//eeprom settings memory location
 const int ES_SSID PROGMEM PROGMEM = 32;
 const int ES_SSIDPASSWORD PROGMEM = 32; //SIZE OF 32
 const int ES_RESETCOUNTER PROGMEM = 65; //SIZE OF 32
 const int ES_SSID_MD5 PROGMEM = 98;
 const int ES_SSIDPASS_MD5 PROGMEM = 102;

//Modbus Registers Offsets (0-9999)
 const int ANALOG_SENSOR_MB_HREG PROGMEM = 1;
 const int TEMPERATURE_SENSOR_MB_HREG PROGMEM = 2;
 const int HUMIDITY_SENSOR_MB_HREG PROGMEM = 3;
 const int THERMOSTAT_HEAT_CALL_PULSE_VALUE_MB_HREG PROGMEM = 4;
 const int THERMOSTAT_COOL_CALL_PULSE_VALUE_MB_HREG PROGMEM= 5;
 const int THERMOSTAT_FAN_CALL_PULSE_VALUE_MB_HREG PROGMEM = 6;
 const int DHT_STATUS_ERR_TIMEOUT_COUNTER_MB_HREG PROGMEM = 100;
 const int DHT_STATUS_ERR_CHECKSUM_COUNTER_MB_HREG PROGMEM = 101;
 const int DHT_STATUS_ERR_MB_HREG PROGMEM= 102;
 const int BLINK_ERROR_CODE_MB_HREG PROGMEM = 103;
 const int WIFI_STATUS_ERR_MB_HREG PROGMEM = 104;
 const int THERMOSTAT_STATUS_ERR_MB_HREG PROGMEM = 105;
 const int ESP_RESET_REASON_MB_HREG PROGMEM= 106;
 const int ESP_CHIP_ID_HIGH_MB_HREG PROGMEM = 107;
 const int ESP_CHIP_ID_LOW_MB_HREG PROGMEM = 108;
 const int ESP_MEMORY_MB_HREG PROGMEM = 109;
 const int WIFI_NOT_CONNECTED_MB_HREG PROGMEM= 110;
 const int DHT_ROUTINE_TIME_MB_HREG PROGMEM= 111;
 const int TIME_HH_MB_HREG PROGMEM= 112;
 const int TIME_MM_MB_HREG PROGMEM= 113;
 const int TIME_SS_MB_HREG PROGMEM= 114;
 const int GOOD_PACKET_COUNTER_MB_REG PROGMEM= 115;
 const int BAD_PACKET_COUNTER_MB_REG PROGMEM= 116;
 const int MB_ROUTINE_TIME_MB_HREG PROGMEM= 117;
 const int PROCESS_MODBUS_TIME_MB_HREG PROGMEM= 118;
 const int THERM_DETECT_ROUTINE_TIME_MB_HREG PROGMEM= 119;
 const int SCREEN_TIME_MB_HREG PROGMEM= 120;
 const int ESP_BOOT_DEVICE_MB_HREG PROGMEM= 121;
 const int ESP_RESET_COUNTER_MB_HREG PROGMEM= 122;
 const int MB_START_DELAY_COUNTER_MB_REG PROGMEM= 123;
 const int MB_OVERRUN_NEG_COUNTER_MB_REG PROGMEM= 124;
 const int MB_OVERRUN_POS_COUNTER_MB_REG PROGMEM= 125;
 const int NO_CLIENT_COUNTER_MB_REG PROGMEM= 126;
 const int NOT_MODBUS_PACKET_COUNTER_MB_REG PROGMEM= 127;
 const int LARGE_FRAME_COUNTER_MB_REG PROGMEM = 128;
 const int FAILED_WRITE_COUNTER_MB_REG PROGMEM= 129;
 const int NTP_LOOP_TIME_MB_HREG PROGMEM = 130;
 const int ESP_MEMORY_LOW_POINT PROGMEM= 131;

//modbus COILS
 const int HEAT_OVERRIDE_MB_COIL PROGMEM= 1;
 const int HEAT_CONTROL_MB_COIL PROGMEM= 2;
 const int COOL_OVERRIDE_MB_COIL PROGMEM= 3;
 const int COOL_CONTROL_MB_COIL PROGMEM= 4;
 const int FAN_OVERRIDE_MB_COIL PROGMEM= 5;
 const int FAN_CONTROL_MB_COIL PROGMEM= 6;
 const int THERMOSTAT_HEAT_CALL_MB_COIL PROGMEM= 7;
 const int THERMOSTAT_COOL_CALL_MB_COIL PROGMEM= 8;
 const int THERMOSTAT_FAN_CALL_MB_COIL PROGMEM= 9;
 const int THERMOSTAT_STATUS_MB_COIL PROGMEM= 10;
 const int ESP_RESTART_MB_COIL PROGMEM = 11;
 const int ESP_CLEAR_SAVECRASH_DATA PROGMEM = 12;

//pin mappping to io expander
 const int HEAT_OVERRIDE_PIN PROGMEM = 0;
 const int HEAT_CONTROL_PIN PROGMEM= 1;
 const int COOL_OVERRIDE_PIN PROGMEM= 2;
 const int COOL_CONTROL_PIN PROGMEM= 3;
 const int FAN_OVERRIDE_PIN PROGMEM= 4;
 const int FAN_CONTROL_PIN PROGMEM= 5;
 const int LED PROGMEM= 7;

//misc variables

//GLOBAL variables

 bool glb_coolcall = false;
 bool glb_DHT11debugOn = 1;
 bool glb_fancall = false;
 bool glb_heatcall= false;
 bool glb_logDataDebug = false;
 bool glb_OTA_Started= false;
 bool glb_tempController= false;
 bool glb_tstatDebugOn = false;
 char glb_lcdTime[20];
 char glb_SSID[32];
 char glb_SSIDpassword[32];
 const bool COIL_OFF PROGMEM = false;        // = false;
 const bool COIL_ON PROGMEM = true;         //= true;
 const char *glb_mdnsName = "HOMESTAT";
 const char *mqtt_server = "10.0.0.26";
 const unsigned int ts_coolcall= 5; // Field to write elapsed time data
 const unsigned int ts_fancall = 6;  // Field to write elapsed time data
 const unsigned int ts_heatcall= 4; // Field to write elapsed time data
 const unsigned int ts_humidity= 2; // Field to write temperature data
 const unsigned int ts_lightsensor = 3; // Field to write elapsed time data
 const unsigned int ts_temperature = 1; // Field to write temperature data
 const word glb_maxCoilSize PROGMEM= 256;
 const word glb_maxHregSize PROGMEM = 256;
 File fsUploadFile;
 File glb_errorLog;
 File glb_temperatureLog;
 float glb_VCC = 0.0;
 int glb_dataLogCount = 0;
 int glb_dataServerCounter= 0;
 int glb_heatRunTimeTotal = 0;
 int glb_humidity = 0;
 int glb_lightSensor = 0;
 int glb_lowMemory = 80000;
 int glb_TaskTimes[30] = {};
 int glb_temperature= 0;
 IPAddress glb_ipAddress;     //ip address
 long glb_wifiRSSI = 0;
 String glb_BootTime = "";
 String glb_dataLogPath = "/datalog.csv";
 String glb_debugLogPath = "/debuglog.txt";
 String glb_dhtStatusError= "";
 String glb_errorLogPath = "/errorlog.txt";
 String glb_systemLogPath = "/systemlog.txt";
 String glb_testLED = "";
 String glb_thermostatStatus = "";
 String glb_timeDay = "";
 String glb_timeHour = "";
 String glb_TimeLong= "";
 String glb_timeMin= "";
 String glb_timeMonth= "";
 String glb_timeSec = "";
 String glb_TimeShort = "";
 String glb_timeWeekDay = "";
 String glb_timeYear= "";
 uint32_t glb_freeHeap= 0;
 uint32_t glb_resetCounter= 0;
 uint64_t chipid;
 volatile word glb_coolPulseCounter= 0;
 volatile word glb_coolPulseDuration = 0;
 volatile word glb_fanPulseCounter = 0;
 volatile word glb_fanPulseDuration = 0;
 volatile word glb_heatPulseCounter = 0;
 volatile word glb_heatPulseDuration = 0;
 word glb_BlinkErrorCode = 1;
 word glb_eepromHregCopy[100] = {};
 word glb_errorDHT = 0;
 word glb_errorThermostat = 0;
 word glb_wifiNotConnectedCounter = 0;
 word glb_WiFiStatus= 0;

Task taskModbusReadData_1(30, TASK_FOREVER, &Modbus_ReadData, NULL);
Task taskDHT11Temp_2(2000, TASK_FOREVER, &DHT11_TempHumidity, NULL);
Task taskTimeRoutine_3(1000, TASK_FOREVER, &TimeRoutine, NULL);
Task taskLED_Error_4(15000, TASK_FOREVER, &LED_Error, NULL);
Task taskWifiCheckStatus_5(1000, TASK_FOREVER, &Wifi_CheckStatus, NULL);
Task taskThermostatDetect_6(1000, TASK_FOREVER, &Thermostat_Detect, NULL);
Task taskIoControlPins_7(100, TASK_FOREVER, &IO_ControlPins, NULL);
Task taskTelnet_8(20, TASK_FOREVER, &TelnetServer_Process, NULL);
Task taskLED_onEnable_9(4000, TASK_ONCE, NULL, NULL, false, &LED_OnEnable, &LED_OnDisable);
Task taskLED_OnDisable_10(TASK_IMMEDIATE, TASK_FOREVER, NULL, NULL, true, NULL, &LED_Off);
Task taskErrorsCodesProcess_11(500, TASK_FOREVER, &ErrorCodes_Process, NULL);
Task taskModbusProcess_12(50, TASK_FOREVER, &Modbus_Process, NULL);
//Task taskEEpromProcess_13(1000, TASK_FOREVER, &EEPROM_Process, NULL);
Task taskMBcoilReg11_14(TASK_IMMEDIATE, TASK_ONCE, NULL, NULL, false, NULL, &ESP_Restart);
Task taskWebServer_Process_15(50, TASK_FOREVER, &WebServer_Process, NULL);
Task taskDataServer_Process_16(10, TASK_FOREVER, &DataServer_Process, NULL);
Task taskModbusClientSend_17(5000, TASK_FOREVER, &Modbus_Client_Send, NULL);
Task taskOTA_Update_18(100, TASK_FOREVER, &OTA_Update, NULL);
Task taskLogDataSave_19(60000, TASK_FOREVER, &FileSystem_DataLogSave, NULL);
Task taskLCDUpdate_20(500, TASK_FOREVER, &LCD_Update, NULL);
Task taskMQTTRUN_21(50, TASK_FOREVER, &MQTT_RunLoop, NULL);
Task taskMQTTUpdate_22(1000, TASK_FOREVER, &MQTT_Publish, NULL);
//************************************************************************************
void testVCC()
{
  pinMode(TEST_PIN, OUTPUT);
  digitalWrite(TEST_PIN, HIGH);
  delay(20);
  glb_VCC = analogRead(A0);
  glb_VCC = glb_VCC / 1000.0;
  Serial.println(glb_VCC);
  pinMode(TEST_PIN, INPUT);
}
//************************************************************************************
void MQTT_RunLoop()
{
  MQTT_Client.loop();
}
//************************************************************************************
void MQTT_Publish()
{
  Serial.println(F("ROUTINE_MQTT_Publish"));

  char msg[50];
  String message = "WALT";
  glb_TimeLong.toCharArray(msg, 50);
  bool debug = 1;
  if (debug)
    Serial.println(" Sending values to MQTT server...");
  if (!MQTT_Client.connected())
  {
    Serial.println("  MQTT reconnecting");
    MQTT_Reconnect();
  }
  else
  {
    MQTT_Client.publish("outTopic", msg);
  }
}
//************************************************************************************
void MQTT_Setup()
{
  Serial.println(F("ROUTINE_MQTT_Setup"));

  Serial.println("  Setting up MQTT client...");
  MQTT_Client.setServer(mqtt_server, 1883);
  MQTT_Client.setCallback(MQTT_Callback);
}
//************************************************************************************
void MQTT_Reconnect()
{
  Serial.println(F("ROUTINE_MQTT_Reconnect"));

  // Loop until we're reconnected
  //while (!MQTT_Client.connected())
  //{
  Serial.println("  Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (MQTT_Client.connect(clientId.c_str()))
  {
    Serial.println("connected");
    // Once connected, publish an announcement...
    MQTT_Client.publish("outTopic", "hello world");
    // ... and resubscribe
    MQTT_Client.subscribe("inTopic");
  }
  else
  {
    Serial.print("failed, rc = ");
    Serial.println(MQTT_Client.state());
    //Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    //delay(5000);
  }
  //}
}
//************************************************************************************
void MQTT_Callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    //digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    Serial.println("************************************Received 1 from MQTT");
  }
  else
  {
    //digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
    Serial.println("************************************Received 0 from MQTT");
  }
}
//************************************************************************************
void TelnetServer_Process()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  Debug.handle();
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[8] = endTimeMicros;
}
//************************************************************************************
void TelnetServer_ProcessCommand()
{
  char tmpglb_SSID[32];
  char tmpglb_SSIDpassword[32];
  char tmpLastCmd[32];

  String lastCmd = Debug.getLastCommand();

  if (lastCmd == (F("file format")))
  {
    if (Debug.isActive(Debug.ANY))
    {
      SPIFFS.format();
      Debug.print(F("File system formatted..."));
    }
  }
  else if (lastCmd == (F("post data")))
  {
    //postData();
    Debug.println("ok");
  }
  else if (lastCmd == (F("file list")))
  {
    FileSystem_ListDirectory();
  }
  else if (lastCmd == (F("temperature")))
  {
    Debug.println(glb_temperature);
  }
  else if (lastCmd == (F("humidity")))
  {
    Debug.println(glb_humidity);
  }
  else if (lastCmd == (F("light sensor")))
  {
    Debug.println(glb_lightSensor);
  }
  else if (lastCmd == (F("time")))
  {
    Debug.println(glb_TimeLong);
  }
  else if (lastCmd.startsWith((F("hreg"))))
  {
    String tReg = lastCmd.substring(5, lastCmd.length());
    int iReg = tReg.toInt();
    Debug.println(mb.Hreg(iReg));
  }
  else if (lastCmd.startsWith((F("coil"))))
  {
    String tReg = lastCmd.substring(5, lastCmd.length());
    int iReg = tReg.toInt();
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith((F("set coil"))))
  {
    String tReg = lastCmd.substring(9, lastCmd.length());
    int iReg = tReg.toInt();
    mb.Coil(iReg, 1);
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith((F("clr coil"))))
  {
    String tReg = lastCmd.substring(9, lastCmd.length());
    int iReg = tReg.toInt();
    mb.Coil(iReg, 0);
    Debug.println(mb.Coil(iReg));
  }
  else if (lastCmd.startsWith((F("set hreg"))))
  {
    String tReg = lastCmd.substring(9, 12);
    int iReg = tReg.toInt();
    int firstSpace = lastCmd.indexOf(" ", 9);
    String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    int iVal = tVal.toInt();
    mb.Hreg(iReg, iVal);
    Debug.println(mb.Hreg(iReg));
  }
  else if (lastCmd.startsWith((F("set esp pin"))))
  {
    String tReg = lastCmd.substring(12, 13);
    int iReg = tReg.toInt();
    //int firstSpace = lastCmd.indexOf(" ", 12);
    //String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    String tVal = lastCmd.substring(15, 15);
    int iVal = tVal.toInt();
    Debug.print(iReg);
    Debug.print(":");
    Debug.print(iVal);
    testEspOutputPin(iReg, iVal);
  }
  else if (lastCmd.startsWith((F("set mcp pin"))))
  {
    String tReg = lastCmd.substring(12, 14);
    Debug.println(lastCmd);
    Debug.println(tReg);
    int iReg = tReg.toInt();
    //int firstSpace = lastCmd.indexOf(" ", 1);
    //String tVal = lastCmd.substring(firstSpace + 1, lastCmd.length());
    String tVal = lastCmd.substring(lastCmd.length() - 1, lastCmd.length());
    Debug.println(tVal);
    int iVal = tVal.toInt();
    Debug.print(iReg);
    Debug.print(":");
    Debug.print(iVal);
    testMcpOutputPin(iReg, iVal);
  }

  else if (lastCmd.startsWith("task enable"))
  {
    String tReg = lastCmd.substring(12, 2);
    int iReg = tReg.toInt();
    if (iReg == 1)
      taskModbusReadData_1.enable();
    if (iReg == 2)
      taskDHT11Temp_2.enable();
    if (iReg == 3)
      taskTimeRoutine_3.enable();
    if (iReg == 4)
      taskLED_Error_4.enable();
    if (iReg == 5)
      taskWifiCheckStatus_5.enable();
    if (iReg == 6)
      taskThermostatDetect_6.enable();
    if (iReg == 7)
      taskIoControlPins_7.enable();
    if (iReg == 8)
      taskTelnet_8.enable();
    if (iReg == 9)
      taskLED_onEnable_9.enable();
    if (iReg == 10)
      taskLED_OnDisable_10.enable();
    if (iReg == 11)
      taskErrorsCodesProcess_11.enable();
    if (iReg == 12)
      taskModbusProcess_12.enable();
    if (iReg == 13)
      //taskEEpromProcess_13.enable();
    if (iReg == 14)
      taskMBcoilReg11_14.enable();
    if (iReg == 15)
      taskWebServer_Process_15.enable();
    if (iReg == 16)
      taskDataServer_Process_16.enable();
    if (iReg == 17)
      taskModbusClientSend_17.enable();
    if (iReg == 18)
      taskOTA_Update_18.enable();
    if (iReg == 19)
      taskLogDataSave_19.enable();
    Debug.println(F(("ok")));
  }
  else if (lastCmd.startsWith("task disable"))
  {
    String tReg = lastCmd.substring(13, 2);
    int iReg = tReg.toInt();
    if (iReg == 1)
      taskModbusReadData_1.disable();
    if (iReg == 2)
      taskDHT11Temp_2.disable();
    if (iReg == 3)
      taskTimeRoutine_3.disable();
    if (iReg == 4)
      taskLED_Error_4.disable();
    if (iReg == 5)
      taskWifiCheckStatus_5.disable();
    if (iReg == 6)
      taskThermostatDetect_6.disable();
    if (iReg == 7)
      taskIoControlPins_7.disable();
    if (iReg == 8)
      taskTelnet_8.disable();
    if (iReg == 9)
      taskLED_onEnable_9.disable();
    if (iReg == 10)
      taskLED_OnDisable_10.disable();
    if (iReg == 11)
      taskErrorsCodesProcess_11.disable();
    if (iReg == 12)
      taskModbusProcess_12.disable();
    if (iReg == 13)
      //taskEEpromProcess_13.disable();
    if (iReg == 14)
      taskMBcoilReg11_14.disable();
    if (iReg == 15)
      taskWebServer_Process_15.disable();
    if (iReg == 16)
      taskDataServer_Process_16.disable();
    if (iReg == 17)
      taskModbusClientSend_17.disable();
    if (iReg == 18)
      taskOTA_Update_18.disable();
    if (iReg == 19)
      taskLogDataSave_19.disable();
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("wifi ssid"))
  {
    //TODO GET SSID FROM PREFERENCES;
    Debug.println(tmpglb_SSID);
  }
  else if (lastCmd.startsWith("set wifi ssid"))
  {
    String tReg = lastCmd.substring(14);
    tReg.toCharArray(tmpLastCmd, tReg.length() + 1);
    Debug.print(F("Old SSID:"));
    //TODO GET SSID FROM PREFERENCES;
    //EEPROM.get(glb_eepromSettingsOffset + ES_SSID, tmpglb_SSID);
    Debug.println(tmpglb_SSID);
    Debug.print(F("New SSID:"));
    Debug.println(tmpLastCmd);
    //TODO CHANGE TO PREFERENCES
    //EEPROM.put(glb_eepromSettingsOffset + ES_SSID, tmpLastCmd);
    //EEPROM.commit();
    Debug.println(F("Will not take affect until restart..."));
  }
  else if (lastCmd.startsWith("wifi pass"))
  {
    //EEPROM.get(glb_eepromSettingsOffset + ES_SSIDPASSWORD, glb_SSIDpassword);
    //TODO GET WIFI PASS FROM PREFERENCES
  }
  else if (lastCmd.startsWith("set wifi pass"))
  {
    String tReg = lastCmd.substring(14);
    tReg.toCharArray(tmpLastCmd, tReg.length() + 1);
    Debug.print(F("Old SSID pass:"));
    //GET FROM PREFERENCES
    //Debug.println(EEPROM.get(glb_eepromSettingsOffset + ES_SSIDPASSWORD, tmpglb_SSIDpassword));
    Debug.print(F("New SSID pass:"));
    Debug.println(tmpLastCmd);
    //TODO CHANGE TO PREFERENCES
    //EEPROM.put(glb_eepromSettingsOffset + ES_SSIDPASSWORD, tmpLastCmd);
    EEPROM.commit();
    Debug.println(F("Will not take affect until restart..."));
  }
  else if (lastCmd.startsWith("wifi scan"))
  {
    int numberOfNetworks = WiFi.scanNetworks();
    for (int i = 0; i < numberOfNetworks; i++)
    {
      Debug.print(F("Network name: "));
      Debug.println(WiFi.SSID(i));
      Debug.print(F("Signal strength: "));
      Debug.println(WiFi.RSSI(i));
      Debug.println(F("-----------------------"));
    }
  }
  else if (lastCmd.startsWith("wifi rssi"))
  {
    Debug.println(WiFi.RSSI());
  }
  else if (lastCmd.startsWith("wifi reset settings"))
  {
    wifiManager.resetSettings();
    Debug.println(WiFi.RSSI());
  }
  else if (lastCmd.startsWith("wifi config"))
  {
    wifiManager.startConfigPortal("HOMESTAT");
    Debug.println(F("wifi config"));
  }
  // else if (lastCmd.startsWith("savecrash clear"))
  // {
  //   SaveCrash.clear();
  //   Debug.println("ok");
  // }
  // else if (lastCmd.startsWith("savecrash print"))
  // {
  //   SaveCrash.print(Debug);
  // }
  // else if (lastCmd.startsWith("savecrash count"))
  // {
  //   Debug.println(SaveCrash.count());
  // }
  else if (lastCmd.startsWith("eeprom erase"))
  {
    //EEPROM_Erase();
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("reset reason"))
  {
    //Debug.println(ESP.getResetReason());
    verbose_print_reset_reason(rtc_get_reset_reason(0));
    verbose_print_reset_reason(rtc_get_reset_reason(1));
  }
  else if (lastCmd.startsWith("free sketch size"))
  {
    //Debug.println(ESP.getSketchSize());
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("log size"))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("tstat status"))
  {
    Debug.println(glb_thermostatStatus);
  }
  else if (lastCmd.startsWith("tstat debug on"))
  {
    glb_tstatDebugOn = true;
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("tstat debug off"))
  {
    glb_tstatDebugOn = false;
    Debug.println("ok");
  }
  else if (lastCmd.startsWith("reset count"))
  {
    Debug.println(glb_resetCounter);
  }
  else if (lastCmd.startsWith("reset clear"))
  {
    glb_resetCounter = 0;
    EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
    EEPROM.commit();
    mb.Hreg(ESP_RESET_COUNTER_MB_HREG, (word)glb_resetCounter);
    Debug.println(glb_resetCounter);
  }
  else if (lastCmd.startsWith("wifi resets"))
  {
    Debug.println(glb_wifiNotConnectedCounter);
  }
  else if (lastCmd.startsWith("debug serial on"))
  {
    Debug.setSerialEnabled(true); // All messages too send to serial too, and can be see in serial monitor
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("debug serial off"))
  {
    Debug.setSerialEnabled(false); // All messages too send to serial too, and can be see in serial monitor
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("dht debug on"))
  {
    glb_DHT11debugOn = true;
    Debug.println(glb_DHT11debugOn);
  }
  else if (lastCmd.startsWith("dht debug off"))
  {
    glb_DHT11debugOn = false;
    Debug.println(glb_DHT11debugOn);
  }
  else if (lastCmd.startsWith("datalog debug on"))
  {
    glb_logDataDebug = true;
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("log debug off"))
  {
    glb_logDataDebug = false;
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("datalog data on"))
  {
    taskLogDataSave_19.enable();
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("datalog data off"))
  {
    taskLogDataSave_19.disable();
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("datalog size"))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("datalog delete"))
  {
    FileSystem_DeleteFile(glb_dataLogPath);
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("file print"))
  {
    String tReg = lastCmd.substring(11, lastCmd.length());
    tReg = "/" + tReg;
    //Debug.println(tReg);
    bool val = FileSystem_PrintFile(tReg, true);
    if (!val)
    {
      Debug.println(F("File not found..."));
    }
  }
  else if (lastCmd.startsWith("errorlog size"))
  {
    File f = SPIFFS.open(glb_errorLogPath, "r");
    Debug.println(f.size());
    f.close();
  }
  else if (lastCmd.startsWith("reboot"))
  {
    Debug.println("Rebooting...");
    ESP_Restart();
  }
  else if (lastCmd.startsWith("errorlog delete"))
  {
    FileSystem_DeleteFile(glb_errorLogPath);
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("dataserver count"))
  {
    Debug.println(glb_dataServerCounter);
  }
  else if (lastCmd.startsWith("debugdata save"))
  {
    FileSystem_DebugDataSave("");
    Debug.println(F("ok"));
  }
  else if (lastCmd.startsWith("boot time"))
  {
    Debug.println(glb_BootTime);
  }
  else if (lastCmd.startsWith("task times"))
  {
    for (int i = 1; i < 21; i++)
    {
      Debug.print(i);
      Debug.print(":");
      Debug.println(glb_TaskTimes[i]);
    }
  }
  else if (lastCmd.startsWith("file delete"))
  {
    String tReg = lastCmd.substring(12, lastCmd.length());
    tReg = "/" + tReg;
    Debug.println(tReg);
    bool val = FileSystem_DeleteFile(tReg);
    if (val)
    {
      Debug.println(F("ok"));
    }
    else
    {
      Debug.println(F("File not found..."));
    }
  }
  else if (lastCmd.startsWith("list"))
  {
    Debug.println(F("file format"));
    Debug.println(F("file list"));
    Debug.println(F("file print xxxx.xxx"));
    Debug.println(F("file delete xxxxx.xxx"));
    Debug.println(F("temperature"));
    Debug.println(F("humidity"));
    Debug.println(F("light sensor"));
    Debug.println(F("time"));
    Debug.println(F("hreg xxx"));
    Debug.println(F("set hreg xxx"));
    Debug.println(F("coil xxx"));
    Debug.println(F("set coil xxx"));
    Debug.println(F("clr coil xxx"));
    Debug.println(F("task enable xx"));
    Debug.println(F("task disable xx"));
    Debug.println(F("wifi ssid"));
    Debug.println(F("set wifi ssid"));
    Debug.println(F("wifi pass"));
    Debug.println(F("set wifi pass"));
    Debug.println(F("wifi scan"));
    Debug.println(F("wifi rssi"));
    Debug.println(F("wifi reset settings"));
    Debug.println(F("wifi config"));
    Debug.println(F("wifi resets"));
    // Debug.println(F("savecrash clear"));
    // Debug.println(F("savecrash print"));
    // Debug.println(F("savecrash count"));
    Debug.println(F("eeprom erase"));
    Debug.println(F("reset reason"));
    Debug.println(F("reset count"));
    Debug.println(F("reset clear"));
    Debug.println(F("free sketch size"));
    Debug.println(F("tstat status"));
    Debug.println(F("debug serial on"));
    Debug.println(F("debug serial off"));
    Debug.println(F("datalog debug on"));
    Debug.println(F("datalog debug off"));
    Debug.println(F("datalog data on"));
    Debug.println(F("datalog data off"));
    Debug.println(F("datalog delete"));
    Debug.println(F("datalog size"));
    Debug.println(F("dht debug on"));
    Debug.println(F("dht debug off"));
    Debug.println(F("errorlog delete"));
    Debug.println(F("errorlog size"));
    Debug.println(F("dataserver count"));
    Debug.println(F("boot time"));
    Debug.println(F("set esp pin xx y"));
    Debug.println(F("set mcp pin xx y"));
    Debug.println(F("task times"));
    Debug.println(F("debugdata save"));
    Debug.println(F("reboot"));
  }
}

//************************************************************************************
void Modbus_ReadData()
{
  bool debug = 0;
  if (debug)
    Debug.println(F("Modbus Routine"));

  if (glb_WiFiStatus == WL_CONNECTED)
  {

    word returnValue = 0;
    int startTimeMicros = micros();
    int endTimeMicros = 0;

    static word taskMbStartDelayCounter = 0;
    static word taskMbOverrunPosCounter = 0;
    static word taskMbOverrunNegCounter = 0;
    static word noClientCounter = 0;
    static word goodPacketCounter = 0;
    static word badPacketCounter = 0;
    static word notModbusPacketCounter = 0;
    static word largeFrameCounter = 0;
    static word failedWriteCounter = 0;

    returnValue = mb.task();

    if (debug)
    {
      if (returnValue != 1)
        Debug.println(returnValue);
    }

    if (taskModbusReadData_1.getStartDelay() > 0)
    {
      taskMbStartDelayCounter++;
      mb.Hreg(MB_START_DELAY_COUNTER_MB_REG, (word)taskMbStartDelayCounter);
    }

    if (taskModbusReadData_1.getOverrun() < 0)
    {
      taskMbOverrunNegCounter++;
      mb.Hreg(MB_OVERRUN_NEG_COUNTER_MB_REG, (word)taskMbOverrunNegCounter);
    }

    if (taskModbusReadData_1.getOverrun() > 0)
    {
      taskMbOverrunPosCounter++;
      mb.Hreg(MB_OVERRUN_POS_COUNTER_MB_REG, (word)taskMbOverrunPosCounter);
    }

    if (returnValue == 0)
    { //NO_CLIENT
      noClientCounter++;
      mb.Hreg(NO_CLIENT_COUNTER_MB_REG, (word)noClientCounter);
    }
    if (returnValue == 1)
    { //GOOD_PACKET_COUNTER
      goodPacketCounter++;
      mb.Hreg(GOOD_PACKET_COUNTER_MB_REG, (word)goodPacketCounter);
    }
    if (returnValue == 2)
    { //BAD_PACKET_COUNTER
      badPacketCounter++;
      mb.Hreg(BAD_PACKET_COUNTER_MB_REG, (word)badPacketCounter);
    }
    if (returnValue == 3)
    { //NOT_MODBUS_PKT
      notModbusPacketCounter++;
      mb.Hreg(NOT_MODBUS_PACKET_COUNTER_MB_REG, (word)notModbusPacketCounter);
    }
    if (returnValue == 4)
    { //LARGE_FRAME
      largeFrameCounter++;
      mb.Hreg(LARGE_FRAME_COUNTER_MB_REG, (word)largeFrameCounter);
    }
    if (returnValue == 5)
    { //FAILED_WRITE
      failedWriteCounter++;
      mb.Hreg(FAILED_WRITE_COUNTER_MB_REG, (word)failedWriteCounter);
    }

    endTimeMicros = micros();
    endTimeMicros = endTimeMicros - startTimeMicros;
    mb.Hreg(MB_ROUTINE_TIME_MB_HREG, int((word)endTimeMicros));
    glb_TaskTimes[1] = endTimeMicros;
  }
}
//************************************************************************************
void TimeRoutine()
{

  Serial.println("ROUTINE_TimeRoutine");

  if (glb_WiFiStatus == WL_CONNECTED)
  {
    bool debug = 0;
    int startTimeMicros = micros();
    int static secondsCounter = 0;
    unsigned long start = millis();
    unsigned long timeout = 1000;
    static bool firstRun = true;
    struct tm *timeinfo;
    time_t now;

    if (firstRun)
      //daylight savings
      configTime(-4 * 3600, 0, "pool.ntp.org");

    secondsCounter++;

    if (firstRun)
      timeout = 5000;

    if (debug)
      Serial.print(F("  secondsCounter:"));
    if (debug)
      Serial.println(secondsCounter);
    if (debug)
      Serial.print(F("  firstRun:"));
    if (debug)
      Serial.println(firstRun);

    if (secondsCounter >= 300 || (firstRun))
    {
      Serial.println(F("  Waiting for time from time sync"));
      while (millis() < (timeout + start))
      {
        now = time(nullptr);
        if (now != 0)
        {
          if (firstRun)
          {
            Serial.println(F("  First run waiting for time from time sync..."));
            firstRun = false;
            glb_BootTime = ctime(&now);
            String tmp = (F("Boot time : "));
            FileSystem_ErrorLogSave(tmp + glb_BootTime);
            Serial.print(F("  Boot time : "));
            Serial.print(glb_BootTime);
          }
          secondsCounter = 0;
          Serial.print("  Got time from timeserver : ");
          Serial.print(ctime(&now));
          break;
        }
        delay(10);
      }
    }

    time(&now);
    timeinfo = localtime(&now);
    glb_TimeLong = ctime(&now);
    glb_TimeLong.trim();
    glb_timeHour = String(timeinfo->tm_hour);
    glb_timeMin = String(timeinfo->tm_min);
    glb_timeSec = String(timeinfo->tm_sec);
    glb_timeMonth = String(timeinfo->tm_mon);
    glb_timeDay = String(timeinfo->tm_mday);
    glb_timeYear = String((timeinfo->tm_year) - 100);
    glb_timeWeekDay = String(timeinfo->tm_wday);
    sprintf(glb_lcdTime, "%02d:%02d:%02d", glb_timeHour.toInt(), glb_timeMin.toInt(), glb_timeSec.toInt());
    mb.Hreg(TIME_HH_MB_HREG, glb_timeHour.toInt());
    mb.Hreg(TIME_MM_MB_HREG, glb_timeMin.toInt());
    mb.Hreg(TIME_SS_MB_HREG, glb_timeSec.toInt());
    int endTimeMicros = micros();
    endTimeMicros = endTimeMicros - startTimeMicros;
    mb.Hreg(NTP_LOOP_TIME_MB_HREG, (word)endTimeMicros);
    glb_TaskTimes[3] = endTimeMicros;
    if (endTimeMicros > 100000)
    {
      if (debug)
        Serial.println(endTimeMicros);
    }
  }
}
//************************************************************************************
void ESP_Restart()
{
  Serial.println("ROUTINE_ESP_Restart");
  bool debug = 1;
  if (debug)
    Serial.println(F("  Restarting ESP"));
  FileSystem_ErrorLogSave(F(" Restart command received..."));
  delay(0);
  ESP.restart();
}
//************************************************************************************
void Modbus_Process()
{
  bool debug = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  
  //TODO: CHANGE TO PREFERENCES
  /*
  
  if (debug)
    Serial.println(F("Processing Modbus..."));

  for (int x = 1; x <= glb_maxCoilSize - 1; x++)
  {

 
    if (glb_eepromCoilCopy[x] != mb.Coil(x))
    {
      glb_eepromCoilCopy[x] = mb.Coil(x);
      if (debug)
        Serial.print(F("mb coil:"));
      if (debug)
        Serial.print(x);
      if (debug)
        Serial.print(F(" value:"));
      if (debug)
        Serial.print(mb.Coil(x));
      if (debug)
        Serial.print(F(" eepromcopy:"));
      if (debug)
        Serial.println(glb_eepromCoilCopy[x]);
      if (debug)
        Serial.println(x);

      if (x == ESP_RESTART_MB_COIL)
      {
        if (debug)
          Serial.println(F("case 11"));
        //mb.Coil(ESP_RESTART_MB_COIL, COIL_OFF);
        //glb_eepromCoilCopy[ESP_RESTART_MB_COIL] = int(mb.Coil(x));
        Serial.print(F("Rebooting unit in 3 seconds..."));
        taskMBcoilReg11_14.enableDelayed(3000);
      }

      if (x == ESP_CLEAR_SAVECRASH_DATA)
      {
        if (debug)
        {
          //Serial.println(F("case 12"));
          //mb.Coil(ESP_CLEAR_SAVECRASH_DATA, COIL_OFF);
          //glb_eepromCoilCopy[ESP_RESTART_MB_COIL] = int(mb.Coil(x));
          //Serial.println(F("Clearing SAVECRASH data..."));
          //SaveCrash.clear();
        }
      }
    }
    else
    {
      glb_eepromCoilCopy[x] = mb.Coil(x);
    }
  }

  for (int x = 1; x <= glb_maxHregSize - 1; x++)
  {
    glb_eepromHregCopy[x] = mb.Hreg(x);
  }

  glb_freeHeap = ESP.getFreeHeap();
  mb.Hreg(ESP_MEMORY_MB_HREG, (word)glb_freeHeap);

  if (glb_freeHeap < glb_lowMemory)
  {
    glb_lowMemory = glb_freeHeap;
    mb.Hreg(ESP_MEMORY_LOW_POINT, (word)(glb_lowMemory));
  }
  */

  endTimeMicros = micros();
  mb.Hreg(PROCESS_MODBUS_TIME_MB_HREG, (word)(endTimeMicros));
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[12] = endTimeMicros;
}
//************************************************************************************
void ErrorCodes_Process()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  if (debug)
  {
    Serial.println("ROUTINE_ErrorCodes_Process");
    Debug.println(F(" Processing Error Codes..."));
  }

  if (debug)
    Serial.print(F("  DHT error code:"));
  if (debug)
    Serial.println(glb_errorDHT);
  if (debug)
    Serial.print(F("  Wifi error code:"));
  if (debug)
    Serial.println(glb_WiFiStatus);
  if (debug)
    Serial.print(F("  Thermostat error code:"));
  if (debug)
    Serial.println(glb_errorThermostat);

  if (glb_errorDHT != 0)
  {
    glb_BlinkErrorCode = glb_errorDHT;
  }
  else if (glb_errorThermostat != 0)
  {
    glb_BlinkErrorCode = glb_errorThermostat;
  }
  else if (glb_WiFiStatus != 0)
  {
    if (debug)
      Serial.println(glb_WiFiStatus);
    glb_BlinkErrorCode = glb_WiFiStatus;
  }

  mb.Hreg(BLINK_ERROR_CODE_MB_HREG, (word)glb_BlinkErrorCode);
  if (debug)
    Serial.print(F("  Blink Error Code:"));
  if (debug)
    Serial.println(glb_BlinkErrorCode);

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[11] = endTimeMicros;
}
//************************************************************************************
void LED_Error()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  bool debug = 0;
  if (debug)
    Serial.println(F("  Wrapper callback"));
  taskLED_onEnable_9.restartDelayed();
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[4] = endTimeMicros;
}
//************************************************************************************
// Upon being enabled, taskLED_onEnable_9 will define the parameters
// and enable LED blinking task, which actually controls
// the hardware (LED in this example)
bool LED_OnEnable()
{

  int interval = 250;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  bool debug = 0;
  taskLED_OnDisable_10.setInterval(interval);
  taskLED_OnDisable_10.setCallback(&LED_On);
  taskLED_OnDisable_10.enable();
  taskLED_onEnable_9.setInterval((glb_BlinkErrorCode * interval * 2) - 100);
  if (debug)
    Serial.print(F("  Blink Error Code:"));
  if (debug)
    Serial.println(glb_BlinkErrorCode);
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[9] = endTimeMicros;
  return true; // Task should be enabled
}
//************************************************************************************
// taskLED_onEnable_9 does not really need a callback method
// since it just waits for 5 seconds for the first
// and only iteration to occur. Once the iteration
// takes place, taskLED_onEnable_9 is disabled by the Scheduler,
// thus executing its OnDisable method below.
void LED_OnDisable()
{
  bool debug = 0;
  if (debug)
    Serial.println(F("Blink on disable"));
  taskLED_OnDisable_10.disable();
}
//************************************************************************************
void LED_On()
{
  bool debug = 0;
  if (debug)
    Serial.println(F("LED on"));
  mcp.digitalWrite(LED, HIGH);
  taskLED_OnDisable_10.setCallback(&LED_Off);
}
//************************************************************************************
void LED_Off()
{
  bool debug = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  if (debug)
    Serial.println(F("LED off"));
  mcp.digitalWrite(LED, LOW);
  taskLED_OnDisable_10.setCallback(&LED_On);
  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[10] = endTimeMicros;
}
//************************************************************************************
void Thermostat_Detect()
{

  Serial.println("ROUTINE_Thermostat_Detect");
  bool debug = glb_tstatDebugOn;
  if (debug)
    Serial.println(F("Processing thermostat detect..."));

  int startTimeMicros = micros();
  int endTimeMicros;
  word localval = 0;
  glb_thermostatStatus = (F("No call"));
  static bool hs;
  static bool cs;
  static bool fs;

  //test
  //glb_heatPulseDuration = 1000;
  //glb_heatPulseCounter = 100;

  mb.Hreg(THERMOSTAT_HEAT_CALL_PULSE_VALUE_MB_HREG, (word)glb_heatPulseDuration);
  mb.Hreg(THERMOSTAT_COOL_CALL_PULSE_VALUE_MB_HREG, (word)glb_coolPulseDuration);
  mb.Hreg(THERMOSTAT_FAN_CALL_PULSE_VALUE_MB_HREG, (word)glb_fanPulseDuration);

  if (debug)
    Serial.print(F("HP:"));
  if (debug)
    Serial.print(glb_heatPulseDuration);
  if (debug)
    Serial.print(F(" CP:"));
  if (debug)
    Serial.print(glb_coolPulseDuration);
  if (debug)
    Serial.print(F(" FP:"));
  if (debug)
    Serial.print(glb_fanPulseDuration);

  if (debug)
    Serial.print(F(" HPC:"));
  if (debug)
    Serial.print(glb_heatPulseCounter);
  if (debug)
    Serial.print(F(" CPC:"));
  if (debug)
    Serial.print(glb_coolPulseCounter);
  if (debug)
    Serial.print(F(" FPC:"));
  if (debug)
    Serial.print(glb_fanPulseCounter);
  if (debug)
    Serial.print(F(" BEC:"));
  if (debug)
    Serial.print(glb_BlinkErrorCode);

  if (glb_heatPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL, COIL_ON);
    glb_heatRunTimeTotal++;
    if (hs == false)
      FileSystem_SystemDataSave(F("HEAT ON"));
    glb_heatcall = true;
    hs = true;
  }
  else
  {
    mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL, COIL_OFF);
    if (hs == true)
      FileSystem_SystemDataSave(F("HEAT OFF"));
    glb_heatcall = false;
    hs = false;
  }
  glb_heatPulseCounter = 0;

  if (glb_coolPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL, COIL_ON);
    if (cs == false)
      FileSystem_SystemDataSave(F("COOL ON"));
    glb_coolcall = true;
    cs = true;
  }
  else
  {
    mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL, COIL_OFF);
    if (cs == true)
      FileSystem_SystemDataSave(F("COOL OFF"));
    glb_coolcall = false;
    cs = false;
  }
  glb_coolPulseCounter = 0;

  if (glb_fanPulseCounter >= 30)
  {
    mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL, COIL_ON);
    if (fs == false)
      FileSystem_SystemDataSave(F("FAN ON"));
    glb_fancall = true;
    fs = true;
  }
  else
  {
    mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL, COIL_OFF);
    if (fs == true)
      FileSystem_SystemDataSave(F("FAN OFF"));
    glb_fancall = false;
    fs = false;
  }
  glb_fanPulseCounter = 0;

  if (mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL))
  {
    glb_thermostatStatus = (F("Heat call..."));
  }
  if (mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL))
  {
    glb_thermostatStatus = (F("Cool call..."));
    FileSystem_SystemDataSave(F("Cool call"));
  }
  if (mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL))
  {
    glb_thermostatStatus = (F("Cool call..."));
    FileSystem_SystemDataSave(F("Fan call"));
  }

  //check more than one call. if so then error
  bool pulseCheck = mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL) && mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL) && mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL);
  if (debug)
    Serial.print(F(" EC:"));
  if (debug)
    Serial.println(pulseCheck);
  if (pulseCheck)
  {
    glb_thermostatStatus = (F("Error"));
    FileSystem_ErrorLogSave(F("Thermostat Error..."));
    glb_errorThermostat = 9;
    localval = 1;
  }
  if (debug)
  {
    Serial.print("Thermostat Heat Call Coil : ");
    Serial.println(mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL));
  }

  if (debug)
  {
    Serial.print("Thermostat Cool Call Coil : ");
    Serial.println(mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL));
  }

  if (debug)
  {
    Serial.print("Thermostat Fan Call Coil : ");
    Serial.println(mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL));
  }
  if (pulseCheck == 1)
  {
    //glb_thermostatStatus = "No error";
    glb_errorThermostat = 0;
    localval = 0;
  }

  mb.Hreg(THERMOSTAT_STATUS_ERR_MB_HREG, (word)localval);
  if (debug)
    Serial.print(F("ThermoStatus:"));
  if (debug)
    Serial.println(glb_thermostatStatus);
  if (debug)
    Serial.print(F("Time:"));

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  if (debug)
    Serial.println(endTimeMicros);
  mb.Hreg(THERM_DETECT_ROUTINE_TIME_MB_HREG, (word)(endTimeMicros));
  glb_TaskTimes[6] = endTimeMicros;
}

//************************************************************************************
void Wifi_CheckStatus()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  static bool displayed = true;
  static word wifiNotConnected = 0;
  glb_WiFiStatus = WiFi.status();
  if (debug)
    Serial.print(F("WiFI status:"));
  if (debug)
    Serial.print(glb_WiFiStatus);
  if (debug)
    Serial.print(F(":"));
  if (debug)
    Serial.println(errorCodes[glb_WiFiStatus]);
  if (debug)
    Serial.print(F("displayed:"));
  if (debug)
    Serial.print(displayed);
  if (debug)
    Serial.print(F(":wifiNotConnected Counter:"));
  if (debug)
    Serial.println(wifiNotConnected);

  if (glb_WiFiStatus != WL_CONNECTED)
  {
    wifiNotConnected++;
    displayed = false;
    if (debug)
      Serial.print(F("Disconnect counter:"));
    if (debug)
      Serial.println(wifiNotConnected);
    Debug.println(F("Wifi disconnected. Reconnecting..."));
    if (wifiNotConnected >= 1)
    {
      Serial.println(F("Resetting WiFi..."));
      WiFi.begin(glb_SSID, glb_SSIDpassword);
      if (wifiNotConnected == 5)
        FileSystem_ErrorLogSave(F("Resetting WiFi..."));
    }
    if (wifiNotConnected >= 10)
    {
      Serial.setDebugOutput(true);
    }

    if (wifiNotConnected >= 20)
    {
      wifiNotConnected = 0;
      Serial.println(F("Wifi Failed to connect! Rebooting..."));
      FileSystem_ErrorLogSave(F("Wifi Failed to connect! Rebooting..."));
      delay(1000);
      ESP.restart();
    }
  }

  if (glb_WiFiStatus == 3)
  {
    if (!displayed)
    {
      Serial.println(F("Wifi Reconnected !"));
      FileSystem_ErrorLogSave(F("Wifi Reconnected"));
      glb_wifiRSSI = WiFi.RSSI();
      FileSystem_ErrorLogSave("Wifi RSSI:" + String(glb_wifiRSSI));
      glb_ipAddress = WiFi.localIP();
      displayed = true;
      wifiNotConnected = 0;
      glb_wifiNotConnectedCounter++;
      Serial.setDebugOutput(false);
      mb.Hreg(WIFI_NOT_CONNECTED_MB_HREG, (word)glb_wifiNotConnectedCounter);
      //check to see if servers and clients are still running?
      webServer.begin();
    }
  }

  if (glb_WiFiStatus == 0)
    glb_WiFiStatus = 10;
  mb.Hreg(WIFI_STATUS_ERR_MB_HREG, (word)glb_WiFiStatus);
  endTimeMicros = micros();
  endTimeMicros = micros() - startTimeMicros;
  glb_TaskTimes[5] = endTimeMicros;
}
//************************************************************************************
void DHT11_TempHumidity()
{
  Serial.println("ROUTINE_DHT11_TempHumidity");
  if (glb_DHT11debugOn)
    Debug.println(F(" Processing temperature and humity sensor..."));
  static int err1 = 0;
  static int err2 = 0;
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  int elaspedTimeMicros = 0;
  int tmpTemperature = 0;
  int tmpHumidity = 0;
  String tmpStatus = "";

  tmpHumidity = dht.getHumidity();
  tmpTemperature = dht.getTemperature();
  tmpTemperature = dht.toFahrenheit(tmpTemperature);
  tmpStatus = dht.getStatusString();

  if (glb_DHT11debugOn)
    Debug.println(tmpStatus);

  if (tmpStatus == (F("OK")))
  {
    glb_temperature = tmpTemperature;
    glb_humidity = tmpHumidity;
    glb_dhtStatusError = tmpStatus;

    mb.Hreg(TEMPERATURE_SENSOR_MB_HREG, (word)(glb_humidity));
    mb.Hreg(HUMIDITY_SENSOR_MB_HREG, (word)(glb_temperature));
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 0);
    glb_errorDHT = 0;
  }

  if (tmpStatus == (F("TIMEOUT")))
  {
    FileSystem_ErrorLogSave(F("DHT 11 timeout error..."));
    glb_errorDHT = 7;
    err1++;
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 1);
  }
  mb.Hreg(DHT_STATUS_ERR_TIMEOUT_COUNTER_MB_HREG, (word)err1);

  if (tmpStatus == (F("ERROR")))
  {
    FileSystem_ErrorLogSave(F("DHT 11 Checksum Error..."));
    glb_errorDHT = 8;
    err2++;
    mb.Hreg(DHT_STATUS_ERR_MB_HREG, 2);
  }

  mb.Hreg(DHT_STATUS_ERR_CHECKSUM_COUNTER_MB_HREG, (word)err2);
  if (glb_DHT11debugOn)
    Debug.println(glb_dhtStatusError);

  endTimeMicros = micros();
  elaspedTimeMicros = endTimeMicros - startTimeMicros;
  mb.Hreg(DHT_ROUTINE_TIME_MB_HREG, (word)(elaspedTimeMicros));
  glb_TaskTimes[2] = elaspedTimeMicros;
}

//************************************************************************************
void loop()
{
  runner.execute();
  yield();
}
//************************************************************************************
int BootDevice_Detect(void)
{
  Serial.println("ROUTINE_BootDevice_Detect");
  int bootmode;
  asm(
      "movi %0, 0x60000200\n\t"
      "l32i %0, %0, 0x118\n\t"
      : "+r"(bootmode) /* Output */
      :                /* Inputs (none) */
      : "memory"       /* Clobbered */
  );
  return ((bootmode >> 0x10) & 0x7);
}
//************************************************************************************
void DataServer_Setup()
{
  Serial.println("ROUTINE_DataSetrver_Setup");
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  DataServer.begin();

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[16] = endTimeMicros;
}
//************************************************************************************
void DataServer_Process()
{
  Serial.println("ROUTINE_DataSetrver_Process");
  /*
  bool debug = 0;
  int szHreg = sizeof(glb_eepromHregCopy);
  int szCoil = sizeof(glb_eepromCoilCopy);
  uint8_t rbuf[] = {};
  byte testDataMore[szCoil + szHreg + 1];

  memcpy(testDataMore, glb_eepromHregCopy, szHreg);
  memcpy(testDataMore + szHreg, glb_eepromCoilCopy, szCoil);
  if (debug)
    Serial.println(szHreg);
  if (debug)
    Serial.println(szCoil);

  for (int lc = 0; lc < 100; lc++)
  {
    WiFiClient dataClient = DataServer.available();
    if (dataClient)
    {
      if (dataClient.available())
      {
        dataClient.read(rbuf, dataClient.available());
        if (debug)
          Serial.println(*rbuf);
        if (debug)
          Serial.print(F("webData Connection from: "));
        if (debug)
          Serial.println(dataClient.remoteIP());
      }
      if (dataClient.connected())
      {
        dataClient.write(testDataMore, sizeof(testDataMore));
        glb_dataServerCounter++;
        dataClient.stop();
        break;
      }
    }
    delayMicroseconds(100);
  }
  */
}
//************************************************************************************
void DeepSleepMode()
{
  Serial.println("ROUTINE_DeepSleepMode");
  Serial.println(F("ESP8266 in sleep mode"));
  ESP.deepSleep(10 * 1000000);
}
//************************************************************************************
void Modbus_Client_Send()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  if (glb_WiFiStatus == WL_CONNECTED)
  {

    WiFiClient modbusClient;
    Serial.println(F("Sending"));
    modbusClient.connect("10.0.0.11", 5502);
    modbusClient.print(glb_temperature);
    modbusClient.print(glb_humidity);

    endTimeMicros = micros();
    endTimeMicros = endTimeMicros - startTimeMicros;
    glb_TaskTimes[17] = endTimeMicros;
  }
}
//************************************************************************************
void WebServer_Process()
{
  if (glb_WiFiStatus == WL_CONNECTED)
  {
    webServer.handleClient();
  }
}
//************************************************************************************
void WebServer_HandleRoot()
{
  if (webServer.hasArg("LED"))
  {
    handleSubmit();
  }
  else
  {
    webServer.send(200, (F("text/html")), WebServer_getPage());
  }
}
//************************************************************************************
void handleSubmit()
{
  Serial.println("ROUTINE_handleSubmit");

  // Actualise le GPIO / Update GPIO
  bool debug = 0;
  String LEDValue;
  LEDValue = webServer.arg("LED");
  if (LEDValue == "1")
  {
    //digitalWrite(LEDPIN, 1);
    glb_testLED = (F("On"));
    webServer.send(200, (F("text/html")), WebServer_getPage());
  }
  else if (LEDValue == "0")
  {
    //digitalWrite(LEDPIN, 0);
    glb_testLED = (F("Off"));
    webServer.send(200, (F("text/html")), WebServer_getPage());
  }
  else
  {
    if (debug)
      Serial.println(F("Err Led Value"));
  }
}
//************************************************************************************
String WebServer_getPage()
{

  String page = (F("<html lang=en><head><meta http-equiv='refresh' content='2'/>"));
  page += (F("<title>Home Stat</title>"));
  page += (F("<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>"));
  page += (F("</head><body><h1>ESP8266 Demo Main</h1>"));
  page += (F("<p><a href=/info>Information</a>&emsp;&emsp;"));
  page += (F("<a href=/datalog?action=download>DataLog Download</a>&emsp;&emsp;"));
  page += (F("<a href=/datalog?action=view>DataLog View</a>&emsp;&emsp;"));
  page += (F("<a href=/errorlog?action=download>Errorlog Download</a>&emsp;&emsp;"));
  page += (F("<a href=/errorlog?action=view>Errorlog View</a>"));
  page += (F("<h3>LIVE DATA</h3>"));
  page += (F("<ul><li>Light Sensor : "));
  page += glb_lightSensor;
  page += (F("</li>"));
  page += (F("<li>Temperature : "));
  page += glb_temperature;
  page += (F("<li>Wifi RSSI : "));
  page += WiFi.RSSI();
  page += (F("</li>"));
  page += (F("<li>Thermostat : "));
  page += glb_thermostatStatus;
  page += (F("</li>"));
  page += (F("<li>Humidity : "));
  page += glb_humidity;
  page += (F("%</li></ul><h3>TIME</h3>"));
  page += (F("<ul><li>Time : "));
  page += glb_TimeLong;
  page += (F("</li></ul>"));
  page += (F("<h3>GPIO</h3>"));
  page += (F("<form action='/' method='POST'>"));
  page += (F("<ul><li>D3 (value: "));
  page += glb_testLED;
  page += (F(")"));
  page += (F("<INPUT type='radio' name='LED' value='1'>ON"));
  page += (F("<INPUT type='radio' name='LED' value='0'>OFF</li></ul>"));
  page += (F("<INPUT type='submit' value='Submit'>"));
  //page += "<br><br><p><a hrf='https://diyprojects.io'>diyprojects.io</p>";
  page += (F("</body></html>"));
  return page;
}
//************************************************************************************
void WebServer_HandleErrorLog()
{
  bool debug = 0;
  if (debug)
    Serial.println(F("Error data log starting"));

  int wsargs = webServer.args();
  String action = webServer.arg(0);
  if (debug)
    Serial.print(webServer.argName(0));
  if (debug)
    Serial.print(" Args = ");
  Serial.println(action);

  String filename = "errorlog" + glb_timeMonth + "_" + glb_timeDay + "_" + glb_timeYear + "_";
  filename = filename + glb_timeHour + "_" + glb_timeMin + "_" + glb_timeSec + ".csv";

  String page = (F("<html lang=en><head>"));
  page += (F("<title>Home Stat</title>"));
  page += (F("<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>"));
  page += (F("</head><body><h1>ESP8266 Demo ErrorLog</h1>"));
  page += (F("<p><a href=/>Home</a>&emsp;&emsp;"));
  page += (F("<a href=/info>Information</a>&emsp;&emsp;"));
  page += (F("<a href=/datalog?action=download>DataLog Download</a>&emsp;&emsp;"));
  page += (F("<a href=/datalog?action=view>DataLog View</a><br><br><pre>"));

  if (SPIFFS.exists(glb_dataLogPath))
  {
    File f = SPIFFS.open(glb_errorLogPath, "r");
    if (debug)
      Serial.println(f.size());
    if (f.size() > 0)
    {
      int siz = f.size();
      webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
      if (action == "view")
        webServer.send(200, (F("text/html")), page);
      if (action == "download")
        webServer.sendHeader((F("Content-Disposition")), "attachment; filename=" + filename);
      if (action == "download")
        webServer.streamFile(f, (F("application/octet-stream")));
      if (action == "view")
      {
        while (f.available())
        {
          String tmp = f.readStringUntil('\n');
          webServer.sendContent(tmp);
        }
        webServer.sendContent("");
      }
    }
    else
    {
      webServer.send(200, (F("text/plain")), (F("File Empty")));
      if (debug)
        Serial.println(F(("File not found error...")));
    }
    f.close();
    if (debug)
      Serial.println(F("Closing file"));
  }
}
//************************************************************************************
void WebServer_HandleDatalogUpload()
{
  webServer.serveStatic("/", SPIFFS, "/index.htm");
}
//************************************************************************************
void handleFileCreate()
{
  Serial.println("ROUTINE_handleFileCreate");
  if (webServer.args() == 0)
  {
    return webServer.send(500, (F("text/plain")), (F("BAD ARGS")));
  }
  String path = webServer.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/")
  {
    return webServer.send(500, (F("text/plain")), (F("BAD PATH")));
  }
  if (SPIFFS.exists(path))
  {
    return webServer.send(500, (F("text/plain")), (F("FILE EXISTS")));
  }
  File file = SPIFFS.open(path, "w");
  if (file)
  {
    file.close();
  }
  else
  {
    return webServer.send(500, (F("text/plain")), (F("CREATE FAILED")));
  }
  webServer.send(200, (F("text/plain")), "");
  path = String();
}
//************************************************************************************
void WebServer_HandleFileUpload()
{
  if (webServer.uri() != "/upload")
  {
    return;
  }
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
    {
      filename = "/" + filename;
    }
    Serial.print(F("handleFileUpload Name: "));
    Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile)
    {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {
      fsUploadFile.close();
    }
    Serial.print(F("handleFileUpload Size: "));
    Serial.println(upload.totalSize);
  }
}
//************************************************************************************
void WebServer_HandleTest()
{
  ;
  //webServer.send_P(200, "text/html", testpage);
  // String page = "<!DOCTYPE html>";
  // page += "<html>";
  // page += "<body>";
  // page += "<canvas id='myCanvas'>Your browser does not support the HTML5 canvas tag.</canvas>";
  // page += "<script>";
  // page += "var c = document.getElementById('myCanvas');";
  // page += "var ctx = c.getContext('2d');";
  // page += "ctx.fillStyle = '#FF0000';";
  // page += "ctx.fillRect(0, 0, 80, 100);";
  // page += "</script>";
  // page += "<p><strong>Note:</strong> The canvas tag is not supported in Internet";
  // page += "Explorer 8 and earlier versions.</p>";
  // page += "</body>";
  // page += "</html>";
  // webServer.send(200, (F("text/html")), page);
}
//************************************************************************************
void WebServer_HandleDataLog()
{
  bool debug = 0;
  if (debug)
    Serial.println(F("web data log starting"));

  int wsargs = webServer.args();
  String action = webServer.arg(0);
  if (debug)
    Serial.print(webServer.argName(0));
  if (debug)
    Serial.print(" Args = ");
  Serial.println(action);

  String page = (F("<html lang=en><head>"));
  page += (F("<title>Home Stat</title>"));
  page += (F("<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>"));
  page += (F("</head><body><h1>ESP8266 Demo Datalog</h1>"));
  page += (F("<p><a href=/>Home</a>&emsp;&emsp;"));
  page += (F("<a href=/info>Information</a>&emsp;&emsp;"));
  page += (F("<a href=/errorlog?action=download>Errorlog Download</a>&emsp;&emsp;"));
  page += (F("<a href=/errorlog?action=view>Errorlog View</a><br><br><pre>"));

  String filename = "datalog" + glb_timeMonth + "_" + glb_timeDay + "_" + glb_timeYear + "_";
  filename = filename + glb_timeHour + "_" + glb_timeMin + "_" + glb_timeSec + ".csv";

  if (SPIFFS.exists(glb_dataLogPath))
  {
    File f = SPIFFS.open(glb_dataLogPath, "r");
    if (debug)
      Serial.println(f.size());
    if (f.size() > 0)
    {
      int siz = f.size();
      webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
      if (action == "view")
        webServer.send(200, (F("text/html")), page);
      if (action == "download")
        webServer.sendHeader((F("Content-Disposition")), "attachment; filename=" + filename);
      if (action == "download")
        webServer.streamFile(f, (F("application/octet-stream")));
      if (action == "view")
      {
        while (f.available())
        {
          String tmp = f.readStringUntil('\n');
          webServer.sendContent(tmp);
        }
      }
    }
    else
    {
      webServer.send(200, (F("text/plain")), (F("File Empty")));
      if (debug)
        Serial.println(F(("File not found error...")));
    }
    f.close();
    if (debug)
      Serial.println(F("Closing file"));
  }
}
//************************************************************************************
String xmlResponse()
{
  String res = ""; // String to assemble XML response values
  res += "<?xml version = \"1.0\" ?>\n";
  res += "<lightsensor>";
  res += String(glb_lightSensor);
  res += "</lightsensor>\n";

  return res;
}
//************************************************************************************
void WebServer_HandleInformation()
{
  bool debug = 1;
  if (debug)
    Serial.println(F("Handle webserver information"));

  String page = (F("<html lang=en><head>"));
  page += (F("<title>Home Stat</title>"));
  page += (F("<style> body { background-color: #fffff; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>"));
  page += (F("</head><body><h1>ESP8266 Demo Informatio</h1>"));
  page += (F("<p><a href=/>Home</a>&emsp;&emsp;"));
  page += "<a href=/datalog?action=download>DataLog Download</a>&emsp;&emsp;";
  page += "<a href=/datalog?action=view>DataLog View</a>&emsp;&emsp;";
  page += "<a href=/errorlog?action=download>Errorlog Download</a>&emsp;&emsp;";
  page += "<a href=/errorlog?action=view>Errorlog View</a><br><br><p><pre>";

  FileSystem_DebugDataSave(page);

  if (SPIFFS.exists(glb_debugLogPath))
  {
    File f = SPIFFS.open(glb_debugLogPath, "r");
    int siz = f.size();
    webServer.setContentLength(siz);
    webServer.streamFile(f, "text/html");
    f.close();
    if (debug)
      Serial.println(F("Closing file"));
    page = "";
    page += (F("</body>"));
    webServer.send(200, (F("text/html")), page);
  }
  else
  {
    webServer.send(200, (F("text/plain")), (F("File Empty")));
    if (debug)
      Serial.println(F("File not found error..."));
  }
}
//************************************************************************************
void WebServer_HandleNotFound()
{
  String message = (F("File Not Found\n\n"));
  message += (F("URI: "));
  message += webServer.uri();
  message += (F("\nMethod: "));
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += (F("\nArguments: "));
  message += webServer.args();
  message += (F("\n"));
  for (uint8_t i = 0; i < webServer.args(); i++)
  {
    message += " " + webServer.argName(i) + ": " + webServer.arg(i) + "\n";
  }
  webServer.send(404, (F("text/plain")), message);
}
//************************************************************************************
u_int getFreeHeap()
{
  Serial.print(F("  Free RAM : "));
  glb_freeHeap = ESP.getFreeHeap();
  Serial.println(glb_freeHeap);
  return glb_freeHeap;
}
//************************************************************************************
void setup()
{
  //SET UP AS CONTROLLER OR SENSOR
  glb_tempController = true;
  Serial.begin(115200);
  Serial.println("ROUTINE_setup");

  #ifdef ESP32_WROVER
    Serial.println("Board defined as Espressif ESP - WROVER - KIT");
  #elif ESP32_DEVKIT
  Serial.println("Board defined as Espressif ESP - DEVKIT");
  #endif

    Serial.println();
    delay(1000);
    FileSystem_DeleteFile(glb_errorLogPath);
    Serial.println(getFreeHeap());
    //FileSystem_Format();
    FileSystem_ErrorLogCreate();
    ThermostatMode_Setup();
    WiFiManager_Setup();
    if (glb_tempController)
      TimeSync_Setup();
    TelnetServer_Setup();
    if (glb_tempController)
      LCD_Setup();
    if (glb_tempController)
      Modbus_Registers_Create();
    if (glb_tempController)
      FileSystem_SystemLogCreate();
    FileSystem_DataLogCreate();
    if (glb_tempController)
      FileSystem_DebugLogCreate();
    if (glb_tempController)
      I2C_Setup();
    if (glb_tempController)
      IO_Pins_Setup();
    if (glb_tempController)
      selftestMcp();
    DHT11_Sensor_Setup();
    OTA_Setup();
    if (glb_tempController)
      Modbus_Registers_Setup();
    if (glb_tempController)
      Thermostat_ControlDisable();
    TaskScheduler_Setup();
    MQTT_Setup();
    if (glb_tempController)
      DataServer_Setup();
    if (glb_tempController)
      WebServer_Setup();
    StartupPrinting_Setup();
    Tasks_Enable_Setup();
    mDNS_Setup();
}
//************************************************************************************
void ThermostatMode_Setup()
{
  Serial.println("ROUTINE_ThermostatMode_Setup");

  if (glb_tempController)
  {
    Serial.println(F("  Booting ESP8266 as Temperature Controller"));
    FileSystem_ErrorLogSave(F(" Booting ESP8266 as Temperature Controller"));
  }
  else
  {
    Serial.println(F("  Booting ESP8266 as Temperature sensor"));
    FileSystem_ErrorLogSave(F("   Booting ESP8266  as Temperature sensor"));
  }
}
//************************************************************************************
void mDNS_Setup()
{
  Serial.println(F("ROUTINE_mDNS_Setup"));
  if (!MDNS.begin(glb_mdnsName))
  {
    Serial.println(F("  Error setting up MDNS responder!"));
  }
  else
  {
    Serial.println("  Started mDNS service....");
    MDNS.addService("http", "tcp", 80);
  }
}
//************************************************************************************
void Modbus_Registers_Create()
{
  //create modbus registers and copy contents from eeprom
  Serial.println(F("ROUTINE_Modbus_Registers_Create"));
  Serial.print(F("  Creating Modbus Holding Registers Max size : "));
  Serial.println(glb_maxHregSize);
  word wordTmp;

  for (int x = 1; x <= glb_maxHregSize; x++)
  {
    mb.addHreg(x);
    //wordTmp = EEPROM.read(x);
    wordTmp = x;
    mb.Hreg(x, wordTmp);
    glb_eepromHregCopy[x] = x;
  }

  //create coil registers and copy contents from eepromCopy
  Serial.print(F("  Creating Modbus Coil Registers Max size : "));
  Serial.println(glb_maxCoilSize);
  /* for (int x = 1; x <= glb_maxCoilSize; x++)
  {
    mb.addCoil(x);
    //tmp = EEPROM.read(x);
    mb.Coil(x, COIL_OFF);
    glb_eepromCoilCopy[x] = COIL_OFF;
  } */
}
//************************************************************************************
void LCD_Setup()
{
 
  #ifdef ESP32_WROVER
  Serial.println(F("WROVER_LCD_Setup"));
  tft.begin();
  tft.setRotation(2);
  uint8_t x = 0;
  uint32_t id = tft.readId();
  if (id)
  {
    Serial.println("======= WROVER ST7789V Display Test ========");
  }
  else
  {
    Serial.println("======= WROVER ILI9341 Display Test ========");
  }
  Serial.println("============================================");
  Serial.printf("Display ID:      0x%06X\n", id);

  x = tft.readcommand8(WROVER_RDDST);
  Serial.print("Status:          0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(WROVER_RDDPM);
  Serial.print("Power Mode:      0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(WROVER_RDDMADCTL);
  Serial.print("MADCTL Mode:     0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(WROVER_RDDCOLMOD);
  Serial.print("Pixel Format:    0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(WROVER_RDDIM);
  Serial.print("Image Format:    0x");
  Serial.println(x, HEX);
  x = tft.readcommand8(WROVER_RDDSDR);
  Serial.print("Self Diagnostic: 0x");
  Serial.println(x, HEX);

  tft.fillScreen(WROVER_BLACK);
  #elif ESP32_DEVKIT
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  #endif
}
//************************************************************************************
void LCD_Update()
{
  int startTimeMicros = micros();
  int endTimeMicros = 0;
  uint16_t textcolor = 0;
  uint16_t backgroundcolor = 0;

#ifdef ESP32_WROVER
  textcolor = WROVER_WHITE;
  backgroundcolor = WROVER_BLACK;
#elif ESP32_DEVKIT
  textcolor = TFT_WHITE;
  backgroundcolor = TFT_BLACK;
#endif

  char line1[30] = "";
  char line2[30] = "";
  char line3[30] = "";
  char line4[30] = "";
  char line5[30] = "";
  char line6[30] = "";
  char line7[30] = "";

  sprintf(line1, "Temp    :%02d", glb_temperature);
  sprintf(line2, "Humidity:%02d", glb_humidity);
  sprintf(line3, "Pkts    :%06d", glb_dataServerCounter);
  sprintf(line4, "Status  :%s", glb_dhtStatusError.c_str());
  sprintf(line5, "Time    :%s", glb_lcdTime);
  sprintf(line6, "Free mem:%07d", ESP.getFreeHeap());
  sprintf(line7, "IP Addr :%s", glb_ipAddress.toString().c_str());

  LCD_DrawText(0, 0, line1, textcolor, backgroundcolor);
  LCD_DrawText(0, 10, line2, textcolor, backgroundcolor);
  LCD_DrawText(0, 20, line3, textcolor, backgroundcolor);
  LCD_DrawText(0, 30, line4, textcolor, backgroundcolor);
  LCD_DrawText(0, 40, line5, textcolor, backgroundcolor);
  LCD_DrawText(0, 50, line6, textcolor, backgroundcolor);
  LCD_DrawText(0, 60, line7, textcolor, backgroundcolor);

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[20] = endTimeMicros;
}
//************************************************************************************
void LCD_DrawText(int wid, int hei, char *text, uint16_t textcolor, uint16_t backcolor)
{

  int startTimeMicros = micros();
  int endTimeMicros;

  bool debug = 0;

  if (debug)
    Debug.print(F("Text color:"));
  if (debug)
    Debug.println(textcolor);
  if (debug)
    Debug.print(F("Backcolor:"));
  if (debug)
    Debug.println(backcolor);

  if (debug)
    Debug.print(F("Display:"));
  if (debug)
    //Debug.println(text);
    tft.setTextColor(textcolor, backcolor);
  tft.setTextWrap(false);
  tft.setTextSize(1);
  tft.setCursor(wid, hei);
  tft.print(text);

  endTimeMicros = micros();
  endTimeMicros = micros() - startTimeMicros;
  mb.Hreg(SCREEN_TIME_MB_HREG, (word)(endTimeMicros));
}
//************************************************************************************
void Interrupt_Detect_AC()
{
  bool debug = 0;
  static unsigned long endDetectMicros;
  unsigned long startDetectMicros = micros();
  glb_heatPulseDuration = (startDetectMicros - endDetectMicros);
  if (glb_heatPulseDuration > 10)
  {
    glb_heatPulseCounter++;
    //glb_fanPulseCounter++;
    //glb_coolPulseCounter++;
    if (debug)
      Serial.println(glb_heatPulseDuration);
  }

  endDetectMicros = startDetectMicros;
}
void StartupPrinting_Setup()
{
  Serial.println(F("ROUTINE_StartupPrinting_Setup"));

  Serial.print(F("  MAC : "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("  Version : "));
  Serial.println(TimestampedVersion);
  Serial.print(F("  Sketch size : "));
  Serial.println();
  //Serial.println(ESP.getSketchSize());
  Serial.print(F("  Free sketch size : "));
  Serial.println();
  //Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("  Core version : "));
  Serial.println();
  //Serial.println(ESP.getCoreVersion());
  Serial.print(F("  Code MD5 : "));
  Serial.println();
  //Serial.println(ESP.getSketchMD5());
  Serial.print(F("  CPU Frequency : "));
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(F("mhz"));
  Serial.print(F("  Free RAM : "));
  glb_freeHeap = ESP.getFreeHeap();
  Serial.println(glb_freeHeap);
  Reset_Reason();
  ChipID_Acquire();

  int getBD = BootDevice_Detect();
  if (getBD == 1)
  {
    Serial.println(F("  This sketch has just been uploaded over the UART."));
  }
  if (getBD == 3)
  {
    Serial.println(F("  This sketch started from FLASH."));
  }
  mb.Hreg(ESP_BOOT_DEVICE_MB_HREG, (word)getBD);
}
//************************************************************************************
void Modbus_CreateTCP()
{
  mb.config();
}
//************************************************************************************
void I2C_Setup()
{
  Serial.println(F("ROUTINE_I2C_Setup"));

  #ifdef ESP32_WROVER
  // i2c mode
  // used to override clock and data pins
  //mcp.begin(0, I2C_DATA_PIN, I2C_CLOCK_PIN);
  #elif ESP32_DEVKIT
  mcp.begin(0, I2C_DATA_PIN, I2C_CLOCK_PIN);
  //mcp.begin(0);
  Serial.println("here");
  #endif
}
//************************************************************************************
void IO_Pins_Setup()
{
  Serial.println(F("ROUTINE_IO_Pins_Setup"));

  //mcp pin io setup
  mcp.pinMode(HEAT_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(HEAT_CONTROL_PIN, OUTPUT);
  mcp.pinMode(FAN_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(FAN_CONTROL_PIN, OUTPUT);
  mcp.pinMode(COOL_OVERRIDE_PIN, OUTPUT);
  mcp.pinMode(COOL_CONTROL_PIN, OUTPUT);
  mcp.pinMode(LED, OUTPUT);

  //esp pin io setup
  pinMode(THERMOSTAT_HEAT_CALL_PIN, INPUT);
  //pinMode(THERMOSTAT_COOL_CALL_PIN, INPUT);
  //pinMode(THERMOSTAT_FAN_CALL_PIN, INPUT);
  pinMode(TEST_PIN, OUTPUT);

  //INTERRUPTS
  attachInterrupt(digitalPinToInterrupt(THERMOSTAT_HEAT_CALL_PIN), Interrupt_Detect_AC, RISING);
}
//************************************************************************************
void IO_ControlPins()
{

  int startTimeMicros = micros();
  int endTimeMicros = 0;

  bool debug = 0;
  static int counter;
  counter++;

  if (counter == 10)
  {
    Serial.println(F("ROUTINE_IO_ControlPins"));
    counter = 0;
  }
  //read analog to get light sensor value
  glb_lightSensor = analogRead(A0);
  mb.Hreg(ANALOG_SENSOR_MB_HREG, (word)glb_lightSensor);

  //Serial.print("light Sensor Value - ");
  //Serial.println(glb_lightSensor);

  //do digital writes

  mcp.digitalWrite(FAN_OVERRIDE_PIN, (bool)(mb.Coil(FAN_OVERRIDE_MB_COIL)));
  delay(10);
  mcp.digitalWrite(FAN_CONTROL_PIN, (bool)(mb.Coil(FAN_CONTROL_MB_COIL)));
  delay(10);
  mcp.digitalWrite(HEAT_OVERRIDE_PIN, (bool)(mb.Coil(HEAT_OVERRIDE_MB_COIL)));
  delay(10);
  mcp.digitalWrite(HEAT_CONTROL_PIN, (bool)(mb.Coil(HEAT_CONTROL_MB_COIL)));
  delay(10);
  mcp.digitalWrite(COOL_OVERRIDE_PIN, (bool)(mb.Coil(COOL_OVERRIDE_MB_COIL)));
  delay(10);
  mcp.digitalWrite(COOL_CONTROL_PIN, (bool)(mb.Coil(COOL_CONTROL_MB_COIL)));

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[7] = endTimeMicros;
}
//************************************************************************************
void DHT11_Sensor_Setup()
{
  Serial.println(F("ROUTINE_DHT11_Sensor_Setup"));
  dht.setup(DHT11_DATA_PIN);
}
//************************************************************************************
void OTA_Setup()
{
  Serial.println("ROUTINE_OTA_Setup");

  uint16_t textcolor = 0;
  uint16_t backgroundcolor = 0;

  #ifdef ESP32_WROVER
    #define TEXTCOLOR WROVER_WHITE
    #define BACKGROUNDCOLOR WROVER_BLACK
  #elif ESP32_DEVKIT
    #define TEXTCOLOR TFT_WHITE
    #define BACKGROUNDCOLOR TFT_BLACK
  #endif
  
  ArduinoOTA.onStart([]() {
    String type;
    glb_OTA_Started = true;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    DataServer.stop(); //stop dataserver
    webServer.stop();  //stop WebServer_Root
    mb.stop();         //stop modbus server
    Debug.stop();      //stop debug telnet server

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    if (type == "filesystem")
      SPIFFS.end();

    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update complete");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    //LCD_DrawText(0, 80, "OTA Update  :     " + String(progress / (total / 100)) + "%", TEXTCOLOR, BACKGROUNDCOLOR);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR)
      Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR)
      Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR)
      Serial.println(F("End Failed"));
  });

  Serial.println(F("  Initializing OTA update routines..."));
  ArduinoOTA.begin();
  Serial.println(F("  OTA Update Ready.."));
}
//************************************************************************************
void OTA_Update()
{
  bool debug = 0;

  if (debug)
    Serial.println("ROUTINE_OTA_Update");
  int startTimeMicros = micros();
  int endTimeMicros = 0;

  ArduinoOTA.handle();

  endTimeMicros = micros();
  endTimeMicros = endTimeMicros - startTimeMicros;
  glb_TaskTimes[18] = endTimeMicros;
}
//************************************************************************************
void Tasks_Enable_Setup()
{
  Serial.println(F("ROUTINE_Tasks_Enable_Setup"));
  Serial.println(F("  Enabling Tasks..."));
  taskModbusReadData_1.disable();
  taskDHT11Temp_2.enable();
  taskTimeRoutine_3.enable();
  taskLED_Error_4.enable();
  taskWifiCheckStatus_5.enable();
  taskThermostatDetect_6.enable();
  taskIoControlPins_7.enable();
  taskTelnet_8.enable();
  taskLED_onEnable_9.enable();
  taskLED_OnDisable_10.enable();
  taskErrorsCodesProcess_11.enable();
  taskModbusProcess_12.enable();
  //taskEEpromProcess_13.disable();
  taskWebServer_Process_15.enable();
  taskDataServer_Process_16.disable();
  taskModbusClientSend_17.disable();
  taskOTA_Update_18.enable();
  taskLogDataSave_19.enable();
  taskLCDUpdate_20.enable();
  taskMQTTRUN_21.enable();
  taskMQTTUpdate_22.enable();

  if (taskModbusReadData_1.isEnabled())
  {
    Serial.println(F("  Enabling task taskModbusReadData_1..."));
    taskModbusReadData_1.setId(1);
  }

  if (taskDHT11Temp_2.isEnabled())
  {
    Serial.println(F("  Enabling task taskDHT11Temp_2..."));
    taskDHT11Temp_2.setId(2);
  }

  if (taskTimeRoutine_3.isEnabled())
  {
    Serial.println(F("  Enabling task taskTimeRoutine_3..."));
    taskTimeRoutine_3.setId(3);
  }

  if (taskLED_Error_4.isEnabled())
  {
    Serial.println(F("  Enabling task taskLED_Error_4..."));
    taskLED_Error_4.setId(4);
  }

  if (taskWifiCheckStatus_5.isEnabled())
  {
    Serial.println(F("  Enabling task taskWifiCheckStatus_5..."));
    taskWifiCheckStatus_5.setId(5);
  }

  if (taskThermostatDetect_6.isEnabled())
  {
    Serial.println(F("  Enabling task taskThermostatDetect_6..."));
    taskThermostatDetect_6.setId(6);
  }

  if (taskIoControlPins_7.isEnabled())
  {
    Serial.println(F("  Enabling task taskIoControlPins_7..."));
    taskIoControlPins_7.setId(7);
  }

  if (taskTelnet_8.isEnabled())
  {
    Serial.println(F("  Enabling task taskTelnet_8..."));
    taskTelnet_8.setId(8);
  }

  if (taskLED_onEnable_9.isEnabled())
  {
    Serial.println(F("  Enabling task taskLED_onEnable_9..."));
    taskLED_onEnable_9.setId(9);
  }

  if (taskLED_OnDisable_10.isEnabled())
  {
    Serial.println(F("  Enabling task taskLED_OnDisable_10..."));
    taskLED_OnDisable_10.setId(10);
  }

  if (taskErrorsCodesProcess_11.isEnabled())
  {
    Serial.println(F("  Enabling task taskErrorsCodesProcess_11..."));
    taskErrorsCodesProcess_11.setId(11);
  }

  if (taskModbusProcess_12.isEnabled())
  {
    Serial.println(F("  Enabling task taskModbusProcess_12..."));
    taskModbusProcess_12.setId(12);
  }

  // if (taskEEpromProcess_13.isEnabled())
  // {
  //   Serial.println(F("  Enabling task taskEEpromProcess_13..."));
  //   taskEEpromProcess_13.setId(13);
  // }

  if (taskMBcoilReg11_14.isEnabled())
  {
    Serial.println(F("  Enabling task taskMBcoilReg11_14..."));
    taskMBcoilReg11_14.setId(14);
  }

  if (taskWebServer_Process_15.isEnabled())
  {
    Serial.println(F("  Enabling task taskWebServer_Process_15..."));
    taskWebServer_Process_15.setId(15);
  }

  if (taskDataServer_Process_16.isEnabled())
  {
    Serial.println(F("  Enabling task taskDataServer_Process_16..."));
    taskDataServer_Process_16.setId(16);
  }

  if (taskModbusClientSend_17.isEnabled())
  {
    Serial.println(F("  Enabling task taskModbusClientSend_17..."));
    taskModbusClientSend_17.setId(17);
  }

  if (taskOTA_Update_18.isEnabled())
  {
    Serial.println(F("  Enabling task taskOTA_Update_18..."));
    taskOTA_Update_18.setId(18);
  }

  if (taskLogDataSave_19.isEnabled())
  {
    Serial.println(F("  Enabling task taskLogDataSave_19..."));
    taskLogDataSave_19.setId(19);
  }
  if (taskLCDUpdate_20.isEnabled())
  {
    Serial.println(F("  Enabling task taskLCDUpdate_20..."));
    taskLCDUpdate_20.setId(20);
  }
  if (taskMQTTRUN_21.isEnabled())
  {
    Serial.println(F("  Enabling task taskMQTTRUN_21..."));
    taskMQTTRUN_21.setId(21);
  }
  if (taskMQTTUpdate_22.isEnabled())
  {
    Serial.println(F("  Enabling task taskMQTTUpdate_22..."));
    taskMQTTUpdate_22.setId(21);
  }
}
//************************************************************************************
void TaskScheduler_Setup()
{
  Serial.println(F("ROUTINE_TaskScheduler_Setup"));

  runner.init();
  if (glb_tempController)
    runner.addTask(taskModbusReadData_1);
  runner.addTask(taskDHT11Temp_2);
  runner.addTask(taskTimeRoutine_3);
  if (glb_tempController)
    runner.addTask(taskLED_Error_4);
  if (glb_tempController)
    runner.addTask(taskWifiCheckStatus_5);
  if (glb_tempController)
    runner.addTask(taskThermostatDetect_6);
  if (glb_tempController)
    runner.addTask(taskIoControlPins_7);
  runner.addTask(taskTelnet_8);
  if (glb_tempController)
    runner.addTask(taskLED_onEnable_9);
  if (glb_tempController)
    runner.addTask(taskLED_OnDisable_10);
  if (glb_tempController)
    runner.addTask(taskErrorsCodesProcess_11);
  if (glb_tempController)
    runner.addTask(taskModbusProcess_12);
  if (glb_tempController)
    //runner.addTask(taskEEpromProcess_13);
  if (glb_tempController)
    runner.addTask(taskMBcoilReg11_14);
  if (glb_tempController)
    runner.addTask(taskWebServer_Process_15);
  if (glb_tempController)
    runner.addTask(taskDataServer_Process_16);
  if (glb_tempController)
    runner.addTask(taskModbusClientSend_17);
  runner.addTask(taskOTA_Update_18);
  if (glb_tempController)
    runner.addTask(taskLogDataSave_19);
  if (glb_tempController)
    runner.addTask(taskLCDUpdate_20);
  //runner.addTask(taskMQTTRUN_21);
  //runner.addTask(taskMQTTUpdate_22);
}

//************************************************************************************
void ChipID_Acquire()
{
  char glbChipID[12];
  char glbChipID1[20];

  Serial.println("ROUTINE_ChipID_Acquire");
  chipid = ESP.getEfuseMac();                                        //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("  ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipid);                         //print Low 4bytes.
}
//************************************************************************************
void Thermostat_ControlDisable()
{
  //shutdown thermostat override
  Serial.println(F("ROUTINE_Thermostat_ControlDisable"));
  mb.Coil(HEAT_OVERRIDE_MB_COIL, COIL_OFF); //COIL 1
  mb.Coil(HEAT_CONTROL_MB_COIL, COIL_OFF);  //COIL 2
  mb.Coil(COOL_OVERRIDE_MB_COIL, COIL_OFF); //COIL 3
  mb.Coil(COOL_CONTROL_MB_COIL, COIL_OFF);  //COIL 4
  mb.Coil(FAN_OVERRIDE_MB_COIL, COIL_OFF);  //COIL 5
  mb.Coil(FAN_CONTROL_MB_COIL, COIL_OFF);   //COIL 6
}
//************************************************************************************
void Modbus_Registers_Setup()
{
  Serial.println("ROUTINE_Modbus_Registers_Setup");
  //update registers with important data
  //mb.Hreg(ESP_RESET_REASON_MB_HREG, ESP.getResetReason());
}
//************************************************************************************
void TimeSync_Setup()
{
  Serial.println("ROUTINE_TimeSync_Setup");
  TimeRoutine();
}
//************************************************************************************
void TelnetServer_Setup()
{
  Serial.println(F("ROUTINE_TelnetServer_Setup"));

  Debug.begin("statbasement", 23, 1);                         // Initiaze the telnet server - HOST_NAME is the
                                                              //used in MDNS.begin and set the initial Serial level
  Debug.setSerialEnabled(false);                              // All messages too send to serial too, and can be see
  Debug.setResetCmdEnabled(true);                             // Setup after Serial.begin
  Debug.setCallBackProjectCmds(&TelnetServer_ProcessCommand); //callback function!!!!!
}
//************************************************************************************
void WebServer_Setup()
{

  Serial.println(F("ROUTINE_WebServer_Setup"));
  Serial.println(F("  HTTP server started..."));

  webServer.on((F("/upload")), HTTP_POST, []() {
    webServer.send(200, (F("text/plain")), "");
  },
               WebServer_HandleFileUpload);
  webServer.on((F("/info")), HTTP_GET, WebServer_HandleInformation);
  webServer.on((F("/errorlog")), WebServer_HandleErrorLog);
  webServer.on((F("/datalog")), WebServer_HandleDataLog);
  webServer.on((F("/test")), WebServer_HandleTest);
  webServer.on((F("/")), WebServer_HandleRoot);
  //webServer.on((F("/edit")), HTTP_PUT, handleFileCreate);
  webServer.serveStatic("/", SPIFFS, "/index.htm");

  webServer.on((F("/inline")), []() {
    webServer.send(200, (F("text/plain")), (F("this works as well")));
  });
  webServer.onNotFound(WebServer_HandleNotFound);
  webServer.begin();
}

//************************************************************************************
void testEspOutputPin(int pin, int value)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);
}
//************************************************************************************
void testMcpOutputPin(int pin, int value)
{
  mcp.pinMode(pin, OUTPUT);
  mcp.digitalWrite(pin, value);
}
//************************************************************************************
void selftestMcp()
{
  Serial.println("ROUTINE_selftestMcp");
  for (int i = 0; i < 8; i++)
  {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, HIGH);
    delay(50);
    mcp.digitalWrite(i, LOW);
    delay(50);
  }
}
//************************************************************************************
void Reset_Reason()
{
  Serial.println("ROUTINE_Reset_reason");

  word numResetReason = 0;
  int tmpRR0 = rtc_get_reset_reason(0);
  int tmpRR1 = rtc_get_reset_reason(0);

  String tmpRR = "";
  Serial.print(F("  Reset Reason:Core 0  "));
  verbose_print_reset_reason(rtc_get_reset_reason(0));

  Serial.print(F("  Reset Reason:Core 1  "));
  verbose_print_reset_reason(rtc_get_reset_reason(0));

  //FileSystem_ErrorLogSave(tmpRR);

  mb.Hreg(ESP_RESET_REASON_MB_HREG, (word)numResetReason);
  glb_resetCounter = 0;
  EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
  EEPROM.commit();

  glb_resetCounter = EEPROM.read(ES_RESETCOUNTER);
  glb_resetCounter++;
  EEPROM.write(ES_RESETCOUNTER, glb_resetCounter);
  EEPROM.commit();
  mb.Hreg(ESP_RESET_COUNTER_MB_HREG, (word)glb_resetCounter);
  String tmp = (F("Reset counter : "));
  FileSystem_ErrorLogSave(String(tmp + glb_resetCounter));
  Serial.print(F("  Reset counter : "));
  Serial.println(glb_resetCounter);
}
//************************************************************************************
//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("  Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
}
//************************************************************************************
void WiFiManager_Setup()
{
  Serial.println("ROUTINE_WiFiManager_Setup");
  wifiManager.setDebugOutput(true);
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("HOMESTAT");
  wifiManager.setConfigPortalTimeout(60);
  glb_ipAddress = WiFi.localIP();
  Serial.print(F(" IP address : "));
  Serial.println(WiFi.localIP());
  wifiManager.getPassword().toCharArray(glb_SSIDpassword, 32);
  wifiManager.getSSID().toCharArray(glb_SSID, 32);
  Serial.print(F("  SSID : "));
  Serial.println(glb_SSID);
  Serial.print(F("  SSID password : "));
  Serial.println(glb_SSIDpassword);
  Wifi_CheckStatus();
}
//************************************************************************************
void saveConfigCallback()
{
  Serial.println("Should save config");
}
//************************************************************************************
void print_reset_reason(RESET_REASON reason)
{
  switch (reason)
  {
  case 1:
    Serial.println("POWERON_RESET");
    break; /**<1,  Vbat power on reset*/
  case 3:
    Serial.println("SW_RESET");
    break; /**<3,  Software reset digital core*/
  case 4:
    Serial.println("OWDT_RESET");
    break; /**<4,  Legacy watch dog reset digital core*/
  case 5:
    Serial.println("DEEPSLEEP_RESET");
    break; /**<5,  Deep Sleep reset digital core*/
  case 6:
    Serial.println("SDIO_RESET");
    break; /**<6,  Reset by SLC module, reset digital core*/
  case 7:
    Serial.println("TG0WDT_SYS_RESET");
    break; /**<7,  Timer Group0 Watch dog reset digital core*/
  case 8:
    Serial.println("TG1WDT_SYS_RESET");
    break; /**<8,  Timer Group1 Watch dog reset digital core*/
  case 9:
    Serial.println("RTCWDT_SYS_RESET");
    break; /**<9,  RTC Watch dog Reset digital core*/
  case 10:
    Serial.println("INTRUSION_RESET");
    break; /**<10, Instrusion tested to reset CPU*/
  case 11:
    Serial.println("TGWDT_CPU_RESET");
    break; /**<11, Time Group reset CPU*/
  case 12:
    Serial.println("SW_CPU_RESET");
    break; /**<12, Software reset CPU*/
  case 13:
    Serial.println("RTCWDT_CPU_RESET");
    break; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    Serial.println("EXT_CPU_RESET");
    break; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    Serial.println("RTCWDT_BROWN_OUT_RESET");
    break; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    Serial.println("RTCWDT_RTC_RESET");
    break; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    Serial.println("NO_MEAN");
  }
}
//************************************************************************************
void verbose_print_reset_reason(RESET_REASON reason)
{
  switch (reason)
  {
  case 1:
    Serial.println("Vbat power on reset");
    break;
  case 3:
    Serial.println("Software reset digital core");
    break;
  case 4:
    Serial.println("Legacy watch dog reset digital core");
    break;
  case 5:
    Serial.println("Deep Sleep reset digital core");
    break;
  case 6:
    Serial.println("Reset by SLC module, reset digital core");
    break;
  case 7:
    Serial.println("Timer Group0 Watch dog reset digital core");
    break;
  case 8:
    Serial.println("Timer Group1 Watch dog reset digital core");
    break;
  case 9:
    Serial.println("RTC Watch dog Reset digital core");
    break;
  case 10:
    Serial.println("Instrusion tested to reset CPU");
    break;
  case 11:
    Serial.println("Time Group reset CPU");
    break;
  case 12:
    Serial.println("Software reset CPU");
    break;
  case 13:
    Serial.println("RTC Watch dog Reset CPU");
    break;
  case 14:
    Serial.println("for APP CPU, reseted by PRO CPU");
    break;
  case 15:
    Serial.println("Reset when the vdd voltage is not stable");
    break;
  case 16:
    Serial.println("RTC Watch dog reset digital core and rtc module");
    break;
  default:
    Serial.println("NO_MEAN");
  }
}

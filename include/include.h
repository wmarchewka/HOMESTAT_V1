#ifndef __THERMOSTAT_INCLUDE
#define __THERMOSTAT_INCLUDE

#include <Arduino.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <rom/rtc.h>

//define used in task scheduler
#define _TASK_TIMECRITICAL
#define _TASK_WDT_IDS
#define _TASK_PRIORITY
#define _TASK_STATUS_REQUEST
#define FORMAT_SPIFFS_IF_FAILED true

//dht defines
#define DHTTYPE DHT11 // DHT 11

String errorCodes[12] = {"0", "WIFI NO SSID AVAIL", "WIFI SCAN COMPLETED", "WIFI CONNECTED",
                         "WIFI CONNECT FAILED", "WIFI CONNECTION_LOST", "WIFI DISCONNECTED", "DHT TIMEOUT", "DHT CHECKSUM",
                         "TSTAT DETECT ERR", "WIFI IDLE STATUS"};

//eeprom settings memory location
extern const int ES_SSID PROGMEM PROGMEM = 0;  //size of 32
extern const int ES_SSIDPASSWORD PROGMEM = 32; //SIZE OF 32
extern const int ES_RESETCOUNTER PROGMEM = 65; //SIZE OF 32
extern const int ES_SSID_MD5 PROGMEM = 98;
extern const int ES_SSIDPASS_MD5 PROGMEM = 102;

//Modbus Registers Offsets (0-9999)
extern const int ANALOG_SENSOR_MB_HREG PROGMEM = 1;
extern const int TEMPERATURE_SENSOR_MB_HREG PROGMEM = 2;
extern const int HUMIDITY_SENSOR_MB_HREG PROGMEM = 3;
extern const int THERMOSTAT_HEAT_CALL_PULSE_VALUE_MB_HREG PROGMEM = 4;
extern const int THERMOSTAT_COOL_CALL_PULSE_VALUE_MB_HREG PROGMEM = 5;
extern const int THERMOSTAT_FAN_CALL_PULSE_VALUE_MB_HREG PROGMEM = 6;
extern const int DHT_STATUS_ERR_TIMEOUT_COUNTER_MB_HREG PROGMEM = 100;
extern const int DHT_STATUS_ERR_CHECKSUM_COUNTER_MB_HREG PROGMEM = 101;
extern const int DHT_STATUS_ERR_MB_HREG PROGMEM = 102;
extern const int BLINK_ERROR_CODE_MB_HREG PROGMEM = 103;
extern const int WIFI_STATUS_ERR_MB_HREG PROGMEM = 104;
extern const int THERMOSTAT_STATUS_ERR_MB_HREG PROGMEM = 105;
extern const int ESP_RESET_REASON_MB_HREG PROGMEM = 106;
extern const int ESP_CHIP_ID_HIGH_MB_HREG PROGMEM = 107;
extern const int ESP_CHIP_ID_LOW_MB_HREG PROGMEM = 108;
extern const int ESP_MEMORY_MB_HREG PROGMEM = 109;
extern const int WIFI_NOT_CONNECTED_MB_HREG PROGMEM = 110;
extern const int DHT_ROUTINE_TIME_MB_HREG PROGMEM = 111;
extern const int TIME_HH_MB_HREG PROGMEM = 112;
extern const int TIME_MM_MB_HREG PROGMEM = 113;
extern const int TIME_SS_MB_HREG PROGMEM = 114;
extern const int GOOD_PACKET_COUNTER_MB_REG PROGMEM = 115;
extern const int BAD_PACKET_COUNTER_MB_REG PROGMEM = 116;
extern const int MB_ROUTINE_TIME_MB_HREG PROGMEM = 117;
extern const int PROCESS_MODBUS_TIME_MB_HREG PROGMEM = 118;
extern const int THERM_DETECT_ROUTINE_TIME_MB_HREG PROGMEM = 119;
extern const int SCREEN_TIME_MB_HREG PROGMEM = 120;
extern const int ESP_BOOT_DEVICE_MB_HREG PROGMEM = 121;
extern const int ESP_RESET_COUNTER_MB_HREG PROGMEM = 122;
extern const int MB_START_DELAY_COUNTER_MB_REG PROGMEM = 123;
extern const int MB_OVERRUN_NEG_COUNTER_MB_REG PROGMEM = 124;
extern const int MB_OVERRUN_POS_COUNTER_MB_REG PROGMEM = 125;
extern const int NO_CLIENT_COUNTER_MB_REG PROGMEM = 126;
extern const int NOT_MODBUS_PACKET_COUNTER_MB_REG PROGMEM = 127;
extern const int LARGE_FRAME_COUNTER_MB_REG PROGMEM = 128;
extern const int FAILED_WRITE_COUNTER_MB_REG PROGMEM = 129;
extern const int NTP_LOOP_TIME_MB_HREG PROGMEM = 130;
extern const int ESP_MEMORY_LOW_POINT PROGMEM = 131;

//modbus COILS
const int HEAT_OVERRIDE_MB_COIL PROGMEM = 1;
const int HEAT_CONTROL_MB_COIL PROGMEM = 2;
const int COOL_OVERRIDE_MB_COIL PROGMEM = 3;
const int COOL_CONTROL_MB_COIL PROGMEM = 4;
const int FAN_OVERRIDE_MB_COIL PROGMEM = 5;
const int FAN_CONTROL_MB_COIL PROGMEM = 6;
const int THERMOSTAT_HEAT_CALL_MB_COIL PROGMEM = 7;
const int THERMOSTAT_COOL_CALL_MB_COIL PROGMEM = 8;
const int THERMOSTAT_FAN_CALL_MB_COIL PROGMEM = 9;
const int THERMOSTAT_STATUS_MB_COIL PROGMEM = 10;
const int ESP_RESTART_MB_COIL PROGMEM = 11;
const int ESP_CLEAR_SAVECRASH_DATA PROGMEM = 12;

//pin mappping to io expander
const int HEAT_OVERRIDE_PIN PROGMEM = 0;
const int HEAT_CONTROL_PIN PROGMEM = 1;
const int COOL_OVERRIDE_PIN PROGMEM = 2;
const int COOL_CONTROL_PIN PROGMEM = 3;
const int FAN_OVERRIDE_PIN PROGMEM = 4;
const int FAN_CONTROL_PIN PROGMEM = 5;
const int LED PROGMEM = 7;

//misc variables

//GLOBAL variables

extern bool glb_coolcall = false;
extern bool glb_DHT11debugOn = 1;
extern bool glb_fancall = false;
extern bool glb_heatcall = false;
extern bool glb_logDataDebug = false;
extern bool glb_OTA_Started = false;
extern bool glb_tempController = false;
extern bool glb_tstatDebugOn = false;
extern char glb_lcdTime[20];
extern char glb_SSID[32];
extern char glb_SSIDpassword[32];
extern const bool COIL_OFF PROGMEM = false;
extern const bool COIL_ON PROGMEM = true;
extern const char *glb_mdnsName = "HOMESTAT";
extern const char *mqtt_server = "10.0.0.26";
extern const unsigned int ts_coolcall = 5; // Field to write elapsed time data
extern const unsigned int ts_fancall = 6;  // Field to write elapsed time data
extern const unsigned int ts_heatcall = 4; // Field to write elapsed time data
extern const unsigned int ts_humidity = 2; // Field to write temperature data
extern const unsigned int ts_lightsensor = 3; // Field to write elapsed time data
extern const unsigned int ts_temperature = 1; // Field to write temperature data
extern const word glb_maxCoilSize PROGMEM = 256;
extern const word glb_maxHregSize PROGMEM = 256;
extern File fsUploadFile;
extern File glb_errorLog;
extern File glb_temperatureLog;
extern float glb_VCC = 0.0;
extern int glb_dataLogCount = 0;
extern int glb_dataServerCounter = 0;
extern int glb_heatRunTimeTotal = 0;
extern int glb_humidity = 0;
extern int glb_lightSensor = 0;
extern int glb_lowMemory = 80000;
extern int glb_TaskTimes[30];
extern int glb_temperature = 0;
extern IPAddress glb_ipAddress; //ip address
extern long glb_wifiRSSI = 0;
extern String glb_BootTime = "";
extern String glb_dataLogPath = "/datalog.csv";
extern String glb_debugLogPath = "/debuglog.txt";
extern String glb_dhtStatusError = "";
extern String glb_errorLogPath = "/errorlog.txt";
extern String glb_systemLogPath = "/systemlog.txt";
extern String glb_testLED = "";
extern String glb_thermostatStatus = "";
extern String glb_timeDay = "";
extern String glb_timeHour = "";
extern String glb_TimeLong = "";
extern String glb_timeMin = "";
extern String glb_timeMonth = "";
extern String glb_timeSec = "";
extern String glb_TimeShort = "";
extern String glb_timeWeekDay = "";
extern String glb_timeYear = "";
extern uint32_t glb_freeHeap = 0;
extern uint32_t glb_resetCounter = 0;
extern uint64_t chipid;
extern volatile word glb_coolPulseCounter = 0;
extern volatile word glb_coolPulseDuration = 0;
extern volatile word glb_fanPulseCounter = 0;
extern volatile word glb_fanPulseDuration = 0;
extern volatile word glb_heatPulseCounter = 0;
extern volatile word glb_heatPulseDuration = 0;
extern word glb_BlinkErrorCode = 1;
extern word glb_eepromHregCopy[glb_maxHregSize] = {};
extern word glb_errorDHT = 0;
extern word glb_errorThermostat = 0;
extern word glb_wifiNotConnectedCounter = 0;
extern word glb_WiFiStatus = 0;

//function declarations

bool FileSystem_DeleteFile(String);
bool FileSystem_PrintFile(String, bool);
bool LED_OnEnable();
String WebServer_getPage();
void ChipID_Acquire();
void DataServer_Process();
void DataServer_Setup();
void DeepSleepMode();
void DHT11_Sensor_Setup();
void DHT11_TempHumidity();
void ErrorCodes_Process();
void ESP_Restart();
void FileSystem_CreateHTML();
void FileSystem_DataLogCreate();
void FileSystem_DataLogSave();
void FileSystem_DebugDataSave(String);
void FileSystem_DebugLogCreate();
void FileSystem_ErrorLogCreate();
void FileSystem_ErrorLogSave(String);
void FileSystem_Format();
void FileSystem_ListDirectory();
void FileSystem_SystemDataSave(String);
void FileSystem_SystemLogCreate();
void handleSubmit();
void I2C_Setup();
void Interrupt_Detect_AC();
void IO_ControlPins();
void IO_Pins_Setup();
void LCD_DrawText(int, int, char *, uint16_t, uint16_t);
void LCD_Setup();
void LCD_Update();
void LED_Error();
void LED_Off();
void LED_On();
void LED_OnDisable();
void mDNS_Setup();
void Modbus_Client_Send();
void Modbus_CreateTCP();
void Modbus_Process();
void Modbus_ReadData();
void Modbus_Registers_Create();
void Modbus_Registers_Setup();
void MQTT_Callback(char *, byte *, unsigned int);
void MQTT_Publish();
void MQTT_Reconnect();
void MQTT_RunLoop();
void MQTT_Setup();
void OTA_Setup();
void OTA_Update();
void Reset_Reason();
void saveConfigCallback();
void selftestMcp();
void StartupPrinting_Setup();
void Tasks_Enable_Setup();
void TaskScheduler_Setup();
void TelnetServer_Process();
void TelnetServer_ProcessCommand();
void TelnetServer_Setup();
void testEspOutputPin(int, int);
void testMcpOutputPin(int, int);
void testVCC();
void Thermostat_ControlDisable();
void Thermostat_Detect();
void ThermostatMode_Setup();
void TimeRoutine();
void TimeSync_Setup();
void verbose_print_reset_reason(RESET_REASON);
void WebServer_HandleDataLog();
void WebServer_HandleDatalogUpload();
void WebServer_HandleErrorLog();
void WebServer_HandleFileUpload();
void WebServer_HandleInformation();
void WebServer_HandleNotFound();
void WebServer_HandleRoot();
void WebServer_Process();
void WebServer_Root();
void WebServer_Setup();
void Wifi_CheckStatus();
void Wifi_Setup();
void WiFiManager_Setup();


#endif

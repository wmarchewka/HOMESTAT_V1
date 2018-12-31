#include <Arduino.h>
#include <SPIFFS.h>
#include <include.h>
#include <RemoteDebug.h>
#include <ModbusIP_ESP8266.h>

extern RemoteDebug Debug;
extern ModbusIP mb;

//************************************************************************************
bool FileSystem_DeleteFile(String pathname)
{
    Serial.println("ROUTINE_FileSystem_DeleteFile");

    if (SPIFFS.begin(false))
    {
        Serial.println(F("  File system already mounted..."));
    }

    bool val = SPIFFS.remove(pathname);
    //Serial.println(val);
    if (pathname == glb_dataLogPath)
        glb_dataLogCount = 0;

    return val;
}
//************************************************************************************
void FileSystem_CreateHTML()
{
    Serial.println("ROUTINE_FileSystem_CreateHTML");
    File f = SPIFFS.open("index.html", "w");
    f.close();
}
//************************************************************************************
void FileSystem_ListDirectory()
{
    Serial.println("ROUTINE_FileSystem_ListDirectory");

    /*
 Dir dir = SPIFFS.openDir("/");
 while (dir.next())
  {
    String tmp = dir.fileName();
    tmp.remove(0, 1);
    Debug.print(tmp);
    Debug.print(" : ");
    File f = dir.openFile("r");
    Debug.println(f.size());
    f.close();
  }
  */
}
//************************************************************************************
bool FileSystem_PrintFile(String filepath, bool debug)
{
    Serial.println("ROUTINE_FileSystem_PrintFile");

    String inputString = "";
    if (SPIFFS.exists(filepath))
    {
        //Debug.println("File exists...");
        File f = SPIFFS.open(filepath, "r");
        while (f.available())
        {
            char inChar = (char)f.read();
            if ((inChar == '\n') || (inChar == '\r'))
            {
                char nextChar = (char)f.read();
                if ((nextChar == '\n') || (nextChar == '\r'))
                {
                    Debug.println(inputString);
                    inputString = "";
                }
                else
                {
                    Debug.println(inputString);
                    inputString = "";
                    inputString += nextChar;
                }
            }
            else
            {
                inputString += inChar;
            }
        }
        f.close();
        Debug.println(inputString);
        return true;
    }
    else
    {
        return false;
    }
}
//************************************************************************************
void FileSystem_SystemLogCreate()
{
    bool debug = 0;

    Serial.println(F("ROUTINE_FileSystem_SystemLogCreate"));

    if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println(F("  File system already mounted..."));
    }
    else
    {
        SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
    }

    if (SPIFFS.exists(glb_systemLogPath))
    {
        Serial.println(F("  File exists..."));
        File f = SPIFFS.open(glb_systemLogPath, "r");
        Serial.print(F("  File size is :"));
        Serial.println(f.size());
        f.close();
    }
    else
    {
        Serial.println(F("  File not found error..."));
        File f = SPIFFS.open(glb_systemLogPath, "w");
        f.close();
    }
}
//************************************************************************************
void FileSystem_DebugLogCreate()
{
    bool debug = 0;

    Serial.println(F("ROUTINE_FileSystem_DebugLogCreate"));

    if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println(F("  File system mounted..."));
    }

    if (SPIFFS.exists(glb_debugLogPath))
    {
        Serial.println(F("  File exists..."));
        File f = SPIFFS.open(glb_debugLogPath, "r");
        Serial.print(F("  File size is : "));
        Serial.println(f.size());
        f.close();
    }
    else
    {
        Serial.println(F("  File not found error..."));
        File f = SPIFFS.open(glb_debugLogPath, "w");
        f.close();
    }
}
//************************************************************************************
void FileSystem_DataLogCreate()
{
    bool debug = 1;

    Serial.println(F("ROUTINE_FileSystem_DataLogCreate"));

    if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println(F("  File system mounted:..."));
    }

    if (SPIFFS.exists(glb_dataLogPath))
    {
        Serial.println(F("  File exists..."));
        File f = SPIFFS.open(glb_dataLogPath, "r");
        Serial.print(F("  File size is : "));
        Serial.println(f.size());
        f.close();
    }
    else
    {
        Serial.println(F("  File not found error..."));
        File f = SPIFFS.open(glb_dataLogPath, "w");
        f.close();
    }
}
//************************************************************************************
void FileSystem_DataLogSave()
{
    //currently 49 bytes per entry
    Serial.println("ROUTINE_FileSystem_DataLogSave");

    int startTimeMicros = micros();
    int endTimeMicros = 0;

    String logdata = "";

    if (glb_logDataDebug)
        Debug.println(F("Save log data"));
    if (glb_logDataDebug)
        Debug.print(F("Time:"));
    if (glb_logDataDebug)
        Debug.println(glb_TimeLong);
    if (glb_logDataDebug)
        Debug.print(F("Temp:"));
    if (glb_logDataDebug)
        Debug.println(glb_temperature);
    if (glb_logDataDebug)
        Debug.print(F("Humidity:"));
    if (glb_logDataDebug)
        Debug.println(glb_humidity);

    File glb_temperatureLog = SPIFFS.open(glb_dataLogPath, FILE_APPEND);
    if (glb_logDataDebug)
        Debug.println(glb_temperatureLog);

    if (glb_temperatureLog)
    {
        if (glb_logDataDebug)
            Debug.print(F("Filelog size..."));
        if (glb_logDataDebug)
            Debug.println(glb_temperatureLog.size());
        glb_dataLogCount++;
        logdata = logdata + String(glb_dataLogCount);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeMonth);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeDay);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeYear);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeWeekDay);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeHour);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeMin);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_timeSec);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_temperature);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_humidity);
        logdata = logdata + (F(","));
        logdata = logdata + (glb_lightSensor);
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(THERMOSTAT_HEAT_CALL_MB_COIL));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(THERMOSTAT_COOL_CALL_MB_COIL));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(THERMOSTAT_FAN_CALL_MB_COIL));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(HEAT_OVERRIDE_PIN));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(COOL_OVERRIDE_PIN));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(FAN_OVERRIDE_PIN));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(HEAT_CONTROL_PIN));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(COOL_CONTROL_PIN));
        logdata = logdata + (F(","));
        logdata = logdata + (mb.Coil(FAN_CONTROL_PIN));
        glb_temperatureLog.println(logdata);
        logdata = "";
    }
    else
    {
        if (glb_logDataDebug)
            Debug.println(F("File open failed..."));
    }
    glb_temperatureLog.close();

    endTimeMicros = micros();
    endTimeMicros = endTimeMicros - startTimeMicros;
    glb_TaskTimes[19] = endTimeMicros;
    if (endTimeMicros > 100000)
    {
        if (glb_logDataDebug)
            Serial.print(F("save data went long : "));
        if (glb_logDataDebug)
            Serial.println(endTimeMicros);
    }
}

//************************************************************************************
void FileSystem_SystemDataSave(String logData)
{
    Serial.println("ROUTINE_FileSystem_SystemDataSave");

    bool debug = 0;

    int startTimeMicros = micros();
    int endTimeMicros = 0;

    File logSystemData = SPIFFS.open(glb_systemLogPath, FILE_APPEND);
    if (debug)
        Serial.println(glb_systemLogPath);

    if (logSystemData)
    {
        logSystemData.print(glb_TimeLong);
        logSystemData.print(",");
        logSystemData.println(logData);
        logSystemData.close();
    }
}
//************************************************************************************
void FileSystem_DebugDataSave(String data)
{

    Serial.println("ROUTINE_FileSystem_DebugSaveData");

    bool debug = 0;
    int startTimeMicros = micros();
    int endTimeMicros = 0;

    File logDebugData = SPIFFS.open(glb_debugLogPath, FILE_WRITE);
    if (debug)
    {
        Serial.println(glb_debugLogPath);
        if (logDebugData)
        {
            String webData = data;
            webData += (F("LightSensor="));
            webData += String(glb_lightSensor);
            webData += (F("\n"));
            webData += (F("Temp&Humidity Status="));
            webData += String(glb_dhtStatusError);
            webData += (F("\n"));
            webData += (F("Humidity="));
            webData += String(glb_humidity);
            webData += (F("\n"));
            webData += (F("Temperature="));
            webData += String(glb_temperature);
            webData += (F("\n"));
            webData += (F("WiFi Status="));
            webData += String(WiFi.status());
            webData += (F("\n"));
            webData += (F("IP="));
            webData += String(glb_ipAddress);
            webData += (F("\n"));
            webData += (F("Heat duration="));
            webData += String(glb_heatPulseDuration);
            webData += (F("\n"));
            webData += (F("Cool duration="));
            webData += String(glb_coolPulseDuration);
            webData += (F("\n"));
            webData += F(("Fan duration="));
            webData += String(glb_fanPulseDuration);
            webData += (F("\n"));
            webData += (F("Thermostat Status="));
            webData += String(glb_thermostatStatus);
            webData += (F("\n"));
            webData += (F("Error Code:"));
            webData += String(glb_BlinkErrorCode);
            webData += (F("\n"));
            webData += (F("Coils="));
            for (int x = 23; x > 15; x--)
            {
                webData += String(mb.Coil(x));
            }
            webData += (F(" "));
            for (int x = 15; x > 7; x--)
            {
                webData += String(mb.Coil(x));
            }
            webData += (F(" "));
            for (int x = 7; x >= 0; x--)
            {
                webData += String(mb.Coil(x));
            }
            webData += (F("\n"));

            webData += (F("PINS="));
            for (int x = 23; x > 15; x--)
            {
                webData += String(digitalRead(x));
            }
            webData += (F(" "));
            for (int x = 15; x > 7; x--)
            {
                webData += String(digitalRead(x));
            }
            webData += (F(" "));
            for (int x = 7; x >= 0; x--)
            {
                webData += String(digitalRead(x));
            }
            webData += (F("\n"));

            for (int x = 1; x <= glb_maxCoilSize - 1; x++)
            {
                webData += (F("Modbus coil "));
                webData += String(x);
                webData += (F(" value:"));
                int val = mb.Coil(x);
                webData += String(val);
                webData += (F("\t\t\t"));
                webData += (F("Modbus hReg "));
                webData += String(x);
                webData += (F(" value:"));
                val = mb.Hreg(x);
                webData += String(val);
                webData += (F("\n"));
            }
            logDebugData.print(webData);
            logDebugData.close();
            endTimeMicros = micros();
            endTimeMicros = endTimeMicros - startTimeMicros;
            if (endTimeMicros > 1)
            {
                if (debug)
                    Serial.print(F("Debug data time: "));
                if (debug)
                    Serial.println(endTimeMicros);
            }
        }
    }
}
//************************************************************************************
void FileSystem_ErrorLogSave(String data)
{
    bool debug = 0;
    Serial.println("ROUTINE_FileSystem_ErrorLogSave");
    String tmpdata = "";
    tmpdata = glb_TimeLong + "," + data;
    File glb_errorLog = SPIFFS.open(glb_errorLogPath, FILE_APPEND);
    if (!glb_errorLog)
    {
        Serial.println("  ErrorLogSave - failed to open file for appending");
        return;
    }
    if (glb_errorLog.println(tmpdata))
    {
        Serial.println("  ErrorLogSave - message appended");
    }
    else
    {
        Serial.println("  ErrorLogSave- append failed");
    }
    glb_errorLog.close();
}
//************************************************************************************
void FileSystem_ErrorLogCreate()
{
    Serial.println("ROUTINE_FileSystem_ErrorLog_Create");
    bool debug = 1;

    if (debug)
        Serial.println(F("  Creating ErrorLog..."));

    if (SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
        Serial.println(F("  File system mounted..."));
    }

    if (SPIFFS.exists(glb_errorLogPath))
    {
        Serial.println(F("  File exists..."));
        File f = SPIFFS.open(glb_errorLogPath, "r");
        Serial.print(F("  File size is : "));
        Serial.println(f.size());
        f.close();
    }
    else
    {
        Serial.println(F("  File not found error..."));
        File f = SPIFFS.open(glb_errorLogPath, "w");
        Serial.print(F("  File created : "));
        Serial.println(glb_errorLogPath);
        f.close();
    }
}
//************************************************************************************
void FileSystem_Format()
{
    Serial.println(F("ROUTINE_FileSystem_Format"));
    Serial.println(F("Formatting File System..."));
    SPIFFS.format();
}
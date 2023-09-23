/***********************************************************************
 * Project      :     tiny32-ServerFirmware-update
 * Description  :     tiny32 test update firmware from server
 * Hardware     :     tiny32_v4
 * Author       :     Tenergy Innovation Co., Ltd.
 * Date         :     23/09/2023
 * Revision     :     0.1
 * Rev1.0       :     Origital
 * website      :     http://www.tenergyinnovation.co.th
 * Email        :     admin@innovation.co.th
 * TEL          :     +66 82-380-3299
 ***********************************************************************/
#include <Arduino.h>
#include <tiny32_v3.h>
#include <esp_task_wdt.h>
#include <stdio.h>
#include <stdint.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

/**************************************/
/*          Firmware Version          */
/**************************************/
#define FIRMWARE_VERSION "0.4"
/**************************************/
/*          Header project            */
/**************************************/
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     tiny32-ServerFirmware-update\r\n");
    Serial.printf("* Description  :     tiny32 test update firmware from server \r\n");
    Serial.printf("* Hardware     :     tiny32_v4\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     23/09/2023\r\n");
    Serial.printf("* Firmware     :     %s\r\n", FIRMWARE_VERSION);
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     admin@innovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 82-308-3299\r\n");
    Serial.printf("***********************************************************************/\r\n");
}

/**************************************/
/*        define object variable      */
/**************************************/
tiny32_v3 mcu;
WiFiManager wm;
File file;

/**************************************/
/*            GPIO define             */
/**************************************/

/**************************************/
/*       Constand define value        */
/**************************************/
#define WDT_TIMEOUT 300
#define WIFI_TIMEOUT 20
#define FORMAT_SPIFFS_IF_FAILED true
#define AP_TIMEOUT 300
const bool SUCCESS = 1;
const bool ERROR = 0;
#define ServerUpdate "139.59.237.181"
#define PathUpdate "/firmware/firmware.bin"
#define PathFirmwareCheck "/firmware/smart_water_management/firmware/version.txt"


/**************************************/
/*       eeprom address define        */
/**************************************/
#define EEPROM_SIZE 1024     // กำหนดขนาดของ eeprom *สำคัญมาก
#define upload_flag_eep 0x00 // 1 byte

/**************************************/
/*        define global variable      */
/**************************************/
char unit[15];
StaticJsonDocument<50000> doc;
StaticJsonDocument<1000> doc1;
StaticJsonDocument<1000> doc2;

/**************************************/
/*           define function          */
/**************************************/
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);
void saveConfigCallback();
String getdataFile(fs::FS &fs, const char *path);
bool wifi_config(bool upload_flag);
bool firmware_update();
bool firmware_check();

/***********************************************************************
 * FUNCTION:    firmware_update
 * DESCRIPTION: Update firmware from server
 * PARAMETERS:  nothing
 * RETURNED:    true/false
 ***********************************************************************/
bool firmware_update()
{
    bool _status;
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        httpUpdate.rebootOnUpdate(false); // remove automatic update

        Serial.println(F("Update start now!"));
        mcu.TickBlueLED(0.1);
        mcu.TickRedLED(0.1);
        mcu.TickBuildinLED(0.1);
        // t_httpUpdate_return ret = httpUpdate.update(client, "167.71.223.61", 3000, "/firmware/firmware.bin");
         t_httpUpdate_return ret = httpUpdate.update(client, ServerUpdate, 3000, PathUpdate);

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            Serial.println(F("Retry in 10secs!"));
            delay(10000); // Wait 10secs
            _status = false;
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            _status = false;
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            delay(1000); // Wait a second and restart
            _status = true;
            ESP.restart();
            break;
        }
    }
    else
    {
        _status = false;
        Serial.println("Error: Can't connect WiFi");
        mcu.TickBlueLED(0);
        mcu.TickRedLED(0);
        mcu.TickBuildinLED(0);
        return _status;
    }
}

/***********************************************************************
 * FUNCTION:    firmware_update
 * DESCRIPTION: Update firmware from server
 * PARAMETERS:  nothing
 * RETURNED:    true(not match) / false(match)
 ***********************************************************************/
bool firmware_check()
{
    bool _status;
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;
        // http.begin("167.71.223.61", 80, "/firmware/smart_water_management/firmware/version.txt");
        http.begin(ServerUpdate, 80, PathFirmwareCheck);
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK)
            {

                // get length of document (is -1 when Server sends no Content-Length header)
                int len = http.getSize();

                // create buffer for read
                uint8_t buff[128] = {0};
                char buff2[10];

                // get tcp stream
                WiFiClient *stream = http.getStreamPtr();

                // read all data from server
                String _version = "";
                while (http.connected() && (len > 0 || len == -1))
                {
                    // get available data size
                    size_t size = stream->available();

                    if (size)
                    {
                        // read up to 128 byte
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                        // write it to Serial
                        Serial.write(buff, c);

                        for (int i = 0; i < c; i++)
                        {
                            _version += (char)buff[i];
                        }

                        if (len > 0)
                        {
                            len -= c;
                        }
                    }
                    delay(1);
                }
                Serial.printf("\r\nInfo: Server firmware version = %s\r\n", _version.c_str());
                Serial.printf("Info: Firmware version = %s\r\n", FIRMWARE_VERSION);

                if (_version == FIRMWARE_VERSION)
                {
                    Serial.println("Firmware is match ^^");
                    _status = true;
                }
                else
                {
                    Serial.println("Firmware is not match!! ");
                    _status = false;
                }

                Serial.println();
                Serial.print("[HTTP] connection closed or file end.\n");
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            _status = true;
        }

        http.end();
        return _status;
    }
    else
    {
        _status = true;
        Serial.println("Error: Can't connect WiFi");
        return _status;
    }
}
/***********************************************************************
 * FUNCTION:    setup
 * DESCRIPTION: setup process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void setup()
{
    Serial.begin(115200);
    header_print();

    //*** Define Unit name ***
    String _mac1 = WiFi.macAddress().c_str();
    Serial.printf("Setup: macAddress = %s\r\n", WiFi.macAddress().c_str());
    String _mac2 = _mac1.substring(9, 17);
    _mac2.replace(":", "");
    String _mac3 = "tiny32-" + _mac2;
    _mac3.toCharArray(unit, _mac3.length() + 1);
    Serial.printf("Setup: Unit ID => %s\r\n", unit);

    //*** SPIFSS inital ***
    Serial.println("Setup: SPIFSS initial ...");
    if (!SPIFFS.begin(true))
    {
        Serial.println("\tError: An Error has occurred while mounting SPIFFS");
    }
    else
    {
        Serial.printf("\tPass: List file in root directory\r\n");
        listDir(SPIFFS, "/", 0);
        Serial.println();
    }

    //*** WiFi config ***
    if (true) // ไม่เชื่อมต่อ WiFi ในฟังก์ชั่น setup
    {
        Serial.println("Setup: WiFi initial ...");
        wifi_config(true);
    }

    /* All wifi list */
    String _json_rawFile = getdataFile(SPIFFS, "/wifi.conf");
    String _json_string = "{\"wifi\": [";
    _json_string += _json_rawFile;
    _json_string += "]}";
    DeserializationError _error = deserializeJson(doc1, _json_string);

    if (_error)
    {
        Serial.printf("Error: deserializeJson() failed: %s", _error.c_str());
    }

    else
    {
        /* แสดงค่า WiFi list ทั้งหมด */
        Serial.printf("\r\nSetup: All WiFi list =>\r\n");
        size_t _json_data_size = serializeJsonPretty(doc1, Serial);
        Serial.println("\r\n");
    }

    /* Watchdoc concig*/
    Serial.print("Setup: Configuring WDT...");
    esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);
    Serial.println("done");
    mcu.buzzer_beep(2);
    vTaskDelay(2);

    /* auto firmware update */
    // if(!firmware_check())
    // {
    //     firmware_update();
    // }
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: loop process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void loop()
{
    static long _blacklight_interval = 0;
    static bool _measureSw = false;
    esp_task_wdt_reset();
    vTaskDelay(300);

    /* force update firmware */
    if (mcu.Sw1())
    {
        mcu.buzzer_beep(4);
        while (mcu.Sw1())
            ;
        firmware_update();
    }

    /* check and update firmware */
    if (mcu.Sw2())
    {
        mcu.buzzer_beep(2);
        while (mcu.Sw2())
            ;
        if (firmware_check())
        {
            Serial.printf("Info: Version is same in server\r\n");
        }
        else
        {
            Serial.printf("Info: Version is not  same in server!!\r\n");
            Serial.println("Info: System is ongoing to update firmware Now!!!");
            // firmware_update();
        }
    }

    // if (WiFi.status() == WL_CONNECTED)
    // {
    //     WiFiClient client;

    //     if (mcu.Sw1())
    //     {
    //         mcu.buzzer_beep(2);
    //         while (mcu.Sw1())
    //             ;
    //         httpUpdate.rebootOnUpdate(false); // remove automatic update

    //         Serial.println(F("Update start now!"));
    //         t_httpUpdate_return ret = httpUpdate.update(client, "167.71.223.61", 3000, "/firmware/firmware.bin");

    //         switch (ret)
    //         {
    //         case HTTP_UPDATE_FAILED:
    //             Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    //             Serial.println(F("Retry in 10secs!"));
    //             delay(10000); // Wait 10secs
    //             break;

    //         case HTTP_UPDATE_NO_UPDATES:
    //             Serial.println("HTTP_UPDATE_NO_UPDATES");
    //             break;

    //         case HTTP_UPDATE_OK:
    //             Serial.println("HTTP_UPDATE_OK");
    //             delay(1000); // Wait a second and restart
    //             ESP.restart();
    //             break;
    //         }
    //     }

    //     if (mcu.Sw2())
    //     {
    //         mcu.buzzer_beep(3);
    //         while (mcu.Sw2())
    //             ;
    //         HTTPClient http;
    //         http.begin("167.71.223.61", 80, "/firmware/smart_water_management/firmware/version.txt");
    //         Serial.print("[HTTP] GET...\n");
    //         // start connection and send HTTP header
    //         int httpCode = http.GET();
    //         if (httpCode > 0)
    //         {
    //             // HTTP header has been send and Server response header has been handled
    //             Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    //             // file found at server
    //             if (httpCode == HTTP_CODE_OK)
    //             {

    //                 // get length of document (is -1 when Server sends no Content-Length header)
    //                 int len = http.getSize();

    //                 // create buffer for read
    //                 uint8_t buff[128] = {0};
    //                 char buff2[10];

    //                 // get tcp stream
    //                 WiFiClient *stream = http.getStreamPtr();

    //                 // read all data from server
    //                 String _version = "";
    //                 while (http.connected() && (len > 0 || len == -1))
    //                 {
    //                     // get available data size
    //                     size_t size = stream->available();

    //                     if (size)
    //                     {
    //                         // read up to 128 byte
    //                         int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
    //                         Serial.printf("C -> %d\r\n", c);
    //                         // write it to Serial
    //                         Serial.write(buff, c);

    //                         for (int i = 0; i < c; i++)
    //                         {
    //                             _version += (char)buff[i];
    //                         }

    //                         if (len > 0)
    //                         {
    //                             len -= c;
    //                         }
    //                     }
    //                     delay(1);
    //                 }
    //                 Serial.printf("\r\nInfo: Server firmware version = %s\r\n", _version.c_str());
    //                 Serial.printf("Info: =Firmware version = %s\r\n", FIRMWARE_VERSION);

    //                 if (_version == FIRMWARE_VERSION)
    //                 {
    //                     Serial.println("Firmware is match ^^");
    //                 }
    //                 else
    //                 {
    //                     Serial.println("Firmware is not match!! ^^");
    //                 }

    //                 Serial.println();
    //                 Serial.print("[HTTP] connection closed or file end.\n");
    //             }
    //         }
    //         else
    //         {
    //             Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    //         }

    //         http.end();
    //     }
    // }
}

/***********************************************************************
 * FUNCTION:    wifi_config
 * DESCRIPTION: WiFi config function for check and upload data to server
 * PARAMETERS:  upload_flag (TRUE/FALSE)
 * RETURNED:    nothing
 ***********************************************************************/
bool wifi_config(bool upload_flag)
{
    if (upload_flag)
    {

        Serial.println("\tInfo: Wifi config ...");
        mcu.TickBuildinLED(0.1);

        file = SPIFFS.open("/wifi.conf");
        if (!file)
        {
            Serial.println("Error: Failed to open file 'wifi.config'");
            return false;
        }
        else
        {
            /* All wifi list */
            String _json_rawFile = getdataFile(SPIFFS, "/wifi.conf");
            String _json_string = "{\"wifi\": [";
            _json_string += _json_rawFile;
            _json_string += "]}";
            DeserializationError _error = deserializeJson(doc1, _json_string);

            /* latest wifi */
            String _json_rawFile2 = getdataFile(SPIFFS, "/wifi_latest.conf");
            String _json_string2 = "{\"wifi\": [";
            _json_string2 += _json_rawFile2;
            _json_string2 += "]}";
            DeserializationError _error2 = deserializeJson(doc2, _json_string2);

            if (_error)
            {
                Serial.printf("Error: deserializeJson() failed: %s", _error.c_str());
                return false;
            }
            else if (_error2)
            {
                Serial.printf("Error: deserializeJson() failed: %s", _error2.c_str());
                return false;
            }
            else
            {
                bool _wifiConnected = false; // ตัวแปรตรวจสอบการเช่ือมต่อ WiFi

                /* แสดงค่า WiFi list ทั้งหมด */
                Serial.printf("\r\nInfo: All WiFi list =>\r\n");
                size_t _json_data_size = serializeJsonPretty(doc1, Serial);
                Serial.println("\r\n");
                Serial.printf("Info: Data Size => %d bytes\r\n", _json_data_size);
                int _json_data_number = doc1["wifi"].size();
                Serial.printf("Info: Number of WiFi list => %d\r\n\r\n", _json_data_number);

                /* แสดงค่า WiFi ล่าสุดที่เชื่อมต่อ */
                Serial.printf("\r\nInfo: Latest WiFi =>\r\n");
                size_t _json_data_size2 = serializeJsonPretty(doc2, Serial);
                int _json_data_number2 = doc2["wifi"].size();
                Serial.println("\r\n");

                /* เชื่อมต่อกับ WiFi ตัวล่าสุด */
                String _ssid_string2 = doc2["wifi"][0]["ssid"];
                String _pass_string2 = doc2["wifi"][0]["pass"];
                char _ssid_char2[30];
                char _pass_char2[30];
                _ssid_string2.toCharArray(_ssid_char2, _ssid_string2.length() + 1);
                _pass_string2.toCharArray(_pass_char2, _pass_string2.length() + 1);
                Serial.printf("Info: WiFi Connecting to \r\n");
                Serial.printf("ssid[latest]: %s\r\n", _ssid_char2);
                Serial.printf("password[latest]: %s\r\n", _pass_char2);
                uint8_t _timeoutCount2 = 0;
                Serial.printf("");

                WiFi.begin(_ssid_char2, _pass_char2); // ทำการเชื่อมต่อ WiFi ตัวล่าสุด
                while ((WiFi.status() != WL_CONNECTED) && (_timeoutCount2 < WIFI_TIMEOUT))
                {
                    delay(1000);
                    Serial.print(".");
                    _timeoutCount2++;
                }

                if ((WiFi.status() == WL_CONNECTED)) // เชื่อมต่อ WiFi ตัวล่าสุดสำเร็จ
                {
                    Serial.println("\n\rWiFi connected");
                    Serial.print("IP address: ");
                    Serial.println(WiFi.localIP());
                    Serial.printf("SSID: %s\r\n", WiFi.SSID().c_str());
                    Serial.printf("RSSI: %d\r\n", WiFi.RSSI());
                    _wifiConnected = true;
                    mcu.TickBuildinLED(1.0);
                    return true;
                }
                else // พยายามเชื่อมต่อ WiFi ที่อยู่ใน list ของไฟล์ 'wifi.conf'
                {
                    Serial.println();
                    Serial.printf("Info: WiFi Connecting to \r\n");
                    for (int i = 0; i < _json_data_number; i++)
                    {
                        String _ssid_string = doc1["wifi"][i]["ssid"];
                        String _pass_string = doc1["wifi"][i]["pass"];
                        char _ssid_char[30];
                        char _pass_char[30];
                        _ssid_string.toCharArray(_ssid_char, _ssid_string.length() + 1);
                        _pass_string.toCharArray(_pass_char, _pass_string.length() + 1);
                        Serial.printf("ssid[%d]: %s\r\n", i, _ssid_char);
                        Serial.printf("password[%d]: %s\r\n", i, _pass_char);
                        WiFi.begin(_ssid_char, _pass_char);
                        uint8_t _timeoutCount = 0;
                        Serial.printf("");
                        while ((WiFi.status() != WL_CONNECTED) && (_timeoutCount < WIFI_TIMEOUT))
                        {
                            delay(1000);
                            Serial.print(".");
                            _timeoutCount++;
                        }

                        if (WiFi.status() == WL_CONNECTED)
                        {
                            Serial.println("\n\rWiFi connected");
                            Serial.print("IP address: ");
                            Serial.println(WiFi.localIP());
                            Serial.printf("SSID: %s\r\n", WiFi.SSID().c_str());
                            Serial.printf("RSSI: %d\r\n", WiFi.RSSI());
                            _wifiConnected = true;

                            // record latest wifi to 'wifi_latest.conf'
                            String _json_OverwriteFile = "";
                            _json_OverwriteFile += "{";
                            _json_OverwriteFile += "\"ssid\":\"" + wm.getWiFiSSID() + "\",";
                            _json_OverwriteFile += "\"pass\":\"" + wm.getWiFiPass() + "\"";
                            _json_OverwriteFile += "}";
                            writeFile(SPIFFS, "/wifi_latest.conf", _json_OverwriteFile.c_str());

                            _wifiConnected = true;
                            mcu.TickBuildinLED(1.0);
                            // break;
                            return true;
                        }
                    }

                    /* ถ้าไม่มีการเชื่อมต่อ WiFi จะเข้าสู่ AP Mode สำหรับเชื่อมต่อ และบันทึกค่า WiFi */
                    if (!_wifiConnected)
                    {
                        wm.setConfigPortalTimeout(300); // sets timeout before AP,webserver loop ends and exits even if there has been no setup.
                        if (wm.autoConnect(unit, "password"))
                        {
                            Serial.printf("Success to connected\r\n");
                            Serial.print("IP address: ");
                            Serial.println(WiFi.localIP());
                            Serial.printf("SSID: %s\r\n", WiFi.SSID().c_str());
                            Serial.printf("RSSI: %d\r\n", WiFi.RSSI());

                            /* record ssid and password to 'wifi.conf' */
                            String _json_appendFile = "";
                            if (_json_data_number == 0)
                            {
                                _json_appendFile += "{";
                            }
                            else
                            {
                                _json_appendFile += ",{";
                            }
                            _json_appendFile += "\"ssid\":\"" + wm.getWiFiSSID() + "\",";
                            _json_appendFile += "\"pass\":\"" + wm.getWiFiPass() + "\"";
                            _json_appendFile += "}";
                            appendFile(SPIFFS, "/wifi.conf", _json_appendFile.c_str());

                            /* record latest wifi to 'wifi_latest.conf' */
                            String _json_OverwriteFile = "";
                            _json_OverwriteFile += "{";
                            _json_OverwriteFile += "\"ssid\":\"" + wm.getWiFiSSID() + "\",";
                            _json_OverwriteFile += "\"pass\":\"" + wm.getWiFiPass() + "\"";
                            _json_OverwriteFile += "}";
                            writeFile(SPIFFS, "/wifi_latest.conf", _json_OverwriteFile.c_str());

                            mcu.TickBuildinLED(1.0);
                            return true;
                        }
                        else
                        {
                            Serial.println("Error: Failed to connect wifi");
                            Serial.println("System is resetting");
                            mcu.buzzer_beep(3);

                            // ESP.restart();
                            return false;
                        }
                    }
                }
            }
        }
    }
    else
    {
        Serial.printf("\tInfo: No wifi config");
        return false;
    }
}

/***********************************************************************
 * FUNCTION:    listDir
 * DESCRIPTION: list directory from SPIFFS
 * PARAMETERS:  fs, dirname, levels
 * RETURNED:    nothing
 ***********************************************************************/
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

/***********************************************************************
 * FUNCTION:    readFile
 * DESCRIPTION: read File from SPIFFS
 * PARAMETERS:  fs, path
 * RETURNED:    nothing
 ***********************************************************************/
void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

/***********************************************************************
 * FUNCTION:    writeFile
 * DESCRIPTION: write data to File for SPIFFS
 * PARAMETERS:  fs, path, message
 * RETURNED:    string
 ***********************************************************************/
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

/***********************************************************************
 * FUNCTION:    appendFile
 * DESCRIPTION: appen data to File for SPIFFS
 * PARAMETERS:  fs, path, message
 * RETURNED:    nothing
 ***********************************************************************/
void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
    file.close();
}

/***********************************************************************
 * FUNCTION:    renameFile
 * DESCRIPTION: Remane File for SPIFFS
 * PARAMETERS:  fs, path1, path2
 * RETURNED:    nothing
 ***********************************************************************/
void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

/***********************************************************************
 * FUNCTION:    deleteFile
 * DESCRIPTION: Delete File for SPIFFS
 * PARAMETERS:  fs, path
 * RETURNED:    nothing
 ***********************************************************************/
void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}

/***********************************************************************
 * FUNCTION:    testFileIO
 * DESCRIPTION: test File IO for SPIFFS
 * PARAMETERS:  fs, path
 * RETURNED:    nothing
 ***********************************************************************/
void testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
    }
}

/***********************************************************************
 * FUNCTION:    getdataFile
 * DESCRIPTION: get data of File from SPIFFS
 * PARAMETERS:  fs, path
 * RETURNED:    nothing
 ***********************************************************************/
String getdataFile(fs::FS &fs, const char *path)
{
    String _string = "";
    const int bufferSize = 40000; // Adjust the buffer size as needed
    char buffer[bufferSize];      // Character array to store the data
    int bufferIndex = 0;          // Index to keep track of the current position in the buffer
    boolean newData = false;      // Flag to indicate when new data has been received

    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return _string;
    }

    Serial.println("- read from file:");
    while (file.available())
    {

        char incomingChar = file.read();
        _string += incomingChar;
    }
    file.close();
    return _string;
}
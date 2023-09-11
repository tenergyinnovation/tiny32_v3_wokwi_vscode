/***********************************************************************
 * Project      :     tiny32_v3_wokwi_vscode
 * Description  :     this code is template source code useful for tiny32_v3 and tiny32_v4
 *                    hardware simulate run with wokwi simulate, it works with vscode
 * Hardware     :     tiny32_v3 and tiny32_v4
 * Author       :     Tenergy Innovation Co., Ltd.
 * Date         :     11/09/2023
 * Revision     :     1.0
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

/**************************************/
/*          Firmware Version          */
/**************************************/
String version = "1.0";

/**************************************/
/*          Header project            */
/**************************************/
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     tiny32_v3_wokwi_vscode\r\n");
    Serial.printf("* Description  :     this code is template source code useful for tiny32_v3 and tiny32_v4 \r\n");
    Serial.printf("*                    hardware simulate run with wokwi simulate, it works with vscode \r\n");
    Serial.printf("* Hardware     :     tiny32_v3 and tiny32_v4\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     11/09/2023\r\n");
    Serial.printf("* Revision     :     %s\r\n", version);
    Serial.printf("* Rev1.0       :     Origital\r\n");
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     admin@innovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 82-308-3299\r\n");
    Serial.printf("***********************************************************************/\r\n");
}

/**************************************/
/*        define object variable      */
/**************************************/
tiny32_v3 mcu;

/**************************************/
/*            GPIO define             */
/**************************************/

/**************************************/
/*       Constand define value        */
/**************************************/
#define WDT_TIMEOUT 10

/**************************************/
/*       eeprom address define        */
/**************************************/

/**************************************/
/*        define global variable      */
/**************************************/

/**************************************/
/*           define function          */
/**************************************/

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
    Serial.print("Configuring WDT...");
    esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
    esp_task_wdt_add(NULL);
    Serial.println("done");
    mcu.TickBuildinLED(0.5);
    mcu.TickBlueLED(1);
    mcu.TickRedLED(1.5);
    mcu.buzzer_beep(2);
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: loop process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void loop()
{
    esp_task_wdt_reset();
    vTaskDelay(100);
}

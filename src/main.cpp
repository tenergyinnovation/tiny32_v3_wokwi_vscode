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
#include <LiquidCrystal_I2C.h>

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
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display


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

    lcd.init(); // initialize the lcd
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("*    TENERGY   *");
    lcd.setCursor(0,1);
    lcd.print("*   INNOVATION *");

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
    static int16_t _count = 10;
    esp_task_wdt_reset();
    vTaskDelay(500);
    if(mcu.Sw1())
    {
        mcu.buzzer_beep(1);
        while(mcu.Sw1());
        _count++;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("* LCD Example *");
        lcd.setCursor(0,1);
        lcd.printf("count = %d",_count);
    }

    if(mcu.Sw2())
    {
        mcu.buzzer_beep(1);
        while(mcu.Sw2());
        _count--;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("* LCD Example *");
        lcd.setCursor(0,1);
        lcd.printf("count = %d",_count);
    }

    if(_count > 15)
    {
        mcu.buzzer_beep(2);
        lcd.setCursor(0,0);
        lcd.clear();
        lcd.print("* Relay: ON *");
        mcu.Relay(1);
    }
    else
    {
        mcu.Relay(0);
    }

}

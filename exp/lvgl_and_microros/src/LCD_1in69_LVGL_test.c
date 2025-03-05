/*****************************************************************************
* | File        :   LCD_Test.h
* | Author      :   Waveshare team
* | Function    :   test Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-06-27
* | Info        :   
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "LCD_test.h"
  
int press_time = 0;

int LCD_1in69_LVGL_Test(void)
{
    if (DEV_Module_Init() != 0)
    {
        return -1;
    } 

    printf("LCD_1in69_LCGL_test Demo\r\n");
    /*Init LCD*/
    LCD_1IN69_Init(VERTICAL);
    LCD_1IN69_Clear(WHITE);
    DEV_SET_PWM(60);
    /*Init RTC*/
    PCF85063A_Init();
    /*Init touch screen*/ 
    CST816S_init(CST816S_ALL_Mode);
    /*Init IMU*/
    QMI8658_init();
    /*Init LVGL*/
    LVGL_Init();
    Widgets_Init();

    while(1)
    {
      lv_task_handler();
      DEV_Delay_ms(5); 
      if(DEV_Digital_Read(PWR_KEY_PIN) == 0)
      {
        press_time++;
        if(press_time > 300)//shutdown  
        {
          press_time = 0;
          DEV_Digital_Write(BAT_PWR_PIN, 0);
        }
      }
      else
      {
        press_time = 0;
      }
    }
    

    DEV_Module_Exit();
    return 0;
}

#pragma once
#include <Arduino.h>

#ifndef LCD_WIDTH
#define LCD_WIDTH 20
#endif

class Scroll_Lcd_Text
{
  private:
    // for scolling text
    int Li          = LCD_WIDTH;
    int Lii         = 0;
    int Ri          = -1;
    int Rii         = -1;

  public:
//    Scroll_Lcd_Text();
    String Scroll_LCD_Left(String StrDisplay);
    String Scroll_LCD_Right(String StrDisplay);

    void Clear_Scroll_LCD_Right();
    void Clear_Scroll_LCD_Left();
};

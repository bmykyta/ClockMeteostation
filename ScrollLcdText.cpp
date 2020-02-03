#include "ScrollLcdText.h"

String Scroll_Lcd_Text::Scroll_LCD_Left(String StrDisplay) 
{
  String result;
  String StrProcess = "                    " + StrDisplay + "                    ";
  result = StrProcess.substring(Li, Lii);
  Li++;
  Lii++;
  if (Li > StrProcess.length()) {
    Li = LCD_WIDTH;
    Lii = 0;
  }
  
  return result;
}

void Scroll_Lcd_Text::Clear_Scroll_LCD_Left() 
{
  Li = LCD_WIDTH;
  Lii = 0;
}

String Scroll_Lcd_Text::Scroll_LCD_Right(String StrDisplay) 
{
  String result;
  String StrProcess = "                    " + StrDisplay + "                    ";
  if (Rii < 1) {
    Ri  = StrProcess.length();
    Rii = Ri - LCD_WIDTH;
  }
  result = StrProcess.substring(Rii, Ri);
  Ri--;
  Rii--;
  
  return result;
}

void Scroll_Lcd_Text::Clear_Scroll_LCD_Right() 
{
  Ri = -1;
  Rii = -1;
}

#ifndef USER_APP_H
#define USER_APP_H

#include "lcd_touch_bsp.h"

LcdTouchPanel Custom_GetLcdTouchPanel(void);

void Custom_TouchLoopTest(uint16_t x,uint16_t y);
void user_app_init(void);
void user_ui_init(void);

#endif
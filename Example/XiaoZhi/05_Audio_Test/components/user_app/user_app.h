#ifndef USER_APP_H
#define USER_APP_H

#include "lcd_touch_bsp.h"

LcdTouchPanel Custom_GetLcdTouchPanel(void);

void user_app_init(void);
void user_ui_init(void);

#endif
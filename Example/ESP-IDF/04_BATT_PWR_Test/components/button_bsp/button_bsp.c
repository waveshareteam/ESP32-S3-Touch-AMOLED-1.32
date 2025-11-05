#include <stdio.h>
#include "button_bsp.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "multi_button.h"

EventGroupHandle_t boot_groups;
EventGroupHandle_t pwr_groups;

static Button button1;      // 申请按键
#define BOOT_KEY_PIN 0      // 实际的GPIO
#define BOOT_ID 1           // 按键的ID
#define BOOT_Active 0       // 有效电平

static Button button2;      // 申请按键
#define PWR_KEY_PIN 17       // 实际的GPIO
#define PWR_ID 2            // 按键的ID
#define PWR_Active 0        // 有效电平

/*******************回调事件声明***************/

static void on_pwr_single_click(Button *btn_handle);
static void on_pwr_double_click(Button *btn_handle);
static void on_pwr_long_press_start(Button *btn_handle);
static void on_pwr_press_up(Button *btn_handle);

static void on_boot_single_click(Button *btn_handle);
static void on_boot_double_click(Button *btn_handle);
static void on_boot_long_press_start(Button *btn_handle);
static void on_boot_long_press_hold(Button *btn_handle);

/*********************************************/

static void clock_task_callback(void *arg) {
    button_ticks();
}

static uint8_t read_button_GPIO(uint8_t button_id) 
{
    switch (button_id) {
    case BOOT_ID:
        return gpio_get_level(BOOT_KEY_PIN);
    case PWR_ID:
        return gpio_get_level(PWR_KEY_PIN);
    default:
        break;
    }
    return 1;
}

static void gpio_init(void) {
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type     = GPIO_INTR_DISABLE;
    gpio_conf.mode          = GPIO_MODE_INPUT;
    gpio_conf.pin_bit_mask  = ((uint64_t) 0x01 << BOOT_KEY_PIN) | ((uint64_t) 0x01 << PWR_KEY_PIN);
    gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}

void user_button_init(void) {
    boot_groups = xEventGroupCreate();
    pwr_groups  = xEventGroupCreate();
    gpio_init();

    button_init(&button1, read_button_GPIO, BOOT_Active, BOOT_ID);              // 初始化 初始化对象 回调函数 触发电平 按键ID
    button_attach(&button1, BTN_SINGLE_CLICK, on_boot_single_click);            // 单击事件
    button_attach(&button1, BTN_LONG_PRESS_START, on_boot_long_press_start);    // 长按
    button_attach(&button1, BTN_PRESS_REPEAT, on_boot_double_click);            // 双击
    button_attach(&button1, BTN_LONG_PRESS_HOLD,on_boot_long_press_hold);       //长按一直触发

    button_init(&button2, read_button_GPIO, PWR_Active, PWR_ID);                // 初始化 初始化对象 回调函数 触发电平 按键ID
    button_attach(&button2, BTN_SINGLE_CLICK, on_pwr_single_click);             // 单击事件
    button_attach(&button2, BTN_DOUBLE_CLICK, on_pwr_double_click);             // 双击事件
    button_attach(&button2, BTN_LONG_PRESS_START, on_pwr_long_press_start);     // 长按事件
    button_attach(&button2, BTN_PRESS_UP, on_pwr_press_up);                     // 弹起

    const esp_timer_create_args_t clock_tick_timer_args = {
        .callback = &clock_task_callback,
        .name     = "clock_task",
        .arg      = NULL,
    };
    esp_timer_handle_t clock_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&clock_tick_timer_args, &clock_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(clock_tick_timer, 1000 * 5)); // 5ms
    button_start(&button2);                                                // 启动按键
    button_start(&button1);                                                // 启动按键
}


/*单击*/
static void on_pwr_single_click(Button *btn_handle) {
    xEventGroupSetBits(pwr_groups, set_bit_button(0));
}

/*双击*/
static void on_pwr_double_click(Button *btn_handle) {
    xEventGroupSetBits(pwr_groups, set_bit_button(1));
}

/*长按*/
static void on_pwr_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(pwr_groups, set_bit_button(2));
}

static void on_pwr_press_up(Button *btn_handle) {
    xEventGroupSetBits(pwr_groups, set_bit_button(3));
}

/*boot button*/

/*单击*/
static void on_boot_single_click(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(0));
}

/*双击*/
static void on_boot_double_click(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(1));
}

/*长按*/
static void on_boot_long_press_start(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(2));
}

static void on_boot_long_press_hold(Button *btn_handle) {
    xEventGroupSetBits(boot_groups, set_bit_button(3));
}

/*其他封装函数*/
uint8_t user_button_get_repeat_count(void) {
    return (button_get_repeat_count(&button2));
}

uint8_t user_boot_get_repeat_count(void) {
    return (button_get_repeat_count(&button1));
}

/*
事件:
SINGLE_CLICK :单击
DOUBLE_CLICK :双击
PRESS_DOWN :按下
PRESS_UP :弹起事件
PRESS_REPEAT :重复按下
LONG_PRESS_START :长按触发一次
LONG_PRESS_HOLD :长按一直触发
*/

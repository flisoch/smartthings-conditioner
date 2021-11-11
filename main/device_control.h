
#define GPIO_OUTPUT_MAINLED 2
#define GPIO_INPUT_SWITCH 12
#define GPIO_GND 13 
#define BUTTON_DEBOUNCE_TIME_MS 20


enum switch_onoff_state
{
    SWITCH_OFF = 1,
    SWITCH_ON = 0,
};


enum led_gpio_state
{
    LED_ON = 1,
    LED_OFF = 0,
};

void change_switch_state(int switch_state);
void iot_gpio_init(void);
int get_button_event();
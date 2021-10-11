#include "device_control.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include <stdbool.h>

// static temperature_state_t *temperature_state;

void change_switch_state(int switch_state)
{
	if (switch_state == SWITCH_OFF)
	{
		gpio_set_level(GPIO_OUTPUT_MAINLED, LED_OFF);
	}
	else
	{
		gpio_set_level(GPIO_OUTPUT_MAINLED, LED_ON);
	}
}


int get_button_event()
{
	static uint32_t button_last_state = SWITCH_OFF;
	uint32_t gpio_level = SWITCH_OFF;

	gpio_level = gpio_get_level(GPIO_INPUT_SWITCH);
	if (button_last_state != gpio_level)
	{
		printf("BTN LAST STATE(%d) != gpio level(%d)\n", button_last_state, gpio_level);
		/* wait debounce time to ignore small ripple of currunt */
		vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_TIME_MS));
		gpio_level = gpio_get_level(GPIO_INPUT_SWITCH);
		if (button_last_state != gpio_level)
		{
			printf("Button event, val: %d, \n", gpio_level);
			button_last_state = gpio_level;
			if (gpio_level == SWITCH_ON)
			{
				return 1;
			}
		}
	}

	return 0;
}

void iot_gpio_init(void)
{
	// esp sdk specific
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = 1 << GPIO_OUTPUT_MAINLED;
	io_conf.pull_down_en = 1;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
	io_conf.intr_type = GPIO_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = 1 << GPIO_INPUT_SWITCH;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	gpio_install_isr_service(0);
	gpio_set_level(GPIO_OUTPUT_MAINLED, LED_ON);
}

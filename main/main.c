#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "st_dev.h"
#include "device_control.h"
#include "caps_switch.h"
#include "caps_coolingTemperatureSetpoint.h"


// onboarding_config_start is null-terminated string
extern const uint8_t onboarding_config_start[] asm("_binary_onboarding_config_json_start");
extern const uint8_t onboarding_config_end[] asm("_binary_onboarding_config_json_end");

// device_info_start is null-terminated string
extern const uint8_t device_info_start[] asm("_binary_device_info_json_start");
extern const uint8_t device_info_end[] asm("_binary_device_info_json_end");

static iot_status_t g_iot_status = IOT_STATUS_IDLE;
static iot_stat_lv_t g_iot_stat_lv;

IOT_CTX *ctx = NULL;

// QueueHandle_t temperature_events_q = NULL;

static caps_switch_data_t *cap_switch_data;
static caps_coolingTemperatureSetpoint_data_t *cap_coolingSetpoint_data;

double cooling_temperature = 0;

static void iot_noti_cb(iot_noti_data_t *noti_data, void *noti_usr_data)
{
    printf("Notification message received\n");
    if (noti_data->type == IOT_NOTI_TYPE_DEV_DELETED)
    {
        printf("[device deleted]\n");
    }
    else if (noti_data->type == IOT_NOTI_TYPE_RATE_LIMIT)
    {
        printf("[rate limit] Remaining time:%d, sequence number:%d\n",
               noti_data->raw.rate_limit.remainingTime, noti_data->raw.rate_limit.sequenceNumber);
    }
}
static int get_switch_state(void)
{
    const char *switch_value = cap_switch_data->get_switch_value(cap_switch_data);
    int switch_state = SWITCH_OFF;

    if (!switch_value)
    {
        return -1;
    }

    if (!strcmp(switch_value, caps_helper_switch.attr_switch.value_on))
    {
        switch_state = SWITCH_ON;
    }
    else if (!strcmp(switch_value, caps_helper_switch.attr_switch.value_off))
    {
        switch_state = SWITCH_OFF;
    }
    return switch_state;
}

static void cap_switch_cmd_cb(struct caps_switch_data *caps_data)
{
    int switch_state = get_switch_state();
    change_switch_state(switch_state);
}

static void cap_cooling_temp_cmd_cb(struct caps_coolingTemperatureSetpoint_data *caps_data)
{   
    cooling_temperature = caps_data->get_value(caps_data);
}

static void capability_init()
{
    cap_switch_data = caps_switch_initialize(ctx, "main", NULL, NULL);
    if (cap_switch_data)
    {
        cap_switch_data->cmd_on_usr_cb = cap_switch_cmd_cb;
        cap_switch_data->cmd_off_usr_cb = cap_switch_cmd_cb;

        cap_switch_data->set_switch_value(cap_switch_data, caps_helper_switch.attr_switch.value_off);
    }

    cap_coolingSetpoint_data = caps_coolingTemperatureSetpoint_initialize(ctx, "main", NULL, NULL);
    if (cap_coolingSetpoint_data)
    {
        cap_coolingSetpoint_data->cmd_setCoolingTemperature_usr_cb = cap_cooling_temp_cmd_cb;
        cap_coolingSetpoint_data->set_unit(cap_coolingSetpoint_data, caps_helper_thermostatCoolingSetpoint.attr_coolingSetpoint.unit_C);
    }
}

static void iot_status_cb(iot_status_t status,
                          iot_stat_lv_t stat_lv, void *usr_data)
{
    g_iot_status = status;
    g_iot_stat_lv = stat_lv;

    printf("status: %d, stat: %d\n", g_iot_status, g_iot_stat_lv);
}

static void connection_start(void)
{
    iot_pin_t *pin_num = NULL;
    int err;

    // process on-boarding procedure. There is nothing more to do on the app side than call the API.
    err = st_conn_start(ctx, (st_status_cb)&iot_status_cb, IOT_STATUS_ALL, NULL, pin_num);
    if (err)
    {
        printf("fail to start connection. err:%d\n", err);
    }
}

static void app_main_task(void *arg)
{

    for (;;)
    {
        if (get_button_event())
        {
            cap_switch_data->set_switch_value(cap_switch_data, caps_helper_switch.attr_switch.value_off);
            cap_switch_data->attr_switch_send(cap_switch_data);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    unsigned char *onboarding_config = (unsigned char *)onboarding_config_start;
    unsigned int onboarding_config_len = onboarding_config_end - onboarding_config_start;
    unsigned char *device_info = (unsigned char *)device_info_start;
    unsigned int device_info_len = device_info_end - device_info_start;
    int iot_err;
    // st_dev.h
    ctx = st_conn_init(onboarding_config, onboarding_config_len, device_info, device_info_len);
    if (ctx != NULL)
    {
        iot_err = st_conn_set_noti_cb(ctx, iot_noti_cb, NULL);
        if (iot_err)
            printf("fail to set notification callback function\n");
    }
    else
    {
        printf("fail to create the iot_context\n");
    }

    // caps_*.h
    capability_init();

    // device.h
    iot_gpio_init();
    connection_start();

    // device input handling
    xTaskCreate(app_main_task, "app_main_task", 4096, NULL, 10, NULL);
}
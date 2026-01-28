/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <dk_buttons_and_leds.h>

//initialize a logger module
LOG_MODULE_REGISTER(Projekt_studia, LOG_LEVEL_DBG);
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BLINK_LED	DK_LED1
#define CONNECTED_LED	DK_LED2
#define USER_WRITE_LED	DK_LED3
#define USER_BUTTON	DK_BTN1_MSK
#define BLINK_INTERVAL 500

//define UUID
#define BT_UUID_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x00000001, 0xe239, 0x4152, 0x836d, 0x450bf5ff856c)
#define BT_UUID_BUTTON_VAL \
	BT_UUID_128_ENCODE(0x00000002, 0xe239, 0x4152, 0x836d, 0x450bf5ff856c)
#define BT_UUID_LED_VAL \
	BT_UUID_128_ENCODE(0x00000003, 0xe239, 0x4152, 0x836d, 0x450bf5ff856c)

#define BT_UUID_SERVICE BT_UUID_DECLARE_128(BT_UUID_SERVICE_VAL)
#define BT_UUID_BUTTON BT_UUID_DECLARE_128(BT_UUID_BUTTON_VAL)
#define BT_UUID_LED	BT_UUID_DECLARE_128(BT_UUID_LED_VAL)

static struct bt_gatt_indicate_params indicate_params;
static struct k_work adv_work;
static bool app_button_state = false;


void led_callback(uint8_t led_state){
	dk_set_led(USER_WRITE_LED, (uint32_t)led_state);
}

//write_led function implementation
static ssize_t write_led(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     const void *buf, uint16_t len,
					     uint16_t offset, uint8_t flags){
	LOG_INF("Write_led function, attribute handle: %u\n", attr->handle);
	uint8_t value = *(uint8_t *)buf;
	if(value == 0x01 || value == 0x00){
		led_callback(value);
	}else{
		LOG_WRN("Incorrect value!\n");
		return 1;
	}
	return len;
}


//create a service and characteristics
BT_GATT_SERVICE_DEFINE(my_service, BT_GATT_PRIMARY_SERVICE(BT_UUID_SERVICE),
	BT_GATT_CHARACTERISTIC(BT_UUID_BUTTON, BT_GATT_CHRC_INDICATE, BT_GATT_PERM_READ, NULL, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), //CCCD must be read and write
	BT_GATT_CHARACTERISTIC(BT_UUID_LED, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_led, NULL),
);

//Advertising parameters and data
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONN |
	 BT_LE_ADV_OPT_USE_IDENTITY),
	800, 
	801, 
	NULL); 
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void adv_work_handler(struct k_work *work){
	if(bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0) != 0){
		LOG_ERR("Advertising error!\n");
		return;
	}else{
		LOG_INF("Advertising started!\n");
	}
}

static void advertising_start(void){
	//submit work to system workqueue
	k_work_submit(&adv_work);
}

//create indication function for button change
int user_button_indicate(bool button_state){
	indicate_params.attr = &my_service.attrs[1];
	indicate_params.func = NULL;
	indicate_params.destroy = NULL;
	indicate_params.data = &app_button_state;
	indicate_params.len = sizeof(app_button_state);
	return bt_gatt_indicate(NULL, &indicate_params);
}

//Callback when the button state is changed
static void button_changed(uint32_t button_state, uint32_t has_changed){
	if(USER_BUTTON & has_changed){
		app_button_state = !app_button_state;
		user_button_indicate(app_button_state);
	}
}

//define connections callbacks
void on_connected(struct bt_conn *conn, uint8_t err){
	LOG_INF("Connected\n");
	dk_set_led_on(CONNECTED_LED);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason){
	LOG_INF("Disconnected from the device, reason: %u\n", reason);
	dk_set_led_off(CONNECTED_LED);
}

void on_recycled(void){
	LOG_INF("Disconnect complete, object returned to the pool\n");
	LOG_INF("Starting advertising again!\n");
	advertising_start();
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.recycled = on_recycled,
};


int main(void)
{
	bool blink_state = false;
	if(dk_leds_init() != 0){
		LOG_ERR("LEDs init failed\n");
	}
	if(dk_buttons_init(button_changed) != 0 ){
		LOG_ERR("Buttons init failed\n");
	}
	
	if(bt_enable(NULL) != 0){
		LOG_ERR("Bluetooth init failed\n");
	}

	bt_conn_cb_register(&connection_callbacks);
	k_work_init(&adv_work, adv_work_handler);
	advertising_start();
	
	while(1){
		dk_set_led(BLINK_LED, blink_state);
		blink_state = !blink_state;
		k_sleep(K_MSEC(BLINK_INTERVAL));
	}

}

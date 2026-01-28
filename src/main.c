/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
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



static struct k_work adv_work;
static bool app_button_state = false;

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
		exit(1);
	}else{
		LOG_INF("Advertising started!\n");
	}
}

static void advertising_start(void){
	//submit work to system workqueue
	k_work_submit(&adv_work);
}

//Callback when the button state is changed
static void button_changed(uint32_t button_state, uint32_t has_changed){
	if(USER_BUTTON & has_changed){
		app_button_state = !app_button_state;
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
		dk_set_led(BLINK_LED, !blink_state);
		k_sleep(K_MSEC(BLINK_INTERVAL));
	}

}

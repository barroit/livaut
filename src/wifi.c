#include "wifi.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "wifi-cred.h"
#include <string.h>
#include "termio.h"

#define CE ESP_ERROR_CHECK_WITHOUT_ABORT

#define MAXIMUM_RETRY 5

#define START_FAILURE BIT1
#define START_SUCCESS BIT0

static EventGroupHandle_t wifi_state;
static esp_netif_t *netif;
static esp_event_handler_instance_t wifi_event_handle;
static esp_event_handler_instance_t ip_event_handle;

static size_t retry_count;
static int is_disconnect_call;

static int is_wifi_init_done;
static char router_address[16];

#define TAG "wifi_init"

static void handle_wifi_event(void *, esp_event_base_t, int32_t id, void *)
{
	switch (id) {
	case WIFI_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		if (is_disconnect_call)
			break;
		if (retry_count++ < MAXIMUM_RETRY) {
			if (retry_count % 2)
				info(TAG, "trying to connect to ‘%s’",
				     WIFI_SSID);
			esp_wifi_connect();
		} else {
			xEventGroupSetBits(wifi_state, START_FAILURE);
		}
	}
}

static void handle_ip_event(void *, esp_event_base_t, int32_t id, void *ctx)
{
	switch (id) {
	case IP_EVENT_STA_GOT_IP:
		retry_count = 0;
		xEventGroupSetBits(wifi_state, START_SUCCESS);

		ip_event_got_ip_t *data = ctx;
		snprintf(router_address, 16, IPSTR, IP2STR(&data->ip_info.gw));
	}
}

/* too long, we define an alias */
#define EEHIR esp_event_handler_instance_register
#define EEHIU esp_event_handler_instance_unregister

static int config_event_handle(void)
{
	int err;

	err = CE(esp_event_loop_create_default());
	if (err)
		return 1;

	netif = esp_netif_create_default_wifi_sta();

	err = CE(EEHIR(WIFI_EVENT, ESP_EVENT_ANY_ID, handle_wifi_event,
		       NULL, &wifi_event_handle));
	if (err)
		return 1;

	err = CE(EEHIR(IP_EVENT, IP_EVENT_STA_GOT_IP, handle_ip_event,
		       NULL, &ip_event_handle));
	if (err)
	    	return 1;

	return 0;
}

static void reset_event_handle(void)
{
	esp_event_loop_delete_default();

	esp_netif_destroy_default_wifi(netif);

	EEHIU(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handle);

	EEHIU(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handle);
}

static int setup_wifi_config(void)
{
	int err;

	err = CE(esp_wifi_set_mode(WIFI_MODE_STA));
	if (err)
		return 1;

	wifi_scan_threshold_t thre_conf = {
		.authmode = WIFI_AUTH_WPA3_PSK,
	};

	wifi_sta_config_t sta_conf = {
		.ssid      = WIFI_SSID,
		.password  = WIFI_PASS,
		.threshold = thre_conf,
	};

	wifi_config_t wifi_conf = {
		.sta = sta_conf,
	};

	err = CE(esp_wifi_set_config(WIFI_IF_STA, &wifi_conf));
	if (err)
		return 1;

	return 0;
}

static int setup_wifi(void)
{
	int err;

	/*
	 * when ‘E (15181) wifi:NAN WiFi stop’ gets fixed, we
	 * replace ESP_LOG_NONE with ESP_LOG_ERROR
	 */
	esp_log_level_set("wifi", ESP_LOG_NONE);

	print_task_avail_stack("wifi_init", NULL);

	wifi_state = xEventGroupCreate();

	err = CE(esp_netif_init());
	if (err)
		goto err_netif_init;

	err = config_event_handle();
	if (err)
		goto err_evt_hand_init;

	wifi_init_config_t conf = WIFI_INIT_CONFIG_DEFAULT();
	err = CE(esp_wifi_init(&conf));
	if (err)
		goto err_wifi_init;

	err = setup_wifi_config();
	if(err)
		goto err_wifi_set_conf;

	err = CE(esp_wifi_start());
	if (err)
		goto err_wifi_start;

	EventBits_t bit = xEventGroupWaitBits(wifi_state,
					      START_FAILURE | START_SUCCESS,
					      pdFALSE, pdFALSE,
					      portMAX_DELAY);
	if (bit != START_SUCCESS) {
		warning(TAG, "unable to connect to ‘%s’", WIFI_SSID);
		goto err_wifi_connect;
	}

	is_wifi_init_done = 1;
	return 0;

err_wifi_connect:
	esp_wifi_disconnect();
	esp_wifi_stop();
	is_disconnect_call = 1;
err_wifi_start:
	/* no need to clean up config file */
err_wifi_set_conf:
	esp_wifi_deinit();
err_wifi_init:
	reset_event_handle();
err_evt_hand_init:
	esp_netif_deinit();
err_netif_init:
	return 1;
}

typedef void (*setup_wifi_task_cb)(int err);

void setup_wifi_task(void *cb)
{
	int err;

	err = setup_wifi();

	((setup_wifi_task_cb)cb)(err);

	vTaskDelete(NULL);
}

int is_wifi_connected(void)
{
	return is_wifi_init_done;
}

const char *get_router_address(void)
{
	return router_address;
}

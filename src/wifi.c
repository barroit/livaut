#include "wifi.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include <string.h>
#include "termio.h"

#define RETRY_GIVEUP 5

#define MINIMAL_AP_RSSI -60

#define STA2AP_START_FAILURE BIT1
#define STA2AP_START_SUCCESS BIT0
#define STA2AP_STATES        (STA2AP_START_FAILURE | STA2AP_START_SUCCESS)

static EventGroupHandle_t sta2ap_state;
static esp_netif_t *netif;
static esp_event_handler_instance_t wifi_event_handle;
static esp_event_handler_instance_t ip_event_handle;

static size_t retry_count;
static int is_disconnect_call;

static int is_communicating;
static char router_address[16];

#define TAG "wifi_init"

static void handle_wifi_event(void *, esp_event_base_t, int32_t id, void *)
{
	switch (id) {
	case WIFI_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		if (is_disconnect_call) {
			is_disconnect_call = 0;
			break;
		}

		if (retry_count++ < RETRY_GIVEUP) {
			if (retry_count % 2) {
				info(TAG, "trying to connect to ‘%s’",
				     CONFIG_WIFI_SSID);
			}
			esp_wifi_connect();
		} else {
			xEventGroupSetBits(sta2ap_state, STA2AP_START_FAILURE);
		}
	}
}

static void handle_ip_event(void *, esp_event_base_t, int32_t id, void *ctx)
{
	switch (id) {
	case IP_EVENT_STA_GOT_IP:
		retry_count = 0;
		xEventGroupSetBits(sta2ap_state, STA2AP_START_SUCCESS);

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

static int setup_sta_config(void)
{
	int err;

	err = CE(esp_wifi_set_mode(WIFI_MODE_STA));
	if (err)
		return 1;

	wifi_scan_threshold_t thre_conf = {
		.rssi     = MINIMAL_AP_RSSI,
		.authmode = WIFI_AUTH_WPA3_PSK,
	};

	wifi_sta_config_t sta_conf = {
		.ssid            = CONFIG_WIFI_SSID,
		.password        = CONFIG_WIFI_PASS,
		.threshold       = thre_conf,
		.sort_method     = WIFI_CONNECT_AP_BY_SECURITY,
	};

	wifi_config_t wifi_conf = {
		.sta = sta_conf,
	};

	err = CE(esp_wifi_set_config(WIFI_IF_STA, &wifi_conf));
	if (err)
		return 1;

	return 0;
}

static int init_sta(void)
{
	int err;

	sta2ap_state = xEventGroupCreate();

	/*
	 * when ‘E (15181) wifi:NAN WiFi stop’ gets fixed, we
	 * replace ESP_LOG_NONE with ESP_LOG_ERROR
	 */
	esp_log_level_set("wifi", ESP_LOG_NONE);

	print_task_avail_stack("wifi_init", NULL);

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

	err = setup_sta_config();
	if(err)
		goto err_wifi_set_conf;

	return 0;

err_wifi_set_conf:
	esp_wifi_deinit();
err_wifi_init:
	reset_event_handle();
err_evt_hand_init:
	esp_netif_deinit();
err_netif_init:
	return 1;
}

int is_sta2ap_connected(void)
{
	return is_communicating;
}

void disconnect_sta2ap(void)
{
	is_disconnect_call = 1;
	esp_wifi_disconnect();

	esp_wifi_stop();
	is_communicating = 0;
}

int connect_sta2ap(void)
{
	int err;

	err = CE(esp_wifi_start());
	if (err)
		goto err_wifi_start;

	EventBits_t bit = xEventGroupWaitBits(sta2ap_state, STA2AP_STATES,
					      pdFALSE, pdFALSE, portMAX_DELAY);
	xEventGroupClearBits(sta2ap_state, STA2AP_STATES);

	if (bit != STA2AP_START_SUCCESS) {
		warning(TAG, "unable to connect to ‘%s’", CONFIG_WIFI_SSID);
		goto err_wifi_connect;
	}

	is_communicating = 1;
	return 0;

err_wifi_start:
	disconnect_sta2ap();
err_wifi_connect:
	return 1;
}

typedef void (*setup_wifi_task_cb)(int err);

void make_sta2ap_connection(void *cb)
{
	int err;

	if (*CONFIG_WIFI_SSID == 0 || *CONFIG_WIFI_PASS == 0)
		warning(TAG, "empty ssid or pass");

	err = init_sta();
	if (err)
		goto atconnect;

	err = connect_sta2ap();
	if (err)
		goto atconnect;

atconnect:
	((setup_wifi_task_cb)cb)(err);

	vTaskDelete(NULL);
}

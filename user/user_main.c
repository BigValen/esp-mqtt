#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "dht.h"
#include "ds18b20.h"
#include "gpio.h"
#include "user_interface.h"

MQTT_Client mqttClient;

static volatile os_timer_t blink_timer;
static volatile os_timer_t blank_timer;

void blink_timerfunc(void *arg)
{
  if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2) {
    //Set GPIO2 to LOW
    gpio_output_set(0, BIT2, BIT2, 0);
  } else {
    //Set GPIO2 to HIGH
    gpio_output_set(BIT2, 0, BIT2, 0);
  }
}

void blank_timerfunc(void *arg)
{
  // Turn the LED off
  INFO("Turn off LED\r\n");
  gpio_output_set(0, BIT2, BIT2, 0);
}



void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	}
}
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	MQTT_Subscribe(client, "/house/info", 0);
	MQTT_Subscribe(client, "/house/light/0", 0);
	MQTT_Publish(client, "/sensors/temperature/0", "init", sizeof("init"), 0, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
  // Turn the LED off
  char temperature[20], success[20];
  INFO("reading temp\r\n");
  struct sensor_reading* reading;
  reading = readDS18B20();
  INFO("Got: success: %d, temp %18f", reading->success, reading->temperature);
  os_sprintf(temperature, sizeof(temperature), "%18f", reading->temperature);
  os_sprintf(success, sizeof(success), "%18f", reading->success);
	MQTT_Publish(client, "/sensors/temperature/0", temperature, os_strlen(temperature), 0, 0);
	MQTT_Publish(client, "/sensors/success/0", success, os_strlen(success), 0, 0);
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char topicBuf[64], dataBuf[64];
	MQTT_Client* client = (MQTT_Client*)args;


	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;
  if(!(strcmp("/house/light/0", topicBuf))) {
    INFO("House light!\r\n");
    if(!(strcmp("on", dataBuf))) {
      INFO("Turn LED on!\r\n");
      //Set GPIO2 to HIGH / LED ON
      gpio_output_set(BIT2, 0, BIT2, 0);
      // Arm a timer to switch off the LED in 0.6s
      os_timer_arm(&blank_timer, 2000, 0);
    }
  }

	INFO("MQTT topic: %s, data: %s \r\n", topicBuf, dataBuf);
}


void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	INFO("\r\nSystem starting ...\r\n");
	os_delay_us(1000000);
  os_timer_disarm(&blank_timer);
  os_timer_setfn(&blank_timer, (os_timer_func_t *)blank_timerfunc, NULL);

  //Set GPIO2 to output mode (LED)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  //Set GPIO2 low / LED OFF
  gpio_output_set(0, BIT2, BIT2, 0);

	CFG_Load();

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, SEC_NONSSL);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	INFO("\r\nSystem started!\r\n");
}

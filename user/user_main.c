#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"

MQTT_Client mqttClient;

static volatile os_timer_t switch_timer;
static volatile os_timer_t led_timer;


void switch_timerfunc(void *arg)
{
//  os_timer_disarm(&switch_timer);
  INFO("TIMER0: Turning off switch\r\n");
  //Set GPIO2 & GPIO0 to LOW / LED OFF
  gpio_output_set(0, BIT0|BIT2, BIT0|BIT2, 0);
}

void led_timerfunc(void *arg)
{
  // ARG = BIT2 etc.
  // Turn the LED off
  INFO("TIMER2: Turn off GPIO %d\r\n", arg);
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
  MQTT_Subscribe(client, "/house/door/0", 0);
  MQTT_Publish(client, "/sensors/temperature/0", "init", sizeof("init"), 0,
      0);
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
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len,
  const char *data, uint32_t data_len)
{
  char topicBuf[64], dataBuf[64];
  MQTT_Client* client = (MQTT_Client*)args;

  os_memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  os_memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;
  if(!(strcmp("/house/light/0", topicBuf))) {
    if(!(strcmp("on", dataBuf))) {
      INFO("Turn light on!\r\n");
      //Set GPIO2 to HIGH / LED ON
      gpio_output_set(BIT2, 0, BIT2, 0);
      // Arm a timer to switch off the LED in 5s
      os_timer_disarm(&led_timer);
      os_timer_arm(&led_timer, 500, 0);
    } else {
      INFO("LED got unknown command %s\r\n", dataBuf);
    }
  } else if (!(strcmp("/house/door/0", topicBuf))) {
    if (!(strcmp("toggle", dataBuf))) {
      INFO("Flipping Switch\r\n");
      //Set GPIO0 to HIGH / LED ON
      gpio_output_set(BIT0|BIT2, 0, BIT0|BIT2, 0);
      // Arm a timer to switch off the LED in 0.6s
      os_timer_disarm(&switch_timer);
      os_timer_arm(&switch_timer, 4000, 0);
    } else {
      INFO("House door got unknown command %s\r\n", dataBuf);
    }
  }

  INFO("MQTT topic: %s, data: %s \r\n", topicBuf, dataBuf);
}


void user_init(void)
{
  uart_init(BIT_RATE_115200, BIT_RATE_115200);
  INFO("\r\nSystem starting ...\r\n");
  os_delay_us(1000000);
  os_timer_disarm(&switch_timer); os_timer_disarm(&led_timer);
  os_timer_setfn(&switch_timer, (os_timer_func_t *)switch_timerfunc, BIT0);
  os_timer_setfn(&led_timer, (os_timer_func_t *)led_timerfunc, BIT2);

  //Set GPIO2 to output mode (LED)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  //Set GPIO2 low / LED OFF
  gpio_output_set(0, BIT2|BIT0, BIT2|BIT0, 0);

  CFG_Load();

  MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port,
      SEC_NONSSL);
  MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user,
      sysCfg.mqtt_pass, sysCfg.mqtt_keepalive);
  MQTT_OnConnected(&mqttClient, mqttConnectedCb);
  MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
  MQTT_OnPublished(&mqttClient, mqttPublishedCb);
  MQTT_OnData(&mqttClient, mqttDataCb);

  WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

  INFO("\r\nSystem started!\r\n");
}

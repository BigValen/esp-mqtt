#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define CFG_HOLDER	0x00FF55A4
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST			"nas.66redarches" //or "192.168.11.1"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		120	 /*second*/

#define MQTT_CLIENT_ID		"SENSOR%08X"
#define MQTT_USER			"sensor"
#define MQTT_PASS			"sensor"


#define STA_SSID "1OldbridgeClose-main"
#define STA_PASS "passwd"
#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

//#define CLIENT_SSL_ENABLE
#define DEFAULT_SECURITY	0

#define QUEUE_BUFFER_SIZE		 		2048

#endif

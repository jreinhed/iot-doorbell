/*
 * File: doorbell_pi.c
 * Authors: Johan Reinhed & Stijn Boons
 * Date: 2019-09-23
 * An MQTT client for our doorbell project.
 */
#include <stdio.h>
#include <mosquitto.h>
#include <wiringPi.h>

int buzzer_pin = 8;
int button_pin = 7;

void handle_doorbell(void)
{
	digitalWrite(buzzer_pin, HIGH);
	delay(1000);
	digitalWrite(buzzer_pin, LOW);

	int t = 0;
	while (t <= 50000000) { // Timeout of ~8 seconds
		t++;

		if (digitalRead(button_pin) == HIGH) {
			mosquitto_publish(mosq, NULL,
			    "Doorbell_1/feeds/reply", 3, "Yes", 2, true);
			break;
		}
	}
}

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if (message->payloadlen) {
		printf("%s %s\n", message->topic, message->payload);
	} else {
		printf("%s (null)\n", message->topic);
	}

	handle_doorbell();

	fflush(stdout);
}

void my_connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if (!result) {
		/* Subscribe to broker information topics on successful connect. */
		mosquitto_subscribe(mosq, NULL, "Doorbell_1/feeds/doorbell", 2);
	} else {
		fprintf(stderr, "Connect failed\n");
	}
}

void my_subscribe_callback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
	int i;

	printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for (i = 1; i < qos_count; i++) {
		printf(", %d", granted_qos[i]);
	}
	printf("\n");
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Print all log messages regardless of level. */
	printf("%s\n", str);
}

int main(int argc, char *argv[])
{
	wiringPiSetup();
	pinMode(buzzer_pin, OUTPUT);
	pinMode(button_pin, INPUT);

	int i;
	char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, clean_session, NULL);
	if (!mosq) {
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

	if (mosquitto_connect(mosq, host, port, keepalive)) {
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}

	mosquitto_loop_forever(mosq, -1, 1);

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return 0;
}

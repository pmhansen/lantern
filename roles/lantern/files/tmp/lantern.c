#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>
#include <jansson.h>
#include <time.h>

#define MQTT_BROKER_ADDRESS             "127.0.0.1"
#define MQTT_PORT 1883      
#define MQTT_KEEPALIVE_INTERVAL 60  
#define MQTT_RELAY_DRIVEWAY             "zigbee2mqtt/relay_driveway_light"
#define MQTT_RELAY_LIGHT_SHELF          "zigbee2mqtt/relay_light_shelf"
#define MQTT_SENSOR_TOPIC               "zigbee2mqtt/occupancy_sensor_driveway"
#define QOS 1                           // MQTT qos.
#define RETAIN 0                        // Do not retain message in queue.

#define LIGHT_OFF 100                   // lux
#define LIGHT_ON 10                     // lux
#define OCCUPANCY_DELAY 600             // seconds before turning lights off again.
#define OCCUPANCY_DELAY_SINCE_CHANGE 60 // seconds to disable occupancy since lights change.

struct Lantern {
    char relay_state[4];
    int battery;
    int illuminance;
    int illuminance_lux;
    int occupancy;
    double temperature;
    time_t change_timestamp;
    time_t occupancy_timestamp;
};

struct Lantern data;

void setRelayState(struct mosquitto *mosq, const char* state) {
    char msg[20];
    snprintf(msg, sizeof(msg), "{\"state\":\"%s\"}", state);

    /*
     * Sometimes the relay fails to receive state change.
     * As a remedy we try to send the message three times.
     */ 
    int i;
    char topic[50];
    for (i = 0; i < 3; i++) {
        snprintf(topic, sizeof(topic), "%s/set", MQTT_RELAY_DRIVEWAY);
        if (mosquitto_publish(mosq, NULL, topic, (int)strlen(msg), msg, QOS, RETAIN)) {
            fprintf(stderr, "Failed to publish message");
        }

        snprintf(topic, sizeof(topic), "%s/set", MQTT_RELAY_LIGHT_SHELF);
        if (mosquitto_publish(mosq, NULL, topic, (int)strlen(msg), msg, QOS, RETAIN)) {
            fprintf(stderr, "Failed to publish message");
        }
    }
}

void messageCallback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
    // Print the received message
    fprintf(stdout,"Received message on topic: %s\n", message->topic);
    fprintf(stdout, "Message: %s\n", (char *)message->payload);

    // Parse the json payload
    json_t *json;
    json_error_t error;
    json = json_loads((char *)message->payload, 0, &error);

    if (!json) {
        fprintf(stderr, "Error parsing json data: %s\n", error.text);
        return;
    }

    json_t *state_value = json_object_get(json, "state");
    if (state_value && ((state_value)->type == JSON_STRING)) {
        strncpy(data.relay_state, json_string_value(json_object_get(json, "state")), sizeof(data.relay_state));
    }

    json_t *lux_value = json_object_get(json, "illuminance_lux");
    if (lux_value && ((lux_value)->type == JSON_INTEGER)) {
        data.illuminance_lux = (int)json_integer_value(lux_value);
    }

    json_t *occupancy_value = json_object_get(json, "occupancy");
    if (occupancy_value && json_is_boolean(occupancy_value)) {
        data.occupancy = json_is_true(occupancy_value) ? 1 : 0;
    }

    time_t now;
    struct tm *local_time;
    time(&now);
    local_time = localtime(&now);

    fprintf(stdout, "lux: %d, hour: %d, occupancy: %d, relay_state: %s\n", 
        data.illuminance_lux, local_time->tm_hour, data.occupancy, data.relay_state);

    if (data.occupancy) {
        if (now - data.change_timestamp < OCCUPANCY_DELAY_SINCE_CHANGE) {
            fprintf(stderr, "light state just changed, ignoring occupancy detection.\n");
            return;
        }
        fprintf(stdout, "movement detected\n");
        data.occupancy_timestamp = now;
    } else {
        if (data.occupancy_timestamp > 0 && now - data.occupancy_timestamp > OCCUPANCY_DELAY) {
            fprintf(stdout, "movement delay ran out\n");
            data.occupancy_timestamp = 0;
        }

        // movement delay has not expired yet.
        if (data.occupancy_timestamp > 0) {
            return;
        }
    }

    // Turn on light if it is dark and time is before 23:00 or after 5:00 or movement is detected.
    if (data.illuminance_lux < LIGHT_ON && strcmp(data.relay_state, "OFF") == 0) { 
        if ((local_time->tm_hour < 23 && local_time->tm_hour >= 5) || data.occupancy) {
            fprintf(stdout, "turn lights on\n");
            data.change_timestamp = now;            
            setRelayState(mosq, "ON");
        }
    }
    // Turn off light if there is daylight or time is after 23:00 or time is before 5.
    else if (strcmp(data.relay_state, "ON") == 0) {
        if (data.illuminance_lux > LIGHT_OFF || local_time->tm_hour >= 23 || local_time->tm_hour < 5) {
            fprintf(stdout, "turn lights off\n");
            data.change_timestamp = now;            
            setRelayState(mosq, "OFF");
        }
    }

    // Cleanup json object
    json_decref(json);
}

int main() {
    struct mosquitto *mosq;
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);

    if (!mosq) {
        fprintf(stderr, "Error: Unable to initialize the Mosquitto library.\n");
        return EXIT_FAILURE;
    }

    mosquitto_message_callback_set(mosq, messageCallback);

    if (mosquitto_connect(mosq, MQTT_BROKER_ADDRESS, MQTT_PORT, MQTT_KEEPALIVE_INTERVAL)) {
        fprintf(stderr, "Unable to connect to the MQTT broker.\n");
        return EXIT_FAILURE;
    }

    mosquitto_subscribe(mosq, NULL, MQTT_RELAY_DRIVEWAY, 0);
    mosquitto_subscribe(mosq, NULL, MQTT_RELAY_LIGHT_SHELF, 0);
    mosquitto_subscribe(mosq, NULL, MQTT_SENSOR_TOPIC, 0);

    memset(&data, 0, sizeof(struct Lantern));
    strncpy(data.relay_state, "OFF", sizeof data.relay_state);
    data.illuminance_lux = 0;

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return EXIT_SUCCESS;
}

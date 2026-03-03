#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt_manager.h"
#include "sensor_manager.h"
#include "config.h"

static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

static unsigned long lastPublishMs      = 0;
static unsigned long lastReconnectMs    = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 5000;

/*
Attempt a single (non-blocking) reconnect to the MQTT broker.
Returns immediately if the back-off interval has not elapsed.
*/
static void tryReconnectMQTT()
{
    unsigned long now = millis();
    if (now - lastReconnectMs < RECONNECT_INTERVAL_MS)
    {
        return; // Back-off: do not spam reconnect attempts
    }
    lastReconnectMs = now;

    Serial.printf("[MQTT] Connecting to broker %s:%d ... ", MQTT_BROKER_IP, MQTT_BROKER_PORT);

    if (mqttClient.connect(MQTT_CLIENT_ID))
    {
        Serial.println("connected.");
    }
    else
    {
        Serial.printf("failed (rc=%d). Next attempt in %lus\n",
                      mqttClient.state(), RECONNECT_INTERVAL_MS / 1000);
    }
}

/*
Initialize the MQTT client with the broker address/port from config.h.
*/
void initMQTT()
{
    mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
    Serial.printf("[MQTT] Configured broker: %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
    Serial.printf("[MQTT] Topics: %s  %s  %s\n",
                  MQTT_TOPIC_SENSOR1, MQTT_TOPIC_SENSOR2, MQTT_TOPIC_TOTAL);
}

/*
Maintain MQTT connection and publish weight readings on schedule.
Must be called from a loop running on the network-capable core.
*/
void handleMQTT()
{
    // Skip entirely if WiFi is not in STA mode / not connected
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if (!mqttClient.connected())
    {
        tryReconnectMQTT();
        return; // Do not publish if not connected
    }

    mqttClient.loop();

    unsigned long now = millis();
    if (now - lastPublishMs < MQTT_PUBLISH_INTERVAL_MS)
    {
        return;
    }
    lastPublishMs = now;

    char buf[16];

    snprintf(buf, sizeof(buf), "%.3f", getCachedWeight1());
    mqttClient.publish(MQTT_TOPIC_SENSOR1, buf);

    snprintf(buf, sizeof(buf), "%.3f", getCachedWeight2());
    mqttClient.publish(MQTT_TOPIC_SENSOR2, buf);

    snprintf(buf, sizeof(buf), "%.3f", getCachedWeight());
    mqttClient.publish(MQTT_TOPIC_TOTAL, buf);

    Serial.printf("[MQTT] Published -> S1=%.3f kg  S2=%.3f kg  Total=%.3f kg\n",
                  getCachedWeight1(), getCachedWeight2(), getCachedWeight());
}

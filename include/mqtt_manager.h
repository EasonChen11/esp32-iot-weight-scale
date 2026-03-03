#pragma once

/*
Initialize the MQTT client and configure the broker endpoint defined in config.h.
Call once from setup() after initWiFi().

Parameters:
  none

Returns:
  void
*/
void initMQTT();

/*
Keep the MQTT connection alive and publish weight data on a timed interval.
Call repeatedly from the web/tasks loop (Core 0).

- Reconnects automatically if the broker is unreachable (non-blocking, 5s back-off).
- Publishes sensor1, sensor2, and total weight every MQTT_PUBLISH_INTERVAL_MS.

Parameters:
  none

Returns:
  void
*/
void handleMQTT();

#include "sensor_manager.h"
#include "HX711.h"
#include "config.h"
#include <Arduino.h>

static HX711 scale;
static float internal_cached_weight = 0.0;
static unsigned long last_read_time = 0;

#if SIMULATE_SENSOR
static float sim_weight = 0.0;
#endif

/*
Internal function to read weight from the sensor or simulator.
This is a private helper function that handles both real sensor readings
and simulated data based on the SIMULATE_SENSOR configuration.

Returns:
  float: The weight reading in kilograms, or -1.0 if sensor is not ready
*/
static float _doRead()
{
#if SIMULATE_SENSOR
    sim_weight += 0.05;
    if (sim_weight > 75.0)
        sim_weight = 10.0;
    return sim_weight;
#else
    if (!scale.is_ready())
        return -1.0;

    float raw = scale.get_units(5);

    if (abs(raw) < 0.01)
        raw = 0.0;

    return raw;
#endif
}

/*
Initialize the weight sensor with optional absolute zero offset.
If no saved offset is provided, the sensor will auto-tare on startup.

Parameters:
  savedOffset (long): The absolute zero-point offset value to apply.
                      If 0, the sensor will perform automatic tare. Default is 0.

Returns:
  void
*/
void initSensor(long savedOffset)
{
    Serial.println("[Sensor] savedOffset: " + String(savedOffset));
#if !SIMULATE_SENSOR
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    // scale.set_scale(LOADCELL_SCALE_FACTOR);
    Serial.println("[Sensor] Initializing weight sensor...");
    if (scale.wait_ready_timeout(2000)) {
        scale.set_scale(LOADCELL_SCALE_FACTOR);
        Serial.println("[Sensor] HX711 is ready and initialized");
    } else {
        Serial.println("[Sensor] error: HX711 not found or not responding");
        // init again with a delay to allow for sensor startup
        delay(1000);
        initSensor(savedOffset);
        return;
    }
    if (savedOffset != 0)
    {
        scale.set_offset(savedOffset);
        Serial.printf("[Sensor] Calibration offset applied: %ld\n", savedOffset);
    }
    else
    {
        scale.tare();
        Serial.println("[Sensor] No calibration found, performing auto-tare");
    }
#else
    Serial.println("[Sensor] Simulation mode activated");
    sim_weight = 10.0;
#endif
}

/*
Update the cached weight value at regular 500ms intervals.
This function should be called continuously in the main loop() to ensure
fresh weight data is available for retrieval.

Parameters:
  none

Returns:
  void
*/
void updateSensor()
{
    if (millis() - last_read_time >= 500)
    {
        internal_cached_weight = _doRead();
        last_read_time = millis();
        Serial.printf("[Sensor] New weight reading: %.3f kg\n", internal_cached_weight);
    }
}

/*
Retrieve the current cached weight value in kilograms.
This returns the most recently read weight from the sensor buffer,
updated every 500ms by the updateSensor() function.

Parameters:
  none

Returns:
  float: The current cached weight in kilograms (kg)
*/
float getCachedWeight()
{
    return internal_cached_weight;
}

/*
Perform temporary tare (zero adjustment) in memory only.
This zeros out the current weight reading but does not persist
the absolute offset to storage. Use setAbsoluteZero() for permanent calibration.

Parameters:
  none

Returns:
  void
*/
void tareSensor()
{
#if SIMULATE_SENSOR
    sim_weight = 0.0;
    internal_cached_weight = 0.0;
#else
    if (scale.is_ready() || scale.wait_ready_timeout(2000))
        scale.tare();
#endif
    Serial.println("[Sensor] Temporary tare performed");
}

/*
Capture and return the current absolute zero-point offset value.
This function performs a tare operation and retrieves the raw offset
value that corresponds to the current zero state of the load cell.

Parameters:
  none

Returns:
  long: The raw offset value for the current zero state
*/
long captureAbsoluteOffset()
{
#if !SIMULATE_SENSOR
    if (scale.is_ready() || scale.wait_ready_timeout(2000))
    {
        scale.tare();
        Serial.println("[Sensor] Capturing absolute offset...");
        return scale.get_offset();
    }
    Serial.println("[Sensor] Sensor not ready, cannot capture offset");
    return 0;
#else
    return 123456;
#endif
}
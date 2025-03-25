#pragma once

#include "sensor_interface.h"
#include "nmea_parser.h"

class GpsSensor : public ApoSensor
{
public:
    GpsSensor(const GpsNmeaConfig &cfg);
    ~GpsSensor();

    // ApoSensor interface:
    sensor_status initialize() override;
    sensor_reading read() override;
    static void vreadTask(void *pvParameters) override;
    sensor_type getType() const override;
    uint8_t getDevID() override;

private:
    void configure() override; // might do nothing

    // static event handler, which calls into this->onGpsEvent
    static void s_gpsEventHandler(void *handler_args,
                                  esp_event_base_t base,
                                  int32_t id,
                                  void *event_data);
    void onGpsEvent(int32_t id, void *event_data);

private:
    GpsNmeaConfig cfg_;
    GpsNmea gps_driver_;
    sensor_reading latest_reading_;
};

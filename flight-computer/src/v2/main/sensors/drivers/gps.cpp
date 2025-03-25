#include "gps.h"
#include "esp_log.h"
#include <cstring>

// just for logging:
static const char *TAG = "GpsSensor";

GpsSensor::GpsSensor(const GpsNmeaConfig &cfg)
    : cfg_(cfg), gps_driver_(cfg)
{
    // initialize our sensor reading
    memset(&latest_reading_, 0, sizeof(latest_reading_)); // doing this for safety
}

GpsSensor::~GpsSensor()
{
    gps_driver_.deinit();
}

sensor_status GpsSensor::initialize()
{
    if (!gps_driver_.init())
    {
        ESP_LOGE(TAG, "Failed to init GpsNmea driver");
        return SENSOR_ERR;
    }

    sensor_status ok = gps_driver_.addHandler(&GpsSensor::s_gpsEventHandler, this);
    if (ok != SENSOR_OK)
    {
        ESP_LOGE(TAG, "Failed to register GPS event handler");
        return SENSOR_ERR;
    }

    // If everything is okay, we might want to return SENSOR_OK
    ESP_LOGI(TAG, "GPS sensor initialized");

    return SENSOR_OK;
}

static void GpsSensor::vreadTask(void *pvParameters)
{
    while (true)
    {
        sensor_reading curr_reading = read();
        ts_sensor_data *data = static_cast<ts_sensor_data *>(pvParameters);

        if (curr_reading.value.type != GPS)
            continue;

        if (curr_reading.status == SENSOR_OK)
        {
            data->gps_lat.store(curr_reading.value.data.gps.lat, std::memory_order_relaxed);
            data->gps_lon.store(curr_reading.value.data.gps.lon, std::memory_order_relaxed);
            data->gps_alt.store(curr_reading.value.data.gps.alt, std::memory_order_relaxed);
            data->gps_speed.store(curr_reading.value.data.gps.speed, std::memory_order_relaxed);
            data->gps_cog.store(curr_reading.value.data.gps.cog, std::memory_order_relaxed);
            data->gps_mag_vari.store(curr_reading.value.data.gps.mag_vari, std::memory_order_relaxed);

            data->gps_num_sats.store(curr_reading.value.data.gps.num_sats, std::memory_order_relaxed);
            data->gps_fix_status.store(curr_reading.value.data.gps.fix_status, std::memory_order_relaxed);
            data->gps_fix_valid.store(curr_reading.value.data.gps.fix_valid, std::memory_order_relaxed);

            data->gps_year.store(curr_reading.value.data.gps.year, std::memory_order_relaxed);
            data->gps_month.store(curr_reading.value.data.gps.month, std::memory_order_relaxed);
            data->gps_day.store(curr_reading.value.data.gps.day, std::memory_order_relaxed);
            data->gps_hour.store(curr_reading.value.data.gps.hour, std::memory_order_relaxed);
            data->gps_minute.store(curr_reading.value.data.gps.minute, std::memory_order_relaxed);
            data->gps_second.store(curr_reading.value.data.gps.second, std::memory_order_relaxed);
        }
    }
}

// return last reading we got from the background driver
sensor_reading GpsSensor::read()
{
    sensor_reading result;

    result = latest_reading_;

    return result;
}

sensor_type GpsSensor::getType() const
{
    return GPS;
}

// gps modules dont usually have a WHO_AM_I value so we send 0 instead
uint8_t GpsSensor::getDevID()
{
    return 0;
}

void GpsSensor::configure()
{
}

// the function pointer we pass to the GpsNmea driver
// its a static function, so we manually get "this" from handler_args, then call onGpsEvent()
void GpsSensor::s_gpsEventHandler(void *handler_args,
                                  esp_event_base_t base,
                                  int32_t id,
                                  void *event_data)
{
    GpsSensor *self = static_cast<GpsSensor *>(handler_args);
    self->onGpsEvent(id, event_data);
}

// called whenever there's a new NMEA line
// If it's GPS_UPDATE, we copy the new gps_data into our sensor_reading
//    so that read() can return it
void GpsSensor::onGpsEvent(int32_t id, void *event_data)
{
    switch (id)
    {
    case GPS_UPDATE:
    {
        // event_data is a pointer to a gps_data
        gps_data *gd = static_cast<gps_data *>(event_data);
        if (!gd)
            return;

        sensor_reading result;
        result.value.type = GPS;

        result.value.data.gps.lat = gd->latitude;
        result.value.data.gps.lon = gd->longitude;
        result.value.data.gps.alt = gd->altitude;

        result.value.data.gps.speed = gd->speed;
        result.value.data.gps.cog = gd->cog;
        result.value.data.gps.mag_vari = gd->variation;

        result.value.data.gps.num_sats = gd->sats_in_use;
        result.value.data.gps.fix_status = gd->fix;
        result.value.data.gps.fix_valid = gd->fix_valid;

        result.value.data.gps.year = gd->date.year;
        result.value.data.gps.month = gd->date.month;
        result.value.data.gps.day = gd->date.day;
        result.value.data.gps.hour = gd->date.hour;
        result.value.data.gps.minute = gd->date.minute;
        result.value.data.gps.second = gd->date.second;

        if (!gd->fix_valid)
        {
            result.status = SENSOR_ERR_READ; // or something
        }
        else
        {
            result.status = SENSOR_OK;
        }

        latest_reading_ = result;
        break;
    }
    case GPS_UNKNOWN:
    {
        // should prob do something more here
        char *line = (char *)event_data;
        ESP_LOGW(TAG, "Got unknown NMEA line: %s", line);
        break;
    }
    default:
        break;
    }
}

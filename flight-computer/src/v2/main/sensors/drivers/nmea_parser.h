#ifndef GPS_NMEA_H_
#define GPS_NMEA_H_

#include <cstdint>
#include <cstdlib>
#include "esp_event.h"
#include "driver/uart.h"
#include "./sensor_types.h"

#ifdef __cplusplus
extern "C"
{
#endif
    ///  custom event base for our GPS
    ESP_EVENT_DECLARE_BASE(GPS_EVENT);
#ifdef __cplusplus
}
#endif

#define GPS_MAX_SATELLITES_IN_USE (12)
#define GPS_MAX_SATELLITES_IN_VIEW (16)
#define TIME_ZONE (-5)   // EST Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

enum class GpsFixType : uint8_t
{
    INVALID = 0,
    GPS,
    DGPS
};

enum class GpsFixMode : uint8_t
{
    INVALID = 1,
    MODE_2D,
    MODE_3D
};

struct GpsSatellite
{
    uint8_t num;
    uint8_t elevation;
    uint16_t azimuth;
    uint8_t snr;
};

struct GpsTime
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t thousand; ///< fractional thousandths
};

struct GpsDate
{
    uint8_t day;
    uint8_t month;
    uint16_t year; ///< offset from 2000
};

struct GpsData
{
    double latitude;
    double longitude;
    double altitude;
    GpsFixType fix;
    uint8_t sats_in_use;

    GpsTime tim;
    GpsFixMode fix_mode;
    uint8_t sats_id_in_use[GPS_MAX_SATELLITES_IN_USE];

    float dop_h;
    float dop_p;
    float dop_v;

    uint8_t sats_in_view = 0;
    GpsSatellite sats_desc_in_view[GPS_MAX_SATELLITES_IN_VIEW] = {};

    GpsDate date;
    bool fix_valid;
    float speed;
    float cog;
    float variation;
};

// GPS Event IDs
typedef enum
{
    GPS_UPDATE,
    GPS_UNKNOWN
} gps_event_id_t;

/**
 * @brief A config for the GPS driver
 */
struct GpsNmeaConfig
{
    uart_port_t uart_port;   ///< e.g. UART_NUM_2
    int rx_pin;              ///< e.g. 17
    uint32_t baud_rate;      ///< e.g. 9600
    int event_queue_size;    ///< e.g. 16
    size_t ring_buffer_size; ///< e.g. 1024 or 2048
};

// default config
static inline GpsNmeaConfig GpsNmeaConfigDefault()
{
    GpsNmeaConfig c = {
        .uart_port = UART_NUM_2,
        .rx_pin = 17,
        .baud_rate = 9600,
        .event_queue_size = 16,
        .ring_buffer_size = 2048};
    return c;
}

class GpsNmea
{
public:
    explicit GpsNmea(const GpsNmeaConfig &cfg);
    ~GpsNmea();

    sensor_status init();
    sensor_status deinit();
    sensor_status addHandler(esp_event_handler_t handler, void *handler_args);
    sensor_status removeHandler(esp_event_handler_t handler);

private:
    // Main background task that blocks on UART events
    static void gpsTaskEntry(void *arg);
    void gpsTask();

    // Called by gpsTask() on pattern detection to read line & parse
    void handleUartPattern();

    // NMEA decode
    bool checkNmeaChecksum(const char *line);
    void parseNmeaLine(const char *line);
    // Sub‐parsers
    void parseGGA(const char **tokens, int count);
    void parseGSA(const char **tokens, int count);
    void parseGSV(const char **tokens, int count);
    void parseRMC(const char **tokens, int count);
    void parseGLL(const char **tokens, int count);
    void parseVTG(const char **tokens, int count);
    // Helpers
    float parseLatLong(const char *field);
    void parseUtcTime(const char *field);

private:
    GpsNmeaConfig cfg_;
    bool running_ = false;

    //  ESP-IDF approach uses a queue to receive UART events
    QueueHandle_t uart_queue_ = nullptr;
    // event loop handle
    esp_event_loop_handle_t event_loop_ = nullptr;
    // handle for background task
    TaskHandle_t task_handle_ = nullptr;

    // "current" GPS data object
    GpsData data_;
};
#endif

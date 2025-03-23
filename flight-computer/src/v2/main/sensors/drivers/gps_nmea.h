#ifndef GPS_NMEA_H_
#define GPS_NMEA_H_

#include <cstdint>
#include <cstdlib>
#include "esp_event.h"
#include "driver/uart.h"

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
    uint8_t num = 0;
    uint8_t elevation = 0;
    uint16_t azimuth = 0;
    uint8_t snr = 0;
};

struct GpsTime
{
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint16_t thousand = 0; ///< fractional thousandths
};

struct GpsDate
{
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0; ///< offset from 2000
};

struct GpsData
{
    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    GpsFixType fix = GpsFixType::INVALID;
    uint8_t sats_in_use = 0;

    GpsTime tim;
    GpsFixMode fix_mode = GpsFixMode::INVALID;
    uint8_t sats_id_in_use[GPS_MAX_SATELLITES_IN_USE] = {0};

    float dop_h = 0.0f;
    float dop_p = 0.0f;
    float dop_v = 0.0f;

    uint8_t sats_in_view = 0;
    GpsSatellite sats_desc_in_view[GPS_MAX_SATELLITES_IN_VIEW] = {};

    GpsDate date;
    bool valid = false;
    float speed = 0.0f;
    float cog = 0.0f;
    float variation = 0.0f;
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

    sensor_status initialize() override;
    sensor_type getType() const override;
    uint8_t getDevID() override;

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
    QueueHandle_t uartQueue_ = nullptr;
    // event loop handle
    esp_event_loop_handle_t eventLoop_ = nullptr;
    // handle for background task
    TaskHandle_t taskHandle_ = nullptr;

    // "current" GPS data object
    GpsData data_;
};
#endif

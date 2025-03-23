#include "gps_nmea.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cstdlib>
#include <cmath>

// Declare event base for our GPS driver
ESP_EVENT_DEFINE_BASE(GPS_EVENT);

static const char *TAG = "GpsNmea";

/********************************************************************
 *  Helper for reading 2 digits
 *******************************************************************/
static inline uint8_t twoDigitToInt(const char *c)
{
    return (uint8_t)((c[0] - '0') * 10 + (c[1] - '0'));
}

/********************************************************************
 *  Constructor
 *******************************************************************/
GpsNmea::GpsNmea(const GpsNmeaConfig &cfg)
    : cfg_(cfg)
{
}

GpsNmea::~GpsNmea()
{
    deinit();
}

sensor_status GpsNmea::init()
{
    esp_err_t err = uart_driver_install(cfg_.uart_port,
                                        cfg_.ring_buffer_size,
                                        0,
                                        cfg_.event_queue_size,
                                        &uartQueue_,
                                        0);
    if (err != ESP_OK)
    {
        return SENSOR_ERR_INIT;
    }
    // uart params
    uart_config_t uc = {
        .baud_rate = (int)cfg_.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT};
    err = uart_param_config(cfg_.uart_port, &uc);
    if (err != ESP_OK)
    {
        return SENSOR_ERR_INIT;
    }
    // setting the pins (only using RX)
    err = uart_set_pin(cfg_.uart_port,
                       UART_PIN_NO_CHANGE,
                       cfg_.rx_pin,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        return SENSOR_ERR_INIT;
    }
    // enable pattern detection for '\n'
    //   so we can detect the end of each NMEA line
    uart_enable_pattern_det_baud_intr(cfg_.uart_port, '\n', 1, 9, 0, 0);
    // resetting the pattern queue length
    uart_pattern_queue_reset(cfg_.uart_port, cfg_.event_queue_size);

    // create an event loop
    esp_event_loop_args_t loop_args = {
        .queue_size = 8,
        .task_name = nullptr // no dedicated task for the loop
    };
    err = esp_event_loop_create(&loop_args, &eventLoop_);
    if (err != ESP_OK)
    {
        return SENSOR_ERR_INIT;
    }

    // make the background task
    running_ = true;
    BaseType_t t_err = xTaskCreate(
        gpsTaskEntry,
        "gpsNmeaTask",
        4096, // stack
        this, // arg
        5,    // priority
        &taskHandle_);
    if (t_err != pdTRUE)
    {
        running_ = false;
        return SENSOR_ERR_TASK;
    }

    ESP_LOGI(TAG, "GPS NMEA parser initialized");
    return SENSOR_OK;
}

sensor_status GpsNmea::deinit()
{
    if (running_ && taskHandle_)
    {
        running_ = false;
        vTaskDelay(pdMS_TO_TICKS(50));
        vTaskDelete(taskHandle_);
        taskHandle_ = nullptr;
    }
    if (eventLoop_)
    {
        esp_event_loop_delete(eventLoop_);
        eventLoop_ = nullptr;
    }
    if (uartQueue_)
    {
        uart_driver_delete(cfg_.uart_port);
        uartQueue_ = nullptr;
    }
    return SENSOR_OK;
}

sensor_status GpsNmea::addHandler(esp_event_handler_t handler, void *handler_args)
{
    if (!eventLoop_)
        return SENSOR_ERR;
    esp_err_t err = esp_event_handler_register_with(
        eventLoop_,
        GPS_EVENT,
        ESP_EVENT_ANY_ID,
        handler,
        handler_args);
    if (err != ESP_OK)
        return SENSOR_ERR_TASK;
    return SENSOR_OK;
}

sensor_status GpsNmea::removeHandler(esp_event_handler_t handler)
{
    if (!eventLoop_)
        return SENSOR_ERR;
    esp_err_t err = esp_event_handler_unregister_with(
        eventLoop_,
        GPS_EVENT,
        ESP_EVENT_ANY_ID,
        handler);
    if (err != ESP_OK)
        return SENSOR_ERR_TASK;
    return SENSOR_OK;
}

void GpsNmea::gpsTaskEntry(void *arg)
{
    GpsNmea *self = static_cast<GpsNmea *>(arg);
    self->gpsTask();
}

// wait on uart events
// on pattern detection, read line, parse
void GpsNmea::gpsTask()
{
    uart_event_t event;
    while (running_)
    {
        if (xQueueReceive(uartQueue_, &event, pdMS_TO_TICKS(200)))
        {
            switch (event.type)
            {
            case UARTdata_:
                // ignore raw data events, we rely on pattern for line-based reading
                break;
            case UART_PATTERN_DET:
                // found a \n in the ring buffer -> read that line
                handleUartPattern();
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(TAG, "UART buffer full. Flushing.");
                uart_flush(cfg_.uart_port);
                xQueueReset(uartQueue_);
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(TAG, "UART FIFO overflow. Flushing.");
                uart_flush(cfg_.uart_port);
                xQueueReset(uartQueue_);
                break;
            default:
                // handle other events if needed
                break;
            }
        }
        // TODO: should call esp_event_loop_run() in sensoraggregator
        // to process posted events in this same task
    }
    vTaskDelete(nullptr); // hmmmm
}

// called from gpsTask() when \n is detected
// read the line from the ring buffer, parse
void GpsNmea::handleUartPattern()
{
    // find the position of the pattern
    int pos = uart_pattern_pop_pos(cfg_.uart_port);
    if (pos == -1)
    {
        // pattern queue too small or an error, gotta flush input
        ESP_LOGW(TAG, "Pattern pos not found");
        uart_flush_input(cfg_.uart_port);
        return;
    }
    // read exactly `pos+1` bytes to get the entire line (including \n)
    const int READ_BUF_LEN = 256;
    char buffer[READ_BUF_LEN + 1] = {0};

    int read_len = uart_read_bytes(cfg_.uart_port, (uint8_t *)buffer, pos + 1, 100 / portTICK_PERIOD_MS);
    if (read_len <= 0)
    {
        return;
    }
    buffer[read_len] = '\0';

    // line ends with "\r\n" or just "\n" so we stript them
    if (read_len > 0 && buffer[read_len - 1] == '\n')
    {
        buffer[read_len - 1] = '\0';
        --read_len;
    }
    if (read_len > 0 && buffer[read_len - 1] == '\r')
    {
        buffer[read_len - 1] = '\0';
        --read_len;
    }

    // yea we got a line
    if (!checkNmeaChecksum(buffer))
    {
        // if it fails checksum, post GPS_UNKNOWN or just ignore
        esp_event_post_to(eventLoop_, GPS_EVENT, GPS_UNKNOWN,
                          buffer, read_len + 1, pdMS_TO_TICKS(100));
        return;
    }

    // good line = parse
    parseNmeaLine(buffer);

    esp_event_post_to(eventLoop_, MY_GPS_EVENT, GPS_UPDATE,
                      &data_, sizeof(GpsData),
                      pdMS_TO_TICKS(100));
}

bool GpsNmea::checkNmeaChecksum(const char *line)
{
    const char *start = strchr(line, '$');
    if (!start)
        return false;
    const char *star = strchr(start, '*');
    if (!star)
        return false;

    // 1) parse the 2-digit hex after '*'
    if (strlen(star) < 3)
        return false;
    char hexStr[3];
    hexStr[0] = star[1];
    hexStr[1] = star[2];
    hexStr[2] = '\0';
    uint8_t expected = (uint8_t)strtol(hexStr, nullptr, 16);

    // 2) XOR from start+1 up to (but not including) '*'
    uint8_t actual = 0;
    for (const char *p = start + 1; p < star; p++)
    {
        actual ^= (uint8_t)(*p);
    }
    return (actual == expected);
}

void GpsNmea::parseNmeaLine(const char *line)
{
    // We'll copy it to a temp buffer and remove any trailing "*xx"
    char buf[256];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *star = strchr(buf, '*');
    if (star)
        *star = '\0'; // remove the checksum portion

    // tokenize by ','
    static const int MAX_TOKENS = 32;
    const char *tokens[MAX_TOKENS] = {0};

    int idx = 0;
    char *p = strtok(buf, ",");
    while (p && idx < MAX_TOKENS)
    {
        tokens[idx++] = p;
        p = strtok(nullptr, ",");
    }
    if (idx == 0)
        return;

    // check statement
    if (strstr(tokens[0], "GGA"))
    {
        parseGGA(tokens, idx);
    }
    else if (strstr(tokens[0], "GSA"))
    {
        parseGSA(tokens, idx);
    }
    else if (strstr(tokens[0], "GSV"))
    {
        parseGSV(tokens, idx);
    }
    else if (strstr(tokens[0], "RMC"))
    {
        parseRMC(tokens, idx);
    }
    else if (strstr(tokens[0], "GLL"))
    {
        parseGLL(tokens, idx);
    }
    else if (strstr(tokens[0], "VTG"))
    {
        parseVTG(tokens, idx);
    }
    else
    {
        // unknown
    }
}

float GpsNmea::parseLatLong(const char *field)
{
    float val = strtof(field, nullptr);
    int deg = (int)(val / 100);
    float minutes = val - deg * 100.0f;
    return deg + minutes / 60.0f;
}

void GpsNmea::parseUtcTime(const char *field)
{
    // "hhmmss.sss"
    if (strlen(field) < 6)
        return;
    data_.tim.hour = twoDigitToInt(field + 0);
    data_.tim.minute = twoDigitToInt(field + 2);
    data_.tim.second = twoDigitToInt(field + 4);

    // adding hour offset
    data_.tim.hour += TIME_ZONE;

    const char *dot = strchr(field, '.');
    if (dot)
    {
        data_.tim.thousand = (uint16_t)atoi(dot + 1);
    }
}

void GpsNmea::parseGGA(const char **tokens, int count)
{
    // tokens[1] => utc time
    // tokens[2] => lat
    // tokens[3] => N/S
    // tokens[4] => lon
    // tokens[5] => E/W
    // tokens[6] => fix quality
    // tokens[7] => sats in use
    // tokens[8] => HDOP
    // tokens[9] => altitude
    if (count < 10)
        return;

    parseUtcTime(tokens[1]);

    if (strlen(tokens[2]) > 0)
    {
        float lat = parseLatLong(tokens[2]);
        if (tokens[3][0] == 'S')
            lat = -lat;
        data_.latitude = lat;
    }
    if (strlen(tokens[4]) > 0)
    {
        float lon = parseLatLong(tokens[4]);
        if (tokens[5][0] == 'W')
            lon = -lon;
        data_.longitude = lon;
    }
    int fixQual = atoi(tokens[6]);
    if (fixQual == 1)
    {
        data_.fix = GpsFixType::GPS;
    }
    else if (fixQual == 2)
    {
        data_.fix = GpsFixType::DGPS;
    }
    else
    {
        data_.fix = GpsFixType::INVALID;
    }
    data_.sats_in_use = (uint8_t)atoi(tokens[7]);
    data_.dop_h = strtof(tokens[8], nullptr);
    data_.altitude = strtof(tokens[9], nullptr);
    if (count > 11)
    {
        float geoid = strtof(tokens[11], nullptr);
        data_.altitude += geoid;
    }
}

void GpsNmea::parseGSA(const char **tokens, int count)
{
    if (count < 18)
        return;
    int mode = atoi(tokens[2]);
    if (mode == 2)
        data_.fix_mode = GpsFixMode::MODE_2D;
    else if (mode == 3)
        data_.fix_mode = GpsFixMode::MODE_3D;
    else
        data_.fix_mode = GpsFixMode::INVALID;

    for (int i = 3; i <= 14 && i < count; i++)
    {
        data_.sats_id_in_use[i - 3] = (uint8_t)atoi(tokens[i]);
    }
    data_.dop_p = strtof(tokens[15], nullptr);
    data_.dop_h = strtof(tokens[16], nullptr);
    data_.dop_v = strtof(tokens[17], nullptr);
}

void GpsNmea::parseGSV(const char **tokens, int count)
{
    if (count < 4)
        return;
    data_.sats_in_view = (uint8_t)atoi(tokens[3]);
    int thisMsg = atoi(tokens[2]);

    int idx = 4;
    while ((idx + 3) < count)
    {
        int satIndex = (idx - 4) / 4;
        uint8_t globalIndex = 4 * (thisMsg - 1) + satIndex;
        if (globalIndex < GPS_MAX_SATELLITES_IN_VIEW)
        {
            GpsSatellite &sat = data_.sats_desc_in_view[globalIndex];
            sat.num = (uint8_t)atoi(tokens[idx]);
            sat.elevation = (uint8_t)atoi(tokens[idx + 1]);
            sat.azimuth = (uint16_t)atoi(tokens[idx + 2]);
            sat.snr = (uint8_t)atoi(tokens[idx + 3]);
        }
        idx += 4;
    }
}

void GpsNmea::parseRMC(const char **tokens, int count)
{
    if (count < 10)
        return;
    parseUtcTime(tokens[1]);
    data_.valid = (tokens[2][0] == 'A');

    if (strlen(tokens[3]) > 0)
    {
        float lat = parseLatLong(tokens[3]);
        if (tokens[4][0] == 'S')
            lat = -lat;
        data_.latitude = lat;
    }
    if (strlen(tokens[5]) > 0)
    {
        float lon = parseLatLong(tokens[5]);
        if (tokens[6][0] == 'W')
            lon = -lon;
        data_.longitude = lon;
    }
    float speed_knots = strtof(tokens[7], nullptr);
    data_.speed = speed_knots * 0.514444f;

    data_.cog = strtof(tokens[8], nullptr);

    if (strlen(tokens[9]) == 6)
    {
        data_.date.day = twoDigitToInt(tokens[9] + 0);
        data_.date.month = twoDigitToInt(tokens[9] + 2);
        data_.date.year = twoDigitToInt(tokens[9] + 4);

        // adding year offset
        data_.date.year += YEAR_BASE;
    }
    if (count >= 11)
    {
        data_.variation = strtof(tokens[10], nullptr);
        if (count >= 12 && tokens[11][0] == 'W')
        {
            data_.variation = -data_.variation;
        }
    }
}

void GpsNmea::parseGLL(const char **tokens, int count)
{
    if (count < 7)
        return;
    if (strlen(tokens[1]) > 0)
    {
        float lat = parseLatLong(tokens[1]);
        if (tokens[2][0] == 'S')
            lat = -lat;
        data_.latitude = lat;
    }
    if (strlen(tokens[3]) > 0)
    {
        float lon = parseLatLong(tokens[3]);
        if (tokens[4][0] == 'W')
            lon = -lon;
        data_.longitude = lon;
    }
    parseUtcTime(tokens[5]);
    data_.valid = (tokens[6][0] == 'A');
}

void GpsNmea::parseVTG(const char **tokens, int count)
{
    if (count < 8)
        return;
    data_.cog = strtof(tokens[1], nullptr);

    float speed_knots = strtof(tokens[5], nullptr);
    data_.speed = speed_knots * 0.514444f;
}

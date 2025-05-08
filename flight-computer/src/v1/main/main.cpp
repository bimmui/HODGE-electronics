#include <cstdio>  // <stdio.h>  → C++ header
#include <cstring> // <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <RadioLib.h>

// include the hardware abstraction layer
#include "EspHal.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "sdkconfig.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

// sensor drivers
#include "icm20948.h"
#include "adxl375.h"
#include "bmp581.h"
#include "nmea_parser.h"
#include "tmp1075.h"
#include "StateDetermination.h"

/*-------------- configuration ------------------------------------*/
namespace
{
    constexpr const char *TAG = "example";
    constexpr const char *MOUNT_POINT = "/sdcard";
    constexpr size_t MAX_CHAR_SIZE = 408;
}

/* pin aliases */
#define PIN_NUM_MISO CONFIG_PIN_MISO
#define PIN_NUM_MOSI CONFIG_PIN_MOSI
#define PIN_NUM_CLK CONFIG_PIN_CLK
#define PIN_NUM_CS static_cast<gpio_num_t>(CONFIG_PIN_CS)

// gps defines + constant
#define TIME_ZONE (-5)   // EST Time
#define YEAR_BASE (2000) // date in GPS starts from 2000

// i2c shit
#define I2C_MASTER_NUM I2C_NUM_0

// piezo stuff
#define BUZZER_GPIO GPIO_NUM_21
#define BUZZER_FREQ_HZ 4000 // 4 kHz tone (double check datasheet to see if other tones work)
#define BUZZER_DUTY 4000    // Duty (out of 8192), ADJUST FOR VOLUME PLZ
#define BUZZER_TIMER LEDC_TIMER_0
#define BUZZER_MODE LEDC_LOW_SPEED_MODE
#define BUZZER_CHANNEL LEDC_CHANNEL_0

// static EspHal *hal = new EspHal(PIN_NUM_CLK, PIN_NUM_MISO, PIN_NUM_MOSI);
// static RFM96 radio = new Module(hal, CONFIG_RFM96W_CS_PIN, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC);

// big struct to hold data
typedef struct
{
    ////// GPS stuff
    // date
    uint8_t day;
    uint8_t month;
    uint16_t year;
    // time
    uint8_t hour;
    uint8_t minute;
    // everything else
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float cog;
    int num_sats;
    int fix_status;
    bool fix_valid;
    float mag_vari;
    ////// ADXL375
    float adxl375_accel_x;
    float adxl375_accel_y;
    float adxl375_accel_z;
    ////// BMP581
    float pressure;
    float bmp_temp;
    float bmp_altitude;
    ////// TMP1075
    float tmp_temp;
    ////// ICM20948
    float icm20948_accel_x;
    float icm20948_accel_y;
    float icm20948_accel_z;
    float icm20948_gyro_x;
    float icm20948_gyro_y;
    float icm20948_gyro_z;
    float icm20948_mag_x;
    float icm20948_mag_y;
    float icm20948_mag_z;
    ////// KF values
    float kf_altitude;
    float kf_vert_velo;
    float kf_accel;
    ////// State
} flight_data;

// this is where data will be populated
static flight_data flightlog;
static flight_data gpslog;

/*-------------- tiny file helpers --------------------------------*/
static esp_err_t
write_file(const char *path, const char *data)
{
    // ESP_LOGI(TAG, "open %s", path);
    FILE *f = fopen(path, "a");
    if (!f)
    {
        // ESP_LOGE(TAG, "open failed");
        return ESP_FAIL;
    }
    fputs(data, f); // avoid format-string hazard
    fclose(f);
    // ESP_LOGI(TAG, "written");
    return ESP_OK;
}

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id)
    {
    case GPS_UPDATE:
    {
        gps = (gps_t *)event_data;
        flight_data *data = static_cast<flight_data *>(event_handler_arg);
        printf("in here");
        /* print information parsed from GPS statements */
        data->day = gps->date.day;
        data->month = gps->date.month;
        data->year = gps->date.year + YEAR_BASE;
        data->hour = gps->tim.hour + TIME_ZONE;
        data->minute = gps->tim.minute;
        data->latitude = gps->latitude;
        data->longitude = gps->longitude;
        data->altitude = gps->altitude;
        data->speed = gps->speed;
        data->cog = gps->cog;
        data->num_sats = gps->sats_in_use;
        data->fix_status = gps->fix;
        data->fix_valid = gps->valid ? true : false;
        data->mag_vari = gps->variation;

        ESP_LOGI(TAG,
                 "GPS %02u/%02u/%04u %02u:%02u  "
                 "Lat=%.5f  Lon=%.5f  Alt=%.1f m  "
                 "Spd=%.1f kn  COG=%.1f°  Sats=%d  Fix=%d  Var=%.1f°",
                 data->month, data->day, data->year,
                 data->hour, data->minute,
                 data->latitude, data->longitude, data->altitude,
                 data->speed, data->cog,
                 data->num_sats, data->fix_status, data->mag_vari);

        vTaskDelay(pdMS_TO_TICKS(1000)); // 3-s pause after each GPS print
        break;
    }
    case GPS_UNKNOWN:
    { /* print unknown statements */
        printf("in here 2");
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    }
    default:
        printf("in here 3");
        break;
    }
}

// static esp_err_t read_file(const char *path)
// {
//     char line[MAX_CHAR_SIZE] = {};
//     ESP_LOGI(TAG, "read %s", path);
//     FILE *f = fopen(path, "r");
//     if (!f)
//     {
//         ESP_LOGE(TAG, "open failed");
//         return ESP_FAIL;
//     }
//     fgets(line, sizeof line, f);
//     fclose(f);
//     if (char *nl = strchr(line, '\n'))
//         *nl = '\0';
//     ESP_LOGI(TAG, "got '%s'", line);
//     return ESP_OK;
// }

static const uint16_t notes[] = {
    0, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220,
    233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661,
    1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978};

typedef enum
{
    NO_NOTE = 0,
    NOTE_A0,
    NOTE_AS0,
    NOTE_B0,
    NOTE_C1,
    NOTE_CS1,
    NOTE_D1,
    NOTE_DS1,
    NOTE_E1,
    NOTE_F1,
    NOTE_FS1,
    NOTE_G1,
    NOTE_GS1,
    NOTE_A1,
    NOTE_AS1,
    NOTE_B1,
    NOTE_C2,
    NOTE_CS2,
    NOTE_D2,
    NOTE_DS2,
    NOTE_E2,
    NOTE_F2,
    NOTE_FS2,
    NOTE_G2,
    NOTE_GS2,
    NOTE_A2,
    NOTE_AS2,
    NOTE_B2,
    NOTE_C3,
    NOTE_CS3,
    NOTE_D3,
    NOTE_DS3,
    NOTE_E3,
    NOTE_F3,
    NOTE_FS3,
    NOTE_G3,
    NOTE_GS3,
    NOTE_A3,
    NOTE_AS3,
    NOTE_B3,
    NOTE_C4,
    NOTE_CS4,
    NOTE_D4,
    NOTE_DS4,
    NOTE_E4,
    NOTE_F4,
    NOTE_FS4,
    NOTE_G4,
    NOTE_GS4,
    NOTE_A4,
    NOTE_AS4,
    NOTE_B4,
    NOTE_C5,
    NOTE_CS5,
    NOTE_D5,
    NOTE_DS5,
    NOTE_E5,
    NOTE_F5,
    NOTE_FS5,
    NOTE_G5,
    NOTE_GS5,
    NOTE_A5,
    NOTE_AS5,
    NOTE_B5,
    NOTE_C6,
    NOTE_CS6,
    NOTE_D6,
    NOTE_DS6,
    NOTE_E6,
    NOTE_F6,
    NOTE_FS6,
    NOTE_G6,
    NOTE_GS6,
    NOTE_A6,
    NOTE_AS6,
    NOTE_B6,
    NOTE_C7,
    NOTE_CS7,
    NOTE_D7,
    NOTE_DS7,
    NOTE_E7,
    NOTE_F7,
    NOTE_FS7,
    NOTE_G7,
    NOTE_GS7,
    NOTE_A7,
    NOTE_AS7,
    NOTE_B7,
    NOTE_C8,
} piano_note_t;

// void esp32_beep(unsigned char key_num, unsigned char dur_hms)
// {
//     ledc_timer_config_t ledc_timer;
//     ledc_channel_config_t ledc_channel;

//     ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
//     ledc_timer.freq_hz = notes[key_num];            // frequency of PWM signal
//     ledc_timer.speed_mode = BUZZER_MODE;            // timer mode
//     ledc_timer.timer_num = BUZZER_TIMER;            // timer index
//     ledc_timer.clk_cfg = LEDC_AUTO_CLK;

//     ledc_channel.channel = BUZZER_CHANNEL;
//     ledc_channel.duty = 4096;
//     ledc_channel.gpio_num = BUZZER_GPIO;
//     ledc_channel.speed_mode = BUZZER_MODE;
//     ledc_channel.hpoint = 0;
//     ledc_channel.timer_sel = BUZZER_TIMER;

//     ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
//     ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

//     vTaskDelay(pdMS_TO_TICKS(dur_hms * 100));
//     ledc_stop(BUZZER_MODE, BUZZER_CHANNEL, 0);
// }

void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_MASTER_NUM;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

extern "C" void app_main(void)
{
    i2c_bus_init();
    esp_err_t ret;

    // esp32_beep(NOTE_A7, 2000);

    // PIEZO SETUP
    ledc_timer_config_t timer_conf{};
    timer_conf.speed_mode = BUZZER_MODE;
    timer_conf.timer_num = BUZZER_TIMER;
    timer_conf.duty_resolution = LEDC_TIMER_13_BIT;
    timer_conf.freq_hz = BUZZER_FREQ_HZ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;

    ledc_channel_config_t channel_conf = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    // beep
    ESP_LOGI(TAG, "playing buzzar");
    ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_DUTY);
    ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(2000)); // 2s beep

    // Stop beep
    ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);

    // ESP_LOGI(TAG, "[RFM96] Initializing ... ");
    // int state = radio.begin(434.550);
    // if (state != RADIOLIB_ERR_NONE)
    // {
    //     ESP_LOGI(TAG, "failed, code %d\n", state);
    //     while (true)
    //     {
    //         hal->delay(1000);
    //     }
    // }
    // ESP_LOGI(TAG, "success!\n");

    // radio.setSpreadingFactor(7);
    // radio.setBandwidth(125.0);
    // radio.setCodingRate(7);

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg{};
    mount_cfg.format_if_mount_failed = true; // or false
    mount_cfg.max_files = 5;
    mount_cfg.allocation_unit_size = 16 * 1024;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdmmc_card_t *card = nullptr;

#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_cfg{.ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID};
    sd_pwr_ctrl_handle_t pwr = nullptr;
    ESP_ERROR_CHECK(sd_pwr_ctrl_new_on_chip_ldo(&ldo_cfg, &pwr));
    host.pwr_ctrl_handle = pwr;
#endif

    /* SPI bus ----------------------------------------------------- */
    spi_bus_config_t bus_cfg{};
    bus_cfg.mosi_io_num = PIN_NUM_MOSI;
    bus_cfg.miso_io_num = PIN_NUM_MISO;
    bus_cfg.sclk_io_num = PIN_NUM_CLK;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;
    ESP_ERROR_CHECK(spi_bus_initialize(static_cast<spi_host_device_t>(host.slot),
                                       &bus_cfg, SDSPI_DEFAULT_DMA));

    /* Slot -------------------------------------------------------- */
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = PIN_NUM_CS;
    slot_cfg.host_id = static_cast<spi_host_device_t>(host.slot);

    /* Mount ------------------------------------------------------- */
    ESP_LOGI(TAG, "mount fs");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_cfg,
                                  &mount_cfg, &card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "mount fail (%s)", esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
        check_sd_card_pins(&config, pin_count);
#endif
        return;
    }

    sdmmc_card_print_info(stdout, card);

    // settting up the datalog csv file
    ret = write_file("/sdcard/datalog.csv", "day,month,year,hour,minute,latitude,longitude,gps_altitude,speed,cog,num_sats,fix_status,fix_valid,mag_vari,adxl375_accel_x,adxl375_accel_y,adxl375_accel_z,pressure,bmp_temp,bmp_altitude,tmp_temp,icm20948_accel_x,icm20948_accel_y,icm20948_accel_z,icm20948_gyro_x,icm20948_gyro_y,icm20948_gyro_z,kf_altitude,kf_vert_velo,kf_accel,time\n");
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't write to SD card");
    }

    // setting up all the sensors
    static ADXL375 adxl(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x53, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "ADXL375 object initialized");
    adxl.configureADXL375();

    static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, CONFIG_I2C_MASTER_FREQUENCY);
    bmp581.bmp581_configure();
    bmp581_data sample;

    static TMP1075 tmp(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x48, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "TMP1075 object initialized");
    tmp.configureTMP1075();

    static ICM20948 icm(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x68, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "ICM20948 object initialized");
    icm.configureICM20948(ICM20948::ACCEL_FS_2G, ICM20948::GYRO_FS_1000DPS);
    icm.initAK09916(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x0C, CONFIG_I2C_MASTER_FREQUENCY);
    icm.configureAK09916();

    /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    /* register event handler for NMEA parser library */
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, &gpslog);

    ESP_LOGI(TAG, "Beginning sensor reading");

    while (true)
    {
        adxl375_accel_value_t accels;
        adxl.getAccel(&accels);
        flightlog.adxl375_accel_x = accels.accel_x;
        flightlog.adxl375_accel_y = accels.accel_y;
        flightlog.adxl375_accel_z = accels.accel_z;
        // ESP_LOGI(TAG, "adxl375 readings obtained");

        sample = bmp581.bmp581_get_sample();
        flightlog.pressure = (double)sample.pressure;
        flightlog.bmp_temp = sample.temperature_c;
        flightlog.bmp_altitude = sample.altitude;
        // ESP_LOGI(TAG, "bmp581 readings obtained");

        flightlog.tmp_temp = tmp.readTempC();
        // ESP_LOGI(TAG, "tmp readings obtained");

        icm20948_accel_value_t icm_accels;
        icm20948_gyro_value_t gyros;
        ak09916_mag_value_t mags;
        icm.getAccel(&icm_accels);
        icm.getGyro(&gyros);
        icm.getMag(&mags);
        flightlog.icm20948_accel_x = icm_accels.accel_x;
        flightlog.icm20948_accel_y = icm_accels.accel_y;
        flightlog.icm20948_accel_z = icm_accels.accel_z;
        flightlog.icm20948_gyro_x = gyros.gyro_x;
        flightlog.icm20948_gyro_y = gyros.gyro_y;
        flightlog.icm20948_gyro_z = gyros.gyro_z;
        flightlog.icm20948_mag_x = mags.mag_x;
        flightlog.icm20948_mag_y = mags.mag_y;
        flightlog.icm20948_mag_z = mags.mag_z;
        // ESP_LOGI(TAG, "imu readings obtained");

        float kf_accels[3] = {icm_accels.accel_x, icm_accels.accel_y, icm_accels.accel_z};
        float kf_gyros[3] = {gyros.gyro_x, gyros.gyro_y, gyros.gyro_z};

        StateDeterminer state_determiner = StateDeterminer();
        kf_vals thevals = state_determiner.determineState(kf_accels, kf_gyros, sample.altitude, esp_timer_get_time() / 1000ULL);
        flightlog.kf_altitude = thevals.kf_altitude;
        flightlog.kf_vert_velo = thevals.kf_vert_velo;
        flightlog.kf_accel = thevals.kf_accel;

        char csv_row[MAX_CHAR_SIZE];
        sprintf(csv_row, "%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%lu\n",
                gpslog.day, gpslog.month, gpslog.year,
                gpslog.hour, gpslog.minute,
                gpslog.latitude, gpslog.longitude, gpslog.altitude, gpslog.speed, gpslog.cog,
                gpslog.num_sats, gpslog.fix_status, gpslog.fix_valid, gpslog.mag_vari,
                flightlog.adxl375_accel_x, flightlog.adxl375_accel_y, flightlog.adxl375_accel_z,
                flightlog.pressure, flightlog.bmp_temp, flightlog.bmp_altitude,
                flightlog.tmp_temp,
                flightlog.icm20948_accel_x, flightlog.icm20948_accel_y, flightlog.icm20948_accel_z,
                flightlog.icm20948_gyro_x, flightlog.icm20948_gyro_y, flightlog.icm20948_gyro_z,
                flightlog.icm20948_mag_x, flightlog.icm20948_mag_y, flightlog.icm20948_mag_z,
                flightlog.kf_altitude, flightlog.kf_vert_velo, flightlog.kf_accel,
                (unsigned long)(esp_timer_get_time() / 1000ULL));

        esp_err_t ret;
        ret = write_file("/sdcard/datalog.csv", csv_row);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Couldn't write to SD card");
        }

        //     // state = radio.transmit(csv_row);
        //     // if (state == RADIOLIB_ERR_NONE)
        //     // {
        //     //     // the packet was successfully transmitted
        //     //     ESP_LOGI(TAG, "success!");
        //     // }
        //     // else
        //     // {
        //     //     ESP_LOGI(TAG, "failed, code %d\n", state);
        //     // }

        //     // // wait for a second before transmitting again
        //     // hal->delay(500);
    }
}

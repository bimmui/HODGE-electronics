/********************************************************************
 *  sdspi_example.cpp  –  ESP-IDF SDSPI + FATFS (C-to-C++ port)
 *
 *  Same behaviour as the original C sample; just C++ syntax.
 ********************************************************************/

#include <cstdio>  // <stdio.h>  → C++ header
#include <cstring> // <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
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
    ////// KF values
    float kf_altitude;
    float kf_vert_velo;
    float kf_accel;
    ////// State
} flight_data;

/*-------------- tiny file helpers --------------------------------*/
static esp_err_t
write_file(const char *path, const char *data)
{
    ESP_LOGI(TAG, "open %s", path);
    FILE *f = fopen(path, "w");
    if (!f)
    {
        ESP_LOGE(TAG, "open failed");
        return ESP_FAIL;
    }
    fputs(data, f); // avoid format-string hazard
    fclose(f);
    ESP_LOGI(TAG, "written");
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
    }
    case GPS_UNKNOWN:
    { /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    }
    default:
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

    /* demo I/O ---------------------------------------------------- */
    char txt[MAX_CHAR_SIZE];
    snprintf(txt, sizeof txt, "Hello %s!\n", card->cid.name);
    ESP_ERROR_CHECK(write_file("/sdcard/hello.txt", txt));

    // settting up the datalog csv file
    char data[MAX_CHAR_SIZE];
    snprintf(data, MAX_CHAR_SIZE, "day,month,year,hour,minute,latitude,longitude,gps_altitude,speed,cog,num_sats,fix_status,fix_valid,mag_vari,adxl375_accel_x,adxl375_accel_y,adxl375_accel_z,pressure,bmp_temp,bmp_altitude,tmp_temp,icm20948_accel_x,icm20948_accel_y,icm20948_accel_z,icm20948_gyro_x,icm20948_gyro_y,icm20948_gyro_z,kf_altitude,kf_vert_velo,kf_accel\n");
    ret = write_file("/sdcard/datalog.csv", data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't write to SD card");
    }

    // this is where data will be populated
    flight_data flightlog;

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

    /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    /* register event handler for NMEA parser library */
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, &flightlog);

    ESP_LOGI(TAG, "Beginning sensor reading");
    while (true)
    {
        adxl375_accel_value_t accels;
        adxl.getAccel(&accels);
        flightlog.adxl375_accel_x = accels.accel_x;
        flightlog.adxl375_accel_y = accels.accel_y;
        flightlog.adxl375_accel_z = accels.accel_z;
        ESP_LOGI(TAG, "adxl375 readings obtained");

        sample = bmp581.bmp581_get_sample();
        flightlog.pressure = (double)sample.pressure;
        flightlog.bmp_temp = sample.temperature_c;
        flightlog.altitude = sample.altitude;
        ESP_LOGI(TAG, "bmp581 readings obtained");

        flightlog.tmp_temp = tmp.readTempC();
        ESP_LOGI(TAG, "tmp readings obtained");

        icm20948_accel_value_t icm_accels;
        icm20948_gyro_value_t gyros;
        icm.getAccel(&icm_accels);
        icm.getGyro(&gyros);
        flightlog.icm20948_accel_x = icm_accels.accel_x;
        flightlog.icm20948_accel_y = icm_accels.accel_y;
        flightlog.icm20948_accel_z = icm_accels.accel_z;
        flightlog.icm20948_gyro_x = gyros.gyro_x;
        flightlog.icm20948_gyro_y = gyros.gyro_y;
        flightlog.icm20948_gyro_z = gyros.gyro_z;
        ESP_LOGI(TAG, "imu readings obtained");

        ESP_LOGI(TAG,
                 "ADXL: (%.3f, %.3f, %.3f) g | "
                 "BMP581: P=%.2f Pa  T=%.2f °C  Alt=%.2f m | "
                 "TMP1075: %.2f °C | "
                 "ICM20948 acc: (%.3f, %.3f, %.3f) g  gyro: (%.2f, %.2f, %.2f) rad/s",
                 flightlog.adxl375_accel_x, flightlog.adxl375_accel_y, flightlog.adxl375_accel_z,
                 flightlog.pressure, flightlog.bmp_temp, flightlog.altitude,
                 flightlog.tmp_temp,
                 flightlog.icm20948_accel_x, flightlog.icm20948_accel_y, flightlog.icm20948_accel_z,
                 flightlog.icm20948_gyro_x, flightlog.icm20948_gyro_y, flightlog.icm20948_gyro_z);

        vTaskDelay(pdMS_TO_TICKS(3000));

        // float kf_accels[3] = {icm_accels.accel_x, icm_accels.accel_y, icm_accels.accel_z};
        // float kf_gyros[3] = {gyros.gyro_x, gyros.gyro_y, gyros.gyro_z};

        // StateDeterminer state_determiner = StateDeterminer();
        // kf_vals thevals = state_determiner.determineState(kf_accels, kf_gyros, sample.altitude, esp_timer_get_time() / 1000ULL);
        // flightlog.kf_altitude = thevals.kf_altitude;
        // flightlog.kf_vert_velo = thevals.kf_vert_velo;
        // flightlog.kf_accel = thevals.kf_accel;
    }
}

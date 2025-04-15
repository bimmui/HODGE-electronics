#include <stdio.h>
#include <iostream>
// #include <driver/i2c_master.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "sdkconfig.h"

// sd card shit
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"

// sensor drivers
// #include "icm20948.h"
// #include "adxl375.h"
// #include "bmp581.h"
// #include "nmea_parser.h"
// #include "tmp1075.h"
// #include "StateDetermination.h"

// sd io defines + constants
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#define MAX_CHAR_SIZE 1024
static const char *TAG = "example";
#define MOUNT_POINT "/sdcard"
const char *datalog_path = MOUNT_POINT "/datalog.csv";

#ifdef CONFIG_DEBUG_PIN_CONNECTIONS
const char *names[] = {"CLK ", "MOSI", "MISO", "CS"};
const int pins[] = {CONFIG_PIN_CLK,
                    CONFIG_PIN_MOSI,
                    CONFIG_PIN_MISO,
                    CONFIG_PIN_CS};

const int pin_count = sizeof(pins) / sizeof(pins[0]);
#if CONFIG_ENABLE_ADC_FEATURE
const int adc_channels[] = {CONFIG_ADC_PIN_CLK,
                            CONFIG_ADC_PIN_MOSI,
                            CONFIG_ADC_PIN_MISO,
                            CONFIG_ADC_PIN_CS};
#endif // CONFIG_ENABLE_ADC_FEATURE

pin_configuration_t config = {
    .names = names,
    .pins = pins,
#if CONFIG_ENABLE_ADC_FEATURE
    .adc_channels = adc_channels,
#endif
};
#endif // CONFIG_DEBUG_PIN_CONNECTIONS

// Pin assignments can be set in menuconfig, see "SD SPI Example Configuration" menu.
// You can also change the pin assignments here by changing the following 4 lines.
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

// static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
// {
//     gps_t *gps = NULL;
//     switch (event_id)
//     {
//     case GPS_UPDATE:
//     {
//         gps = (gps_t *)event_data;
//         flight_data *data = static_cast<flight_data *>(event_handler_arg);
//         /* print information parsed from GPS statements */
//         data->day = gps->date.day;
//         data->month = gps->date.month;
//         data->year = gps->date.year + YEAR_BASE;
//         data->hour = gps->tim.hour + TIME_ZONE;
//         data->minute = gps->tim.minute;
//         data->latitude = gps->latitude;
//         data->longitude = gps->longitude;
//         data->altitude = gps->altitude;
//         data->speed = gps->speed;
//         data->cog = gps->cog;
//         data->num_sats = gps->sats_in_use;
//         data->fix_status = gps->fix;
//         data->fix_valid = gps->valid ? true : false;
//         data->mag_vari = gps->variation;
//     }
//     case GPS_UNKNOWN:
//     { /* print unknown statements */
//         ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
//         break;
//     }
//     default:
//         break;
//     }
// }

static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos)
    {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

// esp_err_t mount_filesystem(sdmmc_card_t **out_card)
// {
//     esp_err_t ret;
//     esp_vfs_fat_sdmmc_mount_config_t mount_config = {
// #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
//         .format_if_mount_failed = true,
// #else
//         .format_if_mount_failed = false,
// #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
//         .max_files = 5,
//         .allocation_unit_size = 16 * 1024};
//     const char *mount_point = MOUNT_POINT;
//     sdmmc_host_t host = SDSPI_HOST_DEFAULT();
//     spi_bus_config_t bus_cfg = {
//         .mosi_io_num = PIN_NUM_MOSI,
//         .miso_io_num = PIN_NUM_MISO,
//         .sclk_io_num = PIN_NUM_CLK,
//         .quadwp_io_num = -1,
//         .quadhd_io_num = -1,
//         .max_transfer_sz = 4000,
//     };
//     ret = spi_bus_initialize(
//         static_cast<spi_host_device_t>(host.slot),
//         &bus_cfg,
//         SDSPI_DEFAULT_DMA);
//     if (ret != ESP_OK)
//     {
//         ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
//         return ret;
//     }

//     sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
//     slot_config.gpio_cs = PIN_NUM_CS;
//     slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

//     // Pass out_card (NOT &card)
//     ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, out_card);
//     if (ret == ESP_OK)
//     {
//         // Now *out_card is valid, so print:
//         sdmmc_card_print_info(stdout, *out_card);
//     }
//     return ret;
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

extern "C" void app_main()
{
    // i2c_bus_init();
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Using SPI peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(static_cast<spi_host_device_t>(host.slot), &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // settting up the datalog csv file
    char data[MAX_CHAR_SIZE];
    snprintf(data, MAX_CHAR_SIZE, "day,month,year,hour,minute,latitude,longitude,gps_altitude,speed,cog,num_sats,fix_status,fix_valid,mag_vari,adxl375_accel_x,adxl375_accel_y,adxl375_accel_z,pressure,bmp_temp,bmp_altitude,tmp_temp,icm20948_accel_x,icm20948_accel_y,icm20948_accel_z,icm20948_gyro_x,icm20948_gyro_y,icm20948_gyro_z,kf_altitude,kf_vert_velo,kf_accel\n");
    ret = s_example_write_file(datalog_path, data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't write to SD card");
    }

    // this is where data will be populated
    flight_data flightlog;

    // setting up all the sensors
    // static ADXL375 adxl(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x53, CONFIG_I2C_MASTER_FREQUENCY);
    // ESP_LOGI(TAG, "ADXL375 object initialized");
    // adxl.configureADXL375();

    // static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, CONFIG_I2C_MASTER_FREQUENCY);
    // bmp581.bmp581_configure();
    // bmp581_data sample;

    // static TMP1075 tmp(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x47, CONFIG_I2C_MASTER_FREQUENCY);
    // ESP_LOGI(TAG, "TMP1075 object initialized");
    // tmp.configureTMP1075();

    // static ICM20948 icm(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x68, CONFIG_I2C_MASTER_FREQUENCY);
    // ESP_LOGI(TAG, "ICM20948 object initialized");
    // icm.configureICM20948(ICM20948::ACCEL_FS_2G, ICM20948::GYRO_FS_1000DPS);

    // /* NMEA parser configuration */
    // nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    // /* init NMEA parser library */
    // nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    // /* register event handler for NMEA parser library */
    // nmea_parser_add_handler(nmea_hdl, gps_event_handler, &flightlog);

    // while (true)
    // {
    // adxl375_accel_value_t accels;
    // adxl.getAccel(&accels);
    // flightlog.adxl375_accel_x = accels.accel_x;
    // flightlog.adxl375_accel_y = accels.accel_y;
    // flightlog.adxl375_accel_z = accels.accel_z;

    // sample = bmp581.bmp581_get_sample();
    // flightlog.pressure = (double)sample.pressure;
    // flightlog.bmp_temp = sample.temperature_c;
    // flightlog.altitude = sample.altitude;

    // flightlog.tmp_temp = tmp.readTempC();

    // icm20948_accel_value_t icm_accels;
    // icm20948_gyro_value_t gyros;
    // icm.getAccel(&icm_accels);
    // icm.getGyro(&gyros);
    // flightlog.icm20948_accel_x = icm_accels.accel_x;
    // flightlog.icm20948_accel_y = icm_accels.accel_y;
    // flightlog.icm20948_accel_z = icm_accels.accel_z;
    // flightlog.icm20948_gyro_x = gyros.gyro_x;
    // flightlog.icm20948_gyro_y = gyros.gyro_y;
    // flightlog.icm20948_gyro_z = gyros.gyro_z;

    // float kf_accels[3] = {icm_accels.accel_x, icm_accels.accel_y, icm_accels.accel_z};
    // float kf_gyros[3] = {gyros.gyro_x, gyros.gyro_y, gyros.gyro_z};

    // StateDeterminer state_determiner = StateDeterminer();
    // kf_vals thevals = state_determiner.determineState(kf_accels, kf_gyros, sample.altitude, esp_timer_get_time() / 1000ULL);
    // flightlog.kf_altitude = thevals.kf_altitude;
    // flightlog.kf_vert_velo = thevals.kf_vert_velo;
    // flightlog.kf_accel = thevals.kf_accel;

    // now writing to sd card
    char csv_row[MAX_CHAR_SIZE] = "hey pls fucking work";
    // sprintf(csv_row, "%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
    //         flightlog.day, flightlog.month, flightlog.year,
    //         flightlog.hour, flightlog.minute,
    //         flightlog.latitude, flightlog.longitude, flightlog.altitude, flightlog.speed, flightlog.cog,
    //         flightlog.num_sats, flightlog.fix_status, flightlog.fix_valid, flightlog.mag_vari,
    //         flightlog.adxl375_accel_x, flightlog.adxl375_accel_y, flightlog.adxl375_accel_z,
    //         flightlog.pressure, flightlog.bmp_temp, flightlog.bmp_altitude,
    //         flightlog.tmp_temp,
    //         flightlog.icm20948_accel_x, flightlog.icm20948_accel_y, flightlog.icm20948_accel_z,
    //         flightlog.icm20948_gyro_x, flightlog.icm20948_gyro_y, flightlog.icm20948_gyro_z,
    //         flightlog.kf_altitude, flightlog.kf_vert_velo, flightlog.kf_accel);

    // esp_err_t ret;
    ret = s_example_write_file(datalog_path, csv_row);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't write to SD card");
    }
    // }

    //     // First create a file.
    //     const char *file_hello = MOUNT_POINT "/hello.txt";
    //     char data[MAX_CHAR_SIZE];
    //     snprintf(data, MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    //     ret = s_example_write_file(file_hello, data);
    //     if (ret != ESP_OK)
    //     {
    //         return;
    //     }

    //     const char *file_foo = MOUNT_POINT "/foo.txt";

    //     // Check if destination file exists before renaming
    //     struct stat st;
    //     if (stat(file_foo, &st) == 0)
    //     {
    //         // Delete it if it exists
    //         unlink(file_foo);
    //     }

    //     // Rename original file
    //     ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    //     if (rename(file_hello, file_foo) != 0)
    //     {
    //         ESP_LOGE(TAG, "Rename failed");
    //         return;
    //     }

    //     ret = s_example_read_file(file_foo);
    //     if (ret != ESP_OK)
    //     {
    //         return;
    //     }

    //     // Format FATFS
    // #ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    //     ret = esp_vfs_fat_sdcard_format(mount_point, card);
    //     if (ret != ESP_OK)
    //     {
    //         ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
    //         return;
    //     }

    //     if (stat(file_foo, &st) == 0)
    //     {
    //         ESP_LOGI(TAG, "file still exists");
    //         return;
    //     }
    //     else
    //     {
    //         ESP_LOGI(TAG, "file doesn't exist, formatting done");
    //     }
    // #endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

    //     const char *file_nihao = MOUNT_POINT "/nihao.txt";
    //     memset(data, 0, MAX_CHAR_SIZE);
    //     snprintf(data, MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    //     ret = s_example_write_file(file_nihao, data);
    //     if (ret != ESP_OK)
    //     {
    //         return;
    //     }

    //     // Open file for reading
    //     ret = s_example_read_file(file_nihao);
    //     if (ret != ESP_OK)
    //     {
    //         return;
    //     }

    //     // All done, unmount partition and disable SPI peripheral
    //     esp_vfs_fat_sdcard_unmount(mount_point, card);
    //     ESP_LOGI(TAG, "Card unmounted");

    //     // deinitialize the bus after all devices are removed
    //     spi_bus_free(static_cast<spi_host_device_t>(host.slot));

    //     // Deinitialize the power control driver if it was used
    // #if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    //     ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    //     if (ret != ESP_OK)
    //     {
    //         ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
    //         return;
    //     }
    // #endif
}

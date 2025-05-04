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
#include "bmp581.h"

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
    ////// BMP581
    float pressure;
    float bmp_temp;
    float bmp_altitude;

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
    float icm20948_temp;
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
    snprintf(data, MAX_CHAR_SIZE, "pressure,bmp_temp,bmp_altitude,icm20948_accel_x,icm20948_accel_y,icm20948_accel_z,icm20948_gyro_x,icm20948_gyro_y,icm20948_gyro_z,icm20948_mag_x,icm20948_mag_y,icm20948_mag_z,icm20948_temp\n");
    ret = write_file("/sdcard/datalog.csv", data);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Couldn't write to SD card");
    }

    // this is where data will be populated
    flight_data flightlog;

    static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, CONFIG_I2C_MASTER_FREQUENCY);
    bmp581.bmp581_configure();
    bmp581_data sample;

    static ICM20948 icm(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x68, CONFIG_I2C_MASTER_FREQUENCY);
    ESP_LOGI(TAG, "ICM20948 object initialized");
    icm.configureICM20948(ICM20948::ACCEL_FS_2G, ICM20948::GYRO_FS_1000DPS);

    ESP_LOGI(TAG, "Beginning sensor reading");
    while (true)
    {
        sample = bmp581.bmp581_get_sample();
        flightlog.pressure = (double)sample.pressure;
        flightlog.bmp_temp = sample.temperature_c;
        flightlog.altitude = sample.altitude;
        ESP_LOGI(TAG, "bmp581 readings obtained");

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
        flightlog.icm20948_temp = icm.getTemp();
        ESP_LOGI(TAG, "imu readings obtained");

        char csv_row[MAX_CHAR_SIZE];
        sprintf(csv_row, "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
                flightlog.pressure, flightlog.bmp_temp, flightlog.bmp_altitude,
                flightlog.icm20948_accel_x, flightlog.icm20948_accel_y, flightlog.icm20948_accel_z,
                flightlog.icm20948_gyro_x, flightlog.icm20948_gyro_y, flightlog.icm20948_gyro_z,
                flightlog.icm20948_mag_x, flightlog.icm20948_mag_y, flightlog.icm20948_mag_z flightlog.icm20948_temp);

        esp_err_t ret;
        ret = s_example_write_file("/sdcard/datalog.csv", csv_row);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Couldn't write to SD card");
        }

        // ESP_LOGI(TAG,
        //          "ADXL: (%.3f, %.3f, %.3f) g | "
        //          "BMP581: P=%.2f Pa  T=%.2f °C  Alt=%.2f m | "
        //          "TMP1075: %.2f °C | "
        //          "ICM20948 acc: (%.3f, %.3f, %.3f) g  gyro: (%.2f, %.2f, %.2f) rad/s",
        //          flightlog.adxl375_accel_x, flightlog.adxl375_accel_y, flightlog.adxl375_accel_z,
        //          flightlog.pressure, flightlog.bmp_temp, flightlog.altitude,
        //          flightlog.tmp_temp,
        //          flightlog.icm20948_accel_x, flightlog.icm20948_accel_y, flightlog.icm20948_accel_z,
        //          flightlog.icm20948_gyro_x, flightlog.icm20948_gyro_y, flightlog.icm20948_gyro_z);

        // vTaskDelay(pdMS_TO_TICKS(3000));

        // float kf_accels[3] = {icm_accels.accel_x, icm_accels.accel_y, icm_accels.accel_z};
        // float kf_gyros[3] = {gyros.gyro_x, gyros.gyro_y, gyros.gyro_z};

        // StateDeterminer state_determiner = StateDeterminer();
        // kf_vals thevals = state_determiner.determineState(kf_accels, kf_gyros, sample.altitude, esp_timer_get_time() / 1000ULL);
        // flightlog.kf_altitude = thevals.kf_altitude;
        // flightlog.kf_vert_velo = thevals.kf_vert_velo;
        // flightlog.kf_accel = thevals.kf_accel;
    }
}

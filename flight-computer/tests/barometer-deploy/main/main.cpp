#include <stdio.h>
#include <iostream>
#include <driver/i2c_master.h>
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
#include "WMA.h"

// sd card shit
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"

// sensor drivers
#include "bmp581.h"

// sd io defines + constants
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#define MAX_CHAR_SIZE 1024
static const char *TAG = "example";
#define MOUNT_POINT "/sdcard"
const char *datalog_path = MOUNT_POINT "/datalog.csv";

#ifdef CONFIG_DEBUG_PIN_CONNECTIONS
const char *names[] = {"CLK ", "MOSI", "MISO", "CS  "};
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

#define LOCKOUT_ALTITUDE 500.0f // meters
#define STD 0.097676f

// i2c shit
#define I2C_MASTER_NUM I2C_NUM_0

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

esp_err_t mount_filesystem(sdmmc_card_t **card)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
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
    host.max_freq_khz = 8000;

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return ret;
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
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the CONFIG_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
#ifdef CONFIG_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return ret;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties

    sdmmc_card_print_info(stdout, *card);

    return ret;
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

extern "C" void app_main()
{
    i2c_bus_init();
    sdmmc_card_t *card = nullptr;
    esp_err_t ret;
    ret = mount_filesystem(&card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount filesystem");
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_22);
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(0);
    gpio_config(&io_conf);

    io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_23);
    io_conf.pull_down_en = static_cast<gpio_pulldown_t>(0);
    io_conf.pull_up_en = static_cast<gpio_pullup_t>(0);
    gpio_config(&io_conf);
    // main = 23, drogue = 22

    // settting up the datalog csv file
    // char data[MAX_CHAR_SIZE];
    // snprintf(data, MAX_CHAR_SIZE, "time,pressure,bmp_temp,bmp_altitude,sma\n");
    // ret = s_example_write_file(datalog_path, data);
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Couldn't write to SD card");
    // }

    static BMP581 bmp581(I2C_NUM_0, I2C_ADDR_BIT_LEN_7, 0x46, CONFIG_I2C_MASTER_FREQUENCY);
    bmp581.bmp581_configure();
    bmp581_data sample;

    WeightedMovingAverage wma(32);
    float max_alt = 0;

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        sample = bmp581.bmp581_get_sample();
        wma.addSample(sample.altitude);
        double curr_wma = wma.getAverage();

        // now writing to sd card
        // char csv_row[MAX_CHAR_SIZE];
        // sprintf(csv_row, "%llu,%.6f,%.6f,%.6f,%lf", esp_timer_get_time() / 1000ULL, static_cast<double>(sample.pressure),
        //         sample.temperature_c, sample.altitude, curr_wma);

        // if (sample.altitude > LOCKOUT_ALTITUDE)
        // {
        //     if (curr_wma < (max_alt - (6.8 * STD)))
        //     {
        //         // Set GPIO22 to high
        //         gpio_set_level(GPIO_NUM_22, 1);
        //     }
        // }

        gpio_set_level(GPIO_NUM_22, 1);
        printf("drogue open\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
        gpio_set_level(GPIO_NUM_22, 0);
        printf("drogue close\n");

        vTaskDelay(pdMS_TO_TICKS(1000));

        gpio_set_level(GPIO_NUM_23, 1);
        printf("main open\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
        gpio_set_level(GPIO_NUM_23, 0);
        printf("main close\n");

        // ESP_LOGI(TAG, "Reading:%.6f", sample.altitude);
        // printf("Reading:%.6f\n", sample.altitude);
        // fflush(stdout);

        // esp_err_t ret;
        // ret = s_example_write_file(datalog_path, csv_row);
        // if (ret != ESP_OK)
        // {
        //     ESP_LOGE(TAG, "Couldn't write to SD card");
        // }
    }
}

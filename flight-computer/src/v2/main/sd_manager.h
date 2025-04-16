#include <cstdio>
#include <cstring>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

class SdCardManager
{
public:
    SdCardManager()
        : card_(nullptr), is_mounted_(false)
    {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        mount_config_.format_if_mount_failed = true;
#else
        mount_config_.format_if_mount_failed = false;
#endif
        mount_config_.max_files = 5;
        mount_config_.allocation_unit_size = 16 * 1024;

        snprintf(mount_point_, sizeof(mount_point_), "/sdcard");
    }

    ~SdCardManager()
    {
        // automatically unmount (and free bus) when destructed if still mounted
        unmount();
    }

    // mount the filesystem on the sd card via SPI
    esp_err_t mount()
    {
        if (is_mounted_)
        {
            ESP_LOGI((const char *)"sdcard_init", "SD card is already mounted.");
            return ESP_OK;
        }

        ESP_LOGI((const char *)"sdcard_init", "Initializing SD card (SPI mode).");

        host_ = SDSPI_HOST_DEFAULT();

#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
        sd_pwr_ctrl_ldo_config_t ldo_config = {
            .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
        };
        sd_pwr_ctrl_handle_t pwr_ctrl_handle = nullptr;
        esp_err_t ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE((const char *)"sdcard_init", "Failed to create on-chip LDO power control driver");
            return ret;
        }
        host_.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

        // init SPI bus
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = CONFIG_PIN_MOSI,
            .miso_io_num = CONFIG_PIN_MISO,
            .sclk_io_num = CONFIG_PIN_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
        };
        esp_err_t ret = spi_bus_initialize(host_.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
        if (ret != ESP_OK)
        {
            ESP_LOGE((const char *)"sdcard_init", "Failed to initialize SPI bus.");
            return ret;
        }

        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = CONFIG_SD_PIN_CS;
        slot_config.host_id = host_.slot;

        ESP_LOGI((const char *)"sdcard_init", "Mounting FAT filesystem at '%s'", mount_point_);
        ret = esp_vfs_fat_sdspi_mount(mount_point_, &host_, &slot_config,
                                      &mount_config_, &card_);
        if (ret != ESP_OK)
        {
            spi_bus_free(host_.slot);
            if (ret == ESP_FAIL)
            {
                ESP_LOGE((const char *)"sdcard_init", "Failed to mount filesystem. "
                                                      "If you want the card to be formatted, enable CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED.");
            }
            else
            {
                ESP_LOGE((const char *)"sdcard_init", "Failed to initialize the card (%s). "
                                                      "Make sure SD card lines have pull-up resistors.",
                         esp_err_to_name(ret));
            }
            return ret;
        }

        ESP_LOGI((const char *)"sdcard_init", "Filesystem mounted successfully.");
        sdmmc_card_print_info(stdout, card_);
        is_mounted_ = true;
        return ESP_OK;
    }

    // unmount the SD card and free the SPI bus.
    void unmount()
    {
        if (!is_mounted_)
        {
            return;
        }
        ESP_LOGI((const char *)"sdcard_init", "Unmounting SD card from '%s'", mount_point_);

        // unmount and free resources
        esp_vfs_fat_sdcard_unmount(mount_point_, card_);
        spi_bus_free(host_.slot);
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
        if (host_.pwr_ctrl_handle)
        {
            sd_pwr_ctrl_del_on_chip_ldo(host_.pwr_ctrl_handle);
            host_.pwr_ctrl_handle = nullptr;
        }
#endif
        card_ = nullptr;
        is_mounted_ = false;
    }

    esp_err_t writeFile(const char *path, const char *data)
    {
        if (!is_mounted_)
        {
            return ESP_ERR_INVALID_STATE;
        }

        FILE *f = fopen(path, "w");
        if (!f)
        {
            ESP_LOGE((const char *)"sdcard_init", "Failed to open file '%s' for writing.", path);
            return ESP_FAIL;
        }
        fclose(f);
        return ESP_OK;
    }

    esp_err_t readFile(const char *path)
    {
        if (!is_mounted_)
        {
            ESP_LOGE((const char *)"sdcard_init", "SD card not mounted; cannot read file.");
            return ESP_ERR_INVALID_STATE;
        }

        FILE *f = fopen(path, "r");
        if (!f)
        {
            ESP_LOGE((const char *)"sdcard_init", "Failed to open file '%s' for reading.", path);
            return ESP_FAIL;
        }
        char line[MAX_CHAR_SIZE];
        if (fgets(line, sizeof(line), f) == nullptr)
        {
            ESP_LOGE((const char *)"sdcard_init", "Error reading from file '%s'.", path);
            fclose(f);
            return ESP_FAIL;
        }
        fclose(f);

        return ESP_OK;
    }

    // format mounted FAT filesystem
    esp_err_t formatCard()
    {
        if (!is_mounted_)
        {
            ESP_LOGE((const char *)"sdcard_init", "SD card not mounted; cannot format.");
            return ESP_ERR_INVALID_STATE;
        }

        esp_err_t ret = esp_vfs_fat_sdcard_format(mount_point_, card_);
        if (ret != ESP_OK)
        {
            ESP_LOGE((const char *)"sdcard_init", "Failed to format FATFS (%s)", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI((const char *)"sdcard_init", "Formatting complete.");
        }
        return ret;
    }

private:
    sdmmc_card_t *card_;
    bool is_mounted_;
    sdmmc_host_t host_;
    esp_vfs_fat_sdmmc_mount_config_t mount_config_;
    char mount_point_[16];
};
// TODO: Figure out which of this stuff i dont need to include
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include <sys/time.h>
#include "esp_system.h"
#include "esp_log.h"

#define TIMEOUT_LIMIT_MS 50

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
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
}

i2c_master_dev_handle_t i2c_create_device(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                                          uint16_t dev_address, uint32_t scl_clk_speed)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = addr_len,
        .device_address = dev_address,
        .scl_speed_hz = scl_clk_speed,
    };

    // grab the i2c bus given port and connect that shii
    i2c_master_bus_handle_t bus_handle;
    static i2c_master_dev_handle_t dev_handle;
    // TODO: add logging to the statements below
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_bus_add_device(bus_handle, &dev_cfg, dev_handle));
    return dev_handle;
}

void i2c_remove_device(i2c_master_dev_handle_t dev_handle) { i2c_master_bus_rm_device(dev_handle); }

esp_err_t i2c_write(i2c_master_dev_handle_t sensor,
                    uint8_t const *data_buf, const uint8_t data_len)
{
    esp_err_t ret = i2c_master_transmit(sensor, data_buf, data_len, TIMEOUT_LIMIT_MS);
    return ret;
}

esp_err_t i2c_read(i2c_master_dev_handle_t sensor, const uint8_t reg_start_addr, uint8_t *rx, uint8_t rx_size)
{
    const uint8_t tx[] = {reg_start_addr};

    esp_err_t ret = i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, TIMEOUT_LIMIT_MS);
    return ret;
}
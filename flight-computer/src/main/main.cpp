#include <stdio.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sensors/bmp581/bmp5.h"
#include <driver/i2c_master.h>
// #include <driver/i2c.h>

#define ONE_SECOND_DELAY pdMS_TO_TICKS(1000)

// took this from bosch's examples, https://github.com/boschsensortec/BMP5_SensorAPI/blob/master/examples/read_sensor_data_normal_mode/read_sensor_data_normal_mode.c
/******************************************************************************/
/*!         Static Function Declaration                                       */

/*!
 *  @brief This internal API is used to set configurations of the sensor.
 *
 *  @param[in,out] osr_odr_press_cfg : Structure instance of bmp5_osr_odr_press_config
 *  @param[in] dev                   : Structure instance of bmp5_dev.
 *
 *  @return Status of execution.
 */
static int8_t set_config(struct bmp5_osr_odr_press_config *osr_odr_press_cfg, struct bmp5_dev *dev);

/*!
 *  @brief This internal API is used to get sensor data.
 *
 *  @param[in] osr_odr_press_cfg : Structure instance of bmp5_osr_odr_press_config
 *  @param[in] dev               : Structure instance of bmp5_dev.
 *
 *  @return Status of execution.
 */
static int8_t get_sensor_data(const struct bmp5_osr_odr_press_config *osr_odr_press_cfg, struct bmp5_dev *dev);

/******************************************************************************/

// bullshit sensor class, will rework into abstract sensor class
// class Sensor
// {
// public:
//     Sensor(gpio_num_t pin) : pin_(pin)
//     {
//         esp_rom_gpio_pad_select_gpio(pin_);
//         gpio_set_direction(pin_, GPIO_MODE_INPUT);
//     }

//     int read() const
//     {
//         return gpio_get_level(pin_);
//     }

// private:
//     gpio_num_t pin_;
// };

extern "C" void app_main()
{

    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_NUM_0;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(22);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(21);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // i2c_device_config_t dev_cfg = {
    //     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    //     .device_address = BMP5_I2C_ADDR_PRIM,
    //     .scl_speed_hz = 100000,
    // };

    // i2c_master_dev_handle_t dev_handle;
    // ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    int8_t rslt;
    struct bmp5_dev dev;
    dev.intf = BMP5_I2C_INTF;
    struct bmp5_osr_odr_press_config osr_odr_press_cfg = {0};

    /* Interface reference is given as a parameter
     * For I2C : BMP5_I2C_INTF
     * For SPI : BMP5_SPI_INTF
     */
    const TickType_t xDelay = 5000 / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    rslt = bmp5_init(&dev);
    bmp5_error_codes_print_result("bmp5_init", rslt);

    // if (rslt == BMP5_OK)
    // {
    //     rslt = set_config(&osr_odr_press_cfg, &dev);
    //     bmp5_error_codes_print_result("set_config", rslt);
    // }

    // if (rslt == BMP5_OK)
    // {
    //     rslt = get_sensor_data(&osr_odr_press_cfg, &dev);
    //     bmp5_error_codes_print_result("get_sensor_data", rslt);
    // }
}

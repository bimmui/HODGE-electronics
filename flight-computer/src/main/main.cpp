#include <stdio.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sensors/bmp581/bmp5.h"
#include <driver/i2c.h>

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

int8_t i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    // implementing i2c read function using ESP-IDF, not as easy
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_id << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd); // repeated start condition to switch from writing to reading
    i2c_master_write_byte(cmd, (dev_id << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, ONE_SECOND_DELAY);
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? 0 : -1;
}

int8_t i2c_write(uint8_t dev_id, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    // implementing i2c write function using ESP-IDF, easy peasy
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev_id << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, ONE_SECOND_DELAY);
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? 0 : -1;
}

extern "C" void app_main()
{
    // configuring the esp32 as master
    i2c_config_t config;
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = static_cast<gpio_num_t>(6);
    config.scl_io_num = static_cast<gpio_num_t>(7);
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = 100000;
    i2c_param_config(I2C_NUM_0, &config);
    i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0);

    // apparently i dont need to do shit for the bmp581 bc of their api
    // so the stuff above with setting esp32 as master is useless rn

    int8_t rslt;
    struct bmp5_dev dev;
    struct bmp5_osr_odr_press_config osr_odr_press_cfg = {0};

    /* Interface reference is given as a parameter
     * For I2C : BMP5_I2C_INTF
     * For SPI : BMP5_SPI_INTF
     */

    rslt = bmp5_init(&dev);
    bmp5_error_codes_print_result("bmp5_init", rslt);

    if (rslt == BMP5_OK)
    {
        rslt = set_config(&osr_odr_press_cfg, &dev);
        bmp5_error_codes_print_result("set_config", rslt);
    }

    if (rslt == BMP5_OK)
    {
        rslt = get_sensor_data(&osr_odr_press_cfg, &dev);
        bmp5_error_codes_print_result("get_sensor_data", rslt);
    }

    (void)fflush(stdout);
}

static int8_t set_config(struct bmp5_osr_odr_press_config *osr_odr_press_cfg, struct bmp5_dev *dev)
{
    int8_t rslt;
    struct bmp5_iir_config set_iir_cfg;
    struct bmp5_int_source_select int_source_select;

    rslt = bmp5_set_power_mode(BMP5_POWERMODE_STANDBY, dev);
    bmp5_error_codes_print_result("bmp5_set_power_mode1", rslt);

    if (rslt == BMP5_OK)
    {
        /* Get default odr */
        rslt = bmp5_get_osr_odr_press_config(osr_odr_press_cfg, dev);
        bmp5_error_codes_print_result("bmp5_get_osr_odr_press_config", rslt);

        if (rslt == BMP5_OK)
        {
            /* Set ODR as 50Hz */
            osr_odr_press_cfg->odr = BMP5_ODR_50_HZ;

            /* Enable pressure */
            osr_odr_press_cfg->press_en = BMP5_ENABLE;

            /* Set Over-sampling rate with respect to odr */
            osr_odr_press_cfg->osr_t = BMP5_OVERSAMPLING_64X;
            osr_odr_press_cfg->osr_p = BMP5_OVERSAMPLING_4X;

            rslt = bmp5_set_osr_odr_press_config(osr_odr_press_cfg, dev);
            bmp5_error_codes_print_result("bmp5_set_osr_odr_press_config", rslt);
        }

        if (rslt == BMP5_OK)
        {
            set_iir_cfg.set_iir_t = BMP5_IIR_FILTER_COEFF_1;
            set_iir_cfg.set_iir_p = BMP5_IIR_FILTER_COEFF_1;
            set_iir_cfg.shdw_set_iir_t = BMP5_ENABLE;
            set_iir_cfg.shdw_set_iir_p = BMP5_ENABLE;

            rslt = bmp5_set_iir_config(&set_iir_cfg, dev);
            bmp5_error_codes_print_result("bmp5_set_iir_config", rslt);
        }

        if (rslt == BMP5_OK)
        {
            rslt = bmp5_configure_interrupt(BMP5_PULSED, BMP5_ACTIVE_HIGH, BMP5_INTR_PUSH_PULL, BMP5_INTR_ENABLE, dev);
            bmp5_error_codes_print_result("bmp5_configure_interrupt", rslt);

            if (rslt == BMP5_OK)
            {
                /* Note : Select INT_SOURCE after configuring interrupt */
                int_source_select.drdy_en = BMP5_ENABLE;
                rslt = bmp5_int_source_select(&int_source_select, dev);
                bmp5_error_codes_print_result("bmp5_int_source_select", rslt);
            }
        }

        /* Set powermode as normal */
        rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, dev);
        bmp5_error_codes_print_result("bmp5_set_power_mode", rslt);
    }

    return rslt;
}

static int8_t get_sensor_data(const struct bmp5_osr_odr_press_config *osr_odr_press_cfg, struct bmp5_dev *dev)
{
    int8_t rslt = 0;
    uint8_t idx = 0;
    uint8_t int_status;
    struct bmp5_sensor_data sensor_data;

    printf("\nOutput :\n\n");
    printf("Data, Pressure (Pa), Temperature (deg C)\n");

    while (idx < 50)
    {
        rslt = bmp5_get_interrupt_status(&int_status, dev);
        bmp5_error_codes_print_result("bmp5_get_interrupt_status", rslt);

        if (int_status & BMP5_INT_ASSERTED_DRDY)
        {
            rslt = bmp5_get_sensor_data(&sensor_data, osr_odr_press_cfg, dev);
            bmp5_error_codes_print_result("bmp5_get_sensor_data", rslt);

            if (rslt == BMP5_OK)
            {
#ifdef BMP5_USE_FIXED_POINT
                printf("%d, %lu, %ld\n", idx, (long unsigned int)sensor_data.pressure,
                       (long int)sensor_data.temperature);
#else
                printf("%d, %f, %f\n", idx, sensor_data.pressure, sensor_data.temperature);
#endif

                idx++;
            }
        }
    }

    return rslt;
}
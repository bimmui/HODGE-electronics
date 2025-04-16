#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "LSM9DS1_ESP_IDF.h"
#include "NewUW_MahonyAHRS.h"
#include "AccelStepper.h"
#include <iostream>

// bring some of these defines to the Kconfig file
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA GPIO_NUM_21
#define I2C_MASTER_SCL GPIO_NUM_22
#define I2C_MASTER_FREQ 100000
#define SPR 200
#define SAMPLE_COUNT 100
const float Gscale = (M_PI / 180.0f) * 0.00875f;
const float G_offset[3] = {0.0603f, 0.0349f, -0.0783f};

const float A_B[3] = {-0.347f, -0.1033f, -0.0303f};
const float A_Ainv[3][3] = {
    {0.001025f, -0.00001f, 0.000004f},
    {-0.00001f, 0.001009f, 0.000006f},
    {0.000004f, 0.000006f, 0.001009f}};

const float M_B[3] = {40.991f, -24.653f, -1.26f};
const float M_Ainv[3][3] = {
    {0.580428f, 0.030876f, -0.007f},
    {0.030876f, 0.573477f, -0.016433f},
    {-0.007437f, -0.016433f, 0.633181f}};

float declination = 0; // degrees, no need when outside
float Kp = 50.0f;
float Ki = 0.0f;

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define DELAY_1S DELAY(1000)
#define DELAY_1MS DELAY(1)

static const char *MAIN_TAG = "LSM9DS1_main";
static const char *AVG_YAW = "average_yaw_calc";
static const char *CURRENT_OUTPUTS = "CurrMahonyAHRS";

int radians_to_steps(double radians)
{
    return static_cast<int>((radians / (2.0 * M_PI)) * SPR);
}

long degrees_to_radians(float degrees)
{
    return degrees * (M_PI / 180);
}

float calc_average_yaw(LSM9DS1_ESP_IDF &lsm, MahonyAHRS &ahrs)
{
    double now = 0;
    double last = 0;
    float yaw_sum = 0;
    float yaw_avg = 0;

    ESP_LOGI(AVG_YAW, "Beginning average yaw calculation...");

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        sensors_event_t aevt, megt, gevt, tevt;
        lsm.getEvent(&aevt, &megt, &gevt, &tevt);

        now = esp_timer_get_time();
        double deltat = (now - last) * 1.0e-6; // seconds since last update
        last = now;
        // ESP_LOGI(TIME, "now: %.2lf    last: %.2lf    delta: %.2lf",
        //          now, last, deltat);

        euler_angles angles = ahrs.updateIMU(aevt, megt, gevt, deltat);
        ESP_LOGI(CURRENT_OUTPUTS, "Yaw: %.2f degrees Pitch: %.2f degrees Roll %.2f degrees",
                 angles.yaw, angles.pitch, angles.roll);

        yaw_sum += angles.yaw;
    }
    yaw_avg = yaw_sum / SAMPLE_COUNT;

    ESP_LOGI(AVG_YAW, "Average yaw: %lf", yaw_avg);

    return yaw_avg;
}

extern "C" void app_main(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_MASTER_NUM;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle = NULL;
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(MAIN_TAG, "Failed to create I2C bus: %s", esp_err_to_name(err));
        return;
    }

    // 2) Instantiate LSM9DS1 object and AccelStepper object
    LSM9DS1_ESP_IDF lsm;
    AccelStepper stepper(AccelStepper::DRIVER, static_cast<gpio_num_t>(CONFIG_STEP_PIN), static_cast<gpio_num_t>(CONFIG_DIRECTION_PIN));
    MahonyAHRS ahrs(Gscale, G_offset,
                    A_B, A_Ainv,
                    M_B, M_Ainv,
                    declination,
                    Kp, Ki);

    // stepper.setMaxSpeed(2000);
    // stepper.setAcceleration(1500);

    // 3) Init I2C with 7-bit addresses for both subchips in the LSM9DS1
    err = lsm.initI2C(bus_handle,
                      LSM9DS1_ADDRESS_ACCELGYRO,
                      LSM9DS1_ADDRESS_MAG,
                      I2C_MASTER_FREQ);
    if (err != ESP_OK)
    {
        ESP_LOGE(MAIN_TAG, "LSM9DS1 initI2C failed: %s", esp_err_to_name(err));
        return;
    }

    // 4) Begin (check IDs, do resets, etc.)
    if (!lsm.begin())
    {
        ESP_LOGE(MAIN_TAG, "LSM9DS1 not detected or config error!");
        return;
    }
    ESP_LOGI(MAIN_TAG, "LSM9DS1 found and initialized.");

    // 5) Setting up the stepper
    stepper.setMaxSpeed(2000);
    stepper.setAcceleration(1500);

    // 6) Getting average angular distance from true north and pointing there
    float distance_to_north = calc_average_yaw(lsm, ahrs);
    distance_to_north = 45.0f;
    // stepper.moveTo(-radians_to_steps(degrees_to_radians(distance_to_north)));
    stepper.runToNewPosition(radians_to_steps(degrees_to_radians(distance_to_north)));
}

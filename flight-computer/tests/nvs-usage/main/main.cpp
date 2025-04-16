#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"

#define NVS_NAMESPACE "storage"
#define KEY_COUNT "access_count"
#define KEY_ARRAY1D "array_1d"
#define KEY_ARRAY2D "array_2d"
#define KEY_NUMBER "some_number"

void write_nvs_data(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h));

    // update access counter
    int32_t access_count = 0;
    nvs_get_i32(h, KEY_COUNT, &access_count);
    access_count++;
    ESP_ERROR_CHECK(nvs_set_i32(h, KEY_COUNT, access_count));

    // write a 1D array of 5 ints
    int arr1d[5] = {10, 20, 30, 40, 50};
    ESP_ERROR_CHECK(nvs_set_blob(h, KEY_ARRAY1D, arr1d, sizeof(arr1d)));

    // write a 2×3 2D array of floats
    float arr2d[2][3] = {{1.1f, 2.2f, 3.3f}, {4.4f, 5.5f, 6.6f}};
    ESP_ERROR_CHECK(nvs_set_blob(h, KEY_ARRAY2D, arr2d, sizeof(arr2d)));

    // write a double
    double value = 3.14159;
    ESP_ERROR_CHECK(nvs_set_blob(h, KEY_NUMBER, &value, sizeof(value)));

    ESP_ERROR_CHECK(nvs_commit(h));
    nvs_close(h);
}

void read_nvs_data(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READONLY, &h));

    // read and print access counter
    int32_t access_count = 0;
    nvs_get_i32(h, KEY_COUNT, &access_count);
    printf("NVS accessed %ld time(s)\n", access_count);

    // 1D array
    int arr1d[5];
    size_t size = sizeof(arr1d);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_ARRAY1D, arr1d, &size));
    printf("1D array: ");
    for (int i = 0; i < 5; i++)
        printf("%d ", arr1d[i]);
    printf("\n");

    // 2D array
    float arr2d[2][3];
    size = sizeof(arr2d);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_ARRAY2D, arr2d, &size));
    printf("2D array:\n");
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 3; j++)
            printf("%.2f ", arr2d[i][j]);
        printf("\n");
    }

    // that single number
    double value;
    size = sizeof(value);
    ESP_ERROR_CHECK(nvs_get_blob(h, KEY_NUMBER, &value, &size));
    printf("Stored number = %.5f\n", value);

    nvs_close(h);
}

extern "C" void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    write_nvs_data();
    read_nvs_data();
}

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))

extern "C" void app_main()
{

    while (1)
    {
        printf("This is ESP 2\n");
        DELAY(1000); // Delay for 1 second
    }
}
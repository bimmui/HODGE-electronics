#pragma once

/**
 * The size of a thread stack, in bytes.
 */
#define STACK_SIZE 8192

/**
 * The ESP32 has two physical cores, which will each be dedicated to one group of threads.
 * The SENSOR_CORE runs the threads which write to the sensor_data struct (mostly sensor polling threads).
 */
#define SENSOR_CORE (0)

/**
 * The ESP32 has two physical cores, which will each be dedicated to one group of threads.
 * The DATA_CORE runs the GPS thread, as well as the threads which read from the sensor_data struct (e.g. SD logging).
 */
#define DATA_CORE (1)

#define MAX_CHAR_SIZE 512

#define INIT_FILE "/sdcard/sys_init.txt"

#define DATA_FILE "/sdcard/datalog.txt"
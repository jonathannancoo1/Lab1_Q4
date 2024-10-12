#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/times.h> // ask about this

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_sleep.h"

#include "driver/gpio.h"
#include "driver/hw_timer.h"
#include "driver/uart.h"

#define NUM_SLOTS       4
#define NUM_CYCLES      5
#define SLOT_DURATION   5000
#define BUFFER_SIZE     1024

#define ONE_SHOT_TEST      false   // testing will be done without auto reload (one-shot)
#define RELOAD_TEST        true    // testing will be done with auto reload

int g_timerCounter = 0;
int g_ticksPerSec = 0, g_cycleIdx = 0, g_slotIdx = 0;
int g_currentTime, g_prevTime, g_burnStart;

struct TimeRecord {
    int minutes;
    int seconds;
    int milliseconds;
};

struct TimeRecord g_timeNow;

void timerCallback(void* arg)
{
    g_timerCounter++;
    if (g_timerCounter == 10) {
        g_timeNow.milliseconds++;
        if (g_timeNow.milliseconds == 1000) {
            g_timeNow.seconds++;
            g_timeNow.milliseconds = 0;
            if (g_timeNow.seconds == 60) {
                g_timeNow.minutes++;
                g_timeNow.seconds = 0;
            }
        }
        g_timerCounter = 0;
    }
}

void configureUART()
{
    uint8_t* uartData = (uint8_t*)malloc(BUFFER_SIZE);

    uart_config_t uartConfig;
    uartConfig.baud_rate = 9600;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;

    uart_param_config(UART_NUM_0, &uartConfig);
    uart_driver_install(UART_NUM_0, BUFFER_SIZE * 2, 0, 0, NULL, 0);
}

void enterSleep(uint32_t duration)
{
    esp_sleep_enable_timer_wakeup(duration);
    esp_light_sleep_start();
}

void taskOne()
{
    const char* msg = "Task one running \n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    enterSleep(1000000);
}

void taskTwo()
{
    const char* msg = "Task two running \n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    enterSleep(1000000);
}

void taskThree()
{
    const char* msg = "Task three running \n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    enterSleep(1000000);
}

void taskFour()
{
    const char* msg = "Task four running \n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));
    enterSleep(1000000);
}

int getTimeInSeconds(struct TimeRecord timeRec)
{
    return timeRec.seconds;
}

void burnTime()
{
    const char* msg = "I am stuck in burn \n";
    uart_write_bytes(UART_NUM_0, msg, strlen(msg));

    g_currentTime = getTimeInSeconds(g_timeNow);
    while ((g_currentTime = getTimeInSeconds(g_timeNow) - g_prevTime) < 5)
    {
        /* Burn time here */
    }
    printf("burn time = %2.2ds\n\n", (getTimeInSeconds(g_timeNow) - g_currentTime));
    g_prevTime = g_currentTime;
    g_cycleIdx = NUM_CYCLES;
}

void (*taskTable[NUM_SLOTS][NUM_CYCLES])() = {
    {taskOne, taskTwo, burnTime, burnTime, burnTime},
    {taskOne, taskThree, burnTime, burnTime, burnTime},
    {taskOne, taskFour, burnTime, burnTime, burnTime},
    {burnTime, burnTime, burnTime, burnTime, burnTime}
};

void app_main()
{
    g_ticksPerSec = 1000;
    g_prevTime = 0;
    g_timeNow.milliseconds = 0;
    g_timeNow.seconds = 0;
    g_timeNow.minutes = 0;

    hw_timer_init(timerCallback, NULL);
    hw_timer_alarm_us(100, RELOAD_TEST);
    hw_timer_get_intr_type(1);
    hw_timer_set_reload(1);
    hw_timer_set_load_data(100);

    configureUART();

    g_ticksPerSec = 10000; // number of clock ticks per second
    printf("clock ticks/sec = %d\n\n", g_ticksPerSec);

    while (1) {
        for (g_slotIdx = 0; g_slotIdx < NUM_SLOTS; g_slotIdx++) {
            for (g_cycleIdx = 0; g_cycleIdx < NUM_CYCLES; g_cycleIdx++) {
                (*taskTable[g_slotIdx][g_cycleIdx])();
            }
        }
    }
}
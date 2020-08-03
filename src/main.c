/**
 * Simple LED blinking example for the Tiva Launchpad
 */
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include <utils/ustdlib.h>

#include <heli_config.h>

#include <heli/heli.h>
#include <heli/yaw.h>
#include <heli/height.h>
#include <heli/logging.h>
#include <heli/heli_display.h>

#include <FreeRTOSConfig.h>

#include "FreeRTOS.h"
#include "task.h"

// Blinky function
void BlinkLED(void *pvParameters)
{
    unsigned int whichLed = (unsigned int)pvParameters; /* While pvParameters is technically a pointer, a pointer is nothing
                                                         * more than an unsigned integer of size equal to the architecture's
                                                         * memory address bus width, which is 32-bits in ARM.  We're abusing
                                                         * the parameter then to hold a simple integer value.  Could also have
                                                         * used this as a pointer to a memory location holding the value, but
                                                         * our method uses less memory.
                                                         */


    const uint8_t whichBit = 1 << whichLed; // TivaWare GPIO calls require the pin# as a binary bitmask, not a simple number.
    // Alternately, we could have passed the bitmask into pvParameters instead of a simple number.

    uint8_t currentValue = 0;

    while (1) {
        currentValue ^= whichBit; // XOR keeps flipping the bit on / off alternately each time this runs.
        GPIOPinWrite(GPIO_PORTF_BASE, whichBit, currentValue);
        vTaskDelay(1000 / portTICK_RATE_MS);  // Suspend this task (so others may run) for 1000ms or as close as we can get with the current RTOS tick setting.
    }
    // No way to kill this blinky task unless another task has an xTaskHandle reference to it and can use vTaskDelete() to purge it.
}

void logThing(void* pvParameters) {
    while(1) {
        update_animation(0);
        vTaskDelay(100);
    }
}

void sampleHeight(void* parameters) {
    while(1) {;
        sample_height();
        vTaskDelay(1500);
    }
}

void test(void) {
    char yaw[16];
    int8_t yaw_val = get_height_percentage();
    usprintf(yaw, "%d", yaw_val);
    warn_log(yaw);
}

void errorTime(void* parameters) {
    while(1) {
        error_log("SUP");
        vTaskDelay(1000);
    }
}

int main(void)
{
    // Set the clock rate to 80 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                    SYSCTL_XTAL_16MHZ);

    // For LED blinky task - initialize GPIO port F and then pin #1 (red) for output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // activate internal bus clocking for GPIO port F
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) ; // busy-wait until GPIOF's bus clock is ready

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); // PF_1 as output
    // doesn't need too much drive strength as the RGB LEDs on the TM4C123 launchpad are switched via N-type transistors
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0); // off by default

    heli_init();
    set_max_height(988);
    set_min_height(0);

    set_adc_callback(test);

    if (pdTRUE != xTaskCreate(logThing, "Blinker", 64, (void *)1, 3, NULL)) {
        while(1);   // Oh no! Must not have had enough memory to create the task.
    }
    if (pdTRUE != xTaskCreate(sampleHeight, "Height", 64, (void *)1, 5, NULL)) {
        while(1);   // Oh no! Must not have had enough memory to create the task.
    }
    if (pdTRUE != xTaskCreate(errorTime, "Error", 64, (void *)1, 4, NULL)) {
        while(1);   // Oh no! Must not have had enough memory to create the task.
    }
    

    vTaskStartScheduler();  // Start FreeRTOS!!

    // Should never get here since the RTOS should never "exit".
    while(1);
}


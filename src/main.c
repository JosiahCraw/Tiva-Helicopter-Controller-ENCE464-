/*
 * main.c 
 *
 * Used to initalize modules and configure FreeRTOS.
 * Authors: Jos Craw, Josh Hulbert, Harry Dobbs
 */

#include <stdbool.h>
#include <stdint.h>

#include <heli/heli.h>
#include <heli/yaw.h>
#include <heli/height.h>
#include <heli/logging.h>
#include <heli/rotors.h>
#include <heli/heli_display.h>
#include <heli/menu.h>
#include <heli/heli_display.h>
#include <heli/stickman_image.h>
#include <heli/heli_status.h>
#include <heli/plot.h>
#include <heli/input.h>

#include <FreeRTOSConfig.h>

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

#include "controller.h"
#include "adc_buffer.h"

// ADC Parameters
#define ADC_BUFFER_SIZE 10
#define ADC_TICKS_PER_UPDATE 2

adc_buffer_t* g_adc_buffer;

/**
 * Inserts an ADC sample value into the ADC buffer.
 * 
 * Used as a callback for the ADC, which is called when
 * ever the ADC has a sample ready, this is then inserted
 * into the ADC buffer
 * 
 * @param sample the sample from the ADC to be inserted into the buffer
 */
void receive_adc_sample(uint32_t sample) {
    adc_buffer_insert(g_adc_buffer, (uint16_t)sample);
}

/**
 * Task to trigger an ADC sample
 * 
 * Task that triggers the ADC to take a sample
 */
void sampleHeight(TimerHandle_t timer) {
    sample_height();
}

/**
 * Timer callback for updating the control loop
 * 
 * The control loop is updated at a given rate:
 * CONTROLLER_UPDATE. This is defined within the
 * heli_config file
 */
void update_control_loop(TimerHandle_t timer) {
    update_controllers();
}

/**
 * Task to write pending UART transmissions from
 * the UART queue.
 */
void refresh_uart(void* pvParameters) {
    while(1) {
        send_uart_from_queue();
        vTaskDelay(10);
    }
}

/**
 * Task to check the menu.
 * 
 * This task regularly checks to see if the menu
 * must be updated (a button has be pressed)
 */
void refresh_menu(void* pvParameters) {
    while(1) {
        update_menu();
        vTaskDelay(25);
    }
}

/**
 * Task to update an animation.
 * 
 * Sets the animation to the next frame.
 */
void refresh_animation(void* pvParameters) {
    while(1) {
        update_animation(0); // Only one animation so animation id = 0 
        vTaskDelay(75);
    }
}

/**
 * Task that updates the current plot values 
 * to UART.
 */
void alt_plot_update(void* pvParameters) {
    vTaskDelay(3000);
    while(1) {
        display_plot_alt();
        vTaskDelay(1000);
    }
}

/**
 * Task that prints the current yaw plot values
 * to UART.
 */
void yaw_plot_update(void* pvParameters) {
    vTaskDelay(3000);
    while(1) {
        display_plot_yaw();
        vTaskDelay(1000);
    }
}

/**
 * Task that prints the current altitude plot values
 * to UART.
 */
void update_inputs(void* pvParameters) {
    while(1) {
        updateButtons();
        vTaskDelay(configTICK_RATE_HZ/CONTROLLER_UPDATE);
    }
}

int main(void)
{
    heli_init();
    init_controllers();

    set_adc_callback(receive_adc_sample);
    g_adc_buffer = init_adc_buffer(ADC_BUFFER_SIZE);

    // Menu Creation
    menu_t* main_menu = create_menu("Main Menu");

    menu_t* flight_menu = add_submenu("Flight", main_menu);
    add_menu_item("UP", flight_menu, increment_height, false);
    add_menu_item("DOWN", flight_menu, decrement_height, false);
    add_menu_item("LEFT", flight_menu, increment_angle, false);
    add_menu_item("RIGHT", flight_menu, decrement_angle, false);
    add_menu_item("MID FLIGHT", flight_menu, mid_flight_adjustment, false);
    add_menu_item("180 DEG SPIN", flight_menu, spin_180_deg, false);

    menu_t* status_menu = add_submenu("Status", main_menu);
    add_menu_item("Main PWM", status_menu, false, get_main_pwm_output);
    add_menu_item("Tail PWM", status_menu, false, get_tail_pwm_output);
    add_menu_item("Current Height", status_menu, false, get_height_percent);
    add_menu_item("Current Yaw", status_menu, false, get_yaw_slot);

    set_current_menu(main_menu);

    init_animation();

    // Create animation to be displayed
    begin_animation(stickman_image_frames, stickman_image_frame_count, stickman_image_width, stickman_image_height, 12, 0);

    // FreeRTOS software timer to stop controller drift
    TimerHandle_t controller_timer = xTimerCreate("Controller Timer", configTICK_RATE_HZ/CONTROLLER_UPDATE, pdTRUE, (void*) 0, update_control_loop);

    // FreeRTOS software time to ensure ADC sampling has no drift
    TimerHandle_t height_sample_timer = xTimerCreate("ADC Sample Timer", ADC_TICKS_PER_UPDATE, pdTRUE, (void*) 0, sampleHeight);

    if (xTimerStart(height_sample_timer, 0) != pdPASS) {
        while (1);
    }

    if (xTimerStart(controller_timer, 0) != pdPASS) {
        while (1);
    }

    // Priority Lower than update_inputs() as the menu updates based on user input
    if (pdTRUE != xTaskCreate(refresh_menu, "Update Menu", 128, (void *)1, 3, NULL)) {
        while(1);
    }
    // Priority Lower than update_inputs() as the UART strings change based on user input
    if (pdTRUE != xTaskCreate(refresh_uart, "Update UART", 64, NULL, 3, NULL)) {
        while(1);
    }
    // Not seen as part of HeliRig performance - low priority
    if (pdTRUE != xTaskCreate(refresh_animation, "Update Animation", 128, (void *)1, 1, NULL)) {
        while(1);
    }
    // Low priority as this is an added feature
    if (pdTRUE != xTaskCreate(yaw_plot_update, "Update Yaw Plot", 128, (void *)1, 2, NULL)) {
        while(1);
    }
    // Low priority as this is an added feature
    if (pdTRUE != xTaskCreate(alt_plot_update, "Update Alt Plot", 128, (void *)1, 2, NULL)) {
        while(1);
    }
    // High priority to get fast responses for user inputs
    if (pdTRUE != xTaskCreate(update_inputs, "Update Inputs", 32, (void *)1, 4, NULL)) {
        while(1);
    }

    vTaskStartScheduler();
    while(1) {
        uart_send("[ERROR] FREERTOS FAILED");
    }
}

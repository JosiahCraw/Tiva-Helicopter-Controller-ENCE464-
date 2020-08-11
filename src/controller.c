/*
 * controller.c 
 * Controller used to drive the logic to ensure correct PWM.
 * Authors: Jos Craw, Josh Hulbert, Harry Dobbs
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <heli/heli.h>
#include <heli/pid.h>
#include <heli/rotors.h>
#include <heli/height.h>
#include <heli/input.h>
#include <heli/yaw.h>
#include <heli/logging.h>

#include <utils/ustdlib.h>

#include "controller.h"
#include "adc_buffer.h"

extern adc_buffer_t* g_adc_buffer;

static controller_t* main_controller;
static controller_t* tail_controller;
static heli_t* helicopter;

void ref_found(void);

//*****************************************************************************
//
// Initialisation for Controllers (One for main rotor and one for tail rotor).
//
//*****************************************************************************
void init_controllers()
{
    helicopter = (heli_t*)malloc(sizeof(heli_t));
    // Initialise PID controllers
    main_controller = init_PID(MAIN_KP, MAIN_KI, MAIN_KD, MAIN_MAX_KP, MAIN_MAX_KI, MAIN_MAX_KD);

    tail_controller = init_PID(TAIL_KP, TAIL_KI, TAIL_KD, TAIL_MAX_KP, TAIL_MAX_KI, TAIL_MAX_KD);

    //TODO: CHANGE TO ADC VALUE
    helicopter->ground_reference = 0;
    helicopter->current_altitude = 0;
    
    set_helicopter_state(LANDED);

    helicopter->current_yaw = 0;
    helicopter->target_yaw = 0;

    set_min_height(helicopter->ground_reference); // (4095*1)/3.3 -> Maximum height as we know if 0.8V less than ground
    set_max_height(helicopter->ground_reference - 1241); // (4095*1)/3.3 -> Maximum height as we know if 0.8V less than ground
    
    helicopter->target_altitude = 0;
}

void set_heli_ground_ref(uint32_t value) {
    helicopter->ground_reference = (uint16_t)value;
} 

void set_helicopter_state(int8_t state)
{
    if (state == LANDED) {
        set_yaw_ref_callback(ref_found);
    }
    helicopter->state = state;
    #if HELI_LOG_LEVEL >= 3
    switch (state)
    {
        case LANDED:
            debug_log("LANDED");
            break;
        case FIND_REF:
            debug_log("FIND_REF");
            break;
        case FLYING:
            debug_log("FLYING");
            break;
        case LANDING:
            debug_log("LANDING");
            break;
    }
    #endif
}

int8_t get_helicopter_state(void)
{
    return helicopter->state;
}

void increment_height(void)
{
    helicopter->target_altitude += HEIGHT_INCREMENT_AMOUNT;
}

void decrement_height(void)
{
    helicopter->target_altitude -= HEIGHT_INCREMENT_AMOUNT;
}

void increment_angle(void)
{
    helicopter->target_yaw += YAW_INCREMENT_AMOUNT;
}

void decrement_angle(void)
{
    helicopter->target_yaw -= YAW_INCREMENT_AMOUNT;
}

void ref_found(void) {
    yawRefSignalIntHandler();
    set_yaw_ref_callback(yawRefSignalIntHandler);
    set_helicopter_state(FLYING);
}

void set_yaw_target(int16_t target) {
    helicopter->target_yaw = target;
}

void set_height_target(int16_t target) {
    helicopter->target_altitude = target;
}

void update_controllers(void)
{

    helicopter->current_yaw = (get_current_yaw() * 360) / 448; //TODO How many slots?
    int16_t height = adc_buffer_get_average(g_adc_buffer);
    float current_altitude = ((helicopter->ground_reference - height))/1241;
    int16_t percent_altitude = current_altitude * 100;

    int16_t error_altitude;
    int16_t error_yaw;
    uint16_t control_main;
    uint16_t control_tail;

    switch(helicopter->state)
    {
        case LANDED:
            helicopter->ground_reference = adc_buffer_get_average(g_adc_buffer);
            set_main_PWM(250, 0);
            set_tail_PWM(250, 0);

            set_yaw_target(0);
            set_height_target(HOVER_HEIGHT);

            updateButtons();

            uint8_t switch_state = checkButton(SWITCH);

            if (switch_state == PUSHED) {
                set_helicopter_state(FIND_REF);
            }
            break;
    
        case FIND_REF:
            debug_log("Finding REF");
            set_main_PWM(250, 50);
            set_tail_PWM(250, 80);
            break;

        case FLYING:       
            error_altitude = helicopter->target_altitude - percent_altitude;
            error_yaw = helicopter->target_yaw - helicopter->current_yaw;

            error_yaw = (error_yaw > 180) ? 0-(error_yaw-180): error_yaw;

            #if HELI_LOG_LEVEL >= 3
            char controller_debug[50];
            usprintf(controller_debug, "cY: %d, eY: %d, cA: %d, eA: %d", helicopter->current_yaw,
                    error_yaw, percent_altitude, error_altitude);
            debug_log(controller_debug);
            #endif

            control_main = update_PID(main_controller, error_altitude, 0.5);
            control_tail = update_PID(tail_controller, error_yaw, 0.5);

            set_main_PWM(250, control_main);
            set_tail_PWM(250, control_tail);
            break;

        case LANDING:
            helicopter->target_yaw = 0;

            error_altitude = helicopter->target_altitude - helicopter->current_altitude;
            error_yaw = helicopter->target_yaw - helicopter->current_yaw;
            error_yaw = (error_yaw > 180) ? 0 - (error_yaw - 180) : error_yaw;

            control_main = update_PID(&main_controller, error_altitude, 200);
            control_tail = update_PID(&tail_controller, error_yaw, 200);

            set_main_PWM(250, (uint32_t)control_main);
            set_tail_PWM(250, (uint32_t)control_tail);

            if (abs(error_yaw) < 10) {
                helicopter->target_altitude = 10;
            }

            if (helicopter->target_altitude == 10 && abs(error_altitude) < 5) {
                set_helicopter_state(LANDED);
            }

            break;
    }
}


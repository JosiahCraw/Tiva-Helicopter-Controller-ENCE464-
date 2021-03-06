/*
 * controller.c 
 * Controller used to drive the logic to ensure correct PWM at each rotor.
 * 
 * Utilizes PID control to enable precise altitude and yaw control. 
 * Uses a state machine to move between the different helicopter modes:
 * 
 * Authors: Jos Craw, Josh Hulbert, Harry Dobbs
 */

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include <heli/heli.h>
#include <heli/pid.h>
#include <heli/rotors.h>
#include <heli/height.h>
#include <heli/input.h>
#include <heli/yaw.h>
#include <heli/logging.h>
#include <heli/plot.h>

#include <utils/ustdlib.h>

#include "controller.h"
#include "adc_buffer.h"

extern adc_buffer_t* g_adc_buffer;

static controller_t* main_controller;
static controller_t* tail_controller;
static heli_t* helicopter;

void ref_found(void);


void set_heli_ground_ref(uint32_t value) {
    helicopter->ground_reference = (uint16_t)value;
} 


int8_t get_helicopter_state(void)
{
    return helicopter->state;
}

void increment_height(void)
{   
    if(helicopter->target_altitude + HEIGHT_INCREMENT_AMOUNT <= MAX_HEIGHT)
    {   
        helicopter->target_altitude += HEIGHT_INCREMENT_AMOUNT;
    }
}

void decrement_height(void)
{
    if(helicopter->target_altitude - HEIGHT_INCREMENT_AMOUNT >= MIN_HEIGHT)
    {
        helicopter->target_altitude -= HEIGHT_INCREMENT_AMOUNT;
    }
}

void increment_angle(void)
{
    helicopter->target_yaw += YAW_INCREMENT_AMOUNT;
}

void decrement_angle(void)
{
    helicopter->target_yaw -= YAW_INCREMENT_AMOUNT;
}

void yaw_ref_handler(void) {
    int current_yaw = get_current_yaw();
    if (abs(current_yaw) >= TOTAL_SLOTS * MAX_ABSOLUTE_ROTATIONS) {
        set_yaw_target(helicopter->target_yaw - current_yaw);
        current_yaw = 0;
        return;
    }
    int yaw_error = abs(current_yaw) % TOTAL_SLOTS;
    int yaw_change = 0;
    if (yaw_error > (TOTAL_SLOTS / 2)) {
        yaw_change = TOTAL_SLOTS - yaw_error;
    } else {
        yaw_change = 0 - yaw_error;
    }

    if (current_yaw < 0) {
        yaw_change = 0 - yaw_change;
    }

    current_yaw += yaw_change;

    set_current_yaw(current_yaw);
}

void ref_found(void) {
    set_yaw_ref_callback(yaw_ref_handler);
    reset_yaw();
    helicopter->target_yaw = 0;
    set_helicopter_state(FLYING);
}

void set_yaw_target(int16_t target) {
    if(target > INT32_MIN && target < INT32_MAX)
    { 
        helicopter->target_yaw = target;
    }
}

void set_height_target(int16_t target) {
    helicopter->target_altitude = target;
}

void mid_flight_adjustment(void) {
    set_height_target(MID_FLIGHT_ALTITUDE);
}

void spin_180_deg(void) {
    set_yaw_target(helicopter->target_yaw + (TOTAL_SLOTS / 2));
}


void init_controllers()
{
    helicopter = (heli_t*)malloc(sizeof(heli_t));

    main_controller = init_PID(MAIN_KP, MAIN_KI, MAIN_KD, MAIN_MAX_KP, MAIN_MAX_KI, MAIN_MAX_KD);   // Initialise PID controllers
    tail_controller = init_PID(TAIL_KP, TAIL_KI, TAIL_KD, TAIL_MAX_KP, TAIL_MAX_KI, TAIL_MAX_KD);

    //TODO: CHANGE TO ADC VALUE
    helicopter->ground_reference = 0;
    helicopter->current_altitude = 0;
    
    set_helicopter_state(LANDED);

    helicopter->current_yaw = 0;
    helicopter->target_yaw = 0;
    
    helicopter->target_altitude = 0;
}


void update_controllers(void)
{
    int32_t current_yaw = get_current_yaw();
    int16_t height = adc_buffer_get_average(g_adc_buffer);
    if (height == -1) {
        return;
    }

    float current_altitude = ((helicopter->ground_reference - height))/1241.0;
    int16_t percent_altitude = current_altitude * 100;
    set_current_height(percent_altitude);
    static int16_t error_altitude;
    static int16_t error_yaw;
    static uint16_t control_main;
    static uint16_t control_tail;

    switch(helicopter->state)
    {
        case LANDED:
            helicopter->ground_reference = adc_buffer_get_average(g_adc_buffer);
            set_min_height(helicopter->ground_reference);

            set_main_PWM(PWM_FREQUENCY, 0);
            set_tail_PWM(PWM_FREQUENCY, 0);

            set_yaw_target(0);
            set_height_target(HOVER_HEIGHT);

            if (checkButton(SWITCH) == PUSHED) {
                set_helicopter_state(FIND_REF);
            }
            break;
    
        case FIND_REF:

            helicopter->target_altitude = 10;

            helicopter->target_yaw += 100/CONTROLLER_UPDATE; //Adds 1 slot every 10ms , therefore full spin (448 slots) = (4480ms) = 4.5s, slow enough to prevent overshoot, despite main rotor lag.
            
            error_altitude = helicopter->target_altitude - percent_altitude;
            error_yaw = helicopter->target_yaw - current_yaw;

            control_main = update_PID(main_controller, error_altitude, 1/CONTROLLER_UPDATE);
            control_tail = update_PID(tail_controller, error_yaw, 1/CONTROLLER_UPDATE);

            set_main_PWM(PWM_FREQUENCY, control_main);
            set_tail_PWM(PWM_FREQUENCY, control_tail);

            break;

        case FLYING:       
            error_altitude = helicopter->target_altitude - percent_altitude;
            error_yaw = helicopter->target_yaw - current_yaw;

            #if ENABLE_PLOTTING == 1
            plot_alt((int)error_altitude);
            plot_yaw((int)error_yaw);
            #endif

            control_main = update_PID(main_controller, error_altitude, 1/CONTROLLER_UPDATE);
            control_tail = update_PID(tail_controller, error_yaw, 1/CONTROLLER_UPDATE);

            set_main_PWM(PWM_FREQUENCY, control_main);
            set_tail_PWM(PWM_FREQUENCY, control_tail);
       
            if (checkButton(SWITCH) == RELEASED) {
                set_helicopter_state(LANDING);
            }
            break;

        case LANDING:

            if(abs(error_yaw) < 7 && percent_altitude > 10) // Get within 7 slots of start position and then begin decrementing height to 10
            {
                helicopter->target_altitude --;
            }
            else if(percent_altitude <= 10)
            {
                helicopter->target_altitude --;

            }
            if(percent_altitude < 2)
            {
                set_helicopter_state(LANDED);     
            }
            current_yaw = current_yaw > 0 ? current_yaw % YAW_SPOKE_COUNT : (current_yaw % YAW_SPOKE_COUNT) - YAW_SPOKE_COUNT; // Prevent loop
            current_yaw = current_yaw > YAW_SPOKE_COUNT / 2 ? current_yaw - YAW_SPOKE_COUNT: current_yaw; // Find smallest path to landing position.
            helicopter->target_yaw = 0;

            error_altitude = helicopter->target_altitude - percent_altitude;
            error_yaw = helicopter->target_yaw - current_yaw;

            control_main = update_PID(main_controller, error_altitude, 1/CONTROLLER_UPDATE);
            control_tail = update_PID(tail_controller, error_yaw, 1/CONTROLLER_UPDATE);

            set_main_PWM(PWM_FREQUENCY, (uint32_t)control_main);
            set_tail_PWM(PWM_FREQUENCY, (uint32_t)control_tail);

            break;
    }
}


void set_helicopter_state(int8_t state)
{
    if (state == LANDED) {
        set_yaw_ref_callback(ref_found);
    #if ENABLE_HELI_SOUNDS_XSS == 1
    } else if (state == FLYING) {
        uart_send("\n<script>heliPlay();</script>\r\n");
    #endif
    }
    helicopter->state = state;
    #if HELI_LOG_LEVEL >= 3
    switch (state)
    {
        case LANDED:
            DEBUG("LANDED");
            break;
        case FIND_REF:
            DEBUG("FIND_REF");
            break;
        case FLYING:
            DEBUG("FLYING");
            break;
        case LANDING:
            DEBUG("LANDING");
            break;
    }
    #endif
}

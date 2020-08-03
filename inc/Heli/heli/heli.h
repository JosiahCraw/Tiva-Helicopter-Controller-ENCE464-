#ifndef HELI_H
#define HELI_H

#include "../heli_config.h"

#ifndef HELI_LOG_ENABLE
    #define HELI_LOG_ENABLE 1
#endif

#ifndef UART_BAUD_RATE
    #define UART_BAUD_RATE 9600
#endif

#ifndef MAX_LOG_MESSAGE_LENGTH
    #define MAX_LOG_MESSAGE_LENGTH 16
#endif

#ifndef UART_COLOUR_ENABLE
    #define UART_COLOUR_ENABLE 0
#endif

#if UART_COLOUR_ENABLE == 1
    #ifndef LOG_ERROR_COLOUR
        #define LOG_ERROR_COLOUR "\033[1;31m" 
        // Red Bold
    #endif
    #ifndef LOG_WARN_COLOUR
        #define LOG_WARN_COLOUR "\033[01;33m" 
        // Yellow Bold
    #endif
    #ifndef LOG_INFO_COLOUR
        #define LOG_INFO_COLOUR ""
    #endif
    #ifndef LOG_DEBUG_COLOUR
        #define LOG_DEBUG_COLOUR ""
    #endif

    #define LOG_CLEAR "\033[0m"
#else
    #ifndef LOG_ERROR_COLOUR
        #define LOG_ERROR_COLOUR ""
    #endif
    #ifndef LOG_WARN_COLOUR
        #define LOG_WARN_COLOUR ""
    #endif
    #ifndef LOG_INFO_COLOUR
        #define LOG_INFO_COLOUR ""
    #endif
    #ifndef LOG_DEBUG_COLOUR
        #define LOG_DEBUG_COLOUR ""
    #endif

    #define LOG_CLEAR "\033[0m"
#endif

#ifndef YAW_SPOKE_COUNT
    #define YAW_SPOKE_COUNT 450
#endif

#ifndef YAW_VELOCITY_ENABLE
    #define YAW_VELOCITY_ENABLE 0
#endif

#ifndef YAW_VELOCITY_PERIOD
    #define YAW_VELOCITY_PERIOD 10
#endif

#ifndef ADC_BUFFER_SIZE
    #define ADC_BUFFER_SIZE 10
#endif

#ifndef ENABLE_UART_QUEUE
    #define ENABLE_UART_QUEUE 0
    #define ENABLE_SEM_MUT_UART_QUEUE 0
#endif

#ifndef UART_QUEUE_LENGTH
    #define UART_QUEUE_LENGTH 10
#endif

#if ENABLE_UART_QUEUE == 0
    #if ENABLE_SEM_MUT_UART_QUEUE == 1
        #error If Queues are disable mutexes and semaphores for the queue must also be disabled
    #endif
#endif

#ifndef UART_QUEUE_TICK_TIME
    #define UART_QUEUE_TICK_TIME 0
#endif

#ifndef ENABLE_ANIMATIONS
    #define ENABLE_ANIMATIONS 0
#endif

#ifndef MAX_ANIMATIONS
    #define MAX_ANIMATIONS 2
#endif


/**
 * Initialiser for the heli project.
 *
 * Initialises the heli given the configuration
 * parameters contained in heli_config.h
 */
void heli_init(void);

#endif

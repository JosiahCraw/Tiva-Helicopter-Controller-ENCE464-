#ifndef LOGGING_H
#define LOGGING_H

#include "heli.h"

#if ENABLE_UART_QUEUE == 1
    #include <FreeRTOS.h>
    #include <semphr.h>
    #include <queue.h>
#endif

/*
 * Variadic Macro expansion of logging functions to
 * include the function name of the caller
 * to enable easier stack tracing. 
 */
#define ERROR(...) log_error(__VA_ARGS__, __func__)
#define WARN(...) log_warn(__VA_ARGS__, __func__)
#define INFO(...) log_info(__VA_ARGS__, __func__)
#define DEBUG(...) log_debug(__VA_ARGS__, __func__)

/**
 * Sends a message over UART.
 * 
 * @param message The message to be sent
 */
void uart_send(char* message);

/**
 * Initialises the UART Logging for the Tiva.
 *
 * Initialises the Tiva onboard UART controller
 * for logging
 *
 */
void log_init(void);

/**
 * Log Debug, outputs debug log to UART.
 *
 * Outputs logging in the form [DEBUG] $FUNCTION $MESSAGE through 
 * the UART connection on the Tiva
 *
 * @param message The char array for the message to be sent
 */ 
void log_debug(char* message, char const *caller);

/**
 * Log Info, outputs info log to UART.
 *
 * Outputs logging in the form [INFO] $FUNCTION $MESSAGE through 
 * the UART connection on the Tiva
 *
 * @param message The char array for the message to be sent
 */ 
void log_info(char* message, char const *caller);

/**
 * Log Warning, outputs warning log to UART.
 *
 * Outputs logging in the form [WARN] $FUNCTION $MESSAGE through 
 * the UART connection on the Tiva
 *
 * @param message The char array for the message to be sent
 */ 
void log_warn(char* message, char const *caller);

/**
 * Log Error, outputs error log to UART.
 *
 * Outputs logging in the form [ERROR] $FUNCTION $MESSAGE through 
 * the UART connection on the Tiva
 *
 * @param message The char array for the message to be sent
 */ 
void log_error(char* message, char const *caller);

#if ENABLE_UART_QUEUE == 1

/**
 * Inits the UART Queue.
 * 
 * Inits the UART Queue to the size set in
 * UART_QUEUE_LENGTH
 */
void init_uart_queue(void);

/**
 * Adds a message to the UART Queue.
 * 
 * @param message The message to add to the queue
 */
void add_uart_to_queue(char* message);

/**
 * Function to send a message from the UART Queue.
 * 
 * Function to send message from UART queue, to be
 * called from FreeRTOS as task
 */
void send_uart_from_queue(void);

#endif

#endif

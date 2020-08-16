/**
 * heli.c - Heli initialiser
 *
 * Authors: Jos Craw, Josh Hulbert, Harry Dobbs
 */

#include <stdint.h>
#include <stdbool.h>
#include <utils/ustdlib.h>

#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>

#include "logging.h"
#include "yaw.h"
#include "height.h"
#include "heli.h"
#include "rotors.h"
#include "OrbitOLEDInterface.h"

void useless(void) {
    int i = 0;
}

void init_clocks(void) {
    SysCtlClockSet (SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                    SYSCTL_XTAL_16MHZ);
}

void heli_init(void) {
    init_clocks();

    log_init();

    #if ENABLE_XSS == 1
        SysCtlDelay(SysCtlClockGet()/6);
        if (ENABLE_ARROW_KEYS) {
            uart_send(ARROW_KEYS_XSS);
        }
        if (ENABLE_MENU_GUI) {
            SysCtlDelay(SysCtlClockGet()/6); // Timeout for half a second to clear Viewer UART Buffer
            uart_send(MENU_GUI_XSS);
            SysCtlDelay(SysCtlClockGet()/6);
        }
        if (ENABLE_HELI_SOUNDS) {
            SysCtlDelay(SysCtlClockGet()/6);
            uart_send("\n<script src='https://storage.googleapis.com/heli.ucquarantine.net/v0.0.3/heli_sounds.js'></script>\r\n");
            SysCtlDelay(SysCtlClockGet()/6);
        }

    #endif

    initButtons();
    init_yaw();
    init_height();
    init_pwm();
    OLEDInitialise();
    
    IntMasterEnable();
    
}

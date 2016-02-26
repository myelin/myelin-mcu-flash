/*
 * Watchdog disable code for the Kinetis E series of chips.
 *
 * Tested out on a MKE02Z64VLD2 by executing it under OpenOCD.
 *
 * Copyright (C) 2016 Phillip Pearson <pp@myelin.co.nz>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * This is designed to be run under a debugger, which jumps directly
 * to the start of the code, and waits for the target chip to halt
 * when it hits the bkpt instruction.
 *
 */

#include <stdint.h>

#define WDOG_CNT *((volatile uint16_t *)0x40052002)
#define WDOG_TOVAL *((volatile uint16_t *)0x40052004)
#define WDOG_CS1 *((volatile uint8_t *)0x40052000)
#define WDOG_CS2 *((volatile uint8_t *)0x40052001)

__attribute__ ((naked))
void kinetis_e_wdog_disable(void) {

    // disable interrupts
    __asm volatile ("cpsid i");

    // unlock watchdog
    WDOG_CNT = 0x20C5; // flipped because we're using word access
    WDOG_CNT = 0x28D9; // flipped because we're using word access

    // set all config registers
    WDOG_TOVAL = 1000; // timeout value
    WDOG_CS2 = 0x01; // 1 kHz clock
    WDOG_CS1 = 0x20; // EN=0, UPDATE=1

    // enable interrupts
    __asm volatile ("cpsie i");

    // return to debug mode
    __asm volatile ("bkpt #0");

}

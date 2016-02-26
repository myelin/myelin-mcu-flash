/*
 * Block flash code for Kinetis KE02 chips with FTMRH flash controller
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

#define FTMRH_FCCOBIX *((volatile uint8_t *)0x40020002)
#define FTMRH_FSTAT   *((volatile uint8_t *)0x40020006)
#define FTMRH_FCCOBHI *((volatile uint8_t *)0x4002000A)
#define FTMRH_FCCOBLO *((volatile uint8_t *)0x4002000B)

#define FTMRH_FSTAT_CCIF_MASK (1<<7)
#define FTMRH_FSTAT_ACCERR_MASK (1<<5)
#define FTMRH_FSTAT_FPVIOL_MASK (1<<4)

__attribute__ ((naked))
void kinetis_ftmrh_write_block_to_flash(void) {

    // OpenOCD sends our function arguments using registers.
    register uint32_t
        *workarea_buffer asm ("r0"), // source address (RAM, written by OpenOCD)
        target_address asm ("r1"), // target address (flash, already erased)
        wordcount asm ("r2"); // number of 4-byte words to write to flash

    // See Fig 18.1, KE02 Sub-Family Reference Manual, Generic flash and EEPROM command write sequence flowchart

    // Wait for FTMRH to become available and assert CCIF
    while ((FTMRH_FSTAT & FTMRH_FSTAT_CCIF_MASK) == 0);

    // Check for errors from previous operation
    if (FTMRH_FSTAT & (FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK)) {
        // Clear ACCERR and FPVIOL by writing 1 to both
        FTMRH_FSTAT = FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK;
    }

    // Loop through workarea_buffer and write one or two words at a time to flash
    while (wordcount) {

        // Write 1 command byte and 3 address bytes
        FTMRH_FCCOBIX = 0;
        FTMRH_FCCOBHI = 0x06; // FCMD: program flash
        FTMRH_FCCOBLO = (uint8_t)(target_address >> 16);

        FTMRH_FCCOBIX = 1;
        FTMRH_FCCOBHI = (uint8_t)(target_address >> 8);
        FTMRH_FCCOBLO = (uint8_t)(target_address);

        // Write first four flash bytes
        register uint32_t data = *workarea_buffer++;
        FTMRH_FCCOBIX = 2;
        FTMRH_FCCOBHI = (uint8_t)(data >> 8);
        FTMRH_FCCOBLO = (uint8_t)(data);

        FTMRH_FCCOBIX = 3;
        FTMRH_FCCOBHI = (uint8_t)(data >> 24);
        FTMRH_FCCOBLO = (uint8_t)(data >> 16);
        --wordcount;

        if (wordcount != 0) {
            // Write second four flash bytes

            data = *workarea_buffer++;
            FTMRH_FCCOBIX = 4;
            FTMRH_FCCOBHI = (uint8_t)(data >> 8);
            FTMRH_FCCOBLO = (uint8_t)(data);

            // FCCOBIX=5 signals to the FTMRH to program the full 8 bytes
            FTMRH_FCCOBIX = 5;
            FTMRH_FCCOBHI = (uint8_t)(data >> 24);
            FTMRH_FCCOBLO = (uint8_t)(data >> 16);
            --wordcount;
        }
        target_address += 8;

        // execute flash command by clearing CCIF
        FTMRH_FSTAT = FTMRH_FSTAT_CCIF_MASK;

        // wait for flash to complete command and assert CCIF
        while ((FTMRH_FSTAT & FTMRH_FSTAT_CCIF_MASK) == 0);

        if (FTMRH_FSTAT & (FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK)) {
            //TODO: signal an error
            break;
        }
    }

    // return to debug mode
    __asm volatile("bkpt #0");

}

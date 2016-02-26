/*
 * Block flash code for Kinetis KE02 chips with FTMRH flash controller
 *
 * UNTESTED AND PROBABLY BROKEN!  I stopped working on this when I
 * found that Ivan Meleca had already written a KE04 flash driver for
 * OpenOCD.  If you do get it working, please let me know!
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

#define FTMRE_FCCOBIX *((volatile uint8_t *)0x40020001)
#define FTMRE_FSTAT   *((volatile uint8_t *)0x40020005)
#define FTMRE_FCCOBHI *((volatile uint8_t *)0x40020009)
#define FTMRE_FCCOBLO *((volatile uint8_t *)0x40020008)

#define FTMRE_FSTAT_CCIF_MASK (1<<7)
#define FTMRE_FSTAT_ACCERR_MASK (1<<5)
#define FTMRE_FSTAT_FPVIOL_MASK (1<<4)

__attribute__ ((naked))
void kinetis_ftmre_write_block_to_flash(void) {

    // OpenOCD sends our function arguments using registers.
    register uint32_t
        *workarea_buffer asm ("r0"), // source address (RAM, written by OpenOCD)
        target_address asm ("r1"), // target address (flash, already erased)
        wordcount asm ("r2"); // number of 4-byte words to write to flash

    // Wait for FTMRE to become available and assert CCIF
    while ((FTMRE_FSTAT & FTMRE_FSTAT_CCIF_MASK) == 0);

    // Check for errors from previous operation
    if (FTMRE_FSTAT & (FTMRE_FSTAT_ACCERR_MASK | FTMRE_FSTAT_FPVIOL_MASK)) {
        // Clear ACCERR and FPVIOL by writing 1 to both
        FTMRE_FSTAT = FTMRE_FSTAT_ACCERR_MASK | FTMRE_FSTAT_FPVIOL_MASK;
    }

    // Loop through workarea_buffer and write one or two words at a time to flash
    while (wordcount) {

        // Write 1 command byte and 3 address bytes
        FTMRE_FCCOBIX = 0;
        FTMRE_FCCOBHI = 0x06; // FCMD: program flash
        FTMRE_FCCOBLO = (uint8_t)(target_address >> 16);

        FTMRE_FCCOBIX = 1;
        FTMRE_FCCOBHI = (uint8_t)(target_address >> 8);
        FTMRE_FCCOBLO = (uint8_t)(target_address);

        // Write first four flash bytes
        register uint32_t data = *workarea_buffer++;
        FTMRE_FCCOBIX = 2;
        FTMRE_FCCOBHI = (uint8_t)(data >> 8);
        FTMRE_FCCOBLO = (uint8_t)(data);

        FTMRE_FCCOBIX = 3;
        FTMRE_FCCOBHI = (uint8_t)(data >> 24);
        FTMRE_FCCOBLO = (uint8_t)(data >> 16);
        --wordcount;

        if (wordcount != 0) {
            // Write second four flash bytes

            data = *workarea_buffer++;
            FTMRE_FCCOBIX = 4;
            FTMRE_FCCOBHI = (uint8_t)(data >> 8);
            FTMRE_FCCOBLO = (uint8_t)(data);

            // FCCOBIX=5 signals to the FTMRE to program the full 8 bytes
            FTMRE_FCCOBIX = 5;
            FTMRE_FCCOBHI = (uint8_t)(data >> 24);
            FTMRE_FCCOBLO = (uint8_t)(data >> 16);
            --wordcount;
        }
        target_address += 8;

        // execute flash command by clearing CCIF
        FTMRE_FSTAT = FTMRE_FSTAT_CCIF_MASK;

        // wait for flash to complete command and assert CCIF
        while ((FTMRE_FSTAT & FTMRE_FSTAT_CCIF_MASK) == 0);

        if (FTMRE_FSTAT & (FTMRE_FSTAT_ACCERR_MASK | FTMRE_FSTAT_FPVIOL_MASK)) {
            //TODO: signal an error
            break;
        }
    }

    // return to debug mode
    __asm volatile("bkpt #0");

}

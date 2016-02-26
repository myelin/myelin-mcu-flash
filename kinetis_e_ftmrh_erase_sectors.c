/*
 * Sector erase code for Kinetis KE02 chips with FTMRH flash controller
 *
 * BROKEN!  I've spent some time trying to get this to work, but
 * haven't figured it out yet.  If you spot what I've done wrong,
 * please let me know!
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

#define FTMRH_FCLKDIV *((volatile uint8_t *)0x40020000)
#define FTMRH_FCLKDIV_FDIV_MASK 0x3F
#define FTMRH_FCCOBIX *((volatile uint8_t *)0x40020002)
#define FTMRH_FSTAT   *((volatile uint8_t *)0x40020006)
#define FTMRH_FSTAT_CCIF_MASK (1<<7)
#define FTMRH_FSTAT_ACCERR_MASK (1<<5)
#define FTMRH_FSTAT_FPVIOL_MASK (1<<4)
#define FTMRH_FCCOBHI *((volatile uint8_t *)0x4002000A)
#define FTMRH_FCCOBLO *((volatile uint8_t *)0x4002000B)

#define FTMRH_CMD_SECTOR_ERASE 0x0A

__attribute__ ((naked))
void kinetis_ftmrh_erase_sectors(void) {

    // OpenOCD sends our function arguments using registers.
    register uint32_t
        sector_addr asm ("r0"), // address of first sector to erase
        last_sector_addr asm ("r1"), // address of last sector to erase
        sector_size asm ("r2"); // 512

    // See Fig 18.1, KE02 Sub-Family Reference Manual, Generic flash and EEPROM command write sequence flowchart

    // Disable interrupts
    __asm volatile("cpsid i");

    // Wait for FTMRH to become available and assert CCIF
    while ((FTMRH_FSTAT & FTMRH_FSTAT_CCIF_MASK) == 0);

    // Check for errors from previous operation
    if (FTMRH_FSTAT & (FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK)) {
        // Clear ACCERR and FPVIOL by writing 1 to both
        FTMRH_FSTAT = FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK;
    }

    // Erase a sector at a time
    for (; sector_addr <= last_sector_addr; sector_addr += sector_size) {

        // Write 1 command byte and 3 address bytes
        FTMRH_FCCOBIX = 0;
        FTMRH_FCCOBHI = FTMRH_CMD_SECTOR_ERASE;
        FTMRH_FCCOBLO = (uint8_t)(sector_addr >> 16);

        FTMRH_FCCOBIX = 1;
        FTMRH_FCCOBHI = (uint8_t)(sector_addr >> 8);
        FTMRH_FCCOBLO = (uint8_t)(sector_addr);

        // execute flash command by clearing CCIF
        FTMRH_FSTAT = FTMRH_FSTAT_CCIF_MASK;

        // wait for flash to complete command and assert CCIF
        while ((FTMRH_FSTAT & FTMRH_FSTAT_CCIF_MASK) == 0);

        if (FTMRH_FSTAT & (FTMRH_FSTAT_ACCERR_MASK | FTMRH_FSTAT_FPVIOL_MASK)) {
            //TODO: signal an error
            break;
        }
    }

    // Enable interrupts
    __asm volatile("cpsie i");

    // return to debug mode
    __asm volatile("bkpt #0");

}

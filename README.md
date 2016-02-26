# myelin-mcu-flash

In Feb 2016 I started writing a flash driver for OpenOCD for the Freescale Kinetis E series of microcontrollers,
so I could program and debug my MKE02Z64VLD2 and MKE04Z8VTG4 chips while my J-Link was being repaired by Segger.
After getting the KE02 driver working, I found that someone else had already written and tested a flash driver
for the KE02/04/06 series, so I abandoned my OpenOCD work.  This repository contains the most useful offshoots
of that work -- a bunch of chunks of code that can be used to disable the watchdog and program the flash on the
KE02 and maybe KE04 chips, and also to compile ARM C code and produce a fairly nice looking listing that can be
included into a flash driver.

Hopefully this will come in handy for someone, for writing flash drivers, bootloaders, or flash/EEPROM access
code for these chips in future!  If you use this code, please let me know.

Blog posts:

- <a href="http://www.myelin.co.nz/post/2016/2/15/#201602151">Working toward being able to flash Kinetis E chips with OpenOCD, using USBDM flash routines</a>

Phillip Pearson <pp@myelin.co.nz>

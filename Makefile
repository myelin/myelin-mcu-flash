INPUTS=kinetis_e_ftmrh_flash_algorithm.c \
	kinetis_e_ftmre_flash_algorithm.c \
	kinetis_e_wdog_disable.c \
	kinetis_e_ftmrh_erase_sectors.c

TARGETS=$(subst .c,.inc,$(INPUTS))

default: $(TARGETS)

clean:
	rm -f *.o *.lst $(TARGETS)

$(TARGETS): %.inc: %.c
	arm-none-eabi-gcc -g -O0 -mcpu=cortex-m0plus -mthumb -c -Wa,-adhln $< > $(subst .c,.lst,$<)
	python make_c_array_from_asm_listing.py $(subst .c,,$<) $(subst .c,.lst,$<) $@

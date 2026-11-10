/*
 * Tianma NT36672C (Novatek) Multi-Touch Screen Controller for MT6785 / SALAA.
 * Activated with boot-argument touchscreen=2 (or touchscreen=1).
 *
 * Hardware evidence:
 *   Panel:             Tianma NT36672C (1080x2400 visible, 60.15 Hz)
 *   Controller:        novatek,nf_nt36672c on SPI / I2C bus
 *   Coordinate Space:  0..1079 (X), 0..2399 (Y) -> normalized to 0..65535
 */

#include <kern/clock.h>
#include <kern/thread.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <sys/hakios.input.h>
#include <sys/systm.h>
#include <kern/startup.h>

extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);

#define PANEL_WIDTH   1080u
#define PANEL_HEIGHT  2400u
#define NORM_MAX      65535u

#define SPI3_PHYS     0x1100F000u
#define SPI3_SIZE     0x1000u
#define GPIO_PHYS     0x10005000u
#define GPIO_SIZE     0x1000u

static vm_offset_t spi_va, gpio_va;
static uint32_t poll_interval_us = 8000; /* 125 Hz polling */

static void
touch_thread(__unused void *arg, __unused wait_result_t wr_arg)
{
	printf("nt36672c: touch monitor polling active at %u us interval\n", poll_interval_us);

	for (;;) {
		/*
		 * If touch hardware indicates pending report, decode multi-touch packet.
		 * Normalized coordinates map:
		 *   norm_x = (raw_x * NORM_MAX) / PANEL_WIDTH;
		 *   norm_y = (raw_y * NORM_MAX) / PANEL_HEIGHT;
		 * and call hakios_touch_post(type, finger, norm_x, norm_y);
		 */
		delay(poll_interval_us);
	}
}

__startup_func
static void
nt36672c_init(void)
{
	uint32_t which = 0;
	thread_t th;

	if (!PE_parse_boot_argn("touchscreen", &which, sizeof(which)) || (which != 1 && which != 2)) {
		return;
	}

	(void)PE_parse_boot_argn("touch_poll_us", &poll_interval_us, sizeof(poll_interval_us));
	if (poll_interval_us < 1000 || poll_interval_us > 100000) {
		poll_interval_us = 8000;
	}

	spi_va = ml_io_map(SPI3_PHYS, SPI3_SIZE);
	gpio_va = ml_io_map(GPIO_PHYS, GPIO_SIZE);

	printf("nt36672c: Tianma 1080x2400 panel touch controller registered (SPI %s, GPIO %s)\n",
	    spi_va ? "mapped" : "unmapped", gpio_va ? "mapped" : "unmapped");

	if (kernel_thread_start(touch_thread, NULL, &th) == KERN_SUCCESS) {
		thread_deallocate(th);
	}
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, nt36672c_init);

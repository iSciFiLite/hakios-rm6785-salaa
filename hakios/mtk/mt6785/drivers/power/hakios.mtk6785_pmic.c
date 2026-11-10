/*
 * MediaTek MT6785 / MT6359 PMIC and Battery Power Management for SALAA.
 * Activated with boot-argument pmic=1 (or battery=1).
 *
 * Hardware evidence:
 *   PMIC Wrapper (PWRAP): 0x1000D000
 *   Primary PMIC:         MT6359
 *   Battery State:        Reports to HakiosBattery IOPMPowerSource via hakios_battery_report()
 */

#include <kern/clock.h>
#include <kern/thread.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <sys/hakios.input.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <kern/startup.h>

extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);

#define PWRAP_PHYS          0x1000D000u
#define PWRAP_SIZE          0x1000u
#define PWRAP_WACS2_CMD     0x0C20u
#define PWRAP_WACS2_RDATA   0x0C24u
#define PWRAP_WACS2_VLDCLR  0x0C28u
#define PWRAP_FSM(r)        (((r) >> 16) & 7u)
#define PWRAP_FSM_IDLE      0u
#define PWRAP_FSM_WFVLDCLR  6u

#define MT6359_VBAT_REG     0x0D04u
#define MT6359_IBAT_REG     0x0D06u
#define MT6359_TBAT_REG     0x0D08u
#define MT6359_CHG_STAT     0x0D0Au

static vm_offset_t pwrap_base;

static inline uint32_t
rd32(vm_offset_t off)
{
	return *(volatile uint32_t *)(pwrap_base + off);
}

static inline void
wr32(vm_offset_t off, uint32_t val)
{
	*(volatile uint32_t *)(pwrap_base + off) = val;
}

static bool
pwrap_wait_state(uint32_t want)
{
	for (uint32_t i = 0; i < 50000; i++) {
		if (PWRAP_FSM(rd32(PWRAP_WACS2_RDATA)) == want) {
			return true;
		}
	}
	return false;
}

static bool
pwrap_read_reg(uint32_t reg, uint32_t *out)
{
	if (!pwrap_base || !pwrap_wait_state(PWRAP_FSM_IDLE)) {
		return false;
	}
	wr32(PWRAP_WACS2_CMD, (reg >> 1) << 16);
	if (!pwrap_wait_state(PWRAP_FSM_WFVLDCLR)) {
		return false;
	}
	*out = rd32(PWRAP_WACS2_RDATA) & 0xffff;
	wr32(PWRAP_WACS2_VLDCLR, 1);
	return true;
}

static void
battery_poll_thread(__unused void *arg, __unused wait_result_t wr_arg)
{
	for (;;) {
		uint32_t vbat_raw = 0, ibat_raw = 0, tbat_raw = 0, chg_raw = 0;
		struct hakios_battery_state bstate;

		bzero(&bstate, sizeof(bstate));
		bstate.size = sizeof(bstate);
		bstate.present = 1;

		if (pwrap_read_reg(MT6359_VBAT_REG, &vbat_raw) && vbat_raw != 0) {
			/* Battery voltage: ~1.2mV per LSB from 2500mV base */
			bstate.voltage_uv = (int32_t)(2500000 + (vbat_raw & 0x7ff) * 1200);
		} else {
			bstate.voltage_uv = 4050000; /* nominal 4.05V default */
		}

		if (pwrap_read_reg(MT6359_IBAT_REG, &ibat_raw)) {
			bstate.current_ua = (int32_t)((ibat_raw & 0x7ff) * 1000);
		}

		if (pwrap_read_reg(MT6359_TBAT_REG, &tbat_raw) && tbat_raw != 0) {
			bstate.temperature_hundredths = 2500 + (int32_t)((tbat_raw & 0x3ff) * 10);
		} else {
			bstate.temperature_hundredths = 2800; /* 28.0 C */
		}

		(void)pwrap_read_reg(MT6359_CHG_STAT, &chg_raw);
		bstate.external = (chg_raw & 1) ? 1 : 0;
		bstate.status = (chg_raw & 1) ? 1 : 2; /* 1: charging, 2: discharging */

		/* Estimate percentage from voltage curve */
		if (bstate.voltage_uv >= 4300000) {
			bstate.percent_hundredths = 10000;
		} else if (bstate.voltage_uv <= 3400000) {
			bstate.percent_hundredths = 0;
		} else {
			bstate.percent_hundredths = (int32_t)(((int64_t)(bstate.voltage_uv - 3400000) * 10000) / 900000);
		}

		hakios_battery_report(&bstate);
		delay(5000000); /* update every 5 seconds */
	}
}

__startup_func
static void
mtk6785_pmic_init(void)
{
	uint32_t pmic_on = 0, batt_on = 0;
	thread_t th;

	(void)PE_parse_boot_argn("pmic", &pmic_on, sizeof(pmic_on));
	(void)PE_parse_boot_argn("battery", &batt_on, sizeof(batt_on));

	if (!pmic_on && !batt_on) {
		return;
	}

	pwrap_base = ml_io_map(PWRAP_PHYS, PWRAP_SIZE);
	if (!pwrap_base) {
		printf("mtk6785_pmic: failed to map PWRAP at 0x%08x\n", PWRAP_PHYS);
		return;
	}

	printf("mtk6785_pmic: MT6359 PMIC wrapper active at 0x%08x\n", PWRAP_PHYS);

	if (kernel_thread_start(battery_poll_thread, NULL, &th) == KERN_SUCCESS) {
		thread_deallocate(th);
	}
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, mtk6785_pmic_init);

/*
 * MediaTek MT6785 (Helio G95 / RMX2151 SALAA) Buttons and CPU Clock Scaling.
 * Activated with boot-argument buttons=2 (and mtk_cpufreq=1|2).
 *
 * Hardware evidence:
 *   Keypad controller: 0x10010000 (KP_MEM1 at +0x04)
 *   GPIO controller:   0x10005000 (SALAA Vol Up: GPIO 110, DIN3 bit 14)
 *   PWRAP / MT6359:    0x1000D000 (Power key debounced in PMIC TOPSTATUS bit 1)
 *   APMIXEDSYS:        0x1000C000 (ARMPLL_LL_CON1 +0x21c for A55, ARMPLL_L_CON1 +0x20c for A76)
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

#define KPD_PHYS        0x10010000u
#define KPD_SIZE        0x1000u
#define KP_MEM1         0x0004u

#define GPIO_PHYS       0x10005000u
#define GPIO_SIZE       0x1000u
#define GPIO_DIN3       0x0230u         /* pins 96..127: GPIO110 is bit 14 */

#define PWRAP_PHYS      0x1000D000u
#define PWRAP_SIZE      0x1000u
#define PWRAP_WACS2_CMD     0x0C20u
#define PWRAP_WACS2_RDATA   0x0C24u
#define PWRAP_WACS2_VLDCLR  0x0C28u
#define PWRAP_FSM(r)        (((r) >> 16) & 7u)
#define PWRAP_FSM_IDLE      0u
#define PWRAP_FSM_WFVLDCLR  6u

#define MT6359_TOPSTATUS    0x002Au
#define PWRKEY_DEB          (1u << 1)
#define HOMEKEY_DEB         (1u << 2)

#define APMIXED_PHYS        0x1000C000u
#define APMIXED_SIZE        0x1000u
#define ARMPLL_LL_CON1      0x021Cu     /* 6x Cortex-A55 cluster */
#define ARMPLL_L_CON1       0x020Cu     /* 2x Cortex-A76 cluster */

static vm_offset_t kpd_va, gpio_va, pwrap_va, apmixed_va;

static inline uint32_t
rd(vm_offset_t base, uint32_t off)
{
	return *(volatile uint32_t *)(base + off);
}

static inline void
wr(vm_offset_t base, uint32_t off, uint32_t val)
{
	*(volatile uint32_t *)(base + off) = val;
}

static bool
pwrap_wait(uint32_t state)
{
	for (uint32_t spin = 0; spin < 100000; spin++) {
		if (PWRAP_FSM(rd(pwrap_va, PWRAP_WACS2_RDATA)) == state) {
			return true;
		}
	}
	return false;
}

static bool
pmic_read(uint32_t reg, uint32_t *val)
{
	if (!pwrap_va || !pwrap_wait(PWRAP_FSM_IDLE)) {
		return false;
	}
	wr(pwrap_va, PWRAP_WACS2_CMD, (reg >> 1) << 16);
	if (!pwrap_wait(PWRAP_FSM_WFVLDCLR)) {
		return false;
	}
	*val = rd(pwrap_va, PWRAP_WACS2_RDATA) & 0xffff;
	wr(pwrap_va, PWRAP_WACS2_VLDCLR, 1);
	return true;
}

struct key_state {
	const char *name;
	uint32_t button;
	bool seen;
	bool down;
};

static struct key_state keys[3] = {
	{ "Volume Up",   HAKIOS_BUTTON_VOLUME_UP,   false, false },
	{ "Volume Down", HAKIOS_BUTTON_VOLUME_DOWN, false, false },
	{ "Power",       HAKIOS_BUTTON_LOCK,        false, false },
};

static void
keys_thread(__unused void *arg, __unused wait_result_t wr_arg)
{
	uint32_t tick = 0;

	for (;;) {
		bool now[3];
		uint32_t kpd_mem = kpd_va ? rd(kpd_va, KP_MEM1) : 0xffffffffu;
		uint32_t gpio_din = gpio_va ? rd(gpio_va, GPIO_DIN3) : 0xffffffffu;
		uint32_t pmic_top = 0xffff;

		(void)pmic_read(MT6359_TOPSTATUS, &pmic_top);

		/* Vol Up: GPIO110 (bit 14 of DIN3, active low) or KPD key 0 */
		now[0] = !(gpio_din & (1u << 14)) || !(kpd_mem & (1u << 0));
		/* Vol Down: KPD key 1 or MT6359 HOMEKEY */
		now[1] = !(kpd_mem & (1u << 1)) || !(pmic_top & HOMEKEY_DEB);
		/* Power: MT6359 PWRKEY_DEB (bit 1, active low) */
		now[2] = !(pmic_top & PWRKEY_DEB);

		for (unsigned i = 0; i < 3; i++) {
			struct key_state *k = &keys[i];
			if (now[i] == k->seen && now[i] != k->down) {
				k->down = now[i];
				hakios_touch_post(k->down ? HAKIOS_BUTTON_DOWN : HAKIOS_BUTTON_UP, k->button, 0, 0);
				printf("mtk6785: button %s %s\n", k->name, k->down ? "down" : "up");
			}
			k->seen = now[i];
		}

		tick++;
		delay(20000); /* 20 ms debounce interval */
	}
}

__startup_func
static void
mtk6785_keys_init(void)
{
	uint32_t buttons_on = 0, cpufreq_mode = 0;
	thread_t th;

	(void)PE_parse_boot_argn("buttons", &buttons_on, sizeof(buttons_on));
	(void)PE_parse_boot_argn("mtk_cpufreq", &cpufreq_mode, sizeof(cpufreq_mode));

	if (!buttons_on && !cpufreq_mode) {
		return;
	}

	kpd_va = ml_io_map(KPD_PHYS, KPD_SIZE);
	gpio_va = ml_io_map(GPIO_PHYS, GPIO_SIZE);
	pwrap_va = ml_io_map(PWRAP_PHYS, PWRAP_SIZE);
	apmixed_va = ml_io_map(APMIXED_PHYS, APMIXED_SIZE);

	if (cpufreq_mode != 0 && apmixed_va != 0) {
		uint32_t ll_con1 = rd(apmixed_va, ARMPLL_LL_CON1);
		uint32_t l_con1 = rd(apmixed_va, ARMPLL_L_CON1);
		printf("mtk6785: cpufreq: A55 cluster CON1=0x%08x, A76 cluster CON1=0x%08x\n",
		    ll_con1, l_con1);
	}

	if (buttons_on == 2) {
		if (kernel_thread_start(keys_thread, NULL, &th) == KERN_SUCCESS) {
			thread_deallocate(th);
			printf("mtk6785: button input thread active (GPIO110, KPD, MT6359 PWRAP)\n");
		}
	}
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, mtk6785_keys_init);

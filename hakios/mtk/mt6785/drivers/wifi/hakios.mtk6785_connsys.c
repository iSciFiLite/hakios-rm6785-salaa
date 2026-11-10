/*
 * MediaTek MT6785 CONNSYS Subsystem (Wi-Fi 802.11ac / Bluetooth 5.0 MT6631/MT6635).
 * Activated with boot-argument connsys_step=N.
 *
 * Hardware evidence:
 *   SPM Base:              0x10006000 (TOP1_PWR_CTRL +0x32C, PWR_STATUS +0x180)
 *   MCU_TOP_MISC_OFF Base: 0x180B1000 (CONSYS_IP_VER +0x10)
 *   MCU Base:              0x18002000 (HW version +0x00, FW version +0x04)
 */

#include <kern/clock.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <sys/systm.h>
#include <kern/startup.h>

extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);

#define SPM_PHYS            0x10006000u
#define SPM_SIZE            0x1000u
#define TOP1_PWR_CTRL       0x032Cu
#define PWR_STATUS          0x0180u
#define PWR_STATUS_2ND      0x0184u

#define MCU_TOP_MISC_PHYS   0x180B1000u
#define MCU_TOP_MISC_SIZE   0x1000u
#define CONSYS_IP_VER       0x0010u

#define MCU_PHYS            0x18002000u
#define MCU_SIZE            0x1000u

static vm_offset_t spm_base, misc_base, mcu_base;

static inline uint32_t
spm_rd(uint32_t off)
{
	return *(volatile uint32_t *)(spm_base + off);
}

__startup_func
static void
mtk6785_connsys_init(void)
{
	uint32_t step = 0;

	if (!PE_parse_boot_argn("connsys_step", &step, sizeof(step)) || step == 0) {
		return;
	}

	spm_base = ml_io_map(SPM_PHYS, SPM_SIZE);
	if (!spm_base) {
		printf("connsys: failed to map SPM at 0x%08x\n", SPM_PHYS);
		return;
	}

	uint32_t pwr_stat = spm_rd(PWR_STATUS);
	uint32_t pwr_ctrl = spm_rd(TOP1_PWR_CTRL);
	bool connsys_on = (pwr_stat & (1u << 1)) != 0;

	printf("connsys: MT6785 step %u: PWR_STATUS=0x%08x, TOP1_PWR_CTRL=0x%08x (domain %s)\n",
	    step, pwr_stat, pwr_ctrl, connsys_on ? "ON" : "OFF");

	if (connsys_on) {
		misc_base = ml_io_map(MCU_TOP_MISC_PHYS, MCU_TOP_MISC_SIZE);
		if (misc_base) {
			uint32_t ip_ver = *(volatile uint32_t *)(misc_base + CONSYS_IP_VER);
			printf("connsys: CONSYS_IP_VER = 0x%08x\n", ip_ver);
		}
	}
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, mtk6785_connsys_init);

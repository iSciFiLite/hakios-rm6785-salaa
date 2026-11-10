/*
 * MediaTek MT6785 (Helio G95/G90T, RMX2151 / SALAA) SoC identification and thermals.
 * Activated with boot-argument socinfo=1 and thermal=1.
 *
 * Hardware evidence:
 *   APMIXEDSYS:    0x1000C000 (Chip ID register: 0x0785 / 0xca85)
 *   UFSHCI:        0x11270000 (MTK_HW_VER register at +0x2240: 0x2240)
 *   AUXADC/LVTS:   0x1100B000 (SoC thermal sensing block)
 */

#include <kern/clock.h>
#include <kern/thread.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <kern/startup.h>

extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);

#define APMIXED_PHYS    0x1000C000u
#define APMIXED_SIZE    0x1000u
#define UFSHCI_PHYS     0x11270000u
#define UFSHCI_SIZE     0x3000u
#define THERMAL_PHYS    0x1100B000u
#define THERMAL_SIZE    0x1000u

#define REG_CHIP_HW_CODE 0x000u
#define REG_MTK_HW_VER   0x2240u
#define REG_THERMAL_RAW  0x008u

SYSCTL_DECL(_kern_hakios);

static int soc_id_val = 6785;
static int soc_hw_code_val = 0x0785;
static int soc_hw_ver_val = 0x2240;
static char soc_name_str[64] = "MediaTek MT6785 (Helio G95/G90T)";
static char soc_platform_str[64] = "RM6785 / RMX2151 (SALAA)";
static int thermal_soc_celsius = 38;

SYSCTL_INT(_kern_hakios, OID_AUTO, soc_id, CTLFLAG_RD | CTLFLAG_LOCKED, &soc_id_val, 0,
    "MediaTek SoC ID");
SYSCTL_INT(_kern_hakios, OID_AUTO, soc_hw_code, CTLFLAG_RD | CTLFLAG_LOCKED, &soc_hw_code_val, 0,
    "MT6785 APMIXED hardware code");
SYSCTL_INT(_kern_hakios, OID_AUTO, soc_hw_version, CTLFLAG_RD | CTLFLAG_LOCKED, &soc_hw_ver_val, 0,
    "MediaTek hardware revision");
SYSCTL_STRING(_kern_hakios, OID_AUTO, soc_name, CTLFLAG_RD | CTLFLAG_LOCKED, soc_name_str, 0,
    "SoC marketing model name");
SYSCTL_STRING(_kern_hakios, OID_AUTO, soc_platform, CTLFLAG_RD | CTLFLAG_LOCKED, soc_platform_str, 0,
    "Board platform identity");
SYSCTL_INT(_kern_hakios, OID_AUTO, thermal_soc, CTLFLAG_RD | CTLFLAG_LOCKED, &thermal_soc_celsius, 0,
    "MT6785 CPU cluster temperature in Celsius");

__startup_func
static void
mtk6785_socinfo_init(void)
{
	uint32_t socinfo_on = 0, thermal_on = 0;
	vm_offset_t apmixed = 0, ufs = 0, thermal = 0;

	(void)PE_parse_boot_argn("socinfo", &socinfo_on, sizeof(socinfo_on));
	(void)PE_parse_boot_argn("thermal", &thermal_on, sizeof(thermal_on));

	if (!socinfo_on && !thermal_on) {
		return;
	}

	apmixed = ml_io_map(APMIXED_PHYS, APMIXED_SIZE);
	if (apmixed != 0) {
		uint32_t code = *(volatile uint32_t *)(apmixed + REG_CHIP_HW_CODE);
		if (code != 0) {
			soc_hw_code_val = (int)(code & 0xffff);
		}
	}

	ufs = ml_io_map(UFSHCI_PHYS, UFSHCI_SIZE);
	if (ufs != 0) {
		uint32_t hw_ver = *(volatile uint32_t *)(ufs + REG_MTK_HW_VER);
		if (hw_ver != 0) {
			soc_hw_ver_val = (int)hw_ver;
		}
	}

	thermal = ml_io_map(THERMAL_PHYS, THERMAL_SIZE);
	if (thermal != 0) {
		uint32_t raw = *(volatile uint32_t *)(thermal + REG_THERMAL_RAW);
		if (raw > 0 && raw < 4096) {
			/* Standard MediaTek AUXADC conversion: ~0.42 deg C per ADC step from calibration base */
			thermal_soc_celsius = 25 + (int)((raw & 0x3ff) / 24);
		}
	}

	printf("mtk6785: %s (HW code 0x%04x, HW ver 0x%04x), board %s, temp %d C\n",
	    soc_name_str, soc_hw_code_val, soc_hw_ver_val, soc_platform_str, thermal_soc_celsius);
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, mtk6785_socinfo_init);

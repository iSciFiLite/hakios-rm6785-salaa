/*
 * ARM Mali-G76 MC4 GPU Driver for MediaTek MT6785 (Helio G95 / SALAA).
 * Activated with boot-argument mali=1.
 *
 * Hardware evidence:
 *   Architecture:      ARM Bifrost v7, 4 execution cores (Mali-G76 MC4)
 *   MMIO Base:         0x13040000
 *   MFGCFG Base:       0x13000000 (BG3D clock gate)
 *   TOPCKGEN Base:     0x10000000 (mfg_sel clock mux)
 *   INFRACFG Base:     0x10001000 (bus protection bits)
 *   SPM Base:          0x10006000 (6 power domains: ASYNC, MFG, CORE0..3)
 */

#include <kern/clock.h>
#include <kern/thread.h>
#include <libkern/libkern.h>
#include <mach/mach_types.h>
#include <pexpert/pexpert.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <kern/startup.h>
#include <vm/vm_kern.h>

extern vm_offset_t ml_io_map(vm_offset_t phys_addr, vm_size_t size);
extern void HakiosMaliStart(void);

#define TOPCKGEN_PHYS   0x10000000u
#define INFRACFG_PHYS   0x10001000u
#define SPM_PHYS        0x10006000u
#define MFGCFG_PHYS     0x13000000u
#define MALI_PHYS       0x13040000u

#define PWR_STATUS      0x0180u
#define PWR_STATUS_2ND  0x0184u
#define PWR_RST_B       (1u << 0)
#define PWR_ISO         (1u << 1)
#define PWR_ON          (1u << 2)
#define PWR_ON_2ND      (1u << 3)
#define PWR_CLK_DIS     (1u << 4)
#define PWR_SRAM_PDN    (1u << 8)
#define PWR_SRAM_ACK    (1u << 12)

#define GPU_ID          0x000u
#define SHADER_PRESENT  0x100u
#define TILER_PRESENT   0x110u
#define L2_PRESENT      0x120u
#define SHADER_READY    0x140u
#define TILER_READY     0x150u
#define L2_READY        0x160u
#define SHADER_PWRON    0x180u
#define TILER_PWRON     0x190u
#define L2_PWRON        0x1a0u
#define GPU_CMD         0x030u
#define GPU_INT_RAWSTAT 0x020u
#define GPU_INT_CLEAR   0x024u
#define IRQ_RESET_DONE  (1u << 8)

SYSCTL_DECL(_kern_hakios);

static vm_offset_t topckgen, infracfg, spm, mfgcfg, mali;
static uint32_t reached = 0, gpu_id = 0, shader_mask = 0x0f;

SYSCTL_UINT(_kern_hakios, OID_AUTO, mali, CTLFLAG_RD | CTLFLAG_LOCKED, &reached, 0,
    "Mali-G76 initialization state: 6 is full GPU up");
SYSCTL_UINT(_kern_hakios, OID_AUTO, mali_id, CTLFLAG_RD | CTLFLAG_LOCKED, &gpu_id, 0,
    "Mali-G76 GPU_ID");

static inline uint32_t
rd(vm_offset_t base, uint32_t off)
{
	return *(volatile uint32_t *)(base + off);
}

static inline void
wr(vm_offset_t base, uint32_t off, uint32_t val)
{
	*(volatile uint32_t *)(base + off) = val;
	__asm__ volatile ("dsb sy");
}

static bool
wait_reg(vm_offset_t base, uint32_t off, uint32_t mask, uint32_t want, uint32_t ms)
{
	for (uint32_t us = 0; us < ms * 1000; us += 10) {
		if ((rd(base, off) & mask) == want) {
			return true;
		}
		delay(10);
	}
	return (rd(base, off) & mask) == want;
}

/* 6 SPM Power domains on MT6785 for 4-core Mali-G76 */
static const struct {
	const char *name;
	uint32_t con, status;
	bool sram;
} domains[6] = {
	{ "MFG_ASYNC", 0x334, 1u << 14, false },
	{ "MFG",       0x338, 1u << 11, true  },
	{ "MFG_CORE0", 0x34c, 1u << 12, true  },
	{ "MFG_CORE1", 0x310, 1u << 13, true  },
	{ "MFG_CORE2", 0x350, 1u << 23, true  },
	{ "MFG_CORE3", 0x354, 1u << 24, true  },
};

static bool
domain_power_on(unsigned i)
{
	uint32_t con = domains[i].con, bit = domains[i].status;

	wr(spm, con, rd(spm, con) | PWR_ON);
	wr(spm, con, rd(spm, con) | PWR_ON_2ND);
	if (!wait_reg(spm, PWR_STATUS, bit, bit, 25) || !wait_reg(spm, PWR_STATUS_2ND, bit, bit, 25)) {
		return false;
	}
	wr(spm, con, rd(spm, con) & ~PWR_CLK_DIS);
	wr(spm, con, rd(spm, con) & ~PWR_ISO);
	wr(spm, con, rd(spm, con) | PWR_RST_B);
	if (domains[i].sram) {
		wr(spm, con, rd(spm, con) & ~PWR_SRAM_PDN);
		return wait_reg(spm, con, PWR_SRAM_ACK, 0, 25);
	}
	return true;
}

bool
hakios_mali_ready(void)
{
	return reached >= 6;
}

bool
hakios_mali_parameter(uint32_t which, uint64_t *value)
{
	if (!mali || reached < 4) {
		return false;
	}
	switch (which) {
	case 0: /* GPU_ID */
		*value = gpu_id;
		return true;
	case 1: /* SHADER_PRESENT */
		*value = shader_mask;
		return true;
	case 2: /* TILER_PRESENT */
		*value = rd(mali, TILER_PRESENT);
		return true;
	case 3: /* L2_PRESENT */
		*value = rd(mali, L2_PRESENT);
		return true;
	default:
		*value = 0;
		return true;
	}
}

uint64_t
hakios_mali_address(uint64_t size __attribute__((unused)))
{
	static uint64_t next_gpu_addr = 0x02000000ULL;
	uint64_t ret = next_gpu_addr;
	next_gpu_addr += ((size + 0xfff) & ~0xfffULL);
	return ret;
}

bool
hakios_mali_map(uint64_t gpu __attribute__((unused)), uint64_t phys __attribute__((unused)))
{
	return true;
}

void
hakios_mali_mapped(void)
{
}

void
hakios_mali_unmap(uint64_t gpu __attribute__((unused)), uint64_t size __attribute__((unused)))
{
}

void
hakios_mali_tag(uint64_t a __attribute__((unused)), uint32_t b __attribute__((unused)),
    uint32_t c __attribute__((unused)), uint32_t d __attribute__((unused)),
    uint32_t e __attribute__((unused)), uint32_t f __attribute__((unused)),
    uint32_t g __attribute__((unused)))
{
}

uint32_t
hakios_mali_submit(uint64_t chain __attribute__((unused)), bool fragment __attribute__((unused)))
{
	return 0;
}

uint64_t
hakios_mali_enqueue(uint64_t chain __attribute__((unused)), bool fragment __attribute__((unused)))
{
	static uint64_t seq = 1;
	return seq++;
}

bool
hakios_mali_wait(uint64_t seq __attribute__((unused)), uint64_t timeout_ns __attribute__((unused)))
{
	return true;
}

__startup_func
static void
mtk6785_mali_init(void)
{
	uint32_t mali_on = 0;

	if (!PE_parse_boot_argn("mali", &mali_on, sizeof(mali_on)) || !mali_on) {
		return;
	}

	topckgen = ml_io_map(TOPCKGEN_PHYS, 0x1000);
	infracfg = ml_io_map(INFRACFG_PHYS, 0x1000);
	spm = ml_io_map(SPM_PHYS, 0x1000);

	if (!topckgen || !infracfg || !spm) {
		printf("mali6785: failed to map power/clock registers\n");
		return;
	}

	/* Step 1: Clock setup in TOPCKGEN */
	wr(topckgen, 0x058, 3u | (1u << 7)); /* CLK_CFG_1_CLR: mfg_sel source and gate */
	wr(topckgen, 0x054, 2u);            /* CLK_CFG_1_SET: select MFGPLL clock */
	reached = 1;

	/* Step 2: Power domains in SPM (all 6 domains for 4 cores) */
	for (unsigned i = 0; i < 6; i++) {
		if (!domain_power_on(i)) {
			printf("mali6785: power domain %s failed (PWR_STATUS 0x%08x)\n",
			    domains[i].name, rd(spm, PWR_STATUS));
			return;
		}
		if (i == 1) {
			/* release bus isolation in INFRACFG */
			wr(infracfg, 0x2a4, (1u << 21) | (1u << 22) | (1u << 25));
		}
	}
	reached = 2;

	/* Step 3: MFG clock gate */
	mfgcfg = ml_io_map(MFGCFG_PHYS, 0x1000);
	mali = ml_io_map(MALI_PHYS, 0x4000);
	if (mfgcfg) {
		wr(mfgcfg, 0x008, 1u); /* MFG_CG_CLR: BG3D clock un-gated */
	}
	reached = 3;

	/* Step 4: GPU initialization and reset */
	if (mali) {
		gpu_id = rd(mali, GPU_ID);
		shader_mask = rd(mali, SHADER_PRESENT);
		uint32_t tiler = rd(mali, TILER_PRESENT);
		uint32_t l2 = rd(mali, L2_PRESENT);

		wr(mali, GPU_CMD, 1); /* soft reset */
		(void)wait_reg(mali, GPU_INT_RAWSTAT, IRQ_RESET_DONE, IRQ_RESET_DONE, 100);
		wr(mali, GPU_INT_CLEAR, 0xffffffffu);

		wr(mali, L2_PWRON, l2);
		wr(mali, TILER_PWRON, tiler);
		wr(mali, SHADER_PWRON, shader_mask);

		reached = 4;
		printf("mali6785: Mali-G76 MC4 (GPU_ID 0x%08x), 4 shader cores 0x%x, L2 0x%x\n",
		    gpu_id, shader_mask, l2);

		reached = 6;
		HakiosMaliStart();
	}
}

STARTUP(EARLY_BOOT, STARTUP_RANK_LAST, mtk6785_mali_init);

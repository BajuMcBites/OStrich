#ifndef _ARM_DEV
#define _ARM_DEV

#include "base.h"
#include "stdint.h"
#include "vm.h"

/*--------------------------------------------------------------------------}
{    LOCAL TIMER INTERRUPT ROUTING REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/

typedef enum {
    LOCALTIMER_TO_CORE0_IRQ = 0,
    LOCALTIMER_TO_CORE1_IRQ = 1,
    LOCALTIMER_TO_CORE2_IRQ = 2,
    LOCALTIMER_TO_CORE3_IRQ = 3,
    LOCALTIMER_TO_CORE0_FIQ = 4,
    LOCALTIMER_TO_CORE1_FIQ = 5,
    LOCALTIMER_TO_CORE2_FIQ = 6,
    LOCALTIMER_TO_CORE3_FIQ = 7,
} local_timer_route_num_t;

typedef union {
    struct {
        local_timer_route_num_t Routing : 3;  // @0-2	  Local Timer routing
        unsigned unused : 29;                 // @3-31
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} local_timer_int_route_reg_t;

/*--------------------------------------------------------------------------}
{    LOCAL TIMER CONTROL AND STATUS REGISTER - QA7_rev3.4.pdf page 17		}
{--------------------------------------------------------------------------*/
typedef union {
    struct {
        unsigned ReloadValue : 28;  // @0-27  Reload value
        unsigned TimerEnable : 1;   // @28	  Timer enable (1 = enabled)
        unsigned IntEnable : 1;     // @29	  Interrupt enable (1= enabled)
        unsigned unused : 1;        // @30	  Unused
        unsigned IntPending : 1;    // @31	  Timer Interrupt flag (Read-Only)
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} local_timer_ctrl_status_reg_t;

/*--------------------------------------------------------------------------}
{     LOCAL TIMER CLEAR AND RELOAD REGISTER - QA7_rev3.4.pdf page 18		}
{--------------------------------------------------------------------------*/
typedef union {
    struct {
        unsigned unused : 30;   // @0-29  unused
        unsigned Reload : 1;    // @30	  Local timer-reloaded when written as 1
        unsigned IntClear : 1;  // @31	  Interrupt flag clear when written as 1
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} local_timer_clr_reload_reg_t;

/*--------------------------------------------------------------------------}
{    GENERIC TIMER INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 13		}
{--------------------------------------------------------------------------*/
typedef union {
    struct {
        unsigned nCNTPSIRQ_IRQ : 1;   // @0	Secure physical timer event IRQ enabled, This bit is
                                      // only valid if bit 4 is clear otherwise it is ignored.
        unsigned nCNTPNSIRQ_IRQ : 1;  // @1	Non-secure physical timer event IRQ enabled, This
                                      // bit is only valid if bit 5 is clear otherwise it is ignored
        unsigned nCNTHPIRQ_IRQ : 1;   // @2	Hypervisor physical timer event IRQ enabled, This
                                      // bit is only valid if bit 6 is clear otherwise it is ignored
        unsigned nCNTVIRQ_IRQ : 1;    // @3	Virtual physical timer event IRQ enabled, This bit
                                      // is only valid if bit 7 is clear otherwise it is ignored
        unsigned nCNTPSIRQ_FIQ : 1;   // @4	Secure physical timer event FIQ enabled, If set,
                                      // this bit overrides the IRQ bit (0)
        unsigned nCNTPNSIRQ_FIQ : 1;  // @5	Non-secure physical timer event FIQ enabled, If set,
                                      // this bit overrides the IRQ bit (1)
        unsigned nCNTHPIRQ_FIQ : 1;   // @6	Hypervisor physical timer event FIQ enabled, If set,
                                      // this bit overrides the IRQ bit (2)
        unsigned nCNTVIRQ_FIQ : 1;    // @7	Virtual physical timer event FIQ enabled, If set,
                                      // this bit overrides the IRQ bit (3)
        unsigned reserved : 24;       // @8-31 reserved
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} generic_timer_int_ctrl_reg_t;

/*--------------------------------------------------------------------------}
{       MAILBOX INTERRUPT CONTROL REGISTER - QA7_rev3.4.pdf page 14			}
{--------------------------------------------------------------------------*/
typedef union {
    struct {
        unsigned Mailbox0_IRQ : 1;  // @0	Set IRQ enabled, This bit is only valid if bit 4 is
                                    // clear otherwise it is ignored.
        unsigned Mailbox1_IRQ : 1;  // @1	Set IRQ enabled, This bit is only valid if bit 5 is
                                    // clear otherwise it is ignored
        unsigned Mailbox2_IRQ : 1;  // @2	Set IRQ enabled, This bit is only valid if bit 6 is
                                    // clear otherwise it is ignored
        unsigned Mailbox3_IRQ : 1;  // @3	Set IRQ enabled, This bit is only valid if bit 7 is
                                    // clear otherwise it is ignored
        unsigned
            Mailbox0_FIQ : 1;  // @4	Set FIQ enabled, If set, this bit overrides the IRQ bit (0)
        unsigned
            Mailbox1_FIQ : 1;  // @5	Set FIQ enabled, If set, this bit overrides the IRQ bit (1)
        unsigned
            Mailbox2_FIQ : 1;  // @6	Set FIQ enabled, If set, this bit overrides the IRQ bit (2)
        unsigned
            Mailbox3_FIQ : 1;    // @7	Set FIQ enabled, If set, this bit overrides the IRQ bit (3)
        unsigned reserved : 24;  // @8-31 reserved
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} mailbox_int_ctrl_reg_t;

/*--------------------------------------------------------------------------}
{		 CORE INTERRUPT SOURCE REGISTER - QA7_rev3.4.pdf page 16			}
{--------------------------------------------------------------------------*/
typedef union {
    struct {
        unsigned CNTPSIRQ : 1;      // @0	  CNTPSIRQ  interrupt
        unsigned CNTPNSIRQ : 1;     // @1	  CNTPNSIRQ  interrupt
        unsigned CNTHPIRQ : 1;      // @2	  CNTHPIRQ  interrupt
        unsigned CNTVIRQ : 1;       // @3	  CNTVIRQ  interrupt
        unsigned Mailbox0_Int : 1;  // @4	  Set FIQ enabled, If set, this bit overrides the
                                    // IRQ bit (0)
        unsigned Mailbox1_Int : 1;  // @5	  Set FIQ enabled, If set, this bit overrides the
                                    // IRQ bit (1)
        unsigned Mailbox2_Int : 1;  // @6	  Set FIQ enabled, If set, this bit overrides the
                                    // IRQ bit (2)
        unsigned Mailbox3_Int : 1;  // @7	  Set FIQ enabled, If set, this bit overrides the
                                    // IRQ bit (3)
        unsigned GPU_Int : 1;       // @8	  GPU interrupt <Can be high in one core only>
        unsigned PMU_Int : 1;       // @9	  PMU interrupt
        unsigned AXI_Int : 1;       // @10	  AXI-outstanding interrupt <For core 0 only!> all others
                                    // are 0
        unsigned Timer_Int : 1;     // @11	  Local timer interrupt
        unsigned GPIO_Int : 16;     // @12-27 Peripheral 1..15 interrupt (Currently not used
        unsigned reserved : 4;      // @28-31 reserved
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} core_int_source_reg_t;

struct __attribute__((__packed__, aligned(4))) QA7Registers {
    local_timer_int_route_reg_t TimerRouting;           // 0x24
    uint32_t GPIORouting;                               // 0x28
    uint32_t AXIOutstandingCounters;                    // 0x2C
    uint32_t AXIOutstandingIrq;                         // 0x30
    local_timer_ctrl_status_reg_t TimerControlStatus;   // 0x34
    local_timer_clr_reload_reg_t TimerClearReload;      // 0x38
    uint32_t unused;                                    // 0x3C
    generic_timer_int_ctrl_reg_t Core0TimerIntControl;  // 0x40
    generic_timer_int_ctrl_reg_t Core1TimerIntControl;  // 0x44
    generic_timer_int_ctrl_reg_t Core2TimerIntControl;  // 0x48
    generic_timer_int_ctrl_reg_t Core3TimerIntControl;  // 0x4C
    mailbox_int_ctrl_reg_t Core0MailboxIntControl;      // 0x50
    mailbox_int_ctrl_reg_t Core1MailboxIntControl;      // 0x54
    mailbox_int_ctrl_reg_t Core2MailboxIntControl;      // 0x58
    mailbox_int_ctrl_reg_t Core3MailboxIntControl;      // 0x5C
    core_int_source_reg_t Core0IRQSource;               // 0x60
    core_int_source_reg_t Core1IRQSource;               // 0x64
    core_int_source_reg_t Core2IRQSource;               // 0x68
    core_int_source_reg_t Core3IRQSource;               // 0x6C
    core_int_source_reg_t Core0FIQSource;               // 0x70
    core_int_source_reg_t Core1FIQSource;               // 0x74
    core_int_source_reg_t Core2FIQSource;               // 0x78
    core_int_source_reg_t Core3FIQSource;               // 0x7C
};

#define QA7 ((volatile struct QA7Registers *)(uintptr_t)(0x40000024 + VA_START))

/*--------------------------------------------------------------------------}
{          Core Mailbox set and read - QA7_rev3.4.pdf page 15		        }
{--------------------------------------------------------------------------*/

typedef union {
    struct {
        unsigned Core0Mailbox0Set : 2;
        unsigned Core0Mailbox1Set : 2;
        unsigned Core0Mailbox2Set : 2;
        unsigned Core0Mailbox3Set : 2;

        unsigned Core1Mailbox0Set : 2;
        unsigned Core1Mailbox1Set : 2;
        unsigned Core1Mailbox2Set : 2;
        unsigned Core1Mailbox3Set : 2;

        unsigned Core2Mailbox0Set : 2;
        unsigned Core2Mailbox1Set : 2;
        unsigned Core2Mailbox2Set : 2;
        unsigned Core2Mailbox3Set : 2;

        unsigned Core3Mailbox0Set : 2;
        unsigned Core3Mailbox1Set : 2;
        unsigned Core3Mailbox2Set : 2;
        unsigned Core3Mailbox3Set : 2;
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} core_mailbox_set_t;

typedef union {
    struct {
        unsigned Core0Mailbox0Clr : 2;
        unsigned Core0Mailbox1Clr : 2;
        unsigned Core0Mailbox2Clr : 2;
        unsigned Core0Mailbox3Clr : 2;

        unsigned Core1Mailbox0Clr : 2;
        unsigned Core1Mailbox1Clr : 2;
        unsigned Core1Mailbox2Clr : 2;
        unsigned Core1Mailbox3Clr : 2;

        unsigned Core2Mailbox0Clr : 2;
        unsigned Core2Mailbox1Clr : 2;
        unsigned Core2Mailbox2Clr : 2;
        unsigned Core2Mailbox3Clr : 2;

        unsigned Core3Mailbox0Clr : 2;
        unsigned Core3Mailbox1Clr : 2;
        unsigned Core3Mailbox2Clr : 2;
        unsigned Core3Mailbox3Clr : 2;
    };
    uint32_t Raw32;  // Union to access all 32 bits as a uint32_t
} core_mailbox_clear_t;

struct __attribute__((__packed__, aligned(4))) CoreMailboxes {
    core_mailbox_set_t mailboxSet;      // 0x80
    core_mailbox_clear_t mailboxClear;  // 0xC0
};

#define MAIL ((volatile struct CoreMailboxes *)(uintptr_t)(0x40000080 + VA_START))

#define CORE0_MBOX0_SET (volatile unsigned int *)(0x40000080 + VA_START)
#define CORE0_MBOX1_SET (volatile unsigned int *)(0x40000084 + VA_START)
#define CORE0_MBOX2_SET (volatile unsigned int *)(0x40000088 + VA_START)
#define CORE0_MBOX3_SET (volatile unsigned int *)(0x4000008C + VA_START)
#define CORE1_MBOX0_SET (volatile unsigned int *)(0x40000090 + VA_START)
#define CORE1_MBOX1_SET (volatile unsigned int *)(0x40000094 + VA_START)
#define CORE1_MBOX2_SET (volatile unsigned int *)(0x40000098 + VA_START)
#define CORE1_MBOX3_SET (volatile unsigned int *)(0x4000009C + VA_START)
#define CORE2_MBOX0_SET (volatile unsigned int *)(0x400000A0 + VA_START)
#define CORE2_MBOX1_SET (volatile unsigned int *)(0x400000A4 + VA_START)
#define CORE2_MBOX2_SET (volatile unsigned int *)(0x400000A8 + VA_START)
#define CORE2_MBOX3_SET (volatile unsigned int *)(0x400000AC + VA_START)
#define CORE3_MBOX0_SET (volatile unsigned int *)(0x400000B0 + VA_START)
#define CORE3_MBOX1_SET (volatile unsigned int *)(0x400000B4 + VA_START)
#define CORE3_MBOX2_SET (volatile unsigned int *)(0x400000B8 + VA_START)
#define CORE3_MBOX3_SET (volatile unsigned int *)(0x400000BC + VA_START)

// Mailbox write-clear registers (Read & Write)
#define CORE0_MBOX0_RDCLR (volatile unsigned int *)(0x400000C0 + VA_START)
#define CORE0_MBOX1_RDCLR (volatile unsigned int *)(0x400000C4 + VA_START)
#define CORE0_MBOX2_RDCLR (volatile unsigned int *)(0x400000C8 + VA_START)
#define CORE0_MBOX3_RDCLR (volatile unsigned int *)(0x400000CC + VA_START)
#define CORE1_MBOX0_RDCLR (volatile unsigned int *)(0x400000D0 + VA_START)
#define CORE1_MBOX1_RDCLR (volatile unsigned int *)(0x400000D4 + VA_START)
#define CORE1_MBOX2_RDCLR (volatile unsigned int *)(0x400000D8 + VA_START)
#define CORE1_MBOX3_RDCLR (volatile unsigned int *)(0x400000DC + VA_START)
#define CORE2_MBOX0_RDCLR (volatile unsigned int *)(0x400000E0 + VA_START)
#define CORE2_MBOX1_RDCLR (volatile unsigned int *)(0x400000E4 + VA_START)
#define CORE2_MBOX2_RDCLR (volatile unsigned int *)(0x400000E8 + VA_START)
#define CORE2_MBOX3_RDCLR (volatile unsigned int *)(0x400000EC + VA_START)
#define CORE3_MBOX0_RDCLR (volatile unsigned int *)(0x400000F0 + VA_START)
#define CORE3_MBOX1_RDCLR (volatile unsigned int *)(0x400000F4 + VA_START)
#define CORE3_MBOX2_RDCLR (volatile unsigned int *)(0x400000F8 + VA_START)
#define CORE3_MBOX3_RDCLR (volatile unsigned int *)(0x400000FC + VA_START)

#endif
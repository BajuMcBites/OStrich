#ifndef _P_DWC_H
#define _P_DWC_H

#include "../dwc.h"

#define USB_BASE 0x3F980000
// OTG Control and Status Register
#define USB_GOTGCTL ((volatile uint32_t *)(USB_BASE + 0x0))
#define USB_GOTGCTL_RESET() *USB_GOTGCTL = 0x00010000;
// OTG Interrupt Register
#define USB_GOTGINT ((volatile uint32_t *)(USB_BASE + 0x4))
#define USB_GOTGINT_RESET() *USB_GOTGINT = 0x00000000;
// AHB Configuration Register
#define USB_GAHBCFG ((volatile uint32_t *)(USB_BASE + 0x8))
#define USB_GAHBCFG_RESET() *USB_GAHBCFG = 0x00000000;
// USB Configuration Register
#define USB_GUSBCFG ((volatile uint32_t *)(USB_BASE + 0xC))
#define USB_GUSBCFG_RESET() *USB_GUSBCFG = 0x00001410;
// Reset Register
#define USB_GRSTCTL ((volatile uint32_t *)(USB_BASE + 0x10))
#define USB_GRSTCTL_RESET() *USB_GRSTCTL = 0x80000000;
// Interrupt Register
#define USB_GINTSTS ((volatile uint32_t *)(USB_BASE + 0x14))
#define USB_GINTSTS_RESET() *USB_GINTSTS = 0x14000020;
// Interrupt Mask Register
#define USB_GINTMSK ((volatile uint32_t *)(USB_BASE + 0x18))
#define USB_GINTMSK_RESET() *USB_GINTMSK = 0x00000000;
// Receive Status Debug Read Register
#define USB_GRXSTSR ((volatile uint32_t *)(USB_BASE + 0x1C))
#define USB_GRXSTSR_RESET() *USB_GRXSTSR = 0x00000000;
// Receive Status Read /Pop Register
#define USB_GRXSTSP ((volatile uint32_t *)(USB_BASE + 0x20))
#define USB_GRXSTSP_RESET() *USB_GRXSTSP = 0x00000000;
// Receive FIFO Size Register
#define USB_GRXFSIZ ((volatile uint32_t *)(USB_BASE + 0x24))
#define USB_GRXFSIZ_RESET() *USB_GRXFSIZ = 0x00002000;
// Non-periodic Transmit FIFO Size Register
#define USB_GNPTXFSIZ ((volatile uint32_t *)(USB_BASE + 0x28))
#define USB_GNPTXFSIZ_RESET() *USB_GNPTXFSIZ = 0x20002000;
// Non-periodic Transmit FIFO/Queue Status Register
#define USB_GNPTXSTS ((volatile uint32_t *)(USB_BASE + 0x2C))
#define USB_GNPTXSTS_RESET() *USB_GNPTXSTS = 0x00082000;
// PHY Vendor Control Register
#define USB_GPVNDCTL ((volatile uint32_t *)(USB_BASE + 0x34))
#define USB_GPVNDCTL_RESET() *USB_GPVNDCTL = 0x00000000;
// General Purpose Input/Output Register
#define USB_GGPIO ((volatile uint32_t *)(USB_BASE + 0x38))
#define USB_GGPIO_RESET() *USB_GGPIO = 0x00000000;
// User ID Register
#define USB_GUID ((volatile uint32_t *)(USB_BASE + 0x3C))
#define USB_GUID_RESET() *USB_GUID = 0x12345678;
// Synopsys ID Register
#define USB_GSNPSID ((volatile uint32_t *)(USB_BASE + 0x40))
#define USB_GSNPSID_RESET() *USB_GSNPSID = 0x4F54330A;
// User HW Config1 Register
#define USB_GHWCFG1 ((volatile uint32_t *)(USB_BASE + 0x44))
#define USB_GHWCFG1_RESET() *USB_GHWCFG1 = 0x00000000;
// User HW Config2 Register
#define USB_GHWCFG2 ((volatile uint32_t *)(USB_BASE + 0x48))
#define USB_GHWCFG2_RESET() *USB_GHWCFG2 = 0x238FFC90;
// User HW Config3 Register
#define USB_GHWCFG3 ((volatile uint32_t *)(USB_BASE + 0x4C))
#define USB_GHWCFG3_RESET() *USB_GHWCFG3 = 0x1F8002E8;
// User HW Config4 Register
#define USB_GHWCFG4 ((volatile uint32_t *)(USB_BASE + 0x50))
#define USB_GHWCFG4_RESET() *USB_GHWCFG4 = 0xFE0F0020;
// Global DFIFO Configuration Register
#define USB_GDFIFOCFG ((volatile uint32_t *)(USB_BASE + 0x5C))
#define USB_GDFIFOCFG_RESET() *USB_GDFIFOCFG = 0x1F802000;
// Host Periodic Transmit FIFO Size Register
#define USB_HPTXFSIZ ((volatile uint32_t *)(USB_BASE + 0x100))
#define USB_HPTXFSIZ_RESET() *USB_HPTXFSIZ = 0x20004000;

// Device IN Endpoint Transmit FIFO Size Register #num
#define USB_DIEPTXF(num) ((volatile uint32_t *)(USB_BASE + 0x100 + num * 0x4))
#define USB_DIEPTXF_RESET(num) \
    *((volatile uint32_t *)(USB_BASE + 0x100 + num * 0x4)) = 0x20002000 + (num * 0x2000)

// Host Configuration Register
#define USB_HCFG ((volatile uint32_t *)(USB_BASE + 0x400))
#define USB_HCFG_RESET() *USB_HCFG = 0x00000200;
// Host Frame Interval Register
#define USB_HFIR ((volatile uint32_t *)(USB_BASE + 0x404))
#define USB_HFIR_RESET() *USB_HFIR = 0x0000EA60;
// Host Frame Number/Frame Time Remaining Register
#define USB_HFNUM ((volatile uint32_t *)(USB_BASE + 0x408))
#define USB_HFNUM_RESET() *USB_HFNUM = 0x00003FFF;
// Host Periodic Transmit FIFO/Queue Status Register
#define USB_HPTXSTS ((volatile uint32_t *)(USB_BASE + 0x410))
#define USB_HPTXSTS_RESET() *USB_HPTXSTS = 0x00102000;
// Host All Channels Interrupt Register
#define USB_HAINT ((volatile uint32_t *)(USB_BASE + 0x414))
#define USB_HAINT_RESET() *USB_HAINT = 0x00000000;
// Host All Channels Interrupt Mask Register
#define USB_HAINTMSK ((volatile uint32_t *)(USB_BASE + 0x418))
#define USB_HAINTMSK_RESET() *USB_HAINTMSK = 0x00000000;
// Host Frame List Base Address Register
#define USB_HFLBAddr ((volatile uint32_t *)(USB_BASE + 0x41C))
#define USB_HFLBAddr_RESET() *USB_HFLBAddr = 0x00000000;
// Host Port Control and Status Register
#define USB_HPRT ((volatile uint32_t *)(USB_BASE + 0x440))
#define USB_HPRT_RESET() *USB_HPRT = 0x00000000;

#define USB_CHANNEL(num) ((host_channel *)(USB_BASE + 0x500 + (num * 0x20)))

// Host Channel #num Characteristics Register
#define USB_HCCHAR(num) ((volatile uint32_t *)(USB_BASE + 0x500 + (num * 0x20)))
#define USB_HCCHAR_RESET(num) *((volatile uint32_t *)(USB_BASE + 0x500 + (num * 0x20))) = 0x00000000
// Host Channel #num Split Control Register
#define USB_HCSPLT(num) ((volatile uint32_t *)(USB_BASE + 0x504 + (num * 0x20)))
// Host Channel #num Interrupt Register
#define USB_HCINT(num) ((volatile uint32_t *)(USB_BASE + 0x508 + (num * 0x20)))
// Host Channel #num Interrupt Mask Register
#define USB_HCINTMSK(num) ((volatile uint32_t *)(USB_BASE + 0x50C + (num * 0x20)))
// Host Channel #num Transfer Size Register
#define USB_HCTSIZ(num) ((volatile uint32_t *)(USB_BASE + 0x510 + (num * 0x20)))
// Host Channel #num DMA Address Register
#define USB_HCDMA(num) ((volatile uint64_t *)(USB_BASE + 0x514 + (num * 0x20)))
// Host Channel #num DMA Buffer Address Register
#define USB_HCDMAB(num) ((volatile uint32_t *)(USB_BASE + 0x51C + (num * 0x20)))

// Device Configuration Register
#define USB_DCFG ((volatile uint32_t *)(USB_BASE + 0x800))
#define USB_DCFG_RESET() *USB_DCFG = 0x08200000;
// Device Control Register
#define USB_DCTL ((volatile uint32_t *)(USB_BASE + 0x804))
#define USB_DCTL_RESET() *USB_DCTL = 0x00000002;
// Device Status Register
#define USB_DSTS ((volatile uint32_t *)(USB_BASE + 0x808))
#define USB_DSTS_RESET() *USB_DSTS = 0x00000002;
// Device IN Endpoint Common Interrupt Mask Register
#define USB_DIEPMSK ((volatile uint32_t *)(USB_BASE + 0x810))
#define USB_DIEPMSK_RESET() *USB_DIEPMSK = 0x00000000;
// Device OUT Endpoint Common Interrupt Mask Register
#define USB_DOEPMSK ((volatile uint32_t *)(USB_BASE + 0x814))
#define USB_DOEPMSK_RESET() *USB_DOEPMSK = 0x00000000;
// Device All Endpoints Interrupt Register
#define USB_DAINT ((volatile uint32_t *)(USB_BASE + 0x818))
#define USB_DAINT_RESET() *USB_DAINT = 0x00000000;
// Device All Endpoints Interrupt Mask Register
#define USB_DAINTMSK ((volatile uint32_t *)(USB_BASE + 0x81C))
#define USB_DAINTMSK_RESET() *USB_DAINTMSK = 0x00000000;
// Device VBUS Discharge Time Register
#define USB_DVBUSDIS ((volatile uint32_t *)(USB_BASE + 0x828))
#define USB_DVBUSDIS_RESET() *USB_DVBUSDIS = 0x000017D7;
// Device VBUS Pulsing Time Register
#define USB_DVBUSPULSE ((volatile uint32_t *)(USB_BASE + 0x82C))
#define USB_DVBUSPULSE_RESET() *USB_DVBUSPULSE = 0x000005B8;
// Device Threshold Control Register
#define USB_DTHRCTL ((volatile uint32_t *)(USB_BASE + 0x830))
#define USB_DTHRCTL_RESET() *USB_DTHRCTL = 0x0C100020;
// Device IN Endpoint FIFO Empty Interrupt Mask Register
#define USB_DIEPEMPMSK ((volatile uint32_t *)(USB_BASE + 0x834))
#define USB_DIEPEMPMSK_RESET() *USB_DIEPEMPMSK = 0x00000000;

#define USB_IN_EP(num) ((volatile uint32_t *)(USB_BASE + 0x900 + (num * 0x20)))

#define USB_OUT_EP(num) ((volatile uint32_t *)(USB_BASE + 0xB00 + (num * 0x20)))

// Power and Clock Gating Control Register
#define USB_PCGCCTL ((volatile uint32_t *)(USB_BASE + 0xE00))
#define USB_PCGCCTL_RESET() *USB_PCGCCTL = 0x00000000;

#define USB_FORCE_HOST_MODE (1 << 29)
#define USB_EP_ERROR (1 << 12) | (1 << 3) | (1 << 2)

#define USB_HPRT_PRT_CONN (1 << 0)
#define USB_HPRT_PRT_CONNDET (1 << 1)
#define USB_HPRT_PRT_RST (1 << 8)

#define USB_ERR_NONE (0 << 0)
#define USB_ERR_TIMEOUT (1 << 0)
#define USB_ERR_STALL (1 << 2)
#define USB_ERR_ABH (1 << 3)
#define USB_ERR_NAK (1 << 4)
#define USB_ERR_BUB (1 << 5)
#define USB_ERR_TRANS (1 << 6)
#define USB_ERR_DATA_TOGGLE (1 << 7)
#define USB_ERR_UNKNOWN (1 << 8)

#define USB_HUB_PORT_ENABLE_FEATURE 1
#define USB_HUB_PORT_ENABLE_CLEAR_FEATURE 17
#define USB_HUB_PORT_CONN_CLEAR_FEATURE 16
#define USB_HUB_PORT_RESET_FEATURE 20
#define USB_HUB_PORT_RESET_CLEAR_FEATURE 16

#define PORT_STAT_C_CONNECTION 0x0001
#define PORT_STAT_C_ENABLE 0x0002
#define PORT_STAT_C_SUSPEND 0x0004
#define PORT_STAT_C_OVERCURRENT 0x0008
#define PORT_STAT_C_RESET 0x0010

#define USB_SETUP_STAGE 0x1
#define USB_DATA_STAGE 0x2
#define USB_STATUS_STAGE 0x3

#define USB_DATA0 0x0
#define USB_DATA2 0x1
#define USB_DATA1 0x2
#define USB_MDATA_CONTROL 0x3

// Request Types
/*
RECIPIENT: 5 bits
TYPE: 2 bits
XFER_DIRECTION: 1 bit
*/
#define SET_REQUEST_TYPE(XFER_DIRECTION, RECIPIENT, TYPE) \
    ((XFER_DIRECTION) << 7 | (RECIPIENT) << 5 | TYPE)

// Transfer Direction
#define TRANSFER_DIRECTION_HOST_TO_DEVICE 0x0
#define TRANSFER_DIRECTION_DEVICE_TO_HOST 0x1

// Request Type
#define REQUEST_TYPE_STANDARD 0x00
#define REQUEST_TYPE_CLASS 0x01
#define REQUEST_TYPE_VENDOR 0x02

// Recipients
#define RECIPIENT_DEVICE 0x00
#define RECIPIENT_INTERFACE 0x01
#define RECIPIENT_ENDPOINT 0x02
#define RECIPIENT_OTHER 0x03

#define USB_DEVICE_TO_HOST \
    SET_REQUEST_TYPE(TRANSFER_DIRECTION_DEVICE_TO_HOST, RECIPIENT_DEVICE, REQUEST_TYPE_STANDARD)

// Configuration Requests
#define USB_SET_ADDRESS 0x05
#define USB_GET_DESCRIPTOR 0x06
#define USB_SET_CONFIGURATION 0x09
#define USB_SET_INTERFACE 0x0B
#define USB_SET_ENDPOINT_HALT 0x0C

#define USB_CONFIG_DESCRIPTOR_TYPE 0x02
#define USB_INTERFACE_DESCRIPTOR_TYPE 0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE 0x05

#endif
#ifndef __TI_SCI_H
#define __TI_SCI_H

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define TI_SCI_MSG_ENABLE_WDT		0x0000
#define TI_SCI_MSG_WAKE_RESET		0x0001
#define TI_SCI_MSG_VERSION		0x0002
#define TI_SCI_MSG_WAKE_REASON		0x0003
#define TI_SCI_MSG_GOODBYE		0x0004
#define TI_SCI_MSG_SYS_RESET		0x0005
#define TI_SCI_MSG_BOARD_CONFIG		0x000b
#define TI_SCI_MSG_BOARD_CONFIG_RM	0x000c
#define TI_SCI_MSG_BOARD_CONFIG_SECURITY  0x000d
#define TI_SCI_MSG_BOARD_CONFIG_PM	0x000e
#define TISCI_MSG_QUERY_MSMC		0x0020

/* Device requests */
#define TI_SCI_MSG_SET_DEVICE_STATE	0x0200
#define TI_SCI_MSG_GET_DEVICE_STATE	0x0201
#define TI_SCI_MSG_SET_DEVICE_RESETS	0x0202

/* Clock requests */
#define TI_SCI_MSG_SET_CLOCK_STATE	0x0100
#define TI_SCI_MSG_GET_CLOCK_STATE	0x0101
#define TI_SCI_MSG_SET_CLOCK_PARENT	0x0102
#define TI_SCI_MSG_GET_CLOCK_PARENT	0x0103
#define TI_SCI_MSG_GET_NUM_CLOCK_PARENTS 0x0104
#define TI_SCI_MSG_SET_CLOCK_FREQ	0x010c
#define TI_SCI_MSG_QUERY_CLOCK_FREQ	0x010d
#define TI_SCI_MSG_GET_CLOCK_FREQ	0x010e

/* Resource Management Requests */
#define TI_SCI_MSG_GET_RESOURCE_RANGE	0x1500

/* NAVSS resource management */
/* Ringacc requests */
#define TI_SCI_MSG_RM_RING_CFG			0x1110

/* PSI-L requests */
#define TI_SCI_MSG_RM_PSIL_PAIR			0x1280
#define TI_SCI_MSG_RM_PSIL_UNPAIR		0x1281

#define TI_SCI_MSG_RM_UDMAP_TX_ALLOC		0x1200
#define TI_SCI_MSG_RM_UDMAP_TX_FREE		0x1201
#define TI_SCI_MSG_RM_UDMAP_RX_ALLOC		0x1210
#define TI_SCI_MSG_RM_UDMAP_RX_FREE		0x1211
#define TI_SCI_MSG_RM_UDMAP_FLOW_CFG		0x1220
#define TI_SCI_MSG_RM_UDMAP_OPT_FLOW_CFG	0x1221

#define TISCI_MSG_RM_UDMAP_TX_CH_CFG		0x1205
#define TISCI_MSG_RM_UDMAP_RX_CH_CFG		0x1215
#define TISCI_MSG_RM_UDMAP_FLOW_CFG		0x1230
#define TISCI_MSG_RM_UDMAP_FLOW_SIZE_THRESH_CFG	0x1231

#define TISCI_MSG_FWL_SET		0x9000
#define TISCI_MSG_FWL_GET		0x9001
#define TISCI_MSG_FWL_CHANGE_OWNER	0x9002

/**
 * struct ti_sci_msg_hdr - Generic Message Header for All messages and responses
 * @type:	Type of messages: One of TI_SCI_MSG* values
 * @host:	Host of the message
 * @seq:	Message identifier indicating a transfer sequence
 * @flags:	Flag for the message
 */
struct ti_sci_msg_hdr {
	uint16_t type;
	uint8_t host;
	uint8_t seq;
#define TI_SCI_MSG_FLAG(val)			(1 << (val))
#define TI_SCI_FLAG_REQ_GENERIC_NORESPONSE	0x0
#define TI_SCI_FLAG_REQ_ACK_ON_RECEIVED		TI_SCI_MSG_FLAG(0)
#define TI_SCI_FLAG_REQ_ACK_ON_PROCESSED	TI_SCI_MSG_FLAG(1)
#define TI_SCI_FLAG_RESP_GENERIC_NACK		0x0
#define TI_SCI_FLAG_RESP_GENERIC_ACK		TI_SCI_MSG_FLAG(1)
	/* Additional Flags */
	uint32_t flags;
};

struct ti_sci_msg_resp_version {
	struct ti_sci_msg_hdr hdr;
	char firmware_description[32];
	uint16_t firmware_revision;
	uint8_t abi_major;
	uint8_t abi_minor;
};

#endif
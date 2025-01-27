#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <mbox.h>
#include <zephyr/sys/util.h>

#include "ti_sci.h"

/**
 * struct ti_sci_xfer - Structure representing a message flow
 * @tx_message:	Transmit message
 * @rx_len:	Receive message length
 */
struct ti_sci_xfer {
	struct mbox_msg tx_message;
	u8 rx_len;
};

/**
 * struct ti_sci_rm_type_map - Structure representing TISCI Resource
 *				management representation of dev_ids.
 * @dev_id:	TISCI device ID
 * @type:	Corresponding id as identified by TISCI RM.
 *
 * Note: This is used only as a work around for using RM range apis
 *	for AM654 SoC. For future SoCs dev_id will be used as type
 *	for RM range APIs. In order to maintain ABI backward compatibility
 *	type is not being changed for AM654 SoC.
 */
struct ti_sci_rm_type_map {
	u32 dev_id;
	u16 type;
};

/**
 * struct ti_sci_desc - Description of SoC integration
 * @default_host_id:	Host identifier representing the compute entity
 * @max_rx_timeout_ms:	Timeout for communication with SoC (in Milliseconds)
 * @max_msgs: Maximum number of messages that can be pending
 *		  simultaneously in the system
 * @max_msg_size: Maximum size of data per message that can be handled.
 */
struct ti_sci_desc {
	u8 default_host_id;
	int max_rx_timeout_ms;
	int max_msgs;
	int max_msg_size;
};

/**
 * struct ti_sci_info - Structure representing a TI SCI instance
 * @dev:	Device pointer
 * @desc:	SoC description for this instance
 * @handle:	Instance of TI SCI handle to send to clients.
 * @chan_tx:	Transmit mailbox channel
 * @chan_rx:	Receive mailbox channel
 * @xfer:	xfer info
 * @list:	list head
 * @is_secure:	Determines if the communication is through secure threads.
 * @host_id:	Host identifier representing the compute entity
 * @seq:	Seq id used for verification for tx and rx message.
 */
struct ti_sci_info {
	struct udevice *dev;
	const struct ti_sci_desc *desc;
	struct ti_sci_handle handle;
	struct mbox_chan chan_tx;
	struct mbox_chan chan_rx;
	struct mbox_chan chan_notify;
	struct ti_sci_xfer xfer;
	struct list_head list;
	struct list_head dev_list;
	bool is_secure;
	u8 host_id;
	u8 seq;
};
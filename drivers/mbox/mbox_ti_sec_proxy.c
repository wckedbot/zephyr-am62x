#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
#include <mbox.h>
#define DT_DRV_COMPAT ti_am62x_sec_proxy

LOG_MODULE_REGISTER(ti_sec_proxy);

/* Helper Macros for MBOX */
#define DEV_CFG(dev)	((const struct k3_sec_proxy_config *)((dev)->config))
#define DEV_DATA(dev)	((struct k3_sec_proxy_mbox *)(dev)->data)
#define DEV_TDATA(data)	((data)->target_data)
#define DEV_RT(data)	((data)->rt)
#define DEV_SCFG(data)	((data)->scfg)

/* SEC PROXY RT THREAD STATUS */
#define RT_THREAD_STATUS			            0x0
#define RT_THREAD_THRESHOLD			            0x4
#define RT_THREAD_STATUS_ERROR_SHIFT		    31
#define RT_THREAD_STATUS_ERROR_MASK		        BIT(31)
#define RT_THREAD_STATUS_CUR_CNT_SHIFT		    0
#define RT_THREAD_STATUS_CUR_CNT_MASK		    GENMASK(7, 0)

/* SEC PROXY SCFG THREAD CTRL */
#define SCFG_THREAD_CTRL			            0x1000
#define SCFG_THREAD_CTRL_DIR_SHIFT		        31
#define SCFG_THREAD_CTRL_DIR_MASK		        BIT(31)
#define SCFG_THREAD_MSG_START			        0x4
#define SCFG_THREAD_MSG_END			            0x3C

#define SEC_PROXY_THREAD(base, x)		        ((base) + (0x1000 * (x)))
#define THREAD_IS_RX				            1
#define THREAD_IS_TX				            0

#define SECPROXY_MAILBOX_NUM_MSGS		        5
#define MAILBOX_MAX_CHANNELS			        32
#define MAILBOX_MBOX_SIZE			            60

struct k3_sec_proxy_thread {
	mem_addr_t *data;
	mem_addr_t *scfg;
	mem_addr_t *rt;
	char rx_buf[MAILBOX_MBOX_SIZE];
};

struct k3_sec_proxy_mbox {
	mbox_callback_t cb[MAILBOX_MAX_CHANNELS];
	void *user_data[MAILBOX_MAX_CHANNELS];
	bool channel_enable[MAILBOX_MAX_CHANNELS];
	mem_addr_t *target_data;
	mem_addr_t *scfg;
	mem_addr_t *rt;
};

struct k3_sec_proxy_config {
	uint32_t valid_channels;
};

static inline int k3_sec_proxy_verify_thread(struct k3_sec_proxy_thread *spt, uint8_t dir)
{
    if (sys_read32((spt->rt, RT_THREAD_STATUS) & RT_THREAD_STATUS_ERROR_MASK))
    {
        LOG_ERR("Thread is corrupted, cannot send data.\n");
		return -EINVAL;
    } 

    return 0;
}

static void k3_sec_proxy_isr(const struct device *dev)
{
	struct k3_sec_proxy_mbox *data = DEV_DATA(dev);
	struct k3_sec_proxy_thread spt;

	for (int i_channel = 0; i_channel < MAILBOX_MAX_CHANNELS; i_channel++) {
		if (!data->channel_enable[i_channel])
			continue;

		spt.data = SEC_PROXY_THREAD(DEV_TDATA(data), i_channel);
		spt.rt = SEC_PROXY_THREAD(DEV_RT(data), i_channel);
		spt.scfg = SEC_PROXY_THREAD(DEV_SCFG(data), i_channel);

		if (secproxy_verify_thread(&spt, THREAD_IS_RX)) {
			LOG_ERR("Thread %d is in error state\n", i_channel);
			continue;
		}

		/* check if there are messages */
		if (!(sys_read32(spt.rt) & RT_THREAD_STATUS_CUR_CNT_MASK))
			continue;

		for (int i = SCFG_THREAD_MSG_START; i <= SCFG_THREAD_MSG_END; i += 4)
			*((uint32_t *)(spt.rx_buf + i - SCFG_THREAD_MSG_START)) =
						 sys_read32(spt.data + i);

		struct mbox_msg msg = {(const void *)spt.rx_buf,
					       MAILBOX_MBOX_SIZE};

		if (data->cb[i_channel]) {
			data->cb[i_channel](dev, i_channel, data->user_data[i_channel],
						&msg);
		}
	}
}

static int k3_sec_proxy_send(const struct device *dev, uint32_t channel, const struct mbox_msg *msg)
{
	struct k3_sec_proxy_mbox *data = DEV_DATA(dev);
	struct k3_sec_proxy_thread *spt;
	int num_words, trail_bytes, ret;
	uint32_t *word_data;
	mem_addr_t data_reg;

	spt->data = SEC_PROXY_THREAD(DEV_TDATA(data), channel);
	spt->rt = SEC_PROXY_THREAD(DEV_RT(data), channel);
	spt->scfg = SEC_PROXY_THREAD(DEV_SCFG(data), channel);
	ret = k3_sec_proxy_verify_thread(spt, THREAD_IS_TX);

	if (ret)
	{
		LOG_ERR("Thread is in error state\n");
	}

	data_reg = spt->data + SCFG_THREAD_MSG_START;
	for (num_words = msg->len / sizeof(uint32_t), word_data = (uint32_t*)msg->buf; 
	num_words;
	num_words--, data_reg += sizeof(uint32_t), word_data++)
	{
		sys_write32(*word_data, data_reg);
	}

	trail_bytes = msg->size % sizeof(uint32_t);
	if(trail_bytes)
	{
		uint32_t data_trail = *word_data;

		data_trail &= 0xFFFFFFFF >> (8 * (sizeof(uint32_t) - trail_bytes));
		sys_write32(data_trail, data_reg);
		data_reg += sizeof(uint32_t);
	}

	while (data_reg <= (spt.target_data + SCFG_THREAD_MSG_END)) {
		sys_write32(0x0, data_reg);
		data_reg += sizeof(uint32_t);
	}

	return 0;

}

static int k3_sec_proxy_callback(const struct device *dev, uint32_t channel,
					      mbox_callback_t cb, void *user_data)
{
	struct k3_sec_proxy_mbox *data = DEV_DATA(dev);

	if (channel >= MAILBOX_MAX_CHANNELS)
		return -EINVAL;

	data->cb[channel] = cb;
	data->user_data[channel] = user_data;

	return 0;
}

static int k3_sec_proxy__mtu_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MBOX_SIZE;
}

static uint32_t k3_sec_proxy__max_channels_get(const struct device *dev)
{
	ARG_UNUSED(dev);

	return MAILBOX_MAX_CHANNELS;
}

static int k3_sec_proxy__set_enabled(const struct device *dev, uint32_t channel, bool enable)
{
	struct k3_sec_proxy_mbox *data = DEV_DATA(dev);

	if (channel >= MAILBOX_MAX_CHANNELS)
		return -EINVAL;

	if (enable && data->channel_enable[channel])
		return -EALREADY;

	/* secproxy is enabled by default */
	data->channel_enable[channel] = enable;

	return 0;
}

static const struct mbox_driver_api k3_sec_proxy_driver_api = {
	.send = k3_sec_proxy_send,
	.register_callback = k3_sec_proxy_callback,
	.mtu_get = k3_sec_proxy__mtu_get,
	.max_channels_get = k3_sec_proxy__max_channels_get,
	.set_enabled = k3_sec_proxy__set_enabled,
};

#define MAILBOX_INSTANCE_DEFINE(idx)	\
	static struct k3_sec_proxy_mbox secproxy_mailbox_##idx##_data;	\
	const static struct k3_sec_proxy_config secproxy_mailbox_##idx##_config; \
	static int k3_sec_proxy_mailbox_##idx##_init(const struct device *dev)	\
	{	\
		struct k3_sec_proxy_mbox *data = dev->data;	\
		mem_addr_t addr;	\
		device_map((mm_reg_t *)&addr, DT_INST_REG_ADDR_BY_NAME(idx, target_data),	\
		DT_INST_REG_SIZE_BY_NAME(idx, target_data), K_MEM_CACHE_NONE);	\
		data->mapped_target_data = addr;	\
		device_map((mm_reg_t *)&addr, DT_INST_REG_ADDR_BY_NAME(idx, rt),	\
		DT_INST_REG_SIZE_BY_NAME(idx, rt), K_MEM_CACHE_NONE);	\
		data->mapped_rt = addr;	\
		device_map((mm_reg_t *)&addr, DT_INST_REG_ADDR_BY_NAME(idx, scfg),	\
		DT_INST_REG_SIZE_BY_NAME(idx, scfg), K_MEM_CACHE_NONE);	\
		data->mapped_scfg = addr;	\
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), \
			    secproxy_mailbox_isr,	\
			    DEVICE_DT_INST_GET(idx), 0);	\
		irq_enable(DT_INST_IRQN(idx));	\
		return 0;	\
	}	 \
	DEVICE_DT_INST_DEFINE(idx, secproxy_mailbox_##idx##_init, \
			     NULL, &secproxy_mailbox_##idx##_data,	\
			     &k3_sec_proxy_mailbox_##idx##_config, POST_KERNEL, \
			     CONFIG_MBOX_INIT_PRIORITY,	\
			     &k3_sec_proxy_driver_api)
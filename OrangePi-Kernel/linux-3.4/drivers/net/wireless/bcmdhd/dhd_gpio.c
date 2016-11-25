
#include <osl.h>
#include <dngl_stats.h>
#include <dhd.h>

#ifdef CONFIG_MACH_ODROID_4210
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/devs.h>
#define	sdmmc_channel	s3c_device_hsmmc0
#endif

#ifdef CUSTOMER_HW_ALLWINNER
#include <linux/gpio.h>
#include <mach/sys_config.h>
static int sdc_id = 1;
extern void sunxi_mci_rescan_card(unsigned id, unsigned insert);
extern void wifi_pm_power(int on);
#endif

struct wifi_platform_data dhd_wlan_control = {0};

#ifdef CUSTOMER_OOB
uint bcm_wlan_get_oob_irq(void)
{
	uint host_oob_irq = 0;

#ifdef CONFIG_MACH_ODROID_4210
	printk("GPIO(WL_HOST_WAKE) = EXYNOS4_GPX0(7) = %d\n", EXYNOS4_GPX0(7));
	host_oob_irq = gpio_to_irq(EXYNOS4_GPX0(7));
	gpio_direction_input(EXYNOS4_GPX0(7));
#endif
#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
	int wl_host_wake = 0;

	type = script_get_item("wifi_para", "wl_host_wake", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("get bcmdhd wl_host_wake gpio failed\n");
		return 0;
	} else {
		wl_host_wake = val.gpio.gpio;
	}
	printk("GPIO(WL_HOST_WAKE) = %d\n", wl_host_wake);

	host_oob_irq = gpio_to_irq(wl_host_wake);
	if (IS_ERR_VALUE(host_oob_irq)) {
		printk("map gpio [%d] to virq failed, errno = %d\n",wl_host_wake, host_oob_irq);
		return 0;
	}
#endif

	printk("host_oob_irq: %d \r\n", host_oob_irq);

	return host_oob_irq;
}

uint bcm_wlan_get_oob_irq_flags(void)
{
	uint host_oob_irq_flags = 0;

#ifdef CONFIG_MACH_ODROID_4210
#ifdef HW_OOB
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
#else
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
#endif
#endif

#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
	int host_wake_invert = 0;

	type = script_get_item("wifi_para", "wl_host_wake_invert", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
		printk("has no wl_host_wake_invert\n");
	else
		host_wake_invert = val.val;

	if(!host_wake_invert)
		host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
	else
		host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
#endif
	printk("host_oob_irq_flags=%d\n", host_oob_irq_flags);

	return host_oob_irq_flags;
}
#endif

int bcm_wlan_set_power(bool on)
{
	int err = 0;

	if (on) {
		printk("======== PULL WL_REG_ON HIGH! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = gpio_set_value(EXYNOS4_GPK1(0), 1);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		wifi_pm_power(1);
#endif
		/* Lets customer power to get stable */
		mdelay(100);
	} else {
		printk("======== PULL WL_REG_ON LOW! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = gpio_set_value(EXYNOS4_GPK1(0), 0);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		wifi_pm_power(0);
#endif
	}

	return err;
}

int bcm_wlan_set_carddetect(bool present)
{
	int err = 0;

	if (present) {
		printk("======== Card detection to detect SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = sdhci_s3c_force_presence_change(&sdmmc_channel, 1);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		sunxi_mci_rescan_card(sdc_id, 1);
#endif
	} else {
		printk("======== Card detection to remove SDIO card! ========\n");
#ifdef CONFIG_MACH_ODROID_4210
		err = sdhci_s3c_force_presence_change(&sdmmc_channel, 0);
#endif
#ifdef CUSTOMER_HW_ALLWINNER
		sunxi_mci_rescan_card(sdc_id, 0);
#endif
	}

	return err;
}

#ifdef CUSTOMER_HW_ALLWINNER
extern char *saved_command_line;
#define MAC_KEY_VALUE "wifi_mac"
s32 get_para_from_cmdline(const char *cmdline, const char *name, char *value)
{
	char *value_p = value;

	if (!cmdline || !name || !value) {
		return -1;
	}

	for (; *cmdline != 0;) {
		if (*cmdline++ == ' ') {
			if (0 == strncmp(cmdline, name, strlen(name))) {
				cmdline += strlen(name);
				if (*cmdline++ != '=') {
					continue;
				}
				while (*cmdline != 0 && *cmdline != ' ') {
					*value_p++ = *cmdline++;
				}
				return value_p - value;
			}
		}
	}

	return 0;
}
static u8 key_char2num(u8 ch)
{
	if ((ch>='0') && (ch<='9'))
		return ch - '0';
	else if ((ch>='a') && (ch<='f'))
		return ch - 'a' + 10;
	else if ((ch>='A') && (ch<='F'))
		return ch - 'A' + 10;
	else
		return 0xff;
}
u8 key_2char2num(u8 hch, u8 lch)
{
	return ((key_char2num(hch) << 4) | key_char2num(lch));
}
#endif

int bcm_wlan_get_mac_address(unsigned char *buf)
{
	int err = 0;
	int ret = 0;
	char mac_str[18] = {0};
	u8 mac[ETH_ALEN];
	struct ether_addr mac_addr;
	int jj,kk;
	
	printk("======== %s ========\n", __FUNCTION__);
#ifdef EXAMPLE_GET_MAC
	/* EXAMPLE code */
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif /* EXAMPLE_GET_MAC */

#ifdef CUSTOMER_HW_ALLWINNER
	get_para_from_cmdline(saved_command_line, MAC_KEY_VALUE, mac_str);
	printk("%s wifi_mac=%s\n", __FUNCTION__, mac_str);
/*
	if (mac_str != NULL) {
		//printk(KERN_ERR "mac_str=%s\n",mac_str);
		for ( jj = 0, kk = 0; jj < ETH_ALEN; jj++, kk += 3 ) {
			mac[jj] = key_2char2num(mac_str[kk], mac_str[kk+ 1]);
			printk("%d:", mac[jj]);
		}
		printk("\n");
		if (is_valid_ether_addr(mac)) {
			memcpy(mac_addr.octet, mac, ETHER_ADDR_LEN);
			bcopy((char *)&mac_addr, buf, sizeof(struct ether_addr));
			ret = 0;
		} else 
			ret = -1;
	} else {
		printk("get mac from cmdline failed.\n");
		ret = -1;
	}
*/
	if(mac_str != NULL && is_valid_ether_addr(mac_str)) {
		printk(KERN_ERR "mac_str=%s\n",mac_str);
		for( jj = 0, kk = 0; jj < ETH_ALEN; jj++, kk += 3 ) {
			mac[jj] = key_2char2num(mac_str[kk], mac_str[kk+ 1]);
		}
		memcpy(mac_addr.octet, mac, ETHER_ADDR_LEN);
		bcopy((char *)&mac_addr, buf, sizeof(struct ether_addr));
		ret = 0;
	} else {
		ret = -1;
	}

#endif

	return err;
}

#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *bcmdhd_mem_prealloc(int section, unsigned long size);
void* bcm_wlan_prealloc(int section, unsigned long size)
{
	void *alloc_ptr = NULL;
	alloc_ptr = bcmdhd_mem_prealloc(section, size);
	if (alloc_ptr) {
		printk("success alloc section %d, size %ld\n", section, size);
		if (size != 0L)
			bzero(alloc_ptr, size);
		return alloc_ptr;
	}
	printk("can't alloc section %d\n", section);
	return NULL;
}
#endif

int bcm_wlan_set_plat_data(void) {
#ifdef CUSTOMER_HW_ALLWINNER
	script_item_value_type_e type;
	script_item_u val;
#endif
	printk("======== %s ========\n", __FUNCTION__);
#ifdef CUSTOMER_HW_ALLWINNER
	type = script_get_item("wifi_para", "wifi_sdc_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		printk(("failed to fetch sdio card's sdcid\n"));
		return -1;
	}
	sdc_id = val.val;
#endif
	dhd_wlan_control.set_power = bcm_wlan_set_power;
	dhd_wlan_control.set_carddetect = bcm_wlan_set_carddetect;
//	dhd_wlan_control.get_mac_addr = bcm_wlan_get_mac_address;
#ifdef CONFIG_DHD_USE_STATIC_BUF
	dhd_wlan_control.mem_prealloc = bcm_wlan_prealloc;
#endif
	return 0;
}


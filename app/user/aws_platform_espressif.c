#define _PLATFORM_ESPRESSIF_
#ifdef _PLATFORM_ESPRESSIF_

#include <stdarg.h>
#include "esp_common.h"

#include "aws_platform.h"
#include "aws_lib.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include <espressif/esp_libc.h>	//for aws_printf
#include <espressif/esp_wifi.h> //for wifi_set_channel
#include "alink_export.h"
#ifndef ETH_ALEN
#define ETH_ALEN	(6)
#endif

#if	1
#define aws_printf			os_printf
#else
#define aws_printf
#endif

//一键配置超时时间, 建议超时时间1-3min, APP侧一键配置1min超时
int aws_timeout_period_ms = 60 * 1000;

//一键配置每个信道停留时间, 建议200ms-400ms
int aws_chn_scanning_period_ms = 200;

//系统自boot起来后的时间, 用于判断收包之间的间隔时间
unsigned int vendor_get_time_ms(void)
{
	return (unsigned int)system_get_time() / 1000;
}

void *vendor_malloc(int size)
{
	return malloc(size);
}

void vendor_free(void *ptr)
{
	if (ptr) {
		free(ptr);
		ptr = NULL;
	}
}

//aws库会调用该函数用于信道切换之间的sleep
void vendor_msleep(int ms)
{
	sys_msleep(ms);
}

//系统打印函数, 可不实现
void vendor_printf(int log_level, const char* log_tag, const char* file,
		const char* fun, int line, const char* fmt, ...)
{
}

struct RxControl {
	signed rssi:8;
	unsigned rate:4;
	unsigned is_group:1;
	unsigned:1;
	unsigned sig_mode:2;
	unsigned legacy_length:12;
	unsigned damatch0:1;
	unsigned damatch1:1;
	unsigned bssidmatch0:1;
	unsigned bssidmatch1:1;
	unsigned MCS:7;
	unsigned CWB:1;
	unsigned HT_length:16;
	unsigned Smoothing:1;
	unsigned Not_Sounding:1;
	unsigned:1;
	unsigned Aggregation:1;
	unsigned STBC:2;
	unsigned FEC_CODING:1;
	unsigned SGI:1;
	unsigned rxend_state:8;
	unsigned ampdu_cnt:8;
	unsigned channel:4;
	unsigned:12;
};

struct Ampdu_Info
{
	uint16 length;
	uint16 seq;
	uint8  address3[6];
};

struct sniffer_buf {
	struct RxControl rx_ctrl;		//12Byte
	uint8_t  buf[36];
	uint16_t cnt;
	struct Ampdu_Info ampdu_info[1];
};

struct sniffer_buf2{
	struct RxControl rx_ctrl;
	//uint8 buf[112];
	uint8 buf[496];
	uint16 cnt;
	uint16 len; //length of packet
};

struct ieee80211_hdr {
	u16 frame_control;
	u16 duration_id;
	u8 addr1[ETH_ALEN];
	u8 addr2[ETH_ALEN];
	u8 addr3[ETH_ALEN];
	u16 seq_ctrl;
	u8 addr4[ETH_ALEN];
};

void vendor_data_callback(unsigned char *buf, int length)
{
	aws_80211_frame_handler(buf, length, AWS_LINK_TYPE_NONE, 0);
}

void ICACHE_FLASH_ATTR sniffer_wifi_promiscuous_rx(uint8 *buf, uint16 buf_len)
{
	u8 *data;
	u32 data_len;

	if (buf_len == sizeof(struct sniffer_buf2)) {/* managment frame */
		struct sniffer_buf2 *sniffer = (struct sniffer_buf2 *)buf;
		data_len = sniffer->len;
		if (data_len > sizeof(sniffer->buf))
			data_len = sizeof(sniffer->buf);
		data = sniffer->buf;
		vendor_data_callback(data, data_len);
	} else if (buf_len == sizeof(struct RxControl)) {/* mimo, HT40, LDPC */
		struct RxControl *rx_ctrl= (struct RxControl *)buf;
		//aws_printf("%s mcs:%d %s\n", rx_ctrl->CWB ? "HT40" : ""/* HT20 */,
		//		rx_ctrl->MCS,
		//		rx_ctrl->FEC_CODING ? "LDPC" : "");
	} else {//if (buf_len % 10 == 0) {
		struct sniffer_buf *sniffer = (struct sniffer_buf *)buf;
		data = buf + sizeof(struct RxControl);

		struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)data;

		if (sniffer->cnt == 1) {
			data_len = sniffer->ampdu_info[0].length - 4;
			vendor_data_callback(data, data_len);
		} else {
			int i;
			//aws_printf("rx ampdu %d\n", sniffer->cnt);
			for (i = 1; i < sniffer->cnt; i++) {
				hdr->seq_ctrl = sniffer->ampdu_info[i].seq;
				memcpy(&hdr->addr3, sniffer->ampdu_info[i].address3, 6);

				data_len = sniffer->ampdu_info[i].length - 4;
				vendor_data_callback(data, data_len);
			}
		}
	}
}

//aws库调用该函数来接收80211无线包
//若平台上已注册回调aws_80211_frame_handler()来收包时， 将该函数填为vendor_msleep(100)
int vendor_recv_80211_frame(void)
{
	vendor_msleep(100);
	return 0;
}

//进入monitor模式, 并做好一些准备工作，如
//设置wifi工作在默认信道6
//若是linux平台，初始化socket句柄，绑定网卡，准备收包
//若是rtos的平台，注册收包回调函数aws_80211_frame_handler()到系统接口
void vendor_monitor_open(void)
{
	//wifi_station_disconnect();
	wifi_set_channel(6);
	wifi_promiscuous_enable(0);
	wifi_set_promiscuous_rx_cb(sniffer_wifi_promiscuous_rx);
	wifi_promiscuous_enable(1);
}

//退出monitor模式，回到station模式, 其他资源回收
void vendor_monitor_close(void)
{
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(NULL);
}

//wifi信道切换，信道1-13
void vendor_channel_switch(char primary_channel,
		char secondary_channel, char bssid[6])
{
	wifi_set_channel(primary_channel);
}

int vendor_broadcast_notification(char *msg, int msg_num)
{
	struct sockaddr_in addr;
	int i, ret, fd;

	int buf_len = 1024;
	char *buf = vendor_malloc(buf_len);

	do {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == -1) {
			aws_printf("ERROR: failed to create sock!\n");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
	} while (fd == -1);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(UDP_RX_PORT);
	addr.sin_len = sizeof(addr);
	ret = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (ret)
		aws_printf("aws bind local port ERROR!\r\n");

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	addr.sin_port = htons(UDP_TX_PORT);
	addr.sin_len = sizeof(addr);

	//send notification
	for (i = 0; i < msg_num; i++) {
		ret = sendto(fd, msg, strlen(msg), 0,
			(const struct sockaddr *)&addr, sizeof(addr));
		if (ret < 0) {
			aws_printf("aws send notify msg ERROR!\r\n");
			vendor_msleep(1000);
		}

		struct timeval tv;
		fd_set rfds;

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		tv.tv_sec = 0;
		tv.tv_usec = 1000 * (200 + i * 100); //from 200ms to N * 100ms, N = 25

		ret = select(fd + 1, &rfds, NULL, NULL, &tv);
		if (ret > 0) {
			ret = recv(fd, buf, buf_len, 0);
			if (ret) {
				buf[ret] = '\0';
				aws_printf("rx: %s\n", buf);
				break;
			}
		}

		//vendor_msleep(200 + i * 100);
	}

	vendor_free(buf);
	close(fd);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//sample code

//TODO: update these product info
#define product_model		"ALINKTEST_LIVING_LIGHT_SMARTLED"
#define product_secret		"YJJZjytOCXDhtQqip4EjWbhR95zTgI92RVjzjyZF"
char dev_mac[STR_MAC_LEN]; 
char *vendor_get_model(void) { return product_model; }
char *vendor_get_secret(void) { return product_secret; }
char *vendor_get_mac(void) { return dev_mac; }
char *vendor_get_sn(void) { return NULL; }
int vendor_alink_version(void) { return 10; }

/* connect... to ap and got ip */
int vendor_connect_ap(char *ssid, char *passwd)
{
	struct station_config * config = (struct station_config *)zalloc(
			sizeof(struct station_config));
	strncpy(config->ssid, ssid, 32);
	strncpy(config->password, passwd, 64);
	wifi_station_set_config(config);
	free(config);
	wifi_station_connect();

	aws_printf("waiting network ready...\r\n");
	while (1) {
		int ret = wifi_station_get_connect_status();
		if (ret == STATION_GOT_IP)
			break;
		vTaskDelay(100 / portTICK_RATE_MS);
	}

	return 0;
}
extern int need_notify_app;

int aws_sample(void)
{
	char ssid[32 + 1] = {0};
	char passwd[64 + 1] = {0};
	char bssid[6] ={0};
	
	char auth;
	char encry;
	char channel;
	char macaddr[6] = {0};
	int ret;
	if(!wifi_get_macaddr(0,macaddr)){
		os_printf("[dbg][%s##%u]\n",__FUNCTION__,__LINE__);
	};
    snprintf(dev_mac, sizeof(dev_mac), "%02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(macaddr));
	aws_start(product_model, product_secret, dev_mac, NULL);
	
	os_printf("dev mac:%s\n",dev_mac);
	ret = aws_get_ssid_passwd(&ssid[0], &passwd[0], &bssid[0], &auth, &encry, &channel);
	if (!ret) {
		aws_printf("alink wireless setup timeout!\n");
		ret = -1;
		goto out;
	}

	aws_printf("ssid:%s, passwd:%s\n", ssid, passwd);

	vendor_connect_ap(ssid, passwd);

//	aws_notify_app();
	need_notify_app = 1;


	ret = 0;
out:
	aws_destroy();

	return ret;
}

#endif

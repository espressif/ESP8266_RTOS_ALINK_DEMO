/*
	softap tcp交互部分：目的为APP和设备通过tcp交换ssid和passwd信息
	1）softap流程下 wifi设备开启tcp server，等待APP进行连接
	2）APP连接上设备的tcp server后，发送ssid&passwd&bssid信息给wifi设备，wifi设备进行解析
	3）wifi设备连接通过active scan，获取无线网络加密方式，进行联网
	4）wifi设备联网成功后，发送配网成功的通知给APP。

	//修改记录
	0421：在广播信息里添加sn、mac字段，增加对私有云对接的支持
		绑定过程，设备端等待tcp连接超时时间为2min
	0727： 添加功能说明
	0813： 添加softap dns配置说明
*/
#include "esp_common.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/api.h"
#include "aws_lib.h"
#include "aws_platform.h"


#define SOFTAP_GATEWAY_IP		"172.31.254.250"
#define SOFTAP_TCP_SERVER_PORT		(65125)

#ifndef	info
#define info(format, ...)	printf(format, ##__VA_ARGS__)
#endif
#if 0
#define assert(val) do{\
	if(!(val)){os_printf("[ERROR][%s##%u]\n",__FUNCTION__,__LINE__);system_restart();}\
}while(0)
#else
#define assert(val) do{\
	if(!(val)){os_printf("[ERROR][%s##%u]\n",__FUNCTION__,__LINE__);}\
}while(0)
#endif
/*
	以下为softap配网时，设备起的softap tcp server sample
*/
///////////////softap tcp server sample/////////////////////
#define	STR_SSID_LEN		(32 + 1)
#define STR_PASSWD_LEN		(64 + 1)
char aws_ssid[STR_SSID_LEN];
char aws_passwd[STR_PASSWD_LEN];
unsigned char aws_bssid[6];

/* json info parser */
int get_ssid_and_passwd(char *msg)
{
	char *ptr, *end, *name;
	int len;

	//ssid
	name = "\"ssid\":";
	ptr = strstr(msg, name);
	if (!ptr) {
		info("%s not found!\n", name);
		goto exit;
	}
	ptr += strlen(name);
	while (*ptr++ == ' ');/* eating the beginning " */
	end = strchr(ptr, '"');
	len = end - ptr;

	assert(len < sizeof(aws_ssid));
	strncpy(aws_ssid, ptr, len);
	aws_ssid[len] = '\0';

	//passwd
	name = "\"passwd\":";
	ptr = strstr(msg, name);
	if (!ptr) {
		info("%s not found!\n", name);
		goto exit;
	}

	ptr += strlen(name);
	while (*ptr++ == ' ');/* eating the beginning " */
	end = strchr(ptr, '"');
	len = end - ptr;

	assert(len < sizeof(aws_passwd));
	strncpy(aws_passwd, ptr, len);
	aws_passwd[len] = '\0';

	//bssid-mac
	name = "\"bssid\":";
	ptr = strstr(msg, name);
	if (!ptr) {
		info("%s not found!\n", name);
		goto exit;
	}

	ptr += strlen(name);
	while (*ptr++ == ' ');/* eating the beginning " */
	end = strchr(ptr, '"');
	len = end - ptr;

#if 0
	memset(aws_bssid, 0, sizeof(aws_bssid));

	sscanf(ptr, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
			&aws_bssid[0], &aws_bssid[1], &aws_bssid[2],
			&aws_bssid[3], &aws_bssid[4], &aws_bssid[5]);
#endif

	return 0;
exit:
	return -1;
}

//setup softap server
int aws_softap_tcp_server(void)
{
	struct sockaddr_in server, client;
	socklen_t socklen = sizeof(client);
	int fd = -1, connfd, len, ret;
	char *buf, *msg;
	int opt = 1, buf_size = 512, msg_size = 512;

	info("setup softap & tcp-server\n");

	buf = (char*)malloc(buf_size);
	msg = (char*)malloc(msg_size);
	assert(fd && msg);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(fd >= 0);

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(SOFTAP_GATEWAY_IP);
	server.sin_port = htons(SOFTAP_TCP_SERVER_PORT);

	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	assert(!ret);

	ret = bind(fd, (struct sockaddr *)&server, sizeof(server));
	assert(!ret);

	ret = listen(fd, 10);
	assert(!ret);

	info("server %x %d created\n", ntohl(server.sin_addr.s_addr),
			ntohs(server.sin_port));

	connfd = accept(fd, (struct sockaddr *)&client, &socklen);
	assert(connfd > 0);
	info("client %x %d connected!\n", ntohl(client.sin_addr.s_addr),
			ntohs(client.sin_port));


	len = recvfrom(connfd, buf, buf_size, 0,
			(struct sockaddr *)&client, &socklen);
	assert(len >= 0);

	buf[len] = 0;
	info("softap tcp server recv: %s\n", buf);

	ret = get_ssid_and_passwd(buf);
	if (!ret) {
		snprintf(msg, buf_size,
			"{\"code\":1000, \"msg\":\"format ok\", \"model\":\"%s\", \"mac\":\"%s\"}",
			vendor_get_model(), vendor_get_mac());
	} else
		snprintf(msg, buf_size,
			"{\"code\":2000, \"msg\":\"format error\", \"model\":\"%s\", \"mac\":\"%s\"}",
			vendor_get_model(), vendor_get_mac());

	len = sendto(connfd, msg, strlen(msg), 0,
			(struct sockaddr *)&client, socklen);
	assert(len >= 0);
	info("ack %s\n", msg);

	close(connfd);
	close(fd);

	free(buf);
	free(msg);

	return 0;
}

void aws_softap_setup(void)
{
	/*
	 * wilress params: 11BGN
	 * channel: auto, or 1, 6, 11
	 * authentication: OPEN
	 * encryption: NONE
	 * gatewayip: 172.31.254.250, netmask: 255.255.255.0
	 * DNS server: 172.31.254.250. 	IMPORTANT!!!  ios depend on it!
	 * DHCP: enable
	 * SSID: 32 ascii char at most
	 * softap timeout: 5min
	 */
/*
Step1: Softap config
*/
	char ssid[STR_SSID_LEN]={0};
	//ssid: max 32Bytes(excluding '\0')
	snprintf(ssid, STR_SSID_LEN, "alink_%s", vendor_get_model());
	struct softap_config ap_config;
	bzero(&ap_config,sizeof(ap_config));
	wifi_set_opmode(SOFTAP_MODE);
    memcpy(ap_config.ssid,ssid,strlen(ssid));
    ap_config.ssid_len=strlen(ap_config.ssid);
    ap_config.authmode=AUTH_OPEN;
    ap_config.channel=6;
    ap_config.max_connection=4;
    wifi_softap_set_config(&ap_config);
 /*
Step2: Gateway ip config
 */
    struct ip_info ip_info;
    ip_info.ip.addr=ipaddr_addr("172.31.254.250");
    ip_info.gw.addr=ipaddr_addr("172.31.254.250");
    ip_info.netmask.addr=ipaddr_addr("255.255.255.0");
    wifi_softap_dhcps_stop();
	wifi_set_ip_info(SOFTAP_IF,&ip_info);
	wifi_softap_dhcps_start();
/*
Step3 Check Step1 and Step2
*/
	wifi_softap_get_config(&ap_config);
	wifi_get_ip_info(SOFTAP_IF,&ip_info);
	os_printf("------ Softap Param -----------\n");
	os_printf("ssid:%s\n",ap_config.ssid);
	os_printf("beacon_interval:%u\n",ap_config.beacon_interval);
	os_printf("ip:%s\n",inet_ntoa((ip_info.ip)));
	os_printf("gw:%s\n",inet_ntoa((ip_info.gw)));
	os_printf("netmask:%s\n",inet_ntoa((ip_info.netmask)));


#if 0
	wifi_config_set_opmode(WIFI_MODE_AP);
	wifi_config_set_security_mode(WIFI_AUTH_MODE_OPEN, WIFI_ENCRYPT_TYPE_NONE);
	wifi_config_set_channel(6);
	wifi_config_set_ssid(ssid, strlen(ssid));
	wifi_config_reload_setting();

	{
		char ip_buf[] = "172.31.254.250";
		char mask_buf[] = "255.255.255.0";
		struct ip4_addr addr;

		netif_set_status_callback(&sta_if, NULL);

		nvdm_write_data_item("STA", "IpAddr", NVDM_DATA_ITEM_TYPE_STRING, ip_buf, sizeof(ip_buf));
		nvdm_write_data_item("STA", "IpNetmask", NVDM_DATA_ITEM_TYPE_STRING, mask_buf, sizeof(mask_buf));
		nvdm_write_data_item("STA", "IpGateway", NVDM_DATA_ITEM_TYPE_STRING, ip_buf, sizeof(ip_buf));

		inet_aton(mask_buf, &addr);
		netif_set_netmask(&sta_if, &addr);
		inet_aton(ip_buf, &addr);
		netif_set_ipaddr(&sta_if, &addr);
		netif_set_gw(&sta_if, &addr);

		dhcp_stop(&sta_if);
		netif_set_link_up(&sta_if);
		netif_set_default(&sta_if);
		dhcpd_start(0);
		printf("start dhcpd, stop dhcp. g_supplicant_ready:%d\n", g_supplicant_ready);
	}
#endif
}

void aws_softap_exit(void)
{
	wifi_set_opmode(STATION_MODE);
#if 0
	dhcpd_stop();
	wifi_config_set_opmode(MODE_STA_ONLY);

	dhcp_start(&sta_if);
	netif_set_link_up(&sta_if);
#endif
}
void aws_connect_to_ap(char* ssid,char* password)
{
	os_printf("aws_connect_to ap ssid:%s,password:%s\n",ssid,password);
    vendor_connect_ap(ssid,password);
}
extern int need_notify_app;

int aws_softap_main(void)
{
	
	ESP_DBG(("********ENTER SOFTAP MODE******"));
	/* prepare and setup softap */
	aws_softap_setup();

	/* tcp server to get ssid & passwd */
	aws_softap_tcp_server();

	aws_softap_exit();

	aws_connect_to_ap(aws_ssid, aws_passwd);

	/* after dhcp ready, send notification to APP */
//	aws_notify_app();
	need_notify_app = 1;

	return 0;
}

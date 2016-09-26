/**********************************************************************
 Copyright(C),2016-2018,www.streamax.com
 File name: p4ledfun.c
 Author: ZhaoKun
 Version: V1.0
 Date: 2016-9-22
 Desrciption: This file achieve file p4led.h declaraction function.
 Others: Given MCU 485 serial port config command, Ju Rong LED and MCU
		 watch dog open or close command.
 Function List: 
	char check_xor(const char device_data[], int len);
	int check_sum(char device_data[], int len);
	void hm_sp_cnf_req_fra(char hm_sp_cnf_req[], int len);
	int hm_led_txt_fra(char hm_led_txt[], int len, int zn_en_flag);
	void hm_led_close_fra(char hm_led_close[], int len);
	void hm_led_open_fra(char hm_led_open[], int len);
	void hm_wdog_close_req_fra(char hm_wdog_close_req[], int len);
	void hm_wdog_open_req_fra(char hm_wdog_open_req[], int len);
	int code_convert(char *from_charset, char *to_charset, char inbuf[], 
												int inlen, char outbuf[], int outlen);
	int u2g(char inbuf[], int inlen, char outbuf[], int outlen);
	void code_h2l(unsigned char inbuf[], int inlen);
	void led_serial_init(void);
	void p4_led(char in_utf8[], int str_len, int zn_en_flag);
***********************************************************************/

#include "p4led.h"

static int ttyACM0_fd = -1;

static int power_on_flag = 0;

unsigned char sp_cnf_req[8] = {0x20, 0x06, 0x04, 0x06, 0x00, 0x08, 0x02, 0x00};

unsigned char led_open[52] = {0xA0, 0x34, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x19, 0x18, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x50};
unsigned char led_close[52] = {0xA0, 0x34, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x19, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x01, 0x00, 0x50};

unsigned char led_pkt[1076] = {0}; // 16+32+1024+4
unsigned char led__pkt_head[16] = {0xa0, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00};
static char led__pkt_cmd[32] = {0x01, 0x00, 0x01, 0x00, 0xff, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03};
unsigned char led_pkt_ext[1024] = {0}; 
unsigned char led_pkt_end[4] = {0x00, 0x00, 0x00, 0x50};

unsigned char wdog_close_req[3] = {0x07, 0x01, 0x30};
unsigned char wdog_open_req[3] = {0x07, 0x01, 0x31};

unsigned char hm_sp_cnf_req[13] = {0};
unsigned char hm_sp_cnf_res[8] = {0};

unsigned char hm_led_txt[1081] = {0}; // 1076+5 
unsigned char hm_led_close[57] = {0};
unsigned char hm_led_open[57] = {0};

unsigned char hm_wdog_close_req[8] = {0};
unsigned char hm_wdog_close_res[8] = {0};
unsigned char hm_wdog_open_req[8] = {0};
unsigned char hm_wdog_open_res[8] = {0};

char check_xor(const char device_data[], int len)
{
	char ret = 0;
	int i = 0;
	
	for (; i < len; i++)
	{
		ret ^= device_data[i];
	}
	return ret;
}

int check_sum(const char device_data[], int len)
{
	int i = 0;
	int sum = 0;

	for (; i < len; i++) 
	{
		sum += (unsigned char)device_data[i];
	}

	return sum;
}

int hm_led_txt_fra(char *led_text, int text_len, int zn_en_flag)
{
	int i = 0;
	int ret = 0;
	char tmp_l = 0;
	char tmp_h = 0;
	int text_check = 0;
	int total_len = 0;
	int tmp_len = 0;
	int crt_len = 0;

	if (zn_en_flag == 0) //utf8-en 
	{
		crt_len = text_len;
	}
	if (zn_en_flag == 1) //utf8-zn 
	{
		crt_len = text_len*2/3;
	}
	char crt_text[crt_len];

	u2g(led_text, text_len, crt_text, crt_len);

	if (zn_en_flag == 1)
	{
		code_h2l(crt_text, crt_len);
	}
	
	#ifdef DEBUG
		for (i = 0; i < crt_len; i++)
		{
			printf("%02x ", (unsigned char)crt_text[i]);
		}
		printf("\n");
	#endif

	for (i = 0; i < crt_len; i++)
	{
		led_pkt_ext[i] = crt_text[i];
	}
	
	total_len = 16 + 32 + crt_len + 4;

	tmp_l = total_len & 0x000000ff;
	tmp_h = (total_len >> 8) & 0x000000ff;	
	led__pkt_head[1] = tmp_l;
	led__pkt_head[2] = tmp_h;
	
	tmp_l = crt_len & 0x000000ff;	
	tmp_h = (crt_len >> 8) & 0x000000ff;	
	led__pkt_cmd[28] = tmp_l;
	led__pkt_cmd[29] = tmp_h;

	//if (power_on_flag != 1)
	//{
	//	led__pkt_cmd[31] = 0x03;
	//}

	for (i = 0; i < 16; i++)
	{
		led_pkt[i] = led__pkt_head[i];
	}
	for (i = 0; i < 32; i++)
	{
		led_pkt[i+16] = led__pkt_cmd[i];
	}
	for (i = 0; i < crt_len; i++)
	{
		led_pkt[i+48] = crt_text[i];
	}

	tmp_len = 48 + crt_len;
	text_check = check_sum(led_pkt, tmp_len);
	
	tmp_l = text_check & 0x000000ff;	
	tmp_h = (text_check >> 8) & 0x000000ff;	
	led_pkt_end[0] = tmp_l;
	led_pkt_end[1] = tmp_h;


	for (i = 0; i < 4; i++)
	{
		led_pkt[i+tmp_len] = led_pkt_end[i];
	}
	
	hm_led_txt[0] = 0x7E;
	hm_led_txt[1] = 0xA0;
	hm_led_txt[2] = 0xFF;
	hm_led_txt[3] = check_xor(led_pkt, total_len);
	for (i = 0; i < total_len; i++)
	{
		hm_led_txt[i+4] = led_pkt[i];
	}
	hm_led_txt[total_len + 4] = 0x7F;
	
	ret = total_len + 5;

	//power_on_flag = 0;

	return ret;
}

void hm_sp_cnf_req_fra(char hm_sp_cnf_req[], int len)
{
	int i = 0;
	hm_sp_cnf_req[0] = 0x7E;
	hm_sp_cnf_req[1] = 0x80;
	hm_sp_cnf_req[2] = 0x01;
	hm_sp_cnf_req[3] = check_xor(sp_cnf_req, sizeof(sp_cnf_req));
	
	for (; i < sizeof(sp_cnf_req); i++)
	{
		hm_sp_cnf_req[i+4] = sp_cnf_req[i];
	}
	hm_sp_cnf_req[len-1] = 0x7F;

	return;
}

void hm_led_close_fra(char hm_led_close[], int len)
{
	int i = 0;
	hm_led_close[0] = 0x7E;
	hm_led_close[1] = 0xA0;
	hm_led_close[2] = 0xFF;
	hm_led_close[3] = check_xor(led_close, sizeof(led_close));

	for (; i < sizeof(led_close); i++)
	{
		hm_led_close[i+4] = led_close[i];
	}
	hm_led_close[len-1] = 0x7F;

	return;
}

void hm_led_open_fra(char hm_led_open[], int len)
{
	int i = 0;
	hm_led_open[0] = 0x7E;
	hm_led_open[1] = 0xA0;
	hm_led_open[2] = 0xFF;
	hm_led_open[3] = check_xor(led_open, sizeof(led_open));

	for (; i < sizeof(led_open); i++)
	{
		hm_led_open[i+4] = led_open[i];
	}
	hm_led_open[len-1] = 0x7F;	

	return;
}

void hm_wdog_close_req_fra(char hm_wdog_close_req[], int len)
{
	int i = 0;
	hm_wdog_close_req[0] = 0x7E;
	hm_wdog_close_req[1] = 0x80;
	hm_wdog_close_req[2] = 0x01;
	hm_wdog_close_req[3] = check_xor(wdog_close_req, sizeof(wdog_close_req));
	
	for (; i < sizeof(wdog_close_req); i++)
	{
		hm_wdog_close_req[i+4] = wdog_close_req[i];
	}
	hm_wdog_close_req[len-1] = 0x7F;	

	return;
}

void hm_wdog_open_req_fra(char hm_wdog_open_req[], int len)
{
	int i = 0;
	hm_wdog_open_req[0] = 0x7E;
	hm_wdog_open_req[1] = 0x80;
	hm_wdog_open_req[2] = 0x01;
	hm_wdog_open_req[3] = check_xor(wdog_open_req, sizeof(wdog_open_req));
	
	for (; i < sizeof(wdog_open_req); i++)
	{
		hm_wdog_open_req[i+4] = wdog_open_req[i];
	}
	hm_wdog_open_req[len-1] = 0x7F;		

	return;
}

int code_convert(char *from_charset, char *to_charset, char inbuf[], int inlen, char outbuf[], int outlen)
{
	int ret = -1;
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);

	memset(outbuf, 0, outlen);

	ret = iconv(cd, pin, &inlen, pout, &outlen);
	if (ret == -1)
	{
		fprintf(stderr, "iconv fail.\n");
		return -1;
	}

	iconv_close(cd);

	return 0;
}

int u2g(char inbuf[], int inlen, char outbuf[], int outlen)
{
	return code_convert("utf-8","gb2312", inbuf, inlen, outbuf, outlen);
}

void code_h2l(char inbuf[], int inlen)
{
	int i = 0;
	char tmp = 0;

	for (; i < inlen; i+=2)
	{
		tmp = inbuf[i];
		inbuf[i] = inbuf[i+1];
		inbuf[i+1] = tmp;
	}

	return;
}

void led_serial_init(void)
{
	int ret = -1;
	char init_utf8[] = "Welcome to GH4";
	power_on_flag = 1;

	ttyACM0_fd = open("/dev/ttyACM0", O_RDWR);	
	if (ttyACM0_fd == -1)
	{
		perror("open /dev/ttyACM0 fail");
		exit(EXIT_FAILURE);
	}
	
	hm_wdog_close_req_fra(hm_wdog_close_req, sizeof(hm_wdog_close_req));
	ret = write(ttyACM0_fd, hm_wdog_close_req, sizeof(hm_wdog_close_req));
	if (ret == -1)
	{
		fprintf(stderr, "hm_wdog_close_req write /dev/ttyACM0 fail");
		return;	
	}

	hm_sp_cnf_req_fra(hm_sp_cnf_req, sizeof(hm_sp_cnf_req));
	ret = write(ttyACM0_fd, hm_sp_cnf_req, sizeof(hm_sp_cnf_req));
	if (ret == -1)
	{
		fprintf(stderr, "hm_sp_cnf_req write /dev/ttyACM0 fail");
		return;	
	}

	return;
}

void p4_led(char in_utf8[], int str_len, int zn_en_flag)
{
	int ret = -1;
	int cnt = 0;

	hm_led_close_fra(hm_led_close, sizeof(hm_led_close));
	ret = write(ttyACM0_fd, hm_led_close, sizeof(hm_led_close));
	if (ret == -1)
	{
		fprintf(stderr, "hm_led_close_fra write /dev/ttyACM0 fail");
		return;	
	}
	usleep(20000);

	cnt = hm_led_txt_fra(in_utf8, str_len, zn_en_flag);
	#ifdef DEBUG
		printf("cnt = %d\n", cnt);
		int i = 0;
		for (; i < cnt; i++)
		{
			printf("%02x ", (unsigned char)hm_led_txt[i]);
		}
		printf("\n");
	#endif

	ret = write(ttyACM0_fd, hm_led_txt, cnt);
	if (ret == -1)
	{
		fprintf(stderr, "hm_led_txt write /dev/ttyACM0 fail");
		return;	
	}

	usleep(80000);
	hm_led_open_fra(hm_led_open, sizeof(hm_led_open));
	ret = write(ttyACM0_fd, hm_led_open, sizeof(hm_led_open));
	if (ret == -1)
	{
		fprintf(stderr, "hm_led_open_fra write /dev/ttyACM0 fail");
		return;	
	}

	return;
}



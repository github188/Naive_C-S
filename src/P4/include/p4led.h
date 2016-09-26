/**********************************************************************
 Copyright(C),2016-2018,www.streamax.com
 File name: p4led.h
 Author: ZhaoKun
 Version: V1.0
 Date: 2016-9-22
 Desrciption: This file is used to declare some function of operate Ju 
			  Rong LED, and these functions are called by device module.
 Others: Given MCU 485 serial port config command and communication protocol 
         from Host to MCU.
***********************************************************************/

/*********************************************************************** 
MCU 485 port-4 config 
	cmd   len  UartId  BaudRate  Parity  WordLen  StopBits  FlowCtrl
	0x20  0x06  0x04     0x06     0x00    0x08      0x02      0x00 

Host-MCU
	Header	Destination Path	Devide ID	Check	Device Data		End
 	0x7e    10100000			 0xff		 0x00		xxx			0x7f
************************************************************************/

#ifndef _P4LED_H
#define _P4LED_H 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "iconv.h"

//#define DEBUG 1

#define ZN_FLAG 1 /* LED shows Chinese. */
#define EN_FLAG 0 /* LED shows English. */

/* The function check_xor is generate XOR check for every byte of the device_data array.
return the result of XOR check. */
extern char check_xor(const char device_data[], int len);

/* The function check_sum is generate SUM check for every byte of the device_data array.
return the result of SUM check. */
extern int check_sum(const char device_data[], int len);

/* The function hm_sp_cnf_req_fra is generate 485 serial port config command of MCU. */
extern void hm_sp_cnf_req_fra(char hm_sp_cnf_req[], int len);

/* The function hm_led_txt_fra is generate Ju Rong LED show text, return text length. */
extern int hm_led_txt_fra(char hm_led_txt[], int len, int zn_en_flag);

/* The function hm_led_close_fra is generate Ju Rong LED clear command. */
extern void hm_led_close_fra(char hm_led_close[], int len);

/* The function hm_led_open_fra is generate Ju Rong LED open command. */
extern void hm_led_open_fra(char hm_led_open[], int len);

/* The function hm_wdog_close_req_fra is generate MCU watch dog close command. */
extern void hm_wdog_close_req_fra(char hm_wdog_close_req[], int len);

/* The function hm_wdog_open_req_fra is generate MCU watch dog open command. */
extern void hm_wdog_open_req_fra(char hm_wdog_open_req[], int len);

/* The function code_convert is convert character code. */
extern int code_convert(char *from_charset, char *to_charset, char inbuf[]/*in*/, int inlen, char outbuf[]/*in-out*/, int outlen);

/* The function u2g is convert character code from utf-8 to gb2312, if input utf-8 chinese,
punctuation need input full width*/
extern int u2g(char inbuf[], int inlen, char outbuf[]/*in-out*/, int outlen);

/* The function code_h2l is change gb2312 code byte queue, use Little-endian. */
extern void code_h2l(char inbuf[]/*in-out*/, int inlen);

/* The function led_serial_init complete a serial initialization work, include open file /dev/ttyACM0,
close MUC watch dog, and config MUC 485 serial port that is connect LED.  */
extern void led_serial_init(void);

/* The function p4_led call hm_led_close_fra,hm_led_txt_fra and hm_led_open_fra, it is the interface function of device module. */
extern void p4_led(char in_utf8[], int str_len, int zn_en_flag);

#endif /* p4led.h */





/*=============================================================================+
|                                                                              |
| Copyright 2015                                                               |
| Montage Inc. All right reserved.                                             |
|                                                                              |
+=============================================================================*/
/*!
*   \file at_command_demo.c
*   \brief main entry
*   \author Montage
*/

/*=============================================================================+
| Included Files                                                               |
+=============================================================================*/
#include <stdint.h>
#include <common.h>
#include <flash_api.h>
#include <lynx_debug.h>
#include <gpio.h>
#include <serial.h>
#include <event.h>
#include <os_api.h>
#include <net_api.h>
#include <wla_api.h>
#include <gpio.h>
#include <cfg_api_new.h>
#include <user_config.h>

#if defined(CONFIG_FREERTOS)
#include <FreeRTOS.h>
#include <task.h>
#endif
#if defined(CONFIG_LWIP)
#include <net_api.h>
#endif
#if defined(CONFIG_MINIFS)
#include <minifs/mfs.h>
#endif

#define UART0   0//Lynx UART1

sdk_param g_atParam;

/*AT查询，此函数将在串口输入AT+TEST执行
下例中，执行结果为+ok=Mylinks */
void at_exeCmdCtest(uint8_t id){
	at_backOkHead;
	uart0_sendStr("Mylinks");
	at_backTail;
	return;
}


/*AT设置功能，此函数将在串口输入AT+TEST=Mylinks执行
  当输入AT+TEST=mylinks,*pPara指向"Mylinks",即pPara="Mylinks"
  用户可以自行进行处理,以下例子实现了:
  当AT指令输入AT+Mylinks时，串口回复:+ok,否者输出:+ERR=-1 */
void at_setupCmdtest(uint8_t id, char *pPara){
	if(!strcmp(pPara,"Mylinks")){
		at_backOk;
	}else{
		at_backError;
		uart0_sendStr("-1");
	}
	return;
}

at_funType at_test[]={
	{"TEST", 4, at_setupCmdtest, at_exeCmdCtest},
	//自定义的AT指令，必须以以下方式结尾
	{NULL, 0, NULL, NULL},
};

/*----------------------------------------------------------------*/
/**
 * The function is called once application start-up. Users can initial
 * structures or global parameters here.
 *
 * @param None.
 * @return int Protothread state.
 */
/*----------------------------------------------------------------*/
int app_main(void)
{
#ifdef CONFIG_WLA_LED
	pin_mode(WIFI_LED_PIN, 1);
	digital_write(WIFI_LED_PIN, 0);
#endif
	/* Do not add any process here, user should add process in user_thread */
	hw_sys_init();
	//serial_conf(8, 0, 1, 1,0 );
	return PT_EXITED;
}




void uart_init(void)
{
	int threshold = 0;	
	serial_conf(baudrate_select(g_atParam.ur_param.baudrate), g_atParam.ur_param.parity, g_atParam.ur_param.stopbits, UART0,threshold );
	serial_init((int)UART0);

#ifdef CONFIG_UR_FLOW_CONTROL
	pin_mode(CONFIG_UR_CTS, 0);
	digital_write(CONFIG_UR_RTS, 0);//RTS active
	pin_en_intr(CONFIG_UR_CTS, 0, (gpio_func_cb)uart_flow_cts_stat, NULL);//0: rising
	pin_en_intr(CONFIG_UR_CTS, 1, (gpio_func_cb)uart_flow_cts_stat, NULL);//1: falling
	pin_mode(CONFIG_UR_RTS, 1);
#endif	
}


void user_init(void){
	//串口设置功能
	uart_init();
	//at_test:用户也自定义at指令
	at_cmd_init(at_test);

	//挂载mini文件系统
	if(mfmount(FS1_FLASH_ADDR) == RES_OK){
		//当连接AP时，可以通过下面的网页来打开内置网页
		dnsserver_init("set.mqlinks.com");
		//启动内置web服务器
		webserver_init();
	}
	for(;;)
	{
		//AT系统中WIFI断开后的重连接
		wifi_reset_proc();
		//AT系统中两路socket的重连接
		at_net_proc();
#ifdef CONFIG_OMNICONFIG
		//smartconfig成功后回复APP处理
		omni_state_connecting();
#endif
		//tcp服务器超时断开未响应处理
		tcp_server_timeout_proc();
		/*GPIO检测功能,当GPIO6按下后会进入smartconfig功能
		  !注意:如果些函数不使用，需要用sys_msleep(100);来代替
		  否则user_init任务会僵死
		*/
		mylinks_gpio_proc();
	}
	return;
}


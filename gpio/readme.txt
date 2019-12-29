/*
*=========================================================
* Using AM335X Processor GPIO Pinmap on bbb
*=========================================================
*
*=========================================================
* Output Description  (PIN / NAME / CONFMOD / MODE / INDEX)
*=========================================================
*pin 12 : 
* -PROC(T12)
* -NAME(GPIO1_12)
* -conf_gpmc_ad12
* -gpio1[12]
* -am335x_gpio_pin_0                      
*
*pin 14 : 
* -PROC(T11) 
* -NAME(GPIO0_26)
* -conf_gpmc_ad10
* -gpio0[26]
* -am335x_gpio_pin_1
* 
*pin 16 : 
* -PROC(V13)
* -NAME(GPIO1_14)
* -conf_gpmc_ad14
* -gpio1[14]
* -am335x_gpio_pin_2
*
*pin 18 : 
* -PROC(V12)
* -NAME(GPIO2_1)
* -conf_gpmc_clk_mux0 / gpio2[1]
* -am335x_gpio_pin_3
*
*pin 20 :
* -PROC(V9)
* -NAME(GPIO1_31)
* -conf_gpmc_csn2
* -gpio1[31]
* -am335x_gpio_pin_4
*
*pin 22 :
* -PROC(V8)
* -NAME(GPIO1_5)
* -conf_gpmc_ad5
* -gpio1[5]
* -am335x_gpio_pin_5
*
*
*=========================================================
*MODULE MAJOR 125
*insmod am335x_gpio.ko
*mknode /dev/am335x_gpio c 125 0
*=========================================================
*/


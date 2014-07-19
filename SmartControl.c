/* Copyright (c) ANSHUMAN WADHERA NUSID - A0106504U */

#include <RTL.h>
#include <91x_lib.H>
#include "LCD.h"

unsigned char A0,A1;
unsigned char B0 = 0,B1 = 0,B2 = 0,B3 = 0,B4 = 0,B5 = 0,B6 = 0,B7 = 0; //B0-B7 represent LED's 0 through 7
unsigned int i;
const unsigned int blinkperiod = 10;
unsigned int active = 90;
unsigned int count = 0;
unsigned int totDuration = 0;
unsigned int leftpct = 0;
int doorState = 0;
int lightState = 0;
int acState = 0;
int blinkState = 0;

void read_input()
{
	A0 = !(GPIO3->DR[0x080]>>5); 
	A1 = !(GPIO3->DR[0x100]>>6);
}

void print_uns_char (unsigned char value)
{	 
	int flag = 0,i;
	char c[4];
	do {
	   int rem = value%10;
	   value = value/10;
	   c[flag] = rem+48;
	   flag++;
	}while(value>0);
	for(i=flag-1;i>=0;i--)
		LCD_putc(c[i]);
}

void write_led()
{
	unsigned char mask = 0;
	B0 = 1;
	B1 = 1;
	B2 = 1;
	B3 = 1;
	B4 = 1;
	B5 = 1;
  mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
  GPIO7->DR[0x3FC] = mask;
}

void clear_led()
{
	unsigned char mask = 0;
	B0 = 0;
	B1 = 0;
	B2 = 0;
	B3 = 0;
	B4 = 0;
	B5 = 0;
  mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
  GPIO7->DR[0x3FC] = mask;
}

void write_blink_led()
{
	unsigned char mask = 0;
	B6 = 1;
	B7 = 1;
	mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
	GPIO7->DR[0x3FC] = mask;
}
void clear_blink_led()
{
	unsigned char mask = 0;
	B6 = 0;
	B7 = 0;
	mask  = (B0<<0);
  mask |= (B1<<1);
  mask |= (B2<<2);
  mask |= (B3<<3);
  mask |= (B4<<4);
  mask |= (B5<<5);
  mask |= (B6<<6);
  mask |= (B7<<7);
	GPIO7->DR[0x3FC] = mask;
}

OS_TID Dim_Led_id; 
OS_TID Duty_id;
OS_TID Door_ctl_id;
OS_TID Door_Open_id;
OS_TID Door_Close_id;
OS_TID Ac_id;
OS_TID Blink_Led_id;

__task void DUTY_LED(void)
{	
	unsigned int pct = 0;
		while(1)
		{
		os_evt_wait_or(0x0004,0xFFFF);
		pct =(unsigned int)((active*blinkperiod)/100);
		write_led();
		os_dly_wait(pct);
		clear_led();
		os_evt_set(0x0004,Dim_Led_id);
		}
}

__task void DIM_LED(void) 
{
	Duty_id = os_tsk_create(DUTY_LED,25);
	while(1)
	{
		os_evt_set(0x0004,Duty_id);
		os_evt_wait_or(0x0004,0xFFFF);
		leftpct = blinkperiod - (unsigned int)((active*blinkperiod)/100);
		os_dly_wait(leftpct);
	  totDuration=totDuration+blinkperiod;
		if(count==8)
		{
			lightState = 3;
			if(totDuration>=1000)
				totDuration = 0;
		}
		else
		{
			if(totDuration>=1000)
			{
				active = active-10;
				count++;
				totDuration = 0;
			}
		}	
	}
}

__task void DOOR_CLOSE_HANDLER(void)
{
	while(1)
	{
			os_evt_wait_or(0x0003,0xFFFF);
			if(acState == 1)
			{
				if(lightState == 2 || lightState == 3)
				{
						os_tsk_delete(Door_Open_id);
						lightState = 2;
						os_dly_wait(10000);
						os_tsk_delete(Duty_id);
						os_tsk_delete(Dim_Led_id);
					  active = 100;
						count = 0;
						totDuration = 0;
						lightState = 3;
						clear_led();
				}
				else if(lightState == 1)
				{
					os_tsk_delete(Door_Open_id);
					lightState = 2;
					Dim_Led_id = os_tsk_create(DIM_LED,30);
					os_dly_wait(10000);
					os_tsk_delete(Duty_id);
					os_tsk_delete(Dim_Led_id);
					active = 100;
					count = 0;
					totDuration = 0;
					lightState = 3;
					clear_led();
				}
				lightState = 0;
			}
			else
			{
				lightState = 1;
				write_led();
				os_dly_wait(10000);
				lightState = 2;
				Dim_Led_id = os_tsk_create(DIM_LED,30);
			}
	}
}

__task void LED_BLINK(void)
{
	while(1)
	{
			write_blink_led();
			os_dly_wait(300);
			clear_blink_led();			
			os_dly_wait(300);
	}
}

__task void AC_HANDLER(void)
{
		while(1)
		{
			os_evt_wait_or(0x0006,0xFFFF);
			if(doorState!=0)
			{
				blinkState = 1;
				Blink_Led_id = os_tsk_create(LED_BLINK,15);
			}
			else if(doorState == 0)
			{
				if(lightState == 2 || lightState == 3)
				{
						os_tsk_delete(Door_Close_id);
						lightState = 2;
						os_dly_wait(10000);
						os_tsk_delete(Duty_id);
						os_tsk_delete(Dim_Led_id);
						active = 100;
						count = 0;
						totDuration = 0;
						lightState = 3;
						clear_led();
				}
				else if(lightState == 1)
				{
					os_tsk_delete(Door_Close_id);
					lightState = 2;
					Dim_Led_id = os_tsk_create(DIM_LED,30);
					os_dly_wait(10000);
					os_tsk_delete(Duty_id);
					os_tsk_delete(Dim_Led_id);
					active = 100;
					count = 0;
					totDuration = 0;
					lightState = 3;
					clear_led();
				}
				lightState = 0;
			}
		}
}

__task void DOOR_OPEN_HANDLER(void)
{
	while(1)
	{
			os_evt_wait_or(0x0002,0xFFFF);
			if(acState == 1)
			{
					os_tsk_delete(Ac_id);
					Ac_id = os_tsk_create(AC_HANDLER,26);
					os_evt_set(0x0006,Ac_id);
			}
			lightState = 1;	
			write_led();
			os_dly_wait(20000);
			lightState = 2;
			Dim_Led_id = os_tsk_create(DIM_LED,30);
	}
}

__task void SENSOR(void)
{
		int prevCase = 0;
		int prevCaseAc = 0;
		while(1)
		{
			read_input();
			switch(A0)
			{
				case 1:
					if(prevCase != 1)
					{
						if(doorState==0)
						{
							doorState = 1;
							if(lightState!=0)
							{
								if(lightState == 2 || lightState == 3)
								{
									os_tsk_delete(Duty_id);
									os_tsk_delete(Dim_Led_id);
									active = 100;
									count = 0;
									totDuration = 0;
								}
								os_tsk_delete(Ac_id);
								os_tsk_delete(Door_Close_id);
							}
							Door_Open_id = os_tsk_create(DOOR_OPEN_HANDLER,20);
							os_evt_set(0x0002,Door_Open_id);
						}
						else if(doorState==1)
						{
							doorState = 0;
							if(acState == 1)
							{
								if(blinkState == 1)
								{
									os_tsk_delete(Blink_Led_id);
									clear_blink_led();
									os_tsk_delete(Ac_id);
									blinkState = 0;
								}
							}
							else
							{
								if(lightState!=0)
								{
									if(lightState == 2 || lightState == 3)
									{
										os_tsk_delete(Duty_id);
										os_tsk_delete(Dim_Led_id);
										active = 100;
										count = 0;
										totDuration = 0;
									}
									os_tsk_delete(Door_Open_id);
								}
							}
							Door_Close_id = os_tsk_create(DOOR_CLOSE_HANDLER,20);
							os_evt_set(0x0003,Door_Close_id);
						}
					}
					prevCase = 1;
					break;
				case 0:
					prevCase = 0;
					break;
			}
			switch(A1)
			{
				case 1:
					if(prevCaseAc != 1)
					{
						if(acState == 0)
						{
							acState = 1;
							Ac_id = os_tsk_create(AC_HANDLER,26);
							os_evt_set(0x0006,Ac_id);
						}
						else if(acState == 1)
						{
							acState = 0;
							if(blinkState == 1)
							{
								os_tsk_delete(Blink_Led_id);
								clear_blink_led();
								os_tsk_delete(Ac_id);
								blinkState = 0;
							}
						}
					}
					prevCaseAc = 1;
					break;
				case 0:
					prevCaseAc = 0;
					break;
			}
			LCD_cls();
			LCD_gotoxy(1,1);
			if(doorState == 0 && acState == 0)
			{
				LCD_puts("DOOR CLOSED");
				LCD_gotoxy(1,2);  
				LCD_puts("AC OFF");
			}
			else if(doorState == 0 && acState == 1)
			{
				LCD_puts("DOOR CLOSED");
				LCD_gotoxy(1,2);  
				LCD_puts("AC ON");
			}
			else if(doorState == 1 && acState == 0)
			{
				LCD_puts("DOOR OPEN");
				LCD_gotoxy(1,2); 
				LCD_puts("AC OFF");
			}
			else if(doorState == 1 && acState == 1)
			{
				LCD_puts("DOOR OPEN");
				LCD_gotoxy(1,2); 
				LCD_puts("AC ON");
			}
				LCD_cur_off ();
	  }  
}

__task void init (void) {

  SCU->GPIOOUT[7]  = 0x5555;
  GPIO7->DDR       = 0xFF;
  GPIO7->DR[0x3FC] = 0x00;
  GPIO8->DDR       = 0xFF;
  GPIO9->DDR       = 0x07;
  SCU->GPIOIN[3]  |= 0x60;
  SCU->GPIOOUT[3] &= 0xC3FF;
  GPIO3->DDR      &= 0x9F;
  LCD_init();
  LCD_cur_off();
  LCD_cls(); 
 	Door_ctl_id = os_tsk_create(SENSOR,10);
  os_tsk_delete_self ();
}

int main (void) 
{
  os_sys_init (init);
  return 0;
}

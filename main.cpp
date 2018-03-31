#include "stm32f10x_rcc.h"
#include "stm32f10x_rtc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "string.h"
#include "modIndicator.h"

/**
Требования:
- считать время, отображать часы минуты
- засыпать, просыпатся каждую секунду, проверять состояние входа (кнопки) - нажато - не входить в сон еще минуту.
- каждые 15 минут работать минуту - позже засыпать
- принимать по uart новое время
- мерять напряжение банки? предупреждать о разряде?

Особенности:
должен мало жрать, много спать
*/

/**
TODO:
+настроить часы,
+настроить будильник на 1 секунду,
+взять индикацию с ИТП11, приспособить, отобразить время
научить засыпать и просыпатся (по будильнику)
*/

unsigned long itick = 0, RTCTicks; 
cIndicator indic;
extern unsigned char textTemp[10];
unsigned char uartDataCounter = 0, uartData[16];

#define STARTRIME	0
#define STOPTIME	10
#define SETTIME		19
#define CALIBTIME	27

const char commands[] = 
{
's','t','a','r','t','T','i','m','e', 0x00,
's','t','o','p','T','i','m','e', 0x00,
's','e','t','T','i','m','e', 0x00,
'c','a','l','i','b','R','T','C', 0x00,
};

typedef struct{
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	unsigned long ticks;
}time_t;

time_t counter;

unsigned long wakeUpTime = 0, activeTime = 0;

int main(void)
{
	/**RCC*/
	{
	RCC_HSICmd(ENABLE);
	RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_4);
	RCC_PLLCmd(ENABLE);
	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
	RCC_HCLKConfig(RCC_SYSCLK_Div16);
	RCC_PCLK1Config(RCC_HCLK_Div1);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	SystemCoreClockUpdate();
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
	
	SysTick_Config(SystemCoreClock / (/*0.1ms*/10000));
	NVIC_SetPriorityGrouping(3); 
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0)); 

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP|RCC_APB1Periph_PWR, ENABLE);
	}
	
	/**GPIO*/
	{
	GPIO_ResetBits(GPIOA, 0x000000ff);
	GPIO_InitTypeDef gpio;
	GPIO_StructInit(&gpio);
	/**0-7*/
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);
	/**9-12*/	
	gpio.GPIO_Mode = GPIO_Mode_Out_OD;
	gpio.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);
		
	/**Button*/
	gpio.GPIO_Pin = GPIO_Pin_13;
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &gpio);
	}
	
	/**UART*/
	{
	USART_InitTypeDef uartInit;
	uartInit.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	uartInit.USART_BaudRate = 9600;
	uartInit.USART_Parity = USART_Parity_No;
	uartInit.USART_StopBits = USART_StopBits_1;
	uartInit.USART_WordLength = USART_WordLength_8b;
	uartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART1, &uartInit);
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	}
	
	NVIC_InitTypeDef nvicInit;	
	nvicInit.NVIC_IRQChannel = USART1_IRQn;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
	nvicInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInit);
	
	
	/**Debug in low power mode*/
	{
		DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP;
		DBGMCU->CR |= DBGMCU_CR_DBG_STOP;
		DBGMCU->CR |= DBGMCU_CR_DBG_STANDBY;
	}
	
	/**RTC*/
	{
	/**Дать доступ к backup области*/
	PWR_BackupAccessCmd(ENABLE);
	/**Если запуск просыпание будильника - не инициализировать*/	
	if( BKP_ReadBackupRegister(BKP_DR1) != 0xa55a )
	{	
		RCC_ClearFlag();
		/**Сбросить backup область*/
		RCC_BackupResetCmd(ENABLE);
		RCC_BackupResetCmd(DISABLE);
		/**Вкл LSI*/
		RCC_LSICmd(ENABLE);
		/**Настроит тактирование RTC от LSI*/
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		/**Запустить RTC*/
		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForLastTask();
		RTC_SetPrescaler(40000 - 1);
		RTC_WaitForLastTask();
		RTC_SetCounter(0);
		BKP_WriteBackupRegister(BKP_DR1, 0xa55a);
	}
	else
	{
		RCC_LSICmd(DISABLE);
		RCC_LSICmd(ENABLE);
		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
	}
	nvicInit.NVIC_IRQChannel = RTC_IRQn;
	nvicInit.NVIC_IRQChannelCmd = ENABLE;
	nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
	nvicInit.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvicInit);
	}
	
	indic.clrPict();
	indic.convToPict((unsigned char *)"----");
	
	RTC_WaitForSynchro();
	wakeUpTime = RTC_GetCounter();

	if(PWR_GetFlagStatus(PWR_FLAG_SB) == SET)
	{
		PWR_ClearFlag(PWR_FLAG_SB);
	}
	
	if(RTC_GetFlagStatus(RTC_FLAG_ALR) == SET)
	{
		RTC_ClearFlag(RTC_FLAG_ALR);
	}
	
	while(1)
	{
		if(((activeTime + 500) <= itick) && uartDataCounter)
		{
			if(strcmp((const char *) uartData,(const char *) &commands[STARTRIME]) == 0)
			{
				RCC_LSICmd(ENABLE);
			}
			else if(strcmp((const char *) uartData,(const char *) &commands[STOPTIME]) == 0)
			{
				RCC_LSICmd(DISABLE);
			}
			else if(strncmp((const char *) uartData,(const char *) &commands[SETTIME], 7) == 0)
			{
				unsigned long newTime = 0;
				newTime = (((uartData[8] - 0x30) * 10) + (uartData[9] - 0x30)) * 60 * 60;
				newTime += (((uartData[11] - 0x30) * 10) + (uartData[12] - 0x30)) * 60;
				newTime += (((uartData[14] - 0x30) * 10) + (uartData[15] - 0x30));
				RTC_WaitForLastTask();
				RTC_SetCounter(newTime);
			}
			else if(strncmp((const char *) uartData,(const char *) &commands[CALIBTIME], 9) == 0)
			{
				unsigned long newPrescaler = 0;
				newPrescaler = (uartData[10] - 0x30) * 10000;
				newPrescaler += (uartData[11] - 0x30) * 1000;
				newPrescaler += (uartData[12] - 0x30) * 100;
				newPrescaler += (uartData[13] - 0x30) * 10;
				newPrescaler += (uartData[14] - 0x30) * 1;
				RTC_WaitForLastTask();
				RTC_SetPrescaler(newPrescaler - 1);
			}
			
			for(unsigned char i = 0; i < 16; i++)
			{
				uartData[i] = 0;
			}
			
			uartDataCounter = 0;
			RTC_WaitForSynchro();
			wakeUpTime = RTC_GetCounter();
		}
		
		RTC_WaitForSynchro();
		RTCTicks = RTC_GetCounter();
		if(RTCTicks != counter.ticks)
		{
			counter.ticks = RTCTicks;
			counter.hour = (RTCTicks/(60*60))%24;
			counter.minute = (RTCTicks/60)%60;
			counter.second = (RTCTicks/1)%60;
			
			float val = counter.second;
			val /= 100;
			val += counter.minute;
			convCharMass(val, 2);
			indic.convToPict(textTemp);
			
			if( (wakeUpTime + 60) <= RTCTicks)
			{
				RTC_WaitForLastTask();
				/**Установить время ближайшее к 15 минутам*/
				unsigned long alarmTime = RTCTicks%(15*60);
				alarmTime = RTCTicks - alarmTime;
				alarmTime += 15*60;
				RTC_SetAlarm( alarmTime - 1);
				RTC_WaitForLastTask();
				RTC_ITConfig(RTC_IT_ALR, ENABLE);
				PWR_EnterSTANDBYMode();
			}
		}
	}
}



#ifdef __cplusplus
extern "C"{
#endif
	
/**0.1ms*/
void SysTick_Handler(void)
{
	static unsigned char localTimeCount = 0, ledTimer = 0;

	if(++localTimeCount >= 10)
	{
		localTimeCount = 0;
		itick++;
	}
	
	if(++ledTimer >= 7)
	{
		ledTimer = 0;
	}
	
	if(ledTimer == 0)
	{
		indic.indicAvt(0);
	}
	else if(ledTimer == 6)
	{
		indic.indicAvt(1);
	}
		
}

void RTC_IRQHandler(void)
{
	RTC_ClearFlag(RTC_FLAG_ALR);
}

void USART1_IRQHandler(void)
{
	activeTime = itick;
	uartData[uartDataCounter++] = USART_ReceiveData(USART1);
}
#ifdef __cplusplus
}
#endif

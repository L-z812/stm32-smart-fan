#include "stm32f10x.h"                  // Device header

/**
  * 说    明：两个指示灯接在 PA8（红）和 PA9（绿），低电平点亮。
  *           原来这份代码把灯写在 PA1/PA2 上，其中 PA2 是 PWM 输出，
  *           LED_Init() 又从来没被调用，等于一颗"定时炸弹"。
  *           这里统一改到 PA8/PA9 并真正初始化，谁用谁先 Init。
  */

/**
  * 函    数：LED初始化
  * 参    数：无
  * 返 回 值：无
  */
void LED_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);						//将PA8和PA9引脚初始化为推挽输出
	
	/*默认全灭：低电平点亮，所以初始化写高*/
	GPIO_SetBits(GPIOA, GPIO_Pin_8 | GPIO_Pin_9);
}

/**
  * 函    数：红色指示灯开关
  * 参    数：On 1 = 亮，0 = 灭
  * 返 回 值：无
  */
void LED_Red(uint8_t On)
{
	if (On)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_8);		//PA8 低电平 → 亮
	}
	else
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_8);		//PA8 高电平 → 灭
	}
}

/**
  * 函    数：绿色指示灯开关
  * 参    数：On 1 = 亮，0 = 灭
  * 返 回 值：无
  */
void LED_Green(uint8_t On)
{
	if (On)
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_9);		//PA9 低电平 → 亮
	}
	else
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_9);		//PA9 高电平 → 灭
	}
}

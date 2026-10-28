#include "stm32f10x.h"                  // Device header

/**
  * 说    明：本模块同时管两件事
  *           1. TIM3 的 1kHz 更新中断 —— 给蜂鸣器产生方波
  *           2. 蜂鸣器引脚 PB12 的 GPIO 初始化
  *           用 TIM3 而不是 TIM2，是因为 TIM2 已经被 PWM 占用了：
  *           一个定时器不能既当 PWM 时基又当另一个频率的中断源，
  *           原来的工程就是这里踩了坑（PWM 的 ARR 被改成了 9）。
  */
static volatile uint8_t Buzzer_On = 0;		//1 = 蜂鸣器正在响

/**
  * 函    数：定时中断初始化
  * 参    数：无
  * 返 回 值：无
  */
void Timer_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);		//开启TIM3的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
																//蜂鸣器引脚在本模块初始化，不再"蹭"OLED 的引脚配置
	
	/*蜂鸣器引脚初始化：PB12 推挽输出，默认低电平（不响）*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	
	/*配置时钟源*/
	TIM_InternalClockConfig(TIM3);		//选择TIM3为内部时钟
	
	/*时基单元初始化：72MHz / 72 = 1MHz，1MHz / 1000 = 1kHz 中断*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
	TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;				//计数周期，即ARR的值
	TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;				//预分频器，即PSC的值
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);				//配置TIM3的时基单元
	
	/*中断输出配置*/
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);						//清除更新标志位，避免一开中断就立刻进一次
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);					//开启TIM3的更新中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);				//配置NVIC为分组2，整工程调用一次即可
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;				//选择配置NVIC的TIM3线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;	//抢占优先级2（比编码器低，编码器不丢脉冲更重要）
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//响应优先级1
	NVIC_Init(&NVIC_InitStructure);								//配置NVIC外设
	
	/*TIM使能*/
	TIM_Cmd(TIM3, ENABLE);			//使能TIM3，定时器开始运行
}

/**
  * 函    数：蜂鸣器开关
  * 参    数：On 1 = 响，0 = 停
  * 返 回 值：无
  * 注意事项：只是置一个标志位，真正的方波由 TIM3 中断翻转
  */
void Buzzer_Set(uint8_t On)
{
	Buzzer_On = On;
	if (On == 0)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//不响时把引脚拉低，不要悬着
	}
}

/**
  * 函    数：TIM3 中断函数
  * 参    数：无
  * 返 回 值：无
  * 注意事项：中断里只做翻转，不做延时和打印，否则会拖慢整个系统
  */
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		if (Buzzer_On)
		{
			/*每 1ms 翻转一次引脚，得到 500Hz 方波*/
			if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12) == 0)
			{
				GPIO_SetBits(GPIOB, GPIO_Pin_12);
			}
			else
			{
				GPIO_ResetBits(GPIOB, GPIO_Pin_12);
			}
		}
		
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);		//中断标志位必须清除
	}
}

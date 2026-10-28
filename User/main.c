#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"
#include "Encoder.h"
#include "AD.h"
#include "TIMER.h"
uint8_t KeyNum;		//定义用于接收按键键码的变量
int8_t Speed;		//定义速度变量
float Voltage;				//定义电压变量
uint16_t ADValue;			//定义AD值变量
int flag=0;					//用来防止温度传感和旋转编码冲突的
int i=0;					//控制蜂鸣器翻转频率
int main(void)
{
	/*模块初始化*/
	OLED_Init();		//OLED初始化
	Motor_Init();		//直流电机初始化
	Key_Init();			//按键初始化（没用到）
	Encoder_Init();		//旋转编码器初始化
	Timer_Init();		//定时器初始化（用于蜂鸣器）
	AD_Init();				//AD初始化（用于温度传感器）
	/*显示静态字符串*/
	OLED_ShowString(1, 1, "----Motor---");
	OLED_ShowString(2, 1, "Speed:");		//2行1列显示字符串Speed:
	OLED_ShowString(3, 1, "Voltage:");		//3行1列显示字符串Voltage:（温度变量）
	while (1)
	{
		ADValue = AD_GetValue();				//获取AD转换的值
		Voltage =100-((ADValue-1730)/2.1);		//将AD值线性变换到0~100的范围
		KeyNum = Key_GetNum();					//获取按键键码
		if(Voltage>25){
		if(Voltage>65)							//温度变量达到65以上
		{
		flag=1;									//蜂鸣器标志位
		GPIO_ResetBits(GPIOA, GPIO_Pin_8);		//LED红灯亮
		GPIO_SetBits(GPIOA, GPIO_Pin_9);
		}
		if(Voltage>55&&Voltage<65){GPIO_SetBits(GPIOA, GPIO_Pin_8);GPIO_SetBits(GPIOA, GPIO_Pin_9);}
		if(Voltage<55)
		{
		flag=0;
			GPIO_SetBits(GPIOA, GPIO_Pin_8);	//LED红灯不亮
		GPIO_ResetBits(GPIOA, GPIO_Pin_9);
		}
		Speed=Voltage;
		if(Voltage>=100){Speed=99;}				//边界判断
		}
		if(Voltage<=25&&Voltage>=20){Speed=0;}
		if(Voltage<20){
		Speed+=4*Encoder_Get();
		}
		if(Speed>=100||Speed<=-100){Speed=0;}
		Motor_SetSpeed(Speed);				//设置直流电机的速度为速度变量
		OLED_ShowSignedNum(2, 7, Speed, 3);	//OLED显示速度变量
		OLED_ShowNum(3, 9, Voltage, 3);				//显示电压值的整数部分
	}
}
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		i++;
		if(flag==1){
		TIM_SetAutoreload(TIM2,190);					//中央C的蜂鸣器音调（可自调节）
		if(i%2==0){GPIO_SetBits(GPIOB, GPIO_Pin_12);}
		if(i%2==1){GPIO_ResetBits(GPIOB, GPIO_Pin_12);}
		
		}
		if(flag==0){
			GPIO_SetBits(GPIOB, GPIO_Pin_12);				//红LED不亮
		GPIO_ResetBits(GPIOA, GPIO_Pin_9);
		GPIO_SetBits(GPIOA, GPIO_Pin_8);
		}
		
		
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
	
}

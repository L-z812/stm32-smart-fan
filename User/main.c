#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "LED.h"
#include "Encoder.h"
#include "AD.h"
#include "Timer.h"

/*------------------- 温度分档阈值（单位：℃） -------------------*/
#define TEMP_MANUAL		20		//低于此温度：交给旋钮手动调速
#define TEMP_IDLE		25		//20~25℃：死区，电机停转，防止临界点来回跳
#define TEMP_ALARM_ON	65		//高于此温度：红灯亮 + 蜂鸣器响
#define TEMP_ALARM_OFF	55		//回落到此温度以下才解除报警（10℃ 迟滞）
#define SPEED_MAX		99		//PWM 分辨率 0~99，共 100 级

/*------------------- 全局状态，全部在主循环里更新 -------------------*/
int16_t Temperature;		//当前温度，0~100℃
int16_t Speed;				//当前速度，-99~99，负数代表反转
uint8_t AlarmFlag;			//1 = 报警中
uint8_t WorkMode;			//0=手动 1=死区 2=自动，只用于显示

int main(void)
{
	/*模块初始化：顺序按硬件依赖来编排*/
	OLED_Init();		//PB8/PB9	软件 I2C
	Motor_Init();		//PA4/PA5	方向 + PA2 PWM
	LED_Init();			//PA8 红灯、PA9 绿灯
	Encoder_Init();		//PB0/PB1	外部中断
	Timer_Init();		//PB12		蜂鸣器，TIM3 每 1ms 进一次中断
	AD_Init();			//PA0		温度传感器模拟输入
	
	/*静态字符串只写一次，其余每圈刷新*/
	OLED_ShowString(1, 1, "Smart Fan");
	OLED_ShowString(2, 1, "Speed:");
	OLED_ShowString(3, 1, "Temp:");
	OLED_ShowString(3, 11, "C");
	OLED_ShowString(4, 1, "Mode:");
	
	while (1)
	{
		/*① 采样：连采 8 次取平均，再线性标定到 0~100℃，并限幅*/
		Temperature = AD_GetTemperature();
		
		/*② 报警状态：>65℃ 触发，<55℃ 才解除。迟滞 10℃，避免在阈值上反复横跳*/
		if (Temperature > TEMP_ALARM_ON)
		{
			AlarmFlag = 1;
		}
		else if (Temperature < TEMP_ALARM_OFF)
		{
			AlarmFlag = 0;
		}
		
		/*③ 三档决策*/
		if (Temperature > TEMP_IDLE)				//自动档：温度即速度，越热转得越快
		{
			WorkMode = 2;
			Speed = Temperature;
			(void)Encoder_Get();					//丢掉自动档期间攒下的旋钮增量
		}
		else if (Temperature >= TEMP_MANUAL)		//死区：直接停机
		{
			WorkMode = 1;
			Speed = 0;
			(void)Encoder_Get();
		}
		else										//手动档：旋钮调速，每格 ±4
		{
			WorkMode = 0;
			Speed += 4 * Encoder_Get();
		}
		
		/*④ 限幅。原来是"超范围直接清零"，拧过头会从满速突然掉到停转，手感很突兀*/
		if (Speed > SPEED_MAX)	Speed = SPEED_MAX;
		if (Speed < -SPEED_MAX)	Speed = -SPEED_MAX;
		
		/*⑤ 执行*/
		Motor_SetSpeed(Speed);
		Buzzer_Set(AlarmFlag);
		
		/*⑥ 指示灯：报警红灯、预警带（55~65℃）两灯全灭、其余绿灯*/
		if (AlarmFlag)
		{
			LED_Red(1);
			LED_Green(0);
		}
		else if (Temperature > TEMP_ALARM_OFF)
		{
			LED_Red(0);
			LED_Green(0);
		}
		else
		{
			LED_Red(0);
			LED_Green(1);
		}
		
		/*⑦ 显示*/
		OLED_ShowSignedNum(2, 7, Speed, 3);				//速度，带符号 3 位
		OLED_ShowNum(3, 7, (uint16_t)Temperature, 3);		//温度，3 位整数
		if (AlarmFlag)			OLED_ShowString(4, 7, "ALARM");
		else if (WorkMode == 2)	OLED_ShowString(4, 7, "AUTO ");
		else if (WorkMode == 1)	OLED_ShowString(4, 7, "IDLE ");
		else					OLED_ShowString(4, 7, "MANU ");
	}
}

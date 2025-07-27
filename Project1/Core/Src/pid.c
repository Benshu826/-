#include "pid.h"
#include "encoder.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "motor.h"
//编码器
int Encoder_Left, Encoder_Right;
//MPU6050
float pitch, roll, yaw;
short gyrox, gyroy, gyroz;
short accx, accy, accz;

//闭环控制中间变量
int Vertical_out, Velocity_out, Turn_out, Target_Speed, Target_Turn;
int Motor1, Motor2;
float Med_Angle = -2.75;		//平衡时角度的偏移量

//PID预设值
// -240 -0.45
double Vertical_Kp = -120, Vertical_Kd = -0.6;					//直立环PD控制器(Kp-->0--1000, kd-->0--10)
double Velocity_Kp = -1.2, Velocity_Ki = -0.006;					//速度环PI控制器(Kp-->0--1)
double Turn_Kp = 10, Turn_Kd = 0.1;									//转向环PD控制器
uint8_t stop;

extern TIM_HandleTypeDef htim2, htim4;
extern int Left, Right, Fore, Back;
extern float distance;
#define SPEED_Y 12 //俯仰(前后)最大设定速度
#define SPEED_Z 150//偏航(左右)最大设定速度 


//直立环PD控制器 
//输入：期望角度、真实角度、角速度
//不滤波的原因是MPU6050滤过了。
int Vertical_PD(float Med, float Angle, float gyro_y)
{
	int tmp = Vertical_Kp * (Angle - Med) + Vertical_Kd * gyro_y;
	return tmp;
}



//速度环PI控制器
//输入：期望速度、真实速度（左编码、右编码）
int Velocity_PI(int Target, int Left_Encoder, int Right_Encoder)
{
	//Encoder_S是偏差累加
	static int Err_Lowout_Last, Encoder_S;
	static float a = 0.7;
	//1、计算偏差值
	int Err, Err_Lowout;
	Err = (Left_Encoder + Right_Encoder) - Target;
	//2、低通滤波，为什么这么做，得搜一下
	Err_Lowout = (1 - a) * Err + a * Err_Lowout_Last;
	Err_Lowout_Last = Err_Lowout;
	//3、积分
	Encoder_S += Err_Lowout;
	//4、限幅
	Encoder_S = Encoder_S > 10000 ? 10000 : (Encoder_S < (-10000) ? (-10000) : Encoder_S);
	if(stop == 1)
	{
		Encoder_S=0;
		stop=0;
	}
	//5、速度环计算
	Velocity_Ki = Velocity_Kp / 200;
	int tmp = Velocity_Kp * Err_Lowout + Velocity_Ki * Encoder_S;
	return tmp;
}


//转向环PD控制器
//输入：角速度、角度值
int Turn_PD(int gyro_Z, int Target_Turn)
{
	int tmp = Turn_Kp * Target_Turn + Turn_Kd * gyro_Z;
	return tmp;
}



void Control(void)								//每隔10ms调用，TIMER都用完了，用M6050的INT引脚来进行中断
{
	int PWM_out;
	//1、读取编码器和陀螺仪的数据（角速度、加速度）
	Encoder_Left = Read_Speed(&htim2);
	Encoder_Right = -Read_Speed(&htim4);
	mpu_dmp_get_data(&pitch, &roll, &yaw);
	MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);
	MPU_Get_Accelerometer(&accx, &accy, &accz);
	//遥控
	if(Fore==0 && Back == 0)	Target_Speed = 0;
	if(Fore == 1)
	{
		if(distance < 50)
		{
			Target_Speed++;
		}
		else
		{
			Target_Speed--;
		}
	}
	if(Back == 1)
	{
		Target_Speed++;
	}
	Target_Speed=Target_Speed>SPEED_Y?SPEED_Y:(Target_Speed<-SPEED_Y?(-SPEED_Y):Target_Speed);//限幅
	if((Left==0)&&(Right==0))Turn_Kd=0.1;//若无左右转向指令，则开启转向约束
	else if((Left==1)||(Right==1))Turn_Kd=0;//若左右转向指令接收到，则去掉转向约束
	if(Left==1)Target_Turn+=10;	//左转
	if(Right==1)Target_Turn-=10;	//右转
	Target_Turn=Target_Turn>SPEED_Z?SPEED_Z:(Target_Turn<-SPEED_Z?(-SPEED_Z):Target_Turn);//限幅( (20*100) * 100   )
	
	if((Left==0)&&(Right==0))Turn_Kd=0.1;			//若无左右转向指令，则开启转向约束
	else if((Left==1)||(Right==1))Turn_Kd=0;	//若左右转向指令接收到，则去掉转向约束

	
	//2、将数据输入PID，计算输出结果--->左右电机的转速值
	Velocity_out = Velocity_PI(Target_Speed, Encoder_Left, Encoder_Right);
	Vertical_out = Vertical_PD(Velocity_out+Med_Angle, roll, gyrox);
	Turn_out = Turn_PD(gyroz, Target_Turn);
	
	PWM_out = Vertical_out;
	//差速转向
	Motor1 = PWM_out - Turn_out;
	Motor2 = PWM_out + Turn_out;
	Limit(&Motor1, &Motor2);
	
	Load(Motor1, Motor2);
	Stop(&Med_Angle,&roll);//安全检测
	
}



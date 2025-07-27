#include "pid.h"
#include "encoder.h"
#include "mpu6050.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "motor.h"
//������
int Encoder_Left, Encoder_Right;
//MPU6050
float pitch, roll, yaw;
short gyrox, gyroy, gyroz;
short accx, accy, accz;

//�ջ������м����
int Vertical_out, Velocity_out, Turn_out, Target_Speed, Target_Turn;
int Motor1, Motor2;
float Med_Angle = -2.75;		//ƽ��ʱ�Ƕȵ�ƫ����

//PIDԤ��ֵ
// -240 -0.45
double Vertical_Kp = -120, Vertical_Kd = -0.6;					//ֱ����PD������(Kp-->0--1000, kd-->0--10)
double Velocity_Kp = -1.2, Velocity_Ki = -0.006;					//�ٶȻ�PI������(Kp-->0--1)
double Turn_Kp = 10, Turn_Kd = 0.1;									//ת��PD������
uint8_t stop;

extern TIM_HandleTypeDef htim2, htim4;
extern int Left, Right, Fore, Back;
extern float distance;
#define SPEED_Y 12 //����(ǰ��)����趨�ٶ�
#define SPEED_Z 150//ƫ��(����)����趨�ٶ� 


//ֱ����PD������ 
//���룺�����Ƕȡ���ʵ�Ƕȡ����ٶ�
//���˲���ԭ����MPU6050�˹��ˡ�
int Vertical_PD(float Med, float Angle, float gyro_y)
{
	int tmp = Vertical_Kp * (Angle - Med) + Vertical_Kd * gyro_y;
	return tmp;
}



//�ٶȻ�PI������
//���룺�����ٶȡ���ʵ�ٶȣ�����롢�ұ��룩
int Velocity_PI(int Target, int Left_Encoder, int Right_Encoder)
{
	//Encoder_S��ƫ���ۼ�
	static int Err_Lowout_Last, Encoder_S;
	static float a = 0.7;
	//1������ƫ��ֵ
	int Err, Err_Lowout;
	Err = (Left_Encoder + Right_Encoder) - Target;
	//2����ͨ�˲���Ϊʲô��ô��������һ��
	Err_Lowout = (1 - a) * Err + a * Err_Lowout_Last;
	Err_Lowout_Last = Err_Lowout;
	//3������
	Encoder_S += Err_Lowout;
	//4���޷�
	Encoder_S = Encoder_S > 10000 ? 10000 : (Encoder_S < (-10000) ? (-10000) : Encoder_S);
	if(stop == 1)
	{
		Encoder_S=0;
		stop=0;
	}
	//5���ٶȻ�����
	Velocity_Ki = Velocity_Kp / 200;
	int tmp = Velocity_Kp * Err_Lowout + Velocity_Ki * Encoder_S;
	return tmp;
}


//ת��PD������
//���룺���ٶȡ��Ƕ�ֵ
int Turn_PD(int gyro_Z, int Target_Turn)
{
	int tmp = Turn_Kp * Target_Turn + Turn_Kd * gyro_Z;
	return tmp;
}



void Control(void)								//ÿ��10ms���ã�TIMER�������ˣ���M6050��INT�����������ж�
{
	int PWM_out;
	//1����ȡ�������������ǵ����ݣ����ٶȡ����ٶȣ�
	Encoder_Left = Read_Speed(&htim2);
	Encoder_Right = -Read_Speed(&htim4);
	mpu_dmp_get_data(&pitch, &roll, &yaw);
	MPU_Get_Gyroscope(&gyrox, &gyroy, &gyroz);
	MPU_Get_Accelerometer(&accx, &accy, &accz);
	//ң��
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
	Target_Speed=Target_Speed>SPEED_Y?SPEED_Y:(Target_Speed<-SPEED_Y?(-SPEED_Y):Target_Speed);//�޷�
	if((Left==0)&&(Right==0))Turn_Kd=0.1;//��������ת��ָ�����ת��Լ��
	else if((Left==1)||(Right==1))Turn_Kd=0;//������ת��ָ����յ�����ȥ��ת��Լ��
	if(Left==1)Target_Turn+=10;	//��ת
	if(Right==1)Target_Turn-=10;	//��ת
	Target_Turn=Target_Turn>SPEED_Z?SPEED_Z:(Target_Turn<-SPEED_Z?(-SPEED_Z):Target_Turn);//�޷�( (20*100) * 100   )
	
	if((Left==0)&&(Right==0))Turn_Kd=0.1;			//��������ת��ָ�����ת��Լ��
	else if((Left==1)||(Right==1))Turn_Kd=0;	//������ת��ָ����յ�����ȥ��ת��Լ��

	
	//2������������PID������������--->���ҵ����ת��ֵ
	Velocity_out = Velocity_PI(Target_Speed, Encoder_Left, Encoder_Right);
	Vertical_out = Vertical_PD(Velocity_out+Med_Angle, roll, gyrox);
	Turn_out = Turn_PD(gyroz, Target_Turn);
	
	PWM_out = Vertical_out;
	//����ת��
	Motor1 = PWM_out - Turn_out;
	Motor2 = PWM_out + Turn_out;
	Limit(&Motor1, &Motor2);
	
	Load(Motor1, Motor2);
	Stop(&Med_Angle,&roll);//��ȫ���
	
}



#include "stm32f10x.h"
#include "DHT11.h"
#include "delay.h"

// 全局数据数组
unsigned int rec_data[4];

// 对于stm32来说，是输出
void DHT11_GPIO_Init_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP; //推挽输出
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// 对于stm32来说，是输入
void DHT11_GPIO_Init_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING; //浮空输入
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// 主机发送开始信号
void DHT11_Start(void)
{
    DHT11_GPIO_Init_OUT(); //输出模式
    
    dht11_high; //先拉高
    Delay_us(30);
    
    dht11_low; //拉低电平至少18us
    Delay_ms(20);
    
    dht11_high; //拉高电平20~40us
    Delay_us(30);
    
    DHT11_GPIO_Init_IN(); //输入模式
}

// 获取一个字节
char DHT11_Rec_Byte(void)
{
    unsigned char i = 0;
    unsigned char data = 0;
    uint32_t timeout;
    
    for(i = 0; i < 8; i++) //1个数据就是1个字节byte，1个字节byte有8位bit
    {
        // 等待低电平结束，添加超时保护
        timeout = 0;
        while(Read_Data == 0 && timeout < 1000)
        {
            timeout++;
            Delay_us(1);
        }
        if(timeout >= 1000) return 0; // 超时返回0
        
        Delay_us(30); //延迟30us是为了区别数据0和数据1，0只有26~28us
        
        data <<= 1; //左移
        
        if(Read_Data == 1) //如果过了30us还是高电平的话就是数据1
        {
            data |= 1; //数据+1
        }
        
        // 等待高电平结束，添加超时保护
        timeout = 0;
        while(Read_Data == 1 && timeout < 1000)
        {
            timeout++;
            Delay_us(1);
        }
        if(timeout >= 1000) return 0; // 超时返回0
    }
    
    return data;
}

// 获取数据
void DHT11_REC_Data(void)
{
    unsigned int R_H, R_L, T_H, T_L;
    unsigned char RH = 0, RL = 0, TH = 0, TL = 0, CHECK;
    uint32_t timeout;
    
    DHT11_Start(); //主机发送信号
    dht11_high; //拉高电平
    
    // 等待DHT11响应，添加超时保护
    timeout = 0;
    while(Read_Data == 0 && timeout < 1000)
    {
        timeout++;
        Delay_us(1);
    }
    if(timeout >= 1000) return; // 超时退出
    
    // 等待响应信号结束
    timeout = 0;
    while(Read_Data == 1 && timeout < 1000)
    {
        timeout++;
        Delay_us(1);
    }
    if(timeout >= 1000) return; // 超时退出
    
    // 读取5个字节数据
    R_H = DHT11_Rec_Byte();
    R_L = DHT11_Rec_Byte();
    T_H = DHT11_Rec_Byte();
    T_L = DHT11_Rec_Byte();
    CHECK = DHT11_Rec_Byte();
    
    // 检查数据有效性
    if(R_H == 0 && T_H == 0) return; // 数据无效
    
    dht11_low; //当最后一bit数据传送完毕后，DHT11拉低总线 50us
    Delay_us(55); //这里延时55us
    dht11_high; //随后总线由上拉电阻拉高进入空闲状态。
    
    if(R_H + R_L + T_H + T_L == CHECK) //和检验位对比，判断校验接收到的数据是否正确
    {
        RH = R_H;
        RL = R_L;
        TH = T_H;
        TL = T_L;
    }
    
    rec_data[0] = RH; // 湿度整数
    rec_data[1] = RL; // 湿度小数
    rec_data[2] = TH; // 温度整数
    rec_data[3] = TL; // 温度小数
}

// DHT11初始化
void DHT11_Init(void)
{
    DHT11_GPIO_Init_OUT();
    dht11_high;
}

// 读取温湿度数据（兼容新接口）
uint8_t DHT11_Read_Data(DHT11_Data_TypeDef *data)
{
    DHT11_REC_Data();
    
    if(rec_data[0] != 0 || rec_data[2] != 0) // 简单判断数据有效性
    {
        data->humidity_int = rec_data[0];
        data->humidity_dec = rec_data[1];
        data->temperature_int = rec_data[2];
        data->temperature_dec = rec_data[3];
        data->check_sum = rec_data[0] + rec_data[1] + rec_data[2] + rec_data[3];
        return 0; // 成功
    }
    
    return 1; // 失败
}

// 读取温度值
uint8_t DHT11_Read_Temperature(void)
{
    DHT11_Data_TypeDef data;
    if(DHT11_Read_Data(&data) == 0)
    {
        return data.temperature_int;
    }
    return 0xFF; // 读取失败
}

// 读取湿度值
uint8_t DHT11_Read_Humidity(void)
{
    DHT11_Data_TypeDef data;
    if(DHT11_Read_Data(&data) == 0)
    {
        return data.humidity_int;
    }
    return 0xFF; // 读取失败
}

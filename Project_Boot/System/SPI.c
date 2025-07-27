// SPI1硬件通信驱动程序

#include "SPI.h"

// SPI1初始化
void SPI1_Init(void)
{
	// 使能GPIOA和SPI1时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 配置SCK和MOSI引脚(PA5,PA7)为复用推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 配置MISO引脚(PA6)为浮空输入
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 复位SPI1外设到默认状态
	SPI_I2S_DeInit(SPI1);
	
	// 配置SPI1参数
	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;    // 波特率预分频器：系统时钟/2
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                         // 时钟相位：第二个边沿采样
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                          // 时钟极性：空闲时为高电平
	SPI_InitStructure.SPI_CRCPolynomial = 7;                             // CRC多项式（未使用CRC）
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                     // 数据位宽：8位
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;    // 通信方向：双线全双工
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                    // 数据传输顺序：MSB先传输
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                         // SPI模式：主机模式
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                            // NSS信号管理：软件管理
	SPI_Init(SPI1, &SPI_InitStructure);
	
	// 使能SPI1外设
	SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief SPI1数据收发函数
 * @param tx 要发送的数据
 * @return uint16_t 接收到的数据
 * 
 * 通过SPI1发送一个字节数据，同时接收一个字节数据
 * SPI是全双工通信，发送和接收同时进行
 */
uint16_t SPI1_ReadWrite_Byte(uint16_t tx)
{
	// 等待发送缓冲区空闲
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != 1);
	
	// 发送数据
	SPI_I2S_SendData(SPI1, tx);
	
	// 等待接收缓冲区有数据
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != 1);
	
	// 返回接收到的数据
	return SPI_I2S_ReceiveData(SPI1);
}

/**
 * @brief SPI1批量写入函数
 * @param wdata 写入数据缓冲区指针
 * @param datalen 要写入的数据长度
 * @return 无
 * 
 */
void SPI1_Write(uint8_t *wdata, uint16_t datalen)
{
	for(uint16_t i = 0; i < datalen; i++)
	{
		SPI1_ReadWrite_Byte(wdata[i]);     // 发送每个字节数据
	}
}

/**
 * @brief SPI1批量读取函数
 * @param rdata 读取数据缓冲区指针
 * @param datalen 要读取的数据长度
 * @return 无
 * 
 * 连续接收多个字节数据
 * 读取时发送0xFF作为时钟信号
 */
void SPI1_Read(uint8_t *rdata, uint16_t datalen)
{
	for(uint16_t i = 0; i < datalen; i++)
	{
		rdata[i] = SPI1_ReadWrite_Byte(0xFF);  // 发送0xFF并接收数据
	}
}


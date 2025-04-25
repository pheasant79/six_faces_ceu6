/***************************************************************************************
  * 本代码由江协科技编写，免费开源代码
  * 用户可以查看、使用和修改，并应用到自己的项目之中
  * 版权归江协科技所有，任何人或组织不得将其用于商业
  * 
  * 程序名称：				0.96寸OLED显示屏驱动程序（4针I2C接口）
  * 程序创建时间：			2023.10.24
  * 当前程序版本：			V2.0
  * 当前版本完成时间：		2024.10.20
  * 
  * 江协科技官方网站：		jiangxiekeji.com
  * 江协科技官方淘宝店：	jiangxiekeji.taobao.com
  * 程序更新及教程：	    jiangxiekeji.com/tutorial/oled.html
  * 
  * 如您发现程序中的漏洞或错误，可通过邮件向我们反馈：feedback@jiangxiekeji.com
  * 发送邮件之前，请先到更新教程页面查看最新程序，您所遇到的问题可能已经修复，无需再发邮件
  ***************************************************************************************
  */

#include "main.h"
#include "OLED.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/**
  * 数据存储格式：
  * 8位，位宽为1
  * 每一位Bit对应一个像素
  * 
  *      B0 B0                  B0 B0
  *      B1 B1                  B1 B1
  *      B2 B2                  B2 B2
  *      B3 B3 ------------->  B3 B3 --
  *      B4 B4                  B4 B4  |
  *      B5 B5                  B5 B5  |
  *      B6 B6                  B6 B6  |
  *      B7 B7                  B7 B7  |
  *                                    |
  *  -----------------------------------
  *  |   
  *  |   B0 B0                  B0 B0
  *  |   B1 B1                  B1 B1
  *  |   B2 B2                  B2 B2
  *  --> B3 B3 ------------->  B3 B3
  *      B4 B4                  B4 B4
  *      B5 B5                  B5 B5
  *      B6 B6                  B6 B6
  *      B7 B7                  B7 B7
  * 
  * 屏幕坐标：
  * 左上角为(0, 0)
  * X轴范围：0~127
  * Y轴范围：0~63
  * 
  *       0             X轴           127 
  *      .------------------------------->
  *    0 |
  *      |
  *      |
  *      |
  *  Y轴 |
  *      |
  *      |
  *      |
  *   63 |
  *      v
  * 
  */


/*全屏显示*********************/
uint8_t AniFlag=0;//动画标志位,只执行一次

/**
  * OLED数据更新
  * 显示内容仅限于OLED_DisplayBuf
  * 使用OLED_Update和OLED_UpdateArea函数
  * 将数据写入OLED显示缓冲区
  */
uint8_t OLED_DisplayBuf[8][128];

/*********************全屏显示*/


/*显示操作*********************/

/**
  * 写OLED的SCL引脚
  * 需要写入SCL的电平值范围：0/1
  * 参数：
  * 说明：在需要写入SCL时，调用此函数，函数会自动拉低SCL电平
  *           用户不需要手动拉低SCL电平
  */
void OLED_W_SCL(uint8_t BitValue)
{
	/*BitValue为SCL的电平值，0为低电平，1为高电平*/
	HAL_GPIO_WritePin(GPIOA, OLED_SCL_Pin, (GPIO_PinState)BitValue);
	
	/*使用HAL库的延时函数，模拟I2C的SCL时钟信号*/
	//...
}

/**
  * 写OLED的SDA引脚
  * 需要写入SDA的电平值范围：0/1
  * 参数：
  * 说明：在需要写入SDA时，调用此函数，函数会自动拉低SDA电平
  *           用户不需要手动拉低SDA电平
  */
void OLED_W_SDA(uint8_t BitValue)
{
	/*BitValue为SDA的电平值，0为低电平，1为高电平*/
	HAL_GPIO_WritePin(GPIOA, OLED_SDA_Pin, (GPIO_PinState)BitValue);
	
	/*使用HAL库的延时函数，模拟I2C的SDA信号*/
	//...
}

/**
  * 初始化OLED
  * 参数：
  * 说明：在初始化OLED之前，需要先拉高OLED的电源电压
  */
void OLED_GPIO_Init(void)
{
	
	/*在初始化之前，先延时一段时间，等待OLED启动*/
	HAL_Delay(10);
	
	
	/*拉高SCL和SDA*/
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/*********************显示操作*/


/*通信操作*********************/

/**
  * 开始I2C通信
  * 参数：
  * 说明：在开始I2C通信之前，需要先拉高OLED的SCL电平
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);		//拉高SDA，确保SDA为高电平
	OLED_W_SCL(1);		//拉高SCL，确保SCL为高电平
	OLED_W_SDA(0);		//在SCL高电平期间，拉低SDA，产生起始信号
	OLED_W_SCL(0);		//SCL高电平期间，SCL保持低电平，为保持状态，也是I2C的应答信号
}

/**
  * 停止I2C通信
  * 参数：
  * 说明：在停止I2C通信之前，需要先拉低OLED的SDA电平
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);		//拉低SDA，确保SDA为低电平
	OLED_W_SCL(1);		//拉高SCL，释放SCL总线
	OLED_W_SDA(1);		//在SCL高电平期间，拉高SDA，产生停止信号
}

/**
  * 发送一个字节
  * 参数：
  * 说明：需要发送一个字节的数据，数据范围：0x00~0xFF
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	
	/*循环8次，发送每一位*/
	for (i = 0; i < 8; i++)
	{
		/*使用按位与运算，将Byte的每一位取出，写入SDA*/
		/*如果为0，则SDA为低电平，如果为1，则SDA为高电平*/
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);	//拉高SCL，从SCL输出SDA
		OLED_W_SCL(0);	//拉低SCL，准备发送下一位
	}
	
	OLED_W_SCL(1);		//保持一个时钟周期，等待应答信号
	OLED_W_SCL(0);
}

/**
  * 写命令
  * 参数：
  * 说明：需要写入一个命令，命令范围：0x00~0xFF
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();				//I2C开始
	OLED_I2C_SendByte(0x78);		//发送OLED的I2C地址
	OLED_I2C_SendByte(0x00);		//发送空字节，表示写命令
	OLED_I2C_SendByte(Command);		//发送命令
	OLED_I2C_Stop();				//I2C停止
}

/**
  * 写数据
  * 参数：
  * 说明：需要写入数据，起始地址
  * 参数：
  * 说明：需要写入的数据长度
  */
void OLED_WriteData(uint8_t *Data, uint8_t Count)
{
	uint8_t i;
	
	OLED_I2C_Start();				//I2C开始
	OLED_I2C_SendByte(0x78);		//发送OLED的I2C地址
	OLED_I2C_SendByte(0x40);		//发送空字节，表示写数据
	/*循环Count次，发送数据*/
	for (i = 0; i < Count; i ++)
	{
		OLED_I2C_SendByte(Data[i]);	//发送数据
	}
	OLED_I2C_Stop();				//I2C停止
}

/*********************通信操作*/


/*显示操作*********************/

/**
  * 初始化OLED
  * 参数：
  * 说明：在初始化OLED之前，需要先拉高OLED的电源电压
  */
void OLED_Init(void)
{
	OLED_GPIO_Init();			//使用HAL库的GPIO初始化函数，初始化OLED的GPIO引脚
	
	/*发送一系列命令，初始化OLED*/
	OLED_WriteCommand(0xAE);	//关闭显示/显示
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);	//0x00~0xFF
	
	OLED_WriteCommand(0xA8);	//设置驱动路数
	OLED_WriteCommand(0x3F);	//0x0E~0x3F
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);	//0x00~0x7F
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向
	
	OLED_WriteCommand(0xC8);	//设置上下方向

	OLED_WriteCommand(0xDA);	//设置COM引脚配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度
	OLED_WriteCommand(0xCF);	//0x00~0xFF

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH电平
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置显示/关闭

	OLED_WriteCommand(0xA6);	//设置显示/关闭

	OLED_WriteCommand(0x8D);	//设置电荷泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//设置显示
	
	OLED_Clear();				//清除显示
	OLED_Update();				//更新显示，显示缓冲区中的内容
}

/**
  * 设置光标位置
  * 参数：
  * 说明：Page表示页数，范围：0~7
  * 参数：
  * 说明：X表示列数，范围：0~127
  * 参数：
  * 说明：OLED默认的Y轴，每8位Bit表示一行，共8行
  */
void OLED_SetCursor(uint8_t Page, uint8_t X)
{
	/*使用当前的显示位置，注意OLED的显示位置是从左到右，从上到下*/
	/*因为OLED的显示芯片是SH1106，每页有132列*/
	/*这里假设OLED的显示位置是从第2行开始，即第0行是第1行*/
	/*所以需要将X加上2*/
//	X += 2;
	
	/*通过命令设置显示位置*/
	OLED_WriteCommand(0xB0 | Page);					//设置页地址
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置列地址高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置列地址低4位
}

/*********************显示操作*/


/*显示操作*********************/

/**
  * 画点
  * 参数：
  * 说明：X表示列数，范围：0~127
  * 参数：
  * 说明：Y表示行数，范围：0~63
  * 说明：X和Y表示要画的点的坐标
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//默认值为1
	while (Y --)			//循环Y次
	{
		Result *= X;		//每次乘以X
	}
	return Result;
}

/**
  * 判断点是否在多边形内
  * 参数：
  * 说明：nvert表示多边形的顶点数
  * 参数：
  * 说明：vertx和verty表示多边形的顶点坐标数组
  * 参数：
  * 说明：testx和testy表示要测试的点的坐标
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
	int16_t i, j, c = 0;
	
	/*使用W. Randolph Franklin的算法*/
	/*参考：https://wrfranklin.org/Research/Short_Notes/pnpoly.html*/
	for (i = 0, j = nvert - 1; i < nvert; j = i++)
	{
		if (((verty[i] > testy) != (verty[j] > testy)) &&
			(testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
		{
			c = !c;
		}
	}
	return c;
}

/**
  * 判断点是否在圆内
  * 参数：
  * 说明：X表示圆心的X坐标，范围：-32768~32767
  * 参数：
  * 说明：Y表示圆心的Y坐标，范围：-32768~32767
  * 参数：
  * 说明：Radius表示圆的半径，范围：0~255
  * 参数：
  * 说明：IsFilled表示是否填充圆
  * 返回值：
  * 说明：如果点在圆内，返回1；否则返回0
  */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
	int16_t PointAngle;
	PointAngle = atan2(Y, X) / 3.14 * 180;	//计算角度
	if (StartAngle < EndAngle)	//起始角度小于结束角度
	{
		/*角度在起始角度和结束角度之间，判断点是否在圆内*/
		if (PointAngle >= StartAngle && PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	else			//起始角度大于结束角度
	{
		/*角度在起始角度和结束角度之外，判断点是否在圆内*/
		if (PointAngle >= StartAngle || PointAngle <= EndAngle)
		{
			return 1;
		}
	}
	return 0;		//如果点不在圆内，返回0
}

/*********************显示操作*/


/*显示操作*********************/

/**
  * 更新OLED显示
  * 参数：
  * 说明：将OLED_DisplayBuf中的内容显示到OLED上
  */
void OLED_Update(void)
{
	uint8_t j;
	/*循环每一页*/
	for (j = 0; j < 8; j ++)
	{
		/*使用当前的显示位置，注意OLED的显示位置是从左到右，从上到下*/
		OLED_SetCursor(j, 0);
		/*发送128个数据，将OLED_DisplayBuf中的内容显示到OLED上*/
		OLED_WriteData(OLED_DisplayBuf[j], 128);
	}
}

/**
  * 更新OLED显示区域
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：Width表示宽度，范围：0~128
  * 参数：
  * 说明：Height表示高度，范围：0~64
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t j;
	int16_t Page, Page1;
	
	/*在设置显示位置时，需要考虑偏移*/
	/*(Y + Height - 1) / 8 + 1表示总页数*/
	Page = Y / 8;
	Page1 = (Y + Height - 1) / 8 + 1;
	if (Y < 0)
	{
		Page -= 1;
		Page1 -= 1;
	}
	
	/*设置显示位置*/
	for (j = Page; j < Page1; j ++)
	{
		if (X >= 0 && X <= 127 && j >= 0 && j <= 7)		//如果点在显示范围内
		{
			/*使用当前的显示位置，注意OLED的显示位置是从左到右，从上到下*/
			OLED_SetCursor(j, X);
			/*发送Width个数据，将OLED_DisplayBuf中的内容显示到OLED上*/
			OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
		}
	}
}

/**
  * 清除OLED显示
  * 参数：
  * 说明：清除OLED_DisplayBuf中的所有内容
  */
void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//循环8页
	{
		for (i = 0; i < 128; i ++)			//循环128列
		{
			OLED_DisplayBuf[j][i] = 0x00;	//清除显示缓冲区中的内容
		}
	}
}

/**
  * 清除OLED显示区域
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：Width表示宽度，范围：0~128
  * 参数：
  * 说明：Height表示高度，范围：0~64
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;
	
	for (j = Y; j < Y + Height; j ++)		//循环行数
	{
		for (i = X; i < X + Width; i ++)	//循环列数
		{
			if (i >= 0 && i <= 127 && j >=0 && j <= 63)				//如果点在显示范围内
			{
				OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));	//清除显示缓冲区中的内容
			}
		}
	}
}

/**
  * 反转OLED显示
  * 参数：
  * 说明：反转OLED_DisplayBuf中的所有内容
  */
void OLED_Reverse(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j ++)				//循环8页
	{
		for (i = 0; i < 128; i ++)			//循环128列
		{
			OLED_DisplayBuf[j][i] ^= 0xFF;	//反转显示缓冲区中的内容
		}
	}
}
	
/**
  * 反转OLED显示区域
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：Width表示宽度，范围：0~128
  * 参数：
  * 说明：Height表示高度，范围：0~64
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
	int16_t i, j;
	
	for (j = Y; j < Y + Height; j ++)		//循环行数
	{
		for (i = X; i < X + Width; i ++)	//循环列数
		{
			if (i >= 0 && i <= 127 && j >=0 && j <= 63)			//如果点在显示范围内
			{
				OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);	//反转显示缓冲区中的内容
			}
		}
	}
}

/**
  * 显示一个字符
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Char表示要显示的字符，范围：ASCII码
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
	if (FontSize == OLED_8X16)		//如果是8*16字体
	{
		/*使用OLED_ShowImage函数显示8*16的图片*/
		OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
	}
	else if(FontSize == OLED_6X8)	//如果是6*8字体
	{
		/*使用OLED_ShowImage函数显示6*8的图片*/
		OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
	}
}

/**
  * 显示字符串
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：String表示要显示的字符串，范围：ASCII码
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
	uint16_t i = 0;
	char SingleChar[5];
	uint8_t CharLength = 0;
	uint16_t XOffset = 0;
	uint16_t pIndex;
	
	while (String[i] != '\0')	//字符串结束符
	{
		
#ifdef OLED_CHARSET_UTF8						//字符串为UTF8
		/*获取UTF8字符串的第一个字符，转换为单个字符，存储在SingleChar中*/
		/*判断UTF8字符串的第一个字节是否为0xxxxxxx*/
		if ((String[i] & 0x80) == 0x00)			//第一个字节为0xxxxxxx
		{
			CharLength = 1;						//字符为1个字节
			SingleChar[0] = String[i ++];		//将第一个字节存储在SingleChar的0位置，i指向下一个字节
			SingleChar[1] = '\0';				//为SingleChar字符串添加结束符
		}
		else if ((String[i] & 0xE0) == 0xC0)	//第一个字节为110xxxxx
		{
			CharLength = 2;						//字符为2个字节
			SingleChar[0] = String[i ++];		//将第一个字节存储在SingleChar的0位置，i指向下一个字节
			if (String[i] == '\0') {break;}		//如果字符串结束，则退出循环
			SingleChar[1] = String[i ++];		//将第二个字节存储在SingleChar的1位置，i指向下一个字节
			SingleChar[2] = '\0';				//为SingleChar字符串添加结束符
		}
		else if ((String[i] & 0xF0) == 0xE0)	//第一个字节为1110xxxx
		{
			CharLength = 3;						//字符为3个字节
			SingleChar[0] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[1] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[2] = String[i ++];
			SingleChar[3] = '\0';
		}
		else if ((String[i] & 0xF8) == 0xF0)	//第一个字节为11110xxx
		{
			CharLength = 4;						//字符为4个字节
			SingleChar[0] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[1] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[2] = String[i ++];
			if (String[i] == '\0') {break;}
			SingleChar[3] = String[i ++];
			SingleChar[4] = '\0';
		}
		else
		{
			i ++;			//如果遇到非UTF8字符，则跳过这个字节，继续处理下一个字节
			continue;
		}
#endif
		
#ifdef OLED_CHARSET_GB2312						//字符串为GB2312
		/*获取GB2312字符串的第一个字符，转换为单个字符，存储在SingleChar中*/
		/*判断GB2312字节的最高位是否为0*/
		if ((String[i] & 0x80) == 0x00)			//最高位为0
		{
			CharLength = 1;						//字符为1个字节
			SingleChar[0] = String[i ++];		//将第一个字节存储在SingleChar的0位置，i指向下一个字节
			SingleChar[1] = '\0';				//为SingleChar字符串添加结束符
		}
		else									//最高位为1
		{
			CharLength = 2;						//字符为2个字节
			SingleChar[0] = String[i ++];		//将第一个字节存储在SingleChar的0位置，i指向下一个字节
			if (String[i] == '\0') {break;}		//如果字符串结束，则退出循环
			SingleChar[1] = String[i ++];		//将第二个字节存储在SingleChar的1位置，i指向下一个字节
			SingleChar[2] = '\0';				//为SingleChar字符串添加结束符
		}
#endif
		
		/*显示字符*/
		if (CharLength == 1)	//如果是单个字节
		{
			/*使用OLED_ShowChar函数显示字符*/
			OLED_ShowChar(X + XOffset, Y, SingleChar[0], FontSize);
			XOffset += FontSize;
		}
		else					//如果是多个字节
		{
			/*使用OLED_ShowImage函数显示多个字节的图片*/
			/*使用OLED_CF16x16数组中的数据，因为OLED_CF16x16数组中的数据是按字符顺序排列的*/
			/*如果找不到对应的字符，则停止查找，避免死循环*/
			for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex ++)
			{
				/*匹配字符*/
				if (strcmp(OLED_CF16x16[pIndex].Index, SingleChar) == 0)
				{
					break;		//循环结束时，pIndex为匹配到的字符串的索引
				}
			}
			if (FontSize == OLED_8X16)		//如果是8*16字体
			{
				/*使用OLED_CF16x16数组中的数据，显示16*16的图片*/
				OLED_ShowImage(X + XOffset, Y, 16, 16, OLED_CF16x16[pIndex].Data);
				XOffset += 16;
			}
			else if (FontSize == OLED_6X8)	//如果是6*8字体
			{
				/*使用OLED_ShowChar函数显示'?'*/
				OLED_ShowChar(X + XOffset, Y, '?', OLED_6X8);
				XOffset += OLED_6X8;
			}
		}
	}
}

/**
  * 显示数字
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Number表示要显示的数字，范围：0~4294967295
  * 参数：
  * 说明：Length表示数字的长度，范围：0~10
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//循环每个数字							
	{
		/*使用OLED_ShowChar函数显示每个数字*/
		/*Number / OLED_Pow(10, Length - i - 1) % 10 表示取每个数字*/
		/*+ '0' 转换为字符串格式*/
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * 显示带符号的数字
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Number表示要显示的数字，范围：-2147483648~2147483647
  * 参数：
  * 说明：Length表示数字的长度，范围：0~10
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	uint32_t Number1;
	
	if (Number >= 0)						//如果数字大于等于0
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//显示+号
		Number1 = Number;					//Number1直接赋值给Number
	}
	else									//如果数字小于0
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//显示-号
		Number1 = -Number;					//Number1取负数
	}
	
	for (i = 0; i < Length; i++)			//循环每个数字								
	{
		/*使用OLED_ShowChar函数显示每个数字*/
		/*Number1 / OLED_Pow(10, Length - i - 1) % 10 表示取每个数字*/
		/*+ '0' 转换为字符串格式*/
		OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
	}
}

/**
  * 显示十六进制数字
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Number表示要显示的数字，范围：0x00000000~0xFFFFFFFF
  * 参数：
  * 说明：Length表示数字的长度，范围：0~8
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)		//循环每个数字
	{
		/*取十六进制数字的每一位*/
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		
		if (SingleNumber < 10)			//如果数字小于10
		{
			/*使用OLED_ShowChar函数显示数字*/
			/*+ '0' 转换为字符串格式*/
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
		}
		else							//如果数字大于等于10
		{
			/*使用OLED_ShowChar函数显示字母*/
			/*+ 'A' 转换为大写字母*/
			OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
		}
	}
}

/**
  * 显示二进制数字
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Number表示要显示的数字，范围：0x00000000~0xFFFFFFFF
  * 参数：
  * 说明：Length表示数字的长度，范围：0~16
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
	uint8_t i;
	for (i = 0; i < Length; i++)		//循环每个数字	
	{
		/*使用OLED_ShowChar函数显示每个数字*/
		/*Number / OLED_Pow(2, Length - i - 1) % 2 表示取每个数字*/
		/*+ '0' 转换为字符串格式*/
		OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
	}
}

/**
  * 显示浮点数
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 参数：
  * 说明：Number表示要显示的数字，范围：-4294967295.0~4294967295.0
  * 参数：
  * 说明：IntLength表示整数部分的长度，范围：0~10
  * 参数：
  * 说明：FraLength表示小数部分的长度，范围：0~9，小数点后有几位小数
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：OLED_8X16表示8*16字体，OLED_6X8表示6*8字体
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
	uint32_t PowNum, IntNum, FraNum;
	
	if (Number >= 0)						//如果数字大于等于0
	{
		OLED_ShowChar(X, Y, '+', FontSize);	//显示+号
	}
	else									//如果数字小于0
	{
		OLED_ShowChar(X, Y, '-', FontSize);	//显示-号
		Number = -Number;					//Number取负数
	}
	
	/*取整数和小数部分*/
	IntNum = Number;						//直接赋值给IntNum
	Number -= IntNum;						//Number减去整数部分，得到小数部分
	PowNum = OLED_Pow(10, FraLength);		//计算小数部分的位数
	FraNum = round(Number * PowNum);		//将小数部分转换为整数，并加上小数点后的数字
	IntNum += FraNum / PowNum;				//将小数部分转换为整数，并加上小数点后的数字
	
	/*显示数字*/
	OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);
	
	/*显示小数点*/
	OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);
	
	/*显示小数部分*/
	OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

/**
  * 显示图片
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：Width表示宽度，范围：0~128
  * 参数：
  * 说明：Height表示高度，范围：0~64
  * 参数：
  * 说明：Image表示要显示的图片
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
	uint8_t i = 0, j = 0;
	int16_t Page, Shift;
	
	/*清除显示区域*/
	OLED_ClearArea(X, Y, Width, Height);
	
	/*设置图片的显示位置*/
	/*(Height - 1) / 8 + 1表示总页数*/
	for (j = 0; j < (Height - 1) / 8 + 1; j ++)
	{
		/*设置图片的显示位置*/
		for (i = 0; i < Width; i ++)
		{
			if (X + i >= 0 && X + i <= 127)		//如果点在显示范围内
			{
				/*在设置显示位置时，需要考虑偏移*/
				Page = Y / 8;
				Shift = Y % 8;
				if (Y < 0)
				{
					Page -= 1;
					Shift += 8;
				}
				
				if (Page + j >= 0 && Page + j <= 7)		//如果点在显示范围内
				{
					/*显示图片*/
					OLED_DisplayBuf[Page + j][X + i] |= Image[j * Width + i] << (Shift);
				}
				
				if (Page + j + 1 >= 0 && Page + j + 1 <= 7)		//如果点在显示范围内
				{					
					/*显示图片*/
					OLED_DisplayBuf[Page + j + 1][X + i] |= Image[j * Width + i] >> (8 - Shift);
				}
			}
		}
	}
}

/**
  * 使用printf函数显示字符串
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：FontSize表示字体大小
  * 参数：
  * 说明：format表示要显示的格式字符串，范围：ASCII码
  * 参数：
  * 说明：...表示可变参数列表
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
	char String[256];						//定义字符串数组
	va_list arg;							//定义可变参数列表
	va_start(arg, format);					//从format开始，获取可变参数列表
	vsprintf(String, format, arg);			//使用vsprintf函数格式化字符串
	va_end(arg);							//结束可变参数列表
	OLED_ShowString(X, Y, String, FontSize);//使用OLED_ShowString函数显示字符串
}

/**
  * 画点
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//如果点在显示范围内
	{
		/*使用OLED_DisplayBuf中的数据，设置相应的位*/
		OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
	}
}

/**
  * 获取点
  * 参数：
  * 说明：X表示列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示行数，范围：-32768~32767
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
	if (X >= 0 && X <= 127 && Y >=0 && Y <= 63)		//如果点在显示范围内
	{
		/*判断点是否被设置*/
		if (OLED_DisplayBuf[Y / 8][X] & 0x01 << (Y % 8))
		{
			return 1;	//如果点被设置，返回1
		}
	}
	
	return 0;		//如果点不在显示范围内，返回0
}

/**
  * 画线
  * 参数：
  * 说明：X0表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y0表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：X1表示结束列数，范围：-32768~32767
  * 参数：
  * 说明：Y1表示结束行数，范围：-32768~32767
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
	int16_t x, y, dx, dy, d, incrE, incrNE, temp;
	int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
	uint8_t yflag = 0, xyflag = 0;
	
	if (y0 == y1)		//垂直线
	{
		/*0行X0和X1，取小值*/
		if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}
		
		/*画垂直线*/
		for (x = x0; x <= x1; x ++)
		{
			OLED_DrawPoint(x, y0);	//画点
		}
	}
	else if (x0 == x1)	//水平线
	{
		/*0行Y0和Y1，取小值*/
		if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}
		
		/*画水平线*/
		for (y = y0; y <= y1; y ++)
		{
			OLED_DrawPoint(x0, y);	//画点
		}
	}
	else				//斜线
	{
		/*使用Bresenham算法画直线，提高效率*/
		/*参考：https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
		/*参考：https://www.bilibili.com/video/BV1364y1d7Lo*/
		
		if (x0 > x1)	//0行X0和X1，交换X0和X1
		{
			/*交换X0和X1*/
			temp = x0; x0 = x1; x1 = temp;
			temp = y0; y0 = y1; y1 = temp;
		}
		
		if (y0 > y1)	//0行Y0和Y1，交换Y0和Y1
		{
			/*交换Y0和Y1*/
			y0 = -y0;
			y1 = -y1;
			
			/*使用yflag标志位记录当前的旋转方向*/
			yflag = 1;
		}
		
		if (y1 - y0 > x1 - x0)	//斜率大于1
		{
			/*交换X0和Y0*/
			temp = x0; x0 = y0; y0 = temp;
			temp = x1; x1 = y1; y1 = temp;
			
			/*使用xyflag标志位记录当前的旋转方向*/
			xyflag = 1;
		}
		
		/*使用Bresenham算法画直线*/
		/*算法需要从0~45度进行计算*/
		dx = x1 - x0;
		dy = y1 - y0;
		incrE = 2 * dy;
		incrNE = 2 * (dy - dx);
		d = 2 * dy - dx;
		x = x0;
		y = y0;
		
		/*初始化，同时判断标志位*/
		if (yflag && xyflag){OLED_DrawPoint(y, -x);}
		else if (yflag)		{OLED_DrawPoint(x, -y);}
		else if (xyflag)	{OLED_DrawPoint(y, x);}
		else				{OLED_DrawPoint(x, y);}
		
		while (x < x1)		//X轴方向上循环
		{
			x ++;
			if (d < 0)		//当前点在斜线左侧
			{
				d += incrE;
			}
			else			//当前点在斜线右侧
			{
				y ++;
				d += incrNE;
			}
			
			/*每次循环，同时判断标志位*/
			if (yflag && xyflag){OLED_DrawPoint(y, -x);}
			else if (yflag)		{OLED_DrawPoint(x, -y);}
			else if (xyflag)	{OLED_DrawPoint(y, x);}
			else				{OLED_DrawPoint(x, y);}
		}	
	}
}

/**
  * 画矩形
  * 参数：
  * 说明：X表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：Width表示宽度，范围：0~128
  * 参数：
  * 说明：Height表示高度，范围：0~64
  * 参数：
  * 说明：IsFilled表示是否填充
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
	int16_t i, j;
	if (!IsFilled)		//不填充
	{
		/*画X轴方向上的线*/
		for (i = X; i < X + Width; i ++)
		{
			OLED_DrawPoint(i, Y);
			OLED_DrawPoint(i, Y + Height - 1);
		}
		/*画Y轴方向上的线*/
		for (i = Y; i < Y + Height; i ++)
		{
			OLED_DrawPoint(X, i);
			OLED_DrawPoint(X + Width - 1, i);
		}
	}
	else				//填充
	{
		/*画X轴方向上的线*/
		for (i = X; i < X + Width; i ++)
		{
			/*画Y轴方向上的线*/
			for (j = Y; j < Y + Height; j ++)
			{
				/*画点*/
				OLED_DrawPoint(i, j);
			}
		}
	}
}

/**
  * 画三角形
  * 参数：
  * 说明：X0表示起始列数，范围：-32768~32767
  * 参数：
  * 说明：Y0表示起始行数，范围：-32768~32767
  * 参数：
  * 说明：X1表示结束列数，范围：-32768~32767
  * 参数：
  * 说明：Y1表示结束行数，范围：-32768~32767
  * 参数：
  * 说明：X2表示结束列数，范围：-32768~32767
  * 参数：
  * 说明：Y2表示结束行数，范围：-32768~32767
  * 参数：
  * 说明：IsFilled表示是否填充
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
	int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
	int16_t i, j;
	int16_t vx[] = {X0, X1, X2};
	int16_t vy[] = {Y0, Y1, Y2};
	
	if (!IsFilled)			//不填充
	{
		/*画三条边*/
		OLED_DrawLine(X0, Y0, X1, Y1);
		OLED_DrawLine(X0, Y0, X2, Y2);
		OLED_DrawLine(X1, Y1, X2, Y2);
	}
	else					//填充
	{
		/*找到最小和最大的X和Y坐标*/
		if (X1 < minx) {minx = X1;}
		if (X2 < minx) {minx = X2;}
		if (Y1 < miny) {miny = Y1;}
		if (Y2 < miny) {miny = Y2;}
		
		/*找到最大和最小的X和Y坐标*/
		if (X1 > maxx) {maxx = X1;}
		if (X2 > maxx) {maxx = X2;}
		if (Y1 > maxy) {maxy = Y1;}
		if (Y2 > maxy) {maxy = Y2;}
		
		/*计算矩形的宽度和高度*/
		/*需要画矩形的宽度*/
		/*需要画矩形的高度*/
		/*使用OLED_pnpoly函数判断点是否在多边形内*/
		/*如果点在多边形内，则画点*/
		for (i = minx; i <= maxx; i ++)
		{
			/*使用OLED_pnpoly函数判断点是否在多边形内*/
			/*如果点在多边形内，则画点*/
			for (j = miny; j <= maxy; j ++)
			{
				if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j);}
			}
		}
	}
}

/**
  * 画圆
  * 参数：
  * 说明：X表示圆心的X坐标，范围：-32768~32767
  * 参数：
  * 说明：Y表示圆心的Y坐标，范围：-32768~32767
  * 参数：
  * 说明：Radius表示圆的半径，范围：0~255
  * 参数：
  * 说明：IsFilled表示是否填充
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	/*使用Bresenham算法画圆，提高效率*/
	/*参考：https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf*/
	/*参考：https://www.bilibili.com/video/BV1VM4y1u7wJ*/
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	/*每次画圆时，从圆的下半部分开始*/
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X + y, Y + x);
	OLED_DrawPoint(X - y, Y - x);
	
	if (IsFilled)		//填充圆
	{
		/*从Y轴的下半部分开始*/
		for (j = -y; j < y; j ++)
		{
			/*画点*/
			OLED_DrawPoint(X, Y + j);
		}
	}
	
	while (x < y)		//X轴方向上循环
	{
		x ++;
		if (d < 0)		//当前点在圆的左侧
		{
			d += 2 * x + 1;
		}
		else			//当前点在圆的右侧
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/*每次画圆时，画圆的四个象限*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X + y, Y + x);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - y, Y - x);
		OLED_DrawPoint(X + x, Y - y);
		OLED_DrawPoint(X + y, Y - x);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X - y, Y + x);
		
		if (IsFilled)	//填充圆
		{
			/*画圆的下半部分*/
			for (j = -y; j < y; j ++)
			{
				/*画点*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
			
			/*画圆的上半部分*/
			for (j = -x; j < x; j ++)
			{
				/*画点*/
				OLED_DrawPoint(X - y, Y + j);
				OLED_DrawPoint(X + y, Y + j);
			}
		}
	}
}

/**
  * 画椭圆
  * 参数：
  * 说明：X表示椭圆中心的X坐标，范围：-32768~32767
  * 参数：
  * 说明：Y表示椭圆中心的Y坐标，范围：-32768~32767
  * 参数：
  * 说明：A表示椭圆水平方向的半径，范围：0~255
  * 参数：
  * 说明：B表示椭圆垂直方向的半径，范围：0~255
  * 参数：
  * 说明：IsFilled表示是否填充
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
	int16_t x, y, j;
	int16_t a = A, b = B;
	float d1, d2;
	
	/*使用Bresenham算法画椭圆，提高效率*/
	/*参考：https://blog.csdn.net/myf_666/article/details/128167392*/
	
	x = 0;
	y = b;
	d1 = b * b + a * a * (-b + 0.5);
	
	if (IsFilled)	//填充椭圆
	{
		/*从Y轴的下半部分开始*/
		for (j = -y; j < y; j ++)
		{
			/*画点*/
			OLED_DrawPoint(X, Y + j);
			OLED_DrawPoint(X, Y + j);
		}
	}
	
	/*椭圆开始*/
	OLED_DrawPoint(X + x, Y + y);
	OLED_DrawPoint(X - x, Y - y);
	OLED_DrawPoint(X - x, Y + y);
	OLED_DrawPoint(X + x, Y - y);
	
	/*椭圆下半部分*/
	while (b * b * (x + 1) < a * a * (y - 0.5))
	{
		if (d1 <= 0)		//当前点在椭圆左侧
		{
			d1 += b * b * (2 * x + 3);
		}
		else				//当前点在椭圆右侧
		{
			d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
			y --;
		}
		x ++;
		
		if (IsFilled)	//填充椭圆
		{
			/*画椭圆的下半部分*/
			for (j = -y; j < y; j ++)
			{
				/*画点*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}
		
		/*椭圆下半部分椭圆*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
	
	/*椭圆上半部分*/
	d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
	
	while (y > 0)
	{
		if (d2 <= 0)		//当前点在椭圆左侧
		{
			d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
			x ++;
			
		}
		else				//当前点在椭圆右侧
		{
			d2 += a * a * (-2 * y + 3);
		}
		y --;
		
		if (IsFilled)	//填充椭圆
		{
			/*画椭圆的上半部分*/
			for (j = -y; j < y; j ++)
			{
				/*画点*/
				OLED_DrawPoint(X + x, Y + j);
				OLED_DrawPoint(X - x, Y + j);
			}
		}
		
		/*椭圆上半部分椭圆*/
		OLED_DrawPoint(X + x, Y + y);
		OLED_DrawPoint(X - x, Y - y);
		OLED_DrawPoint(X - x, Y + y);
		OLED_DrawPoint(X + x, Y - y);
	}
}

/**
  * 画圆弧
  * 参数：
  * 说明：X表示圆心的X坐标，范围：-32768~32767
  * 参数：
  * 说明：Y表示圆心的Y坐标，范围：-32768~32767
  * 参数：
  * 说明：Radius表示圆的半径，范围：0~255
  * 参数：
  * 说明：StartAngle表示起始角度，范围：-180~180
  * 参数：
  * 说明：EndAngle表示结束角度，范围：-180~180
  * 参数：
  * 说明：IsFilled表示是否填充
  * 返回值：
  * 说明：如果点在多边形内，返回1；否则返回0
  */
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
	int16_t x, y, d, j;
	
	/*使用Bresenham算法画圆弧，提高效率*/
	
	d = 1 - Radius;
	x = 0;
	y = Radius;
	
	/*每次画圆弧时，判断点是否在圆内*/
	if (OLED_IsInAngle(x, y, StartAngle, EndAngle))	{OLED_DrawPoint(X + x, Y + y);}
	if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
	if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
	if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
	
	if (IsFilled)	//填充圆弧
	{
		/*从Y轴的下半部分开始*/
		for (j = -y; j < y; j ++)
		{
			/*每次画圆弧时，判断点是否在圆内*/
			if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j);}
		}
	}
	
	while (x < y)		//X轴方向上循环
	{
		x ++;
		if (d < 0)		//当前点在圆的左侧
		{
			d += 2 * x + 1;
		}
		else			//当前点在圆的右侧
		{
			y --;
			d += 2 * (x - y) + 1;
		}
		
		/*每次画圆弧时，判断点是否在圆内*/
		if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y);}
		if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
		if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
		if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
		if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y - y);}
		if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x);}
		if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y);}
		if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x);}
		
		if (IsFilled)	//填充圆弧
		{
			/*画圆弧的四个象限*/
			for (j = -y; j < y; j ++)
			{
				/*每次画圆弧时，判断点是否在圆内*/
				if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j);}
				if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j);}
			}
			
			/*画圆弧的上半部分*/
			for (j = -x; j < x; j ++)
			{
				/*每次画圆弧时，判断点是否在圆内*/
				if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j);}
				if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j);}
			}
		}
	}
}

/**
OLED动画
说明：使用OLED_AnimUpdate函数，每帧更新一次

##使用方法
OLED显示内容不变，只需要调用OLED_AnimUpdate函数，每帧更新一次
##注意事项
*****************************************************
需要更新OLED显示，请调用OLED_Update();
*****************************************************

  * #  X1,起始X位置,范围:0-127
  * #  Y1,起始Y位置,范围:0-63
  * #  L1,起始长度,范围:0-127
  * #  W1,起始高度,范围:0-63
  * #  X2,目标X位置,范围:0-127
  * #  Y2,目标X位置,范围:0-63
  * #  L2,目标长度,范围:0-127
  * #  W2,目标高度,范围:0-63
  * # 
  */
void OLED_Animation(uint8_t X1,uint8_t Y1,uint8_t L1,uint8_t W1,uint8_t X2,uint8_t Y2,uint8_t L2,uint8_t W2)
{
	OLED_ReverseArea(X1,Y1,L1,W1);//反转区域
	while(AniFlag == 0)
	{
		/*一帧一帧更新*/
		if((X1 < X2)||(Y1 < Y2)||(L1 < L2)||(W1 < W2))
		{
			OLED_ReverseArea(X1,Y1,L1,W1);//更新一帧，不擦除，同时增加，同时减少
			if(X1 < X2){X1 = X1 + ((X2-X1)/2);}
			if(Y1 < Y2){Y1 = Y1 + ((Y2-Y1)/2);}
			if(L1 < L2){L1 = L1 + ((L2-L1)/2);}
			if(W1 < W2){W1 = W1 + ((W2-W1)/2);}
			
			//第二帧,保持原点,只能增加目标值
			if(X1 == X2-1){X1 = X2;}
			if(Y1 == Y2-1){Y1 = Y2;}
			if(L1 == L2-1){L1 = L2;}
			if(W1 == W2-1){W1 = W2;}
			
			OLED_ReverseArea(X1,Y1,L1,W1);//当前帧
			OLED_Update();
			/*达到目标值，设置标志位，停止循环，进入中断*/
			/*2->11
			1st:2+(11-2)/2=2+4=6;
			2st:6+(11-6)/2=6+2=8;
			3st:8+(11-8)/2=8+1=9;
			4st:9+(11-9)/2=9+1=10;
			5st:10+(11-10)/2=10+0=10;
			即当OLED显示内容达到目标值时，可以停止更新，保持显示内容不变
			*/
			if((X1 == X2)&&(Y1 == Y2)&&(L1 == L2)&&(W1 == W2))
			{
				AniFlag = 1;
				break;
			}

		}
		/*一帧一帧更新*/
		if((X1 > X2)||(Y1 > Y2)||(L1 > L2)||(W1 > W2))
		{
			OLED_ReverseArea(X1,Y1,L1,W1);//更新一帧，不擦除，同时增加，同时减少
			if(X1 > X2){X1 = X1 - ((X1-X2)/2);}
			if(Y1 > Y2){Y1 = Y1 - ((Y1-Y2)/2);}
			if(L1 > L2){L1 = L1 - ((L1-L2)/2);}
			if(W1 > W2){W1 = W1 - ((W1-W2)/2);}
			
			//第二帧,保持原点,只能增加目标值
			if(X1 == X2+1){X1 = X2;}
			if(Y1 == Y2+1){Y1 = Y2;}
			if(L1 == L2+1){L1 = L2;}
			if(W1 == W2+1){W1 = W2;}

			OLED_ReverseArea(X1,Y1,L1,W1);//当前帧
			OLED_Update();
			/*达到目标值，设置标志位，停止循环，进入中断*/
			/*11->2
			1st:11-(11-2)/2=11-4=7;
			2st:7-(7-2)/2=7-2=5;
			3st:5-(5-2)/2=5-1=4;
			4st:4-(4-2)/2=4-1=3;
			5st:3-(3-2)/2=3-0=3;
			即当OLED显示内容达到目标值时，可以停止更新，保持显示内容不变
			*/
			if((X1 == X2)&&(Y1 == Y2)&&(L1 == L2)&&(W1 == W2))
			{
				AniFlag = 1;
				break;
			}
		}
	}		
}

/**
  * @brief 更新动画标志位,设置为1
  * @param  
  * @retval 
  */
void OLED_AnimUpdate(void)
{
	AniFlag = 0;
}
/*********************显示操作*/

/*****************OLED|版权****************/
/*****************jiangxiekeji.com*****************/

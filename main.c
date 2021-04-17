#include <REG52.H>
#include <ABSACC.H>
#include <ctype.h>

//全局变量
unsigned char DispBuf[8];	//定义显示缓冲区（由定时中断程序自动扫描）
unsigned long T0Count = 0;	//T1定时器定时时间内，T0计数器记录脉冲数
unsigned long spill = 0;	//记录一定时间内T0计数器总溢出次数
sbit tube2=P2^2;
sbit tube3=P2^3;
sbit tube4=P2^4;



//函数声明；
void DispClear();
void DispChar(unsigned char location,unsigned char display,bit dp);
void DispNum(unsigned long num, bit gear);
void Nixie(unsigned char Location,unsigned char Number);





/*
	函数：位选以及段选数码管
	参数：
		Location:位选
		Number:段选，高电平点亮
*/
void Nixie(unsigned char Location,unsigned char Number)
{
	P0 = 0x00;		//消影
	switch(Location)
	{
		case 0: tube2=1;tube3=1;tube4=1;break;
		case 1: tube2=0;tube3=1;tube4=1;break;
		case 2: tube2=1;tube3=0;tube4=1;break;
		case 3: tube2=0;tube3=0;tube4=1;break;
		case 4: tube2=1;tube3=1;tube4=0;break;
		case 5: tube2=0;tube3=1;tube4=0;break;
		case 6: tube2=1;tube3=0;tube4=0;break;
		case 7: tube2=0;tube3=0;tube4=0;break;
		default: break;
	}

	P0 = Number;
}

/*
	功能：初始化T0计数器，以及T1定时器
*/
void SysInit()
{
	DispClear();	//初始为全灭
	TR0 = 0;
	TMOD = 0x15;
	TH1 = 0xFC;	//如果计数值存在误差，你需要调整TH1，TL1这两项的数值
	TL1 = 0x19;
	TH0 = 0;
	TL0 = 0;

	EA = 1;
	ET1 = 1;
	TR1 = 1;
	ET0 = 1;
	TR0 = 1;
}



/*
函数：T1INTSVC()
功能：定时器T1的中断服务函数
*/
void T1INTSVC() interrupt 3
{
	//code unsigned char com[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	static unsigned char n = 0;
	static unsigned int T1Count = 0;		//总闸门时间=1ms*T1Count
	static unsigned int LEDCount = 0;		//控制LED亮灭	

	TH1 = 0xFC;		//如果计数值存在误差，你需要调整TH1，TL1这两项的数值
	TL1 = 0x19;		//每次计数到：0xFC19~0xFFFF,耗时1ms.


	LEDCount++;
	T1Count++;
	if(T1Count >= 1000)
	{
		T1Count = 0;
		T0Count = spill*65536+TH0*256+TL0;

		//初始化T0计数器的值
		TH0 = 0;
		TL0 = 0;
		spill = 0;
		
	}


	if(LEDCount < 1000)
	{
		
		P3 = 0x00;				//LED点亮1s
	}
	else if(LEDCount >=1000 && LEDCount <= 2000)
	{
		P3 = 0xff;				//LED熄灭1s
		DispNum(T0Count,0);		//在LED熄灭期间，保存计数值
	}
	else
	{
		LEDCount = 0;
		DispNum(0,0);
		DispClear();		
	}


   	//下面注释部分为AT89C51用法，由于未查到你使用的单片机型号的外部存储器地址，故放弃下面的最佳用法
	//XBYTE[0x7800] = 0xFF;		//暂停显示
	//XBYTE[0x7801] = ~DispBuf[n];	//更新扫描数据
	//XBYTE[0x7800] = ~com[n];	//重新显示

	//更改后的用法
	Nixie(n,DispBuf[n]);	

	n++;
	n &= 0x07;
}


void T0INTSVC() interrupt 1		//T0中断函数
{
	TH0 = 0;		//从0开始计数
	TL0 = 0;		//从0开始计数
	spill++;
}



/*
函数：DispClear()
功能：清除数码管的所有显示
*/
void DispClear()
{
	unsigned char i;
	for(i = 2;i<8;i++)
	{
		DispBuf[i] = 0x00;
	}
}



/*
函数：DispChar()
功能：在数码管上显示字符
参数：
	x：数码管的坐标位置（0～7）
	c：要显示的字符（仅限16进制数字和减号）
	dp：是否显示小数点，0－不显示，1－显示
*/
void DispChar(unsigned char location,unsigned char display,bit dp)
{
    unsigned char Tab[] =
	{//定义0123456789AbCdEF的数码管字型数据
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
    unsigned char temp;	//临时变量
    //防止显示位置超出范围
	location &= 0x07;
	location = 7 - location;
    //分析字符c，取得对应的数码管字型数据
	if ( display == '-' )
	{
		temp = 0x40;
	}
    else
    {
        display = toint(display);	//toint()为库函数，详见C:\Keil\C51\HLP\C51.pdf，十六进制数字检查函数，用于转换形参字符为十六进制数字
        if(display<0xf)
        {
            temp = Tab[display];	//查表，取得数码管字型数据
        }
        else
        {
            temp = 0x00;	//如果是其它字符则显示为空白
        }
    }
    
	//检查是否显示小数点
	if ( dp )
	{
		temp |= 0x80;
	}
	else
	{
		temp &= 0x7F;
	}
    
    DispBuf[location] = temp;      
}


/*
	选择切换以Hz和KHz为单位显示一串数字，最多使用六位数码管
	参数num：要显示的数字
	参数gear：档位切换，0为Hz档，1为KHz档
	频率计显示范围为10Hz~740KHz
*/
void DispNum(unsigned long num, bit gear)
{
    code unsigned char s[] = "0123456789abcdef";
    unsigned char data temp_arr[6] = 0;	//临时数组，最多存放六位数字
    unsigned char i = 0;
    bit dp;	//是否显示小数点
    if(num>=0)
    {
        while (num != 0)
        {
            //拆分数字
            temp_arr[i] = num % 10;
            num =num / 10;
            i++;
        }

        if(gear)
        {
            for(i=0;i<6;i++)
            {
                if(i==3)
                    dp=1;
                else
                    dp=0;
                //KHz档位时，数码管显示数字
                DispChar(7-i,s[temp_arr[i]],dp);
            }        
        }
        else
        {
            for(i=0;i<6;i++)
            {
                //Hz档位时，数码管显示数字
                DispChar(7-i,s[temp_arr[i]],0);
            }
        } 
    }
}



void main()
{

	SysInit();
	for(;;)
	{
	}
}

#include <REG52.H>
#include <ABSACC.H>
#include <ctype.h>

//ȫ�ֱ���
unsigned char DispBuf[8];	//������ʾ���������ɶ�ʱ�жϳ����Զ�ɨ�裩
unsigned long T0Count = 0;	//T1��ʱ����ʱʱ���ڣ�T0��������¼������
unsigned long spill = 0;	//��¼һ��ʱ����T0���������������
sbit tube2=P2^2;
sbit tube3=P2^3;
sbit tube4=P2^4;



//����������
void DispClear();
void DispChar(unsigned char location,unsigned char display,bit dp);
void DispNum(unsigned long num, bit gear);
void Nixie(unsigned char Location,unsigned char Number);





/*
	������λѡ�Լ���ѡ�����
	������
		Location:λѡ
		Number:��ѡ���ߵ�ƽ����
*/
void Nixie(unsigned char Location,unsigned char Number)
{
	P0 = 0x00;		//��Ӱ
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
	���ܣ���ʼ��T0���������Լ�T1��ʱ��
*/
void SysInit()
{
	DispClear();	//��ʼΪȫ��
	TR0 = 0;
	TMOD = 0x15;
	TH1 = 0xFA;
	TL1 = 0x28;
	TH0 = 0;
	TL0 = 0;

	EA = 1;
	ET1 = 1;
	TR1 = 1;
	ET0 = 1;
	TR0 = 1;
}



/*
������T1INTSVC()
���ܣ���ʱ��T1���жϷ�����
*/
void T1INTSVC() interrupt 3
{
	//code unsigned char com[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	static unsigned char n = 0;
	static unsigned int T1Count = 0;		//��բ��ʱ��=1ms*T1Count
	static unsigned int LEDCount = 0;		//����LED����	

	TH1 = 0xFA;
	TL1 = 0x28;		//ÿ�μ�������0xFA28~0xFFFF,��ʱ1ms.


	LEDCount++;
	T1Count++;
	if(T1Count >= 1000)
	{
		T1Count = 0;
		T0Count = spill*65536+TH0*256+TL0;

		//��ʼ��T0��������ֵ
		TH0 = 0;
		TL0 = 0;
		spill = 0;
		
	}


	if(LEDCount < 1000)
	{
		
		P3 = 0x00;				//LED����1s
	}
	else if(LEDCount >=1000 && LEDCount <= 2000)
	{
		P3 = 0xff;				//LEDϨ��1s
		DispNum(T0Count,0);		//��LEDϨ���ڼ䣬�������ֵ
	}
	else
	{
		LEDCount = 0;
		DispNum(0,0);
		DispClear();		
	}


   	//����ע�Ͳ���ΪAT89C51�÷�������δ�鵽��ʹ�õĵ�Ƭ���ͺŵ��ⲿ�洢����ַ���ʷ������������÷�
	//XBYTE[0x7800] = 0xFF;		//��ͣ��ʾ
	//XBYTE[0x7801] = ~DispBuf[n];	//����ɨ������
	//XBYTE[0x7800] = ~com[n];	//������ʾ

	//���ĺ���÷�
	Nixie(n,DispBuf[n]);	

	n++;
	n &= 0x07;
}


void T0INTSVC() interrupt 1		//T0�жϺ���
{
	TH0 = 0;		//��0��ʼ����
	TL0 = 0;		//��0��ʼ����
	spill++;
}



/*
������DispClear()
���ܣ��������ܵ�������ʾ
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
������DispChar()
���ܣ������������ʾ�ַ�
������
	x������ܵ�����λ�ã�0��7��
	c��Ҫ��ʾ���ַ�������16�������ֺͼ��ţ�
	dp���Ƿ���ʾС���㣬0������ʾ��1����ʾ
*/
void DispChar(unsigned char location,unsigned char display,bit dp)
{
    unsigned char Tab[] =
	{//����0123456789AbCdEF���������������
		0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,
		0x7F,0x6F,0x77,0x7C,0x39,0x5E,0x79,0x71
	};
    unsigned char temp;	//��ʱ����
    //��ֹ��ʾλ�ó�����Χ
	location &= 0x07;
	location = 7 - location;
    //�����ַ�c��ȡ�ö�Ӧ���������������
	if ( display == '-' )
	{
		temp = 0x40;
	}
    else
    {
        display = toint(display);	//toint()Ϊ�⺯�������C:\Keil\C51\HLP\C51.pdf��ʮ���������ּ�麯��������ת���β��ַ�Ϊʮ����������
        if(display<0xf)
        {
            temp = Tab[display];	//���ȡ���������������
        }
        else
        {
            temp = 0x00;	//����������ַ�����ʾΪ�հ�
        }
    }
    
	//����Ƿ���ʾС����
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
	ѡ���л���Hz��KHzΪ��λ��ʾһ�����֣����ʹ����λ�����
	����num��Ҫ��ʾ������
	����gear����λ�л���0ΪHz����1ΪKHz��
	Ƶ�ʼ���ʾ��ΧΪ10Hz~740KHz
*/
void DispNum(unsigned long num, bit gear)
{
    code unsigned char s[] = "0123456789abcdef";
    unsigned char data temp_arr[6] = 0;	//��ʱ���飬�������λ����
    unsigned char i = 0;
    bit dp;	//�Ƿ���ʾС����
    if(num>=0)
    {
        while (num != 0)
        {
            //�������
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
                //KHz��λʱ���������ʾ����
                DispChar(7-i,s[temp_arr[i]],dp);
            }        
        }
        else
        {
            for(i=0;i<6;i++)
            {
                //Hz��λʱ���������ʾ����
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
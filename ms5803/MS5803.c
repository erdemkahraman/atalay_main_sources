#include "MS5803.h"

PromVar PROMData;
volatile int32_t CurrentPress = 0,scaledpress = 0,altitude = 0;
double CurrentTemp = 0, TEMP;
/* SPI handler declaration */
SPI_HandleTypeDef hspi1;
	
/* Buffer used for transmission */
uint8_t aTxBuffer[4];

uint8_t aRxBuffer[4];

uint32_t D1,D2;

int32_t dT;
int64_t OFF,SENS;
float actualpressurebar;


//****************************************
//*���ܣ���MS5803д������
//*������CMD
//*���أ�True/False

static int WriteCmd(uint8_t CMD)
{
	memset(aTxBuffer,0,sizeof(aTxBuffer));
	aTxBuffer[0] = CMD;
	
	ClrCS();
	
	if(HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)aTxBuffer, (uint8_t *)aRxBuffer, 1, 100))
	{
		SetCS();
		return 1;
	}
	else
	{
		SetCS();
		return 0;
	}
}

//****************************************
//*���ܣ�����ָ���MS5803����Ӧ����
//*������CMD,Count
//*���أ�True/False

static int ReadCmdData(uint8_t CMD,uint8_t Count)
{
	memset(aTxBuffer,0,sizeof(aTxBuffer));
	aTxBuffer[0] = CMD;
	
	ClrCS();

	if(HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)aTxBuffer, (uint8_t *)aRxBuffer, Count+1, 100))
	{
		SetCS();
		return 1;
	}
	else
	{
		SetCS();
		return 0;
	}
	//memmove(aRxBuffer,aRxBuffer+1,Count);//
}

//****************************************
//*���ܣ���λMS5803
//*������
//*���أ�

 void ResetDevice()
{
	WriteCmd(RESET);
	HAL_Delay(3);			//������ʱ3ms
}

//****************************************
//*���ܣ���ȡPROMУ׼����,ֻ��Ҫ��ȡһ��
//*������
//*���أ�

 void GetPromData()
{
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof1,2);
	PROMData.C1 = (aRxBuffer[1]<<8) + aRxBuffer[2];
	
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof2,2);
	PROMData.C2 = (aRxBuffer[1]<<8) + aRxBuffer[2];
	
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof3,2);
	PROMData.C3 = (aRxBuffer[1]<<8) + aRxBuffer[2];
	
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof4,2);
	PROMData.C4 = (aRxBuffer[1]<<8) + aRxBuffer[2];
	
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof5,2);
	PROMData.C5 = (aRxBuffer[1]<<8) + aRxBuffer[2];
	
	memset(aRxBuffer,0,sizeof(aRxBuffer));
	ReadCmdData(Cof6,2);
	PROMData.C6 = (aRxBuffer[1]<<8) + aRxBuffer[2];
}

//****************************************
//*���ܣ�MS5803��ʼ��
//*������
//*���أ�


	//IIC��ʼ������
	void ResetDevice();
	void GetPromData();


//****************************************
//*���ܣ���ȡ�¶Ⱥ�ѹ��
//*������
//*���أ�

void StartCalculation()
{

	
	//ת��ѹ��
	WriteCmd(CD1_4096);
	HAL_Delay(10);			//������ʱ10ms
	ReadCmdData(ADC_Read,3);
	D1 = (aRxBuffer[1]*pow(2,16)) + (aRxBuffer[2]*pow(2,8)) + aRxBuffer[3];
	
	//ת���¶�
	WriteCmd(CD2_4096);
	HAL_Delay(10);			//������ʱ10ms
	ReadCmdData(ADC_Read,3);
	D2 = (aRxBuffer[1]*pow(2,16)) + (aRxBuffer[2]*pow(2,8)) + aRxBuffer[3];
	
	//�����¶�
	dT = D2 - (PROMData.C5*pow(2,8));
	
	if(dT > 16777216)
	{
		dT = 16777216;
	}
	else if(dT < -16776960)
	{
		dT = -16776960;
	}
	
	TEMP = 2000 + ((((long long)dT)*PROMData.C6)/pow(2,23));
	
	//����ѹ��
	OFF = (PROMData.C2*pow(2,18)) + ((((long long)dT)*PROMData.C4)/pow(2,5));
	if(OFF > 51538821120)
	{
		OFF = 51538821120;
	}
	else if(OFF < -34358689800)
	{
		OFF = -34358689800;
	}
	
	SENS = (((long long)PROMData.C1)*pow(2,17)) + ((((long long)dT)*PROMData.C3)/pow(2,6));
	
	if(SENS > 17179607040)
	{
		SENS = 17179607040;
	}
	else if(SENS < -8589672450)
	{
		SENS = -8589672450;
	}
	

	//Second order compensation
	long T2 = 0,OFF2 = 0,SENS2 = 0;
	
	if(TEMP < 2000)
	{
		T2 = (3*pow(dT,2))/pow(2,33);
		OFF2 = (3*pow((TEMP-2000),2))/pow(2,3);
		SENS2 = (7*pow((TEMP-2000),2))/pow(2,3);
		
		if(TEMP < -1500)
		{
			SENS2 += 3 * pow((TEMP+1500),2);
		}
	}
	
	CurrentTemp = (TEMP - T2)/100;
	OFF -= OFF2;
	SENS -= SENS2;
	
	CurrentPress =  (((SENS * D1) / 2097152 ) - OFF) / 32768;
	//40110 53 metre
	//39215 260 metre
	//895   = 207 metre
	//4.3236714975845410628019323671498 = 1 metre

	scaledpress = CurrentPress*2.5079920948616600790513833992095;
    #define atmPress 101325


	 altitude = 44330*(1-(pow((scaledpress/(float)atmPress), 0.19029495718)));


}


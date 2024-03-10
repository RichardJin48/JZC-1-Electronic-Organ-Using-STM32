#include "stm32f10x.h"                  // Device header
#include "Delay.h"

void Key_Init_A(uint16_t pins){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void Key_Init_B(uint16_t pins){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_InitStructure.GPIO_Pin = pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

uint8_t Key_Read_A(uint16_t pins){
	if(GPIO_ReadInputDataBit(GPIOA, pins)){
		return 1;
	}
	return 0;
}

uint8_t Key_Read_B(uint16_t pins){
	if(GPIO_ReadInputDataBit(GPIOB, pins)){
		return 1;
	}
	return 0;
}

/**
  * @brief  �������Ӻ���������Key_Read�������ʹ��
  * @param  var����Ҫ���ӵı���ֵ
  * @param  step�����ӵĲ�����������ʾ����
  * @param  max�����������ֵ
  * @param  set�������������ֵ�����õ���ֵ
  * @retval �µı���ֵ
  */
int16_t Key_Var_Plus(int16_t var, int8_t step, int16_t max, int16_t set){
	var += step;
	if((step>0 && var>max)||(step<0 && var<max)){
		var = set;
	}
	return var;
}

#include "stm32f10x.h"                  // Device head
#include "Delay.h"
#include "LED.h"
#include "Key.h"
#include "Buzzer.h"
#include "OLED.h"
#include "Encoder.h"
#include "Keyboard.h"
#include "JoyStick.h"

#include <math.h>

#define Key1 GPIO_Pin_10  //��Ļ�·��������A10������
#define Key2 GPIO_Pin_15  //��Ļ�·��м�����B15������
#define Key3 GPIO_Pin_14  //��Ļ�·��Ҽ�����B14������
#define Key4 GPIO_Pin_9   //��ת��������������A9������
#define LED1 GPIO_Pin_6   //��һ��LED��������A6����һ�˽�����
#define LED2 GPIO_Pin_4   //�ڶ���LED��������A4����һ�˽�����
#define LED3 GPIO_Pin_2   //������LED��������A2����һ�˽�����

uint8_t voice = 1;      //��ɫ���
uint8_t knob = 1;       //��ť���ܱ��
uint8_t tsp = 5;        //�Ƶ���������Χ0��11����Ļ��ʾΪ-5��+6
uint8_t sft = 3;        //����ƽ�Ʋ�������Χ0��6����Ļ��ʾΪ-3��+3
uint8_t oct = 1;        //�˶Ȳ�����0���Ͱ˶ȡ�1��������2���߰˶�
uint8_t sharp = 0;      //������������0���ء�1����
uint8_t mode = 1;       //ģʽ��ţ�1����ɫģʽ��2������ģʽ
uint8_t style = 1;      //��������
uint16_t tempo = 120;   //�ٶȣ���λΪbpm
uint8_t chordType = 0;  //��������
uint8_t isPlaying = 0;  //�����Ƿ��ڲ��ţ�0�����ڲ��š�1���ڲ���
uint8_t track = 0;      //���쿪��״̬��0����1��������졢1����1��͹�2�졢2��������
uint8_t section = 0;    //ǰ��β�����俪��״̬��0��ǰ���β����1��ǰ�ࡢ2��β����3���ر�
uint8_t arpVoice = 1;   //������ɫ���

/* �������Ƶ��       G   A   B   C   D   E   F   G   A   B   C   D   E   */
uint16_t scale[] = {196,220,247,262,294,330,349,392,440,494,523,587,659};

/* �Ƶ�ϵ��           -5   -4   -3   -2   -1   0   +1   +2   +3   +4   +5   +6   */
float transpose[] = {0.75,0.79,0.84,0.89,0.94,1.0,1.06,1.12,1.19,1.26,1.33,1.41};

/* ��ɫ����ռ�ձ� */
uint8_t voiceDuties[5] = {50,40,30,20,10};

/* OLED��ʾ����ʾ��Ϣ */
char* voiceNames[6] = {"Clarinet","Pipe Organ","Accordion","Oboe","Trumpet","Drums"};
char* styleNames[6] = {"8Beat","16Beat","Slow Rock","Swing","March","Waltz"};
char* trackStatus[3] = {"Dr&Ap","Drums","Arpgo"};
char* sectionStatus[4] = {"In&Ed","Intro","Endng"," OFF "};
char* chordNames[8] = {"C ","Dm","Em","E7","F ","G ","G7","Am"};


/**
  * @brief  ��������Ƶ�����ɺ��������ڰ����������ţ�����������ȫ�ֱ���chordType����
  * @param  number�����ҵĵڼ�������1��4Ϊ���ҵĵ�1����4������5��8�����߰˶ȵ�1��4
  * @retval ��������Ƶ��
  */
int chord(char number){
	uint16_t freq = 0;
  if(number == 0){
    return 0;
  }
  uint16_t major[] = {131,165,196,262};    //��������1��3��5��1
  uint16_t minor[] = {131,156,196,262};    //С������1��b3��5��1
  uint16_t seventh[] = {131,165,196,233};  //���ߺ���1��3��5��b7
  number = number - 1;
  switch(chordType){
    case 0:  //C
			freq = major[number%4]*pow(2,number/4);
			break;
    case 1:  //Dm
			freq = minor[number%4]*pow(2,number/4)*transpose[7];
			break;
    case 2:  //Em
			freq = minor[number%4]*pow(2,number/4)*transpose[9];
			break;
    case 3:  //E7
			freq = seventh[number%4]*pow(2,number/4)*transpose[9];
			break;
    case 4:  //F
			freq = major[number%4]*pow(2,number/4)*transpose[10];
			break;
    case 5:  //G
			freq = major[number%4]*pow(2,number/4)*transpose[0];
			break;
    case 6:  //G7
			freq = seventh[number%4]*pow(2,number/4)*transpose[0];
			break;
    case 7:  //Am
			freq = minor[number%4]*pow(2,number/4)*transpose[2];
			break;
  }
  return freq*transpose[tsp];
}


/**
  * @brief  ǰ�ಥ�ź���
  * @param  introDrums[]��ǰ��ĵ�1�������
  * @param  introCymbals[]��ǰ��ĵ�2�������
	*	@param	beat��ǰ��Ľ��ģ�
	*								6��ʾ��8������������3/4�ģ�
	*								8��ʾ��8������������4/4�ģ�
	*								16��ʾ��16������������4/4�ģ�
	*								12��ʾ������������������6/8��
  * @retval ��
  */
void introPlay(char introDrums[],char introCymbals[],char beat){
	isPlaying = 1;
	OLED_ShowString(2,12,"     ");
	OLED_ShowString(4,1,"OctDw OctUp Shrp");
	if(section == 0 || section == 1){
		OLED_ShowString(3,1,"Intro           ");
		for(int j=0;j<beat;j++){
			if(beat==6){
				switch(j/2%3){
					case 0:
						LED_ON_A(LED1|LED2|LED3);
						break;
					case 1:
						LED_ON_A(LED2);
						break;
					case 2:
						LED_ON_A(LED3);
						break;
				}
			}
			else{
				switch(j/(beat/4)%4){
					case 0:
						LED_ON_A(LED1|LED2|LED3);
						break;
					case 1:
						LED_ON_A(LED1);
						break;
					case 2:
						LED_ON_A(LED2);
						break;
					case 3:
						LED_ON_A(LED3);
						break;
				}
			}
			if(track == 0 || track == 1){
				Buzzer_Drum(introDrums[j]);
				Buzzer_Drum(introCymbals[j]);
			}
			else{
				Delay_ms(60);
			}
			switch(beat){
				case 6:
					Delay_ms(30000/tempo-60);
					break;
				case 8:
					Delay_ms(30000/tempo-60);
					break;
				case 16:
					Delay_ms(30000/(tempo*2)-60);
					break;
				case 12:
					Delay_ms(20000/tempo-60);
					break;
			}
			Buzzer_OFF(2);
			LED_OFF_A(LED1|LED2|LED3);
			if(Encoder_Get() != 0){
				tempo = Encoder_Var(tempo,5,40,240,40,240);
				OLED_ShowNum(2,8,tempo,3);
			}
		}
	}
}


/**
  * @brief  β�����ź���
  * @param  ��
  * @retval ��
  */
void endingPlay(void){
	if(section == 0 || section == 2){
		if(track != 1){
			OLED_ShowString(3,1,"Ending Chord:   ");
			OLED_ShowString(3,15,chordNames[chordType]);
			uint16_t vrX = JoyStick_GetX();
			uint16_t vrY = JoyStick_GetY();
			if(vrX<1024 && vrY<1024){  // ���� Dm����
				chordType = 1;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrX>3072 && vrY<1024){  // ���� E7����
				chordType = 3;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrX<1024 && vrY>3072){  // ���� Am����
				chordType = 7;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrX>3072 && vrY>3072){  // ���� G����
				chordType = 5;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrX<1024){  // �� C����
				chordType = 0;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrX>3072){  // �� F����
				chordType = 4;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrY<1024){  // �� Em����
				chordType = 2;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
			else if(vrY>3072){  // �� G7����
				chordType = 6;
				OLED_ShowString(3,15,chordNames[chordType]);
			}
		}
		else{
			OLED_ShowString(3,1,"Ending          ");
		}
		if(track != 2){
			Buzzer_Drum(1);
			Buzzer_Drum(4);
		}
		if(track != 1){
			Buzzer_Timing(chord(1),60,voiceDuties[arpVoice-1],2);
			Buzzer_Timing(chord(2),60,voiceDuties[arpVoice-1],2);
			Buzzer_Timing(chord(3),60,voiceDuties[arpVoice-1],2);
			Buzzer_Timing(chord(4),60,voiceDuties[arpVoice-1],2);
		}
		else{
			Delay_ms(240);
		}
		Buzzer_OFF(2);
	}
	Delay_ms(500);
	isPlaying = 0;
	OLED_ShowString(3,1,"                ");
	OLED_ShowString(4,1,"                ");
}


/**
  * @brief  ���ಥ�ź���
  * @param  arp[]�����������������
  * @param  drums[]������ĵ�1�������
  * @param  cymbals[]������ĵ�2������飬���ڹر�������ʱ����
  * @param  fillDrums[]���ӻ��ĵ�1�������
  * @param  fillCymbals[]���ӻ��ĵ�2�������
	*	@param	number�����������С����Ϊbeat��������
	*	@param	beat������Ľ��ģ�
	*								6��ʾ��8������������3/4�ģ�
	*								8��ʾ��8������������4/4�ģ�
	*								16��ʾ��16������������4/4�ģ�
	*								12��ʾ������������������6/8��
  * @retval ��
  */
void acmpPlay(char arp[],char drums[],char cymbals[],char fillDrums[],char fillCymbals[],char number,char beat){
  int n;
	introPlay(fillDrums,fillCymbals,beat);  //�˴�ֱ�ӽ��ӻ�����ͬʱ����ǰ�ಥ�ţ��ɰ����������޸Ĵ���
	if(track != 1){
		OLED_ShowString(3,1,"Main   Chord:   ");
		OLED_ShowString(3,15,chordNames[chordType]);
	}
	else{
		OLED_ShowString(3,1,"Main            ");
	}
  while(1){
		for(int i=0;i<number;i++){
			
			/* ����ʶ����� */
			if(track != 1){
				uint16_t vrX = JoyStick_GetX();
				uint16_t vrY = JoyStick_GetY();
				if(vrX<1024 && vrY<1024){  // ���� Dm����
					chordType = 1;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrX>3072 && vrY<1024){  // ���� E7����
					chordType = 3;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrX<1024 && vrY>3072){  // ���� Am����
					chordType = 7;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrX>3072 && vrY>3072){  // ���� G����
					chordType = 5;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrX<1024){  // �� C����
					chordType = 0;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrX>3072){  // �� F����
					chordType = 4;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrY<1024){  // �� Em����
					chordType = 2;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else if(vrY>3072){  // �� G7����
					chordType = 6;
					OLED_ShowString(3,15,chordNames[chordType]);
				}
			}
			
			/* �ӻ����� */
			if(JoyStick_GetSW()){
				OLED_ShowString(3,1,"Fill In         ");
				n = i%beat;
				for(int j=n;j<beat;j++){
					if(beat==6){
						switch(j/2%3){
							case 0:
								LED_ON_A(LED1|LED2|LED3);
								break;
							case 1:
								LED_ON_A(LED2);
								break;
							case 2:
								LED_ON_A(LED3);
								break;
						}
					}
					else{
						switch(j/(beat/4)%4){
							case 0:
								LED_ON_A(LED1|LED2|LED3);
								break;
							case 1:
								LED_ON_A(LED1);
								break;
							case 2:
								LED_ON_A(LED2);
								break;
							case 3:
								LED_ON_A(LED3);
								break;
						}
					}
					if(track == 0 || track == 1){
						Buzzer_Drum(fillDrums[j]);
						Buzzer_Drum(fillCymbals[j]);
					}
					else{
						Delay_ms(60);
					}
					switch(beat){
						case 6:
							Delay_ms(30000/tempo-60);
							break;
						case 8:
							Delay_ms(30000/tempo-60);
							break;
						case 16:
							Delay_ms(30000/(tempo*2)-60);
							break;
						case 12:
							Delay_ms(20000/tempo-60);
							break;
					}
					Buzzer_OFF(2);
					LED_OFF_A(LED1|LED2|LED3);
					if(Encoder_Get() != 0){
						tempo = Encoder_Var(tempo,5,40,240,40,240);
						OLED_ShowNum(2,8,tempo,3);
					}
					if(Key_Read_A(Key4)){
						break;
					}
				}
				if(track != 1){
					OLED_ShowString(3,1,"Main   Chord:   ");
					OLED_ShowString(3,15,chordNames[chordType]);
				}
				else{
					OLED_ShowString(3,1,"Main            ");
				}
				break;
			}
			
			/* ����ָʾ�ƴ��� */
			if(beat==6){
				switch(i/2%3){
					case 0:
						LED_ON_A(LED1|LED2|LED3);
						break;
					case 1:
						LED_ON_A(LED2);
						break;
					case 2:
						LED_ON_A(LED3);
						break;
				}
			}
			else{
				switch(i/(beat/4)%4){
					case 0:
						LED_ON_A(LED1|LED2|LED3);
						break;
					case 1:
						LED_ON_A(LED1);
						break;
					case 2:
						LED_ON_A(LED2);
						break;
					case 3:
						LED_ON_A(LED3);
						break;
				}
			}
			
			/* ���ಥ�Ŵ��� */
			if(track != 2){
				Buzzer_Drum(drums[i]);
			}
			else{
				Delay_ms(30);
			}
			if(track == 1){
				Buzzer_Drum(cymbals[i]);
				switch(beat){
					case 6:
						Delay_ms(30000/tempo-60);
						break;
					case 8:
						Delay_ms(30000/tempo-60);
						break;
					case 16:
						Delay_ms(30000/(tempo*2)-60);
						break;
					case 12:
						Delay_ms(20000/tempo-60);
						break;
				}
			}
			else{
				if(arp[i] != 0){
					switch(beat){
						case 6:
							Buzzer_Timing(chord(arp[i]),30000/tempo-30,voiceDuties[arpVoice-1],2);
							break;
						case 8:
							Buzzer_Timing(chord(arp[i]),30000/tempo-30,voiceDuties[arpVoice-1],2);
							break;
						case 16:
							Buzzer_Timing(chord(arp[i]),30000/(tempo*2)-30,voiceDuties[arpVoice-1],2);
							break;
						case 12:
							Buzzer_Timing(chord(arp[i]),20000/tempo-30,voiceDuties[arpVoice-1],2);
							break;
					}
				}
				else{
						switch(beat){
						case 6:
							Delay_ms(30000/tempo-30);
							break;
						case 8:
							Delay_ms(30000/tempo-30);
							break;
						case 16:
							Delay_ms(30000/(tempo*2)-30);
							break;
						case 12:
							Delay_ms(20000/tempo-30);
							break;
					}
				}
			}
			Buzzer_OFF(2);
			LED_OFF_A(LED1|LED2|LED3);
			if(Encoder_Get() != 0){
				tempo = Encoder_Var(tempo,5,40,240,40,240);
				OLED_ShowNum(2,8,tempo,3);
			}
			if(Key_Read_A(Key4)){
				break;
			}
		}
		if(Key_Read_A(Key4)){
			endingPlay();
			break;
		}
  }
}


/**
  * @brief  ������������,��������������Ӧ���İ���
  * @param  style����������
  * @retval ��
  */
void acmp(char style){
  if(style==1){  // 8Beat
		char arp[16] = {1,2,3,2,4,3,2,3,1,2,3,2,4,2,3,2};
    char drums[16]={1,3,2,1,1,3,2,3,1,3,2,1,1,3,2,1};
		char cymbals[16]={3,0,3,3,3,0,3,0,3,0,3,3,3,0,3,3};
		char fillDrums[8]={1,2,2,2,2,2,2,2};
		char fillCymbals[8]={3,3,3,4,5,5,6,7};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,16,8);
  }
  
  if(style==2){  // 16Beat
		char arp[32] = {1,2,3,4,6,4,3,2,1,2,3,4,6,4,3,2,1,2,3,4,6,4,3,2,4,3,2,1,3,2,1,2};
    char drums[32]={1,3,3,3,2,3,3,1,1,3,3,3,2,3,3,1,1,3,3,3,2,3,3,1,1,3,1,3,2,1,3,4};
		char cymbals[32]={3,0,0,0,3,0,0,3,3,0,0,0,3,0,0,3,3,0,0,0,3,0,0,3,3,0,3,0,3,3,0,0};
    char fillDrums[16]={1,0,0,0,2,0,2,2,1,2,0,2,5,5,6,7};
    char fillCymbals[16]={3,3,3,3,3,3,3,3,4,0,0,0,0,0,0,0};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,32,16);
  }

  if(style==3){  // Slow Rock
		char arp[24] = {1,2,3,4,3,2,1,2,3,4,3,2,1,2,3,4,3,2,4,3,2,3,2,3};
    char drums[24]={1,3,3,2,3,1,1,3,3,2,3,3,1,3,3,2,3,1,1,3,3,2,3,1};
		char cymbals[24]={3,0,0,3,0,3,3,0,0,3,0,0,3,0,0,3,0,3,3,0,0,3,0,4};
		char fillDrums[12]={1,0,0,2,0,2,1,2,2,5,6,7};
    char fillCymbals[12]={3,3,3,3,3,3,4,0,0,0,0,0};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,24,12);
  }
  
	if(style==4){  // Swing
		char arp[12] = {5,0,0,6,0,4,3,0,0,6,0,3};
    char drums[12]={1,0,0,2,0,0,1,0,0,2,0,0};
    char cymbals[12]={4,0,0,3,0,4,4,0,0,3,0,4};
		char fillDrums[12]={1,0,0,2,0,2,1,2,2,5,6,7};
    char fillCymbals[12]={4,0,0,3,0,4,4,0,0,0,0,0};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,12,12);
  }
	
	if(style==5){  // March
		char arp[32] = {1,0,6,0,3,0,6,0,1,0,6,0,3,0,6,0,1,0,6,0,3,0,6,0,1,0,5,4,5,0,5,0};
    char drums[32]={1,0,2,0,1,0,2,0,1,0,2,0,1,0,2,0,1,0,2,0,1,0,2,0,1,0,2,2,2,0,2,0};
    char cymbals[32]={3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,3,0,4,0};
    char fillDrums[16]={1,0,2,2,1,0,1,0,1,2,2,2,2,0,2,2};
    char fillCymbals[16]={4,0,3,0,4,0,4,0,4,0,3,0,3,0,4,0};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,32,16);
  }
	
  if(style==6){  // Waltz
		char arp[12] = {1,2,3,2,3,2,4,2,3,2,3,2};
    char drums[12]={1,0,2,0,2,0,1,0,2,0,2,0};
		char cymbals[12]={3,3,4,3,4,3,3,3,4,3,4,3};
    char fillDrums[6]={1,2,2,0,2,2};
    char fillCymbals[6]={3,3,3,3,4,0};
    acmpPlay(arp,drums,cymbals,fillDrums,fillCymbals,12,6);
  }
}



/* ������ */
int main(void){
	
	/* ��ʼ�� */
	LED_Init_A(LED1|LED2|LED3);//����LED�������Ҹ������ν�A6��A4��A2����һ�˽�����
	Key_Init_A(Key1|Key4);     //��Ļ�·��������ת�������������ֱ��A10��A9����һ�˽�����
	Key_Init_B(Key2|Key3);     //��Ļ�·��м����Ҽ����ֱ��B15��B14����һ�˽�����
	Buzzer_Init();             //����������������Դ���������������ֱ��A0��A7����һ�˽Ӹ���
	OLED_Init();               //OLED��ʾ����4���ͣ���SCL��C15��SDA��C14
	Encoder_Init();            //��ת��������A��B1��B��B0
	Keyboard_Init();           //�߸��ټ������������ν�B12��B13��A8��A11��B3��B6��B9����һ�˽�����
	JoyStick_Init();           //PS2ҡ��ģ�飬VRX��A5��VRY��A3��SW��A1
	
	while(1){
		
		/* ��ɫģʽ */
		if(mode == 1){
			OLED_ShowNum(1,1,voice,1);
			OLED_ShowString(1,3,voiceNames[voice-1]);
			OLED_ShowString(2,1,"Tsp: ");
			OLED_ShowSignedNum(2,6,tsp-5,1);
			OLED_ShowString(2,8,"  Sft: ");
			OLED_ShowSignedNum(2,15,sft-3,1);
			OLED_ShowString(3,11,"M: Voc");
			OLED_ShowString(4,1,"OctDw OctUp Shrp");
			if(Key_Read_A(Key4)){
				knob = Key_Var_Plus(knob,1,3,1);
				Delay_ms(200);
			}
			if(JoyStick_GetSW()){
				mode = Key_Var_Plus(mode,1,2,1);
				knob = 1;
				if(voice == 6){
					voice = 1;
				}
				Delay_ms(200);
				OLED_Clear();
			}
			if(knob == 1){
				OLED_ShowString(3,1,"K: Voice  ");
				if(Encoder_Get() != 0){
					voice = Encoder_Var(voice,1,1,6,6,1);
					OLED_ShowString(1,3,"              ");
				}
			}
			else if(knob == 2){
				OLED_ShowString(3,1,"K: Transp ");
				tsp = Encoder_Var(tsp,1,0,11,0,11);
			}
			else if(knob == 3){
				OLED_ShowString(3,1,"K: Shift  ");
				sft = Encoder_Var(sft,1,0,6,0,6);
			}
		}
		
		/* ����ģʽ */
		else if(mode == 2){
			OLED_ShowNum(1,1,style,1);
			OLED_ShowString(1,3,styleNames[style-1]);
			OLED_ShowString(2,1,"Tempo: ");
			OLED_ShowNum(2,8,tempo,3);
			OLED_ShowString(2,11," ArpV");
			OLED_ShowNum(2,16,arpVoice,1);
			OLED_ShowString(3,11,"M: Sty");
			OLED_ShowString(4,1,"Play ");
			OLED_ShowString(4,6,trackStatus[track]);
			OLED_ShowString(4,12,sectionStatus[section]);
			if(Key_Read_A(Key1)){
				Delay_ms(200);
				acmp(style);
			}
			if(Key_Read_B(Key2)){
				track = Key_Var_Plus(track,1,2,0);
				Delay_ms(200);
			}
			if(Key_Read_B(Key3)){
				section = Key_Var_Plus(section,1,3,0);
				Delay_ms(200);
			}
			if(Key_Read_A(Key4)){
				knob = Key_Var_Plus(knob,1,3,1);
				Delay_ms(200);
			}
			if(JoyStick_GetSW()){
				mode = Key_Var_Plus(mode,1,2,1);
				knob = 1;
				Delay_ms(200);
				OLED_Clear();
			}
			if(knob == 1){
				OLED_ShowString(3,1,"K: Style  ");
				if(Encoder_Get() != 0){
					style = Encoder_Var(style,1,1,6,6,1);
					OLED_ShowString(1,3,"              ");
				}
			}
			else if(knob == 2){
				OLED_ShowString(3,1,"K: Tempo  ");
				tempo = Encoder_Var(tempo,5,40,240,40,240);
			}
			else if(knob == 3){
				OLED_ShowString(3,1,"K: ArpVoc ");
				arpVoice = Encoder_Var(arpVoice,1,1,5,5,1);
			}
		}
	}
}



/* ���̵����жϺ��� */
void EXTI3_IRQHandler(void){
	if(EXTI_GetITStatus(EXTI_Line3) == SET){  // �ټ�5
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_3)==1){  //��������ټ�
			if(voice == 6){  //�����ɫ����Ϊ����
				Buzzer_Drum(5);
				Delay_ms(70);
			}
			else{  //�����ɫ����Ϊ������ɫ
				Buzzer_Reset(1);
				uint16_t freq = scale[4+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{  //����ɿ��ټ�
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line3);
	}
}

void EXTI9_5_IRQHandler(void){
	if(EXTI_GetITStatus(EXTI_Line8) == SET){  // �ټ�3
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8)==1){
			if(voice == 6){
				Buzzer_Drum(3);
				Delay_ms(70);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[2+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line8);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line6) == SET){  // �ټ�6
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6)==1){
			if(voice == 6){
				if(sharp == 1){
					Buzzer_Drum(8);
				}
				else{
					Buzzer_Drum(6);
				}
				Delay_ms(70);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[5+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line6);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line9) == SET){  // �ټ�7
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9)==1){
			if(voice == 6){
				if(sharp == 1){
					Buzzer_Drum(9);
				}
				else{
					Buzzer_Drum(7);
				}
				Delay_ms(70);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[6+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line9);
	}
}

void EXTI15_10_IRQHandler(void){
	if(EXTI_GetITStatus(EXTI_Line11) == SET){  // �ټ�4
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)==1){
			if(voice == 6){
				Buzzer_Drum(4);
				Delay_ms(70);
				Buzzer_OFF(2);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[3+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line11);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line12) == SET){  // �ټ�1
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12)==1){
			if(voice == 6){
				Buzzer_Drum(1);
				Delay_ms(70);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[0+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line12);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line13) == SET){  // �ټ�2
		if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13)==1){
			if(voice == 6){
				Buzzer_Drum(2);
				Delay_ms(70);
			}
			else{
				Buzzer_Reset(1);
				uint16_t freq = scale[1+sft]*transpose[tsp]*pow(2,oct)*pow(1.06,sharp);
				Buzzer_ON(freq,voiceDuties[voice-1],1);
			}
		}
		else{
			Buzzer_OFF(1);
		}
		EXTI_ClearITPendingBit(EXTI_Line13);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line10) == SET){  // ��Ļ�·���������Ͱ˶ȼ���
		if(mode == 1 || isPlaying == 1){  //��������Ϊ��ɫģʽ��������ڲ���ʱ
			if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10)==1){
				oct = 0;
			}
			else{
				oct = 1;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line10);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line15) == SET){  // ��Ļ�·��м������߰˶ȼ���
		if(mode == 1 || isPlaying == 1){
			if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15)==1){
				oct = 2;
			}
			else{
				oct = 1;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line15);
	}
	
	else if(EXTI_GetITStatus(EXTI_Line14) == SET){  // ��Ļ�·��Ҽ������߰�������
		if(mode == 1 || isPlaying == 1){
			if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)==1){
				sharp = 1;
			}
			else{
				sharp = 0;
			}
		}
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
}

/* �������������� �������� ��� noname ����������� ������� � ���� �����.
	��� �� ATtiny44A
	�����: Alex ZhAk
	2017�
	*/

#include <avr/io.h>
#include <avr/interrupt.h>

volatile unsigned short interrupt_count=0;				// ������� ���-�� ������������ ����������
volatile unsigned char flag_second_passed=0;			// ���� "������� ������"
#define INTERRUPTS_IN_SECOND	31250					// ���-�� ���������� �� �������
volatile unsigned char seconds_button=0;				// ������� ������ ��� ������� ������
volatile unsigned char flag_button_is_locked=0;			// ���� "������ �������������"
volatile unsigned long seconds_working=0;				// ������� ������ � ������ ������
#define MAX_SECONDS_WORKING		43200					// ������������ ����� ������ (12 �����) � ��������
volatile unsigned char button_oldstate=0;				// ���������� ��������� ������
volatile unsigned char flag_piezo_is_on=0;				// ���� "��������������� ���"
volatile unsigned char flag_light_is_on=0;				// ���� "��������� ��������"
volatile unsigned char flag_color_setup=0;				// ���� "����� ��������� �����"

volatile unsigned char led_pwm_count=0;					// ������� ��� ����������� ��������� �����
volatile unsigned char pwm_r=0,pwm_g=0,pwm_b=255;		// ���������� ��� �� ������� ����� ����������� ��������� �����
volatile unsigned short color_pwm_cycle=0;				// ���-�� �������� �������� ��� ������
#define PWM_COLOR_CHANGE_STEP	1000					// �������� ����� ������
volatile unsigned short lumi_pwm_cycle=0;				// ���-�� �������� �������� ��� �������
#define PWM_LUMI_CHANGE_STEP	2000					// �������� ����� �������

volatile unsigned char flag_luminance_setup=0;			// ���� "��������� ������������� ��������� �����"
volatile unsigned char pwm_lum=127;						// �������� ������������� ����� ����� (0 - �������, 127 - ������ ����, 255 - �����)
volatile unsigned char src_pwm_r, src_pwm_g, src_pwm_b; // �������� �������� �����

// �������� ������
// ������� ������� �����
#define RAINBOW_NEXT_CYA	0	// ���������
#define RAINBOW_NEXT_GRN	1	// ������
#define RAINBOW_NEXT_YEL	2	// �����
#define RAINBOW_NEXT_RED	3	// �������
#define RAINBOW_NEXT_PUR	4	// ���������
#define RAINBOW_NEXT_BLU	5	// �����
volatile unsigned char rainbow_target_state=RAINBOW_NEXT_CYA;

// ���/���� ���������� ��������� ������
#define BUTTON_LIGHT_ON()	PORTA&=~(1<<PA3)
#define BUTTON_LIGHT_OFF()	PORTA|=(1<<PA3)

// ����������� ����������� ��������� �����
#define LED_R	(1<<PB0)
#define LED_G	(1<<PB1)
#define LED_B	(1<<PA0)

unsigned char tmp_pwm_r, tmp_pwm_g, tmp_pwm_b;

ISR(TIM1_CAPT_vect)
{
	// ��������
	interrupt_count++;
	if(interrupt_count>=INTERRUPTS_IN_SECOND)
	{
		interrupt_count=0;
		flag_second_passed=1;
	}
	
	led_pwm_count++;
	
	// �������� ��������� ��������� ������
	if(interrupt_count==INTERRUPTS_IN_SECOND/2 && !flag_button_is_locked) BUTTON_LIGHT_ON();
		
	// ���������� ��������� �����
	if(led_pwm_count==0 && flag_light_is_on)
	{
		tmp_pwm_r=pwm_r;
		tmp_pwm_g=pwm_g;
		tmp_pwm_b=pwm_b;
		PORTA|=LED_B;
		PORTB|=LED_R|LED_G;
	}
	
	if(led_pwm_count == tmp_pwm_r) PORTB&=~LED_R;
	if(led_pwm_count == tmp_pwm_g) PORTB&=~LED_G;
	if(led_pwm_count == tmp_pwm_b) PORTA&=~LED_B;
	
	if(color_pwm_cycle<PWM_COLOR_CHANGE_STEP) color_pwm_cycle++;
	if(lumi_pwm_cycle<PWM_LUMI_CHANGE_STEP) lumi_pwm_cycle++;
}

// ������ � EEPROM
#define EEPROM_LIGHT_IS_ON	0
#define EEPROM_PWM_R		1
#define EEPROM_PWM_G		2
#define EEPROM_PWM_B		3
#define EEPROM_PWM_LUM		4

// ����� ������������� ����� �����
void set_Luminance(void)
{
	if(pwm_lum==255) pwm_lum=254;
	
	unsigned long lum_percent, tmp_pwm;
	
	if(pwm_lum<=127)
	{ // ���������� �� �������
		lum_percent = 1000 * (unsigned long)pwm_lum / 127;
		
		tmp_pwm = lum_percent * (unsigned long)src_pwm_r / 1000;
		pwm_r = (unsigned char) tmp_pwm;
		
		tmp_pwm = lum_percent * (unsigned long)src_pwm_g / 1000;
		pwm_g = (unsigned char) tmp_pwm;
		
		tmp_pwm = lum_percent * (unsigned long)src_pwm_b / 1000;
		pwm_b = (unsigned char) tmp_pwm;
	}
	else
	{ // ���������� �� ������
		lum_percent = 1000 * ((unsigned long)pwm_lum - 127 ) / 127;
	
		tmp_pwm = (unsigned long)src_pwm_r + lum_percent * (255 - (unsigned long)src_pwm_r) / 1000;
		pwm_r = (unsigned char) tmp_pwm;
		
		tmp_pwm = (unsigned long)src_pwm_g + lum_percent * (255 - (unsigned long)src_pwm_g) / 1000;
		pwm_g = (unsigned char) tmp_pwm;
		
		tmp_pwm = (unsigned long)src_pwm_b + lum_percent * (255 - (unsigned long)src_pwm_b) / 1000;
		pwm_b = (unsigned char) tmp_pwm;
	}
	
	
}

// ������ ����� � ����������������� ������
void EEPROM_write(unsigned char address, unsigned char value)
{
	while(EECR & (1<<EEPE));
	EECR=(0<<EEPM1)|(0<<EEPM0);
	EEARH=0;EEARL=address;
	EEDR=value;
	EECR |= (1<<EEMPE);
	EECR |= (1<<EEPE);
}

// ������ ����� �� ����������������� ������
unsigned char EEPROM_read(unsigned char address)
{
	while(EECR & (1<<EEPE));
	EEARH=0;EEARL=address;
	EECR |= (1<<EERE);
	unsigned char tmp=EEDR;
	return tmp;
}

int main(void)
{
	// ��������� ���������� ������� ��� ���������������
	TCCR0A=0b00100011;
	TCCR0B=0b00001001;
	OCR0A=70;
	OCR0B=35;
	
	// ��������� ������� ��� ������� ������� � ��� ����������� ��������� �����
	TCCR1B=0b00011001;
	TIMSK1=0b00100000;
	ICR1=0x00FF;
	
	// ��������� ������
	PORTA|=(1<<PA1);
	
	// ��������� ���������� � ������
	DDRA|=(1<<PA3);
	
	// ��������� ����������� ��������� �����
	DDRA|=LED_B;
	DDRB|=LED_R|LED_G;
	
	// ��������� ��������� �� ����������������� ������
	flag_light_is_on = EEPROM_read(EEPROM_LIGHT_IS_ON);
	pwm_r=EEPROM_read(EEPROM_PWM_R);
	pwm_g=EEPROM_read(EEPROM_PWM_G);
	pwm_b=EEPROM_read(EEPROM_PWM_B);
	src_pwm_r=pwm_r;
	src_pwm_g=pwm_g;
	src_pwm_b=pwm_b;
	pwm_lum=EEPROM_read(EEPROM_PWM_LUM);
	
	// ����� ������������� ����� �����
	set_Luminance();
	
	// �������� ����������
	sei();
	
	// ������� ����
	for(;;)
	{
		// ���������� ����������������
		if(seconds_working>=MAX_SECONDS_WORKING) flag_piezo_is_on=0;
		
		if(flag_piezo_is_on)
			DDRA|=(1<<PA7);
		else
			DDRA&=~(1<<PA7);
	
		// ������� �������
		if(flag_second_passed)
		{
			flag_second_passed=0;
			if(seconds_button<255 && !flag_button_is_locked) seconds_button++;
			
			if((seconds_button==2 || seconds_button==4 || seconds_button==6 || seconds_button==8) && !flag_color_setup) BUTTON_LIGHT_OFF(); // ������ ����������� ��� �������
			
			if(seconds_working<MAX_SECONDS_WORKING) seconds_working++;
		}
		
		// ����� ������ ����� �����	
		if(flag_color_setup && color_pwm_cycle==PWM_COLOR_CHANGE_STEP)
		{
			color_pwm_cycle=0;
			
			switch(rainbow_target_state)
			{
				case RAINBOW_NEXT_CYA:
				{
					pwm_g++;
					if(pwm_g==255) rainbow_target_state++;
				}
				break;
				
				case RAINBOW_NEXT_GRN:
				{
					pwm_b--;
					if(pwm_b==0) rainbow_target_state++;
				}
				break;
				
				case RAINBOW_NEXT_YEL:
				{
					pwm_r++;
					if(pwm_r==255) rainbow_target_state++;
				}
				break;
				
				case RAINBOW_NEXT_RED:
				{
					pwm_g--;
					if(pwm_g==0) rainbow_target_state++;
				}
				break;
				
				case RAINBOW_NEXT_PUR:
				{
					pwm_b++;
					if(pwm_b==255) rainbow_target_state++;
				}
				break;
				
				case RAINBOW_NEXT_BLU:
				{
					pwm_r--;
					if(pwm_r==0) rainbow_target_state=0;
				}
				break;
			}
		}
		
		// ����� ��������� ������������� ����� �����
		if(flag_luminance_setup && lumi_pwm_cycle==PWM_LUMI_CHANGE_STEP)
		{
			lumi_pwm_cycle=0;
			pwm_lum--;
			set_Luminance();
		}
		
		// �������� ������� ������
		if(PINA & (1<<PA1))
		{ // ���� ������ ������
			flag_button_is_locked=0;
			
			if(button_oldstate) 
			{ // ���� ������ ����� ���� ������
				if(!flag_color_setup && !flag_luminance_setup)
				{ // ���� �� ���. ����� ��������� ���������
					if(seconds_button>=2 && seconds_button<4)	// ������������ ������� 2-4 ���: ���/���� ���������������
					{
						flag_piezo_is_on=!flag_piezo_is_on;
						seconds_working=0;
					}
				
					if(seconds_button>=4 && seconds_button<6)	// ������������ ������� 4-6 ���: ���/���� ���������
					{
						flag_light_is_on=!flag_light_is_on;
						EEPROM_write(EEPROM_LIGHT_IS_ON,flag_light_is_on);
					}
				
					if(seconds_button>=6 && seconds_button<8)	// ������������ ������� 6-8 ���: ��������� ���� ��������� �����
					{
						flag_light_is_on=1;
						flag_color_setup=1;
						pwm_b=255;
						pwm_r=0;
						pwm_g=0;
						rainbow_target_state=RAINBOW_NEXT_CYA;
						pwm_lum=127;
					}
					
					if(seconds_button>=8)	// ������������ ������� 8-... ���: ��������� ������������� ��������� �����
					{
						flag_light_is_on=1;
						flag_luminance_setup=1;
						src_pwm_r=EEPROM_read(EEPROM_PWM_R);
						src_pwm_g=EEPROM_read(EEPROM_PWM_G);
						src_pwm_b=EEPROM_read(EEPROM_PWM_B);
						pwm_lum=127;
					}
				}				
			}
			
			seconds_button=0;
			button_oldstate=0;
		}
		else
		{ // ���� ������ ������
			button_oldstate=1;
			if(flag_color_setup || flag_luminance_setup) // ������������ ����������� ���� � ������������� ��������� �����
			{
				flag_luminance_setup=0;
				flag_button_is_locked=1;
				EEPROM_write(EEPROM_LIGHT_IS_ON,flag_light_is_on);
				if(flag_color_setup)
				{
					EEPROM_write(EEPROM_PWM_R,pwm_r);
					EEPROM_write(EEPROM_PWM_G,pwm_g);
					EEPROM_write(EEPROM_PWM_B,pwm_b);
				}
				EEPROM_write(EEPROM_PWM_LUM,pwm_lum);
				flag_color_setup=0;
				BUTTON_LIGHT_OFF();
			}
			
		}
	}
}
#include "modIndicator.h"
#include "stm32f10x_gpio.h"
//#include "driverlib.h"
//#include <string.h>

//unsigned char avtStep = 0;


unsigned char rawPictData[4];
unsigned char ledCount;
unsigned char segmCount;
rawPict_t rawPict;
unsigned char textTemp[10];

//начало екстовых констант с адреса 0х20
//для _REVERSED_LED варианта
const unsigned char pictSegmConst[] =  
{
/**0x20 ' '*/	0xff,
/**0x21 '!'*/	0xCF,
/**0x22 '"'*/	0xff,
/**0x23 '#'*/	0xff,
/**0x24 '$'*/	0xff,
/**0x25 '%'*/   0xff,
/**0x26 '&'*/	0xff,
/**0x27 '''*/	0xff,
/**0x28 '('*/   0xff,
/**0x29 ')'*/	0xff,
/**0x2a '*'*/	0xff,
/**0x2b '+'*/   0xff,
/**0x2c ','*/	0x7f,
/**0x2d '-'*/	0xbf,
/**0x2e '.'*/   0x7f,
/**0x2f '/'*/	0xff,
/**0x30 '0'*/	0xc0,
/**0x31 '1'*/	0xf9,
/**0x32 '2'*/	0xa4,
/**0x33 '3'*/	0xb0,
/**0x34 '4'*/	0x99,
/**0x35 '5'*/	0x92,
/**0x36 '6'*/	0x82,
/**0x37 '7'*/	0xf8,
/**0x38 '8'*/	0x80,
/**0x39 '9'*/	0x90,
/**0x3a ':'*/	0xff,
/**0x3b ';'*/	0xff,
/**0x3c '<'*/	0xff,
/**0x3d '='*/	0xff,
/**0x3e '>'*/	0xff,
/**0x3f '?'*/	0xff,
/**0x40 '`'*/	0xff,
/**0x41 'A'*/	0x88,
/**0x42 'B'*/	0x83,
/**0x43 'C'*/	0xc6,
/**0x44 'D'*/	0xa1,
/**0x45 'E'*/	0x86,
/**0x46 'F'*/	0x8e,  
/**0x47 'G'*/	0xc2,
/**0x48 'H'*/	0x89,
/**0x49 'I'*/	0xe6,
/**0x4a 'J'*/	0xf2,
/**0x4b 'K'*/	0x8d,
/**0x4c 'L'*/	0xc7,
/**0x4d 'M'*/	0xaa,
/**0x4e 'N'*/	0xab,
/**0x4f 'O'*/	0xa3,
/**0x50 'P'*/	0x8c,
/**0x51 'Q'*/	0x98,
/**0x52 'R'*/	0xaf,
/**0x53 'S'*/	0x92,
/**0x54 'T'*/	0x87,
/**0x55 'U'*/	0xc1,
/**0x56 'V'*/	0xe3,
/**0x57 'W'*/	0x95,
/**0x58 'X'*/	0xe2,
/**0x59 'Y'*/	0x91,
/**0x5a 'Z'*/	0xb6,
/**0x5b '['*/   0xff,
/**0x5c '\'*/   0xff,
/**0x5d ']'*/   0xff,
/**0x5e '^'*/   0xff,
/**0x5f '_'*/   0xf7,
/**0x60 '@'*/	0xff,
/**0x61 'a'*/   0x88,
/**0x62 'b'*/   0x83,
/**0x63 'c'*/   0xc6,
/**0x64 'd'*/   0xa1,
/**0x65 'e'*/   0x86,
/**0x66 'f'*/   0x8e,
/**0x67 'g'*/   0xc2,
/**0x68 'h'*/   0x89,
/**0x69 'i'*/   0xe6,
/**0x6a 'j'*/   0xf2,
/**0x6b 'k'*/   0x8d,
/**0x6c 'l'*/   0xc7,
/**0x6d 'm'*/   0xaa,
/**0x6e 'n'*/   0xab,
/**0x6f 'o'*/   0xa3,
/**0x70 'p'*/   0x8c,
/**0x71 'q'*/   0x98,
/**0x72 'r'*/   0xaf,
/**0x73 's'*/   0x92,
/**0x74 't'*/   0x87,
/**0x75 'u'*/   0xc1,
/**0x76 'v'*/   0xe3,
/**0x77 'w'*/   0x95,
/**0x78 'x'*/   0xe2,
/**0x79 'y'*/   0x91,
/**0x7a 'z'*/   0xb6,
/**0x7b '{'*/   0xff,
/**0x7c '|'*/	0xf9,
/**0x7d '}'*/   0xff,
/**0x7e '~'*/   0xc8		//замена "П"
};      

cIndicator::cIndicator()
{
	ledCount = 0;
	segmCount = 0;
    for(unsigned char i = 0; i < 4; i++)
    {
        rawPict.segmMASK[i] = (1 << (i + SEGM_PIN0));
        for(unsigned char j = 0; j < 8; j++)
            rawPict.ledMASK[i][j] = 0;
    }
}

signed char cIndicator::getSegmCount(void)
{
	if(ledCount == 6)
		return segmCount;
	else
		return -1;
}

/**Функция преобразования в из текстовой строки в маску для сегментов*/
signed char cIndicator::convToPict(unsigned char *pImage)
{
	signed char length = 0;
	unsigned char tempMass[9] = {0,0,0,0,0,0,0,0,0};
	unsigned char CPYrawPictData[4] = {0x00,0x00,0x00,0x00};
	unsigned char ledMSK[4][8] = {{0,0,0,0,0,0,0,0,},
									{0,0,0,0,0,0,0,0,},
									{0,0,0,0,0,0,0,0,},
									{0,0,0,0,0,0,0,0,}};
    
    /**счет количества симолов в строке*/
	while(pImage[length] != 0x00)
	{
		length++;
	}
	
	/**Копирование символов*/
	for(unsigned char i = 0; i < length; i++)
		tempMass[i] = pImage[i];
	/***/
	
	/**Преобразование ascii в маски с совмещением некоторых символов*/
	/**отображение от правого края*/
	signed char i = 3, j = length - 1;
	for(/*signed char i = 3, j = length - 1*/; (i >= 0) && (j >= 0); i--, j--)
	{
		CPYrawPictData[i] |= ~(pictSegmConst[tempMass[j] - 0x20]);
		/**обработка объеденяемых симоволов "|----" "----|" "19.23" "_ _ _ _." */
		if(	(tempMass[j] == '|') || 
			(tempMass[j] == '.') || 
			((tempMass[j - 1] == '!') && (j > 0)) || 
			((tempMass[j - 1] == '-') && (tempMass[j] == '1') && (i == 0)) )
		{
			j--;
			CPYrawPictData[i] |= ~(pictSegmConst[tempMass[j] - 0x20]);
		}

	}

    /**Преобразование в массив [4][8] для быстрой работы в прерываниях*/
    for(unsigned char segm = 0; segm < 4; segm++)
    {
        for(unsigned char led = 0; led < 8; led++)
        {
			ledMSK[segm][led] |= CPYrawPictData[segm] & (1 << led);
        }
    }
	
    /**Копирование из временного массива в основной*/
    for(unsigned char i = 0; i < 4; i++)
    {
        for(unsigned char j = 0; j < 8; j++)
            rawPict.ledMASK[i][j] = ledMSK[i][j];
    }
    
	return (length + 1);		//длинна строки с нуль-терминалом
} 

/**Функция очистки основного буфера*/
void cIndicator::clrPict(void)
{
    for(unsigned char i = 0; i < 4; i++)
    {
        for(unsigned char j = 0; j < 8; j++)
            rawPict.ledMASK[i][j] = 0;
    }
}

/**Низкоуровенвая зависимая функция управления выводами для светодиодов*/
/**Перед вызовом этой функции в ТАЙМЕРЕ обработать кнопку на линии segmCount*/
void cIndicator::indicAvt(unsigned char avtStep)
{
	if(!avtStep)
	{
		ledCount++;
		ledCount &= 0x07;
		
		if(!ledCount)
		{
			/**Досчитали до 0го светодиода - перкл на след сегмент*/
			segmCount++;
			segmCount &= 0x03;
			
			/**Задвинуть новый сегмент в порт*/
			GPIO_SetBits(SEGM_PORT, GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12);
			GPIO_ResetBits(SEGM_PORT, rawPict.segmMASK[segmCount]);
//			SEGM_PORT &= 0xC3;
//			SEGM_PORT |= (1 << (segmCount + SEGM_PIN0));
		}

		/**Задвинуть новый светодиод в порт*/
		GPIO_ResetBits(LED_PORT, 0x000000ff);
		GPIO_SetBits(LED_PORT, rawPict.ledMASK[segmCount][ledCount]);
//        if(ledCount < 2)
//			LED_PORT &= rawPictData[segmCount] | (~(1 << ledCount));
//		else if(ledCount < 6)
//			LED_PORT &= (rawPictData[segmCount] << 2) | (~(1 << (ledCount + 2)));
//        else
//			SEGM_PORT &= (rawPictData[segmCount] >> 6) | (~(1 << (ledCount - 6)));
	}
	else
	{
		/**Очистить порты от светоиодов*/
		GPIO_ResetBits(SEGM_PORT, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);
//		LED_PORT |= 0xf3;
//		SEGM_PORT |= 0x03;			
	}
}

/**Функция инициализация функции мерцания дисплея*/
void cIndicator::initDFNC(unsigned char Logic, 
			  signed short bLow, 
			  signed short bHigh, 
			  unsigned char point)
{
	div = 1;
	
	for(unsigned char i = 0; i < point; i++)
		div *= 10;
	
	SPLO = bLow;
	SPHI = bHigh;
	
	blink_logic = Logic;
}

/**Функция вычисления допусков и принятия решении о мерцании*/
signed char cIndicator::dFnc(float val)
{
	signed char ret = 0;
	float tmpVal;
	
	if(blink_logic)
	{
		tmpVal = val * div;
		
		if((tmpVal >= SPLO) && ((tmpVal < SPHI)))
			ret = 1;
		else
			ret = 0;
		
		/**Инверсия при U*/
		if(blink_logic == U)
			ret ^= 1;
	}
	else
	{
		return 0;
	}
	
	return ret;
}

#pragma anon_unions

/**Функция преобразования цифр в массив вида {"88.88", 0x00} */
void convCharMass(float number, unsigned char precision)
{
	/**Идея:
	*	Получать число, округлять его до "precision" знаков после 
	*	запятой, разбивать на целую и дробную часть,
	*	преобразовывать в char обе эти части
	*/

	union {
        struct{
            unsigned minus: 2;
            unsigned point: 2;
            unsigned length: 4;
        };
        unsigned char msk;
	}str_properties;

	str_properties.minus = 0;
	str_properties.point = 0;
	str_properties.length = 0;

	float div = 1;
	volatile signed long integral = 0, fraction = 0, mult = 1;
	char temp_c[10]; 
	unsigned char temp_c_count = 9;

    /**Вычисление умножителей и делителей*/
	for (unsigned char i = 0; i < precision; i++)
	{
		mult *= 10;
		div *= 0.1;
	}

    /**Если число < 0 - поставить флаг знака минус*/
	if(number < 0)
	{
		str_properties.minus = 1;
		number *= -1;
	}

    /**Окргулить до нужной точности*/
	number *= mult;
	number += 0.5;

	integral = static_cast <signed long> (number);

    /**Если есть дробная часть - перобразовать в знаки и добавить точку*/
    if(precision)
    {
        str_properties.point = 1;
        while(precision)
        {
            temp_c[temp_c_count] = (integral%10) + 0x30;
            integral *= 0.1;
            temp_c_count--;
            precision--;
        }

		temp_c[temp_c_count] = '.';
		temp_c_count--;
    }

    /**Преобразовать целую часть хотя бы раз (для отображения "0")*/
    do{
        temp_c[temp_c_count] = (integral%10) + 0x30;
        integral *= 0.1;
        temp_c_count--;
    }while(integral);

    /**Если есть флаг минуса - посавить минус*/
    if(str_properties.minus)
    {
		temp_c[temp_c_count] = '-';
		temp_c_count--;
    }

	/**Если есть дробная часть проводить проверку нужды ее сокращать*/
	if(str_properties.point)
	{
		str_properties.length = 8 - temp_c_count;		//без учета 0х00 и ","

		if(str_properties.minus)
			str_properties.length--;

		/**Если количество симовлов безу учета - и , >4 - обрезать*/
		while(str_properties.length > 4)
		{
			str_properties.length--;
			temp_c_count++;
		}

	}

	unsigned char i = 0;

	for(unsigned char j = temp_c_count + 1 ; j < 10; j++, i++)
		textTemp[i] = temp_c[j];

    for(;i < 10; i++)
		textTemp[i] = 0x00;
}



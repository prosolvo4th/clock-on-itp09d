#ifndef __MODINDIC
#define __MODINDIC

#define LED_PORT	GPIOA	/* Port х Output */
#define LED_PIN0	0
#define SEGM_PORT	GPIOA
#define SEGM_PIN0	9

#define OFF			0
#define U			2
#define P			1

typedef struct{
    unsigned short segmMASK[4];
    unsigned char ledMASK[4][8];
}rawPict_t;

extern unsigned char rawPictData[4];
extern unsigned char ledCount;
extern unsigned char segmCount;
extern rawPict_t rawPict;

void convCharMass(float number, unsigned char precision = 0);

class cIndicator{
	public:
		cIndicator();
		signed char convToPict(unsigned char *pImage);
		void indicAvt(unsigned char avtStep);				//автомат для вызова по таймерам
		signed char getSegmCount(void);
		void clrPict(void);
		signed char dFnc(float val);
		void initDFNC(unsigned char Logic, 
					  signed short bLow, 
					  signed short bHigh, 
					  unsigned char point);
	private:
//		unsigned char rawPictData[4];
//		unsigned char ledCount;
//		unsigned char segmCount;
		signed short SPLO, SPHI;
		unsigned short div;
		unsigned char blink_logic;
		unsigned long blink_delay;
	protected:
};

#endif //__MODINDIC

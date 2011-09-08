/*
	Author: Timothy Alexander
	Objective: Configure ADC for Burst DMA Mode Operation with max sampling rate
*/
#include mbed.h
#define CLKS_PER_SAMPLE 65 //This is the number of cycles it takes to convert a sample


////////////////////Configuring the ADC for DMA + Burst Mode Operation////////////////////

//	1. Set PCADC bit in PCONP register to Power On
LPC_SC->PCONP |= (1 << 12);

//	2. Set PCLK_ADC bit in PCLKSEL0 Register
LPC_SC->PCLKSEL0 &= ~(0x3 << 24);

//	3. Enable Pins in PINSEL registers
LPC_SC 
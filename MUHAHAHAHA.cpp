/*
 * This example was provided to support Mbed forum thread:-
 * http://mbed.org/forum/mbed/topic/1798
 */
 
#include "mbed.h"
#include "MODDMA.h"

#define SAMPLE_BUFFER_LENGTH 1000

DigitalOut led1(LED1);
DigitalOut led2(LED2);

MODDMA dma;

//	This is going to alternate the different hands
//	Forcing it for now
bool control_ticker = true ;

//Method to return 
bool cal_slope(int x, int y);

// ISR set's this when transfer complete.
bool dmaTransferComplete = false;

void configadc(void);
// Function prototypes for IRQ callbacks.
// See definitions following main() below.
void TC0_callback(void);
void ERR0_callback(void);

LocalFileSystem local("local");

bool slope = false;


int main() {

    uint32_t adcInputBuffer[SAMPLE_BUFFER_LENGTH];    
    memset(adcInputBuffer, 0, sizeof(adcInputBuffer));
    
    // We use the ADC irq to trigger DMA and the manual says
    // that in this case the NVIC for ADC must be disabled.
    NVIC_DisableIRQ(ADC_IRQn);
    
    // Power up the ADC and set PCLK
    LPC_SC->PCONP    |=  (1UL << 12);
    LPC_SC->PCLKSEL0 &= ~(3UL << 24); // PCLK = CCLK/4 96M/4 = 24MHz
    
    // Enable the ADC, 12MHz,  ADC0.0 & .1
    LPC_ADC->ADCR  = (1UL << 21) | (1UL << 8) | (1UL << 0); 
    
    /* 
     * Every time the program loops the ADC pins are zeroed out
     * but depending on the rotation only 1 pin is active for sampling
     * MAY HAVE TO ADD ALOT MORE INSIDE THIS CONTROL STRUCTURE
     */
	LPC_PINCON->PINSEL1 &= ~(3UL << 14);  /* P0.23, Mbed p15. */
	LPC_PINCON->PINSEL1 &= ~(3UL << 16);  /* P0.24, Mbed p16. */

    switch (control_ticker){
		case true:LPC_PINCON->PINSEL1 |=  (1UL << 14); break;
		case false:LPC_PINCON->PINSEL1 |=  (1UL << 16); break;
	}
    
    
    
    // Open up output file for debugging
    FILE *log = fopen("/local/log.txt","w");
    fprintf(log,"ADC with DMA example\n");
    fprintf(log,"====================\n");
    
    
    //Configuration for DMA
    config_adc();
    
    //Burst Mode Configuration Stuffz
	switch (control_ticker){
		case true:LPC_ADC->ADCR |= (1UL << 16); break;
		case false:LPC_ADC->ADCR |= (2UL << 16); break;
	}
    
    
    
        // When transfer complete do this block.
        if (dmaTransferComplete) {
            delete conf; // No memory leaks, delete the configuration.
            dmaTransferComplete = false;
            
			//RAW DATA PRINTING BLOCK
            for (int i = 0; i < SAMPLE_BUFFER_LENGTH; i++) {
                int channel = (adcInputBuffer[i] >> 24) & 0x7;
                int iVal = (adcInputBuffer[i] >> 4) & 0xFFF;
                double fVal = 3.3 * (double)((double)iVal) / ((double)0x1000); // scale to 0v to 3.3v
                fprintf(log,"Array index %02d : ADC input channel %d = 0x%03x %01.3f volts\n", i, channel, iVal, fVal);                  
            }
            
            //Starting off the Peak Detector Sequence
            //Slope Bool holds the current state of the slope on the wave
            //PeakBuf holds the indices of the peaks
            //PeakTicker is the counter for PeakBuf
            
            
            fprintf(log, "Peak Detection Starting");
            slope = cal_slope(adcInputBuffer[2], adcInputBuffer[1]);
            fprintf(log, "Slope starts %s", slope);
            int PeakBuf[10];
                memset(PeakBuf, 0, sizeof(PeakBuf));
            int diffBuff[9];
				memset(diffBuff, 0, sizeof(PeakBuf));
			int PeakTicker = 0;
            double sum =0;
            for(int i = 1; i < SAMPLE_BUFFER_LENGTH - 1; i++){
                fprintf(log, "Slope is %s @ index %d\n", slope,i);
                switch(cal_slope(adcInputBuffer[i], adcInputBuffer[i-1])){
					case 1:
						if (slope == false){  // This is the condition in which you have found a Peak
							PeakBuf[PeakTicker] = i - 1; //Detects Peak After it has passed
							PeakTicker++;				 //Increment index of PeakBuf
							fprintf(log, "There is a peak @ index %d\n",i);

						 }
						 break;
					default:
						break;
				}
				if (PeakTicker == 10) break;				//Once the Peak Buffer is full quit
			}
			for (int i = 1; i ; i++){
				diffBuff[i] = PeakBuf[i] + PeakBuf[i+1];	 
			}
			for (int i = 0; i < sizeof(PeakBuf); i++){
				sum += diffBuff[i];
			}
			int mean = 200000/sum;
			
			fprintf(log,"The estimated frequency is %d\n", (int)mean);
			fclose(log);

        // Just flash LED1 for something to do.
        led1 = !led1;
        wait(0.25);        
    }	
}



// Configuration callback on TC
void TC0_callback(void) {
    
    MODDMA_Config *config = dma.getConfig();
    
    // Disbale burst mode and switch off the IRQ flag.
    LPC_ADC->ADCR &= ~(1UL << 16);
    LPC_ADC->ADINTEN = 0;    
    
    // Finish the DMA cycle by shutting down the channel.
    dma.haltAndWaitChannelComplete( (MODDMA::CHANNELS)config->channelNum());
    dma.Disable( (MODDMA::CHANNELS)config->channelNum() );
    
    // Tell main() while(1) loop to print the results.
    dmaTransferComplete = true;            
    
    // Switch on LED2 to show transfer complete.
    led2 = 1;        
    
    // Clear DMA IRQ flags.
    if (dma.irqType() == MODDMA::TcIrq) dma.clearTcIrq();    
    if (dma.irqType() == MODDMA::ErrIrq) dma.clearErrIrq();
}

// Configuration callback on Error
void ERR0_callback(void) {
    // Switch off burst conversions.
    LPC_ADC->ADCR |= ~(1UL << 16);
    LPC_ADC->ADINTEN = 0;
    error("Oh no! My Mbed EXPLODED! :( Only kidding, go find the problem");
}

void config_adc(void){
	// Prepare an ADC configuration.
    MODDMA_Config *conf = new MODDMA_Config;
    conf
     ->channelNum    ( MODDMA::Channel_0 )
     ->srcMemAddr    ( 0 )
     ->dstMemAddr    ( (uint32_t)adcInputBuffer )
     ->transferSize  ( SAMPLE_BUFFER_LENGTH )
     ->transferType  ( MODDMA::p2m )
     ->transferWidth ( MODDMA::word )
     ->srcConn       ( MODDMA::ADC )
     ->dstConn       ( 0 )
     ->dmaLLI        ( 0 )
     ->attach_tc     ( &TC0_callback )
     ->attach_err    ( &ERR0_callback )
    ; // end conf.
    
    // Prepare configuration.
    dma.Setup( conf );
    
    // Enable configuration.
    dma.Enable( conf );
    
    // Enable ADC irq flag (to DMA).
    // Note, don't set the individual flags,
    // just set the global flag.
    LPC_ADC->ADINTEN = 0x100;
}

bool cal_slope(int x, int y) {
	bool res = false;;
	if (x-y > 0) res = true; 
	return res;
}		    

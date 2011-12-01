/*
	Allowing for Static Pin Switchiing via Boolean
 */
 
#include "mbed.h"
#include "MODDMA.h"

#define SAMPLE_BUFFER_LENGTH 1000

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

MODDMA dma;
MODDMA_Config conf;


//	TRUE  => Pin 16
// 	FALSE => Pin 20
bool control_ticker = true ;

//Method to return 
bool cal_slope(int x, int y);

void rescale(void);

int amdf(void);

// ISR set's this when transfer complete.
bool dmaTransferComplete = false;

void config_adc(void);
// Function prototypes for IRQ callbacks.
// See definitions following main() below.
void TC0_callback(void);
void ERR0_callback(void);

LocalFileSystem local("local");

bool slope = false;

uint32_t adcInputBuffer[SAMPLE_BUFFER_LENGTH];    

int main() {
    
     // Open up output file for debugging
    FILE *log = fopen("/local/log.txt","w");
    fprintf(log,"ADC with DMA example\n");
    fprintf(log,"====================\n");
    
    
    // We use the ADC irq to trigger DMA and the manual says
    // that in this case the NVIC for ADC must be disabled.
    NVIC_DisableIRQ(ADC_IRQn);
    // Power up the ADC and set PCLK
    LPC_SC->PCONP    |=  (1UL << 12);
    LPC_SC->PCLKSEL0 &= ~(3UL << 24); // PCLK = CCLK/4 96M/4 = 24MHz
    
    
    memset(adcInputBuffer, 0, sizeof(adcInputBuffer));

    
    /*
		To set the Sampling Rate
		13Mhz is the default rate once the Clock is initialized
		With 8 Bits the lowest the Sampling Rate can be is ~50K
	 
		Bit 21 is Operational/Power Down Mode Bit
		Bits 15:8 are CLKDIV bits for setting Clock Divisor
		Bits 7:0 are Chip Pins selection bits
		AD0[0] --> p15 --> P0[23]
		AD0[1] --> p16 --> P0[24]
		AD0[2] --> p17 --> P0[25]
		AD0[3] --> p18 --> P0[26]
		AD0[4] --> p19 --> P3[30]
		AD0[5] --> p20 --> P3[31] 
		AD0[6] --> N/A --> P0[26] !!!NOT USED ON MBED!!!
		AD0[7] --> N/A --> P0[26] !!!NOT USED ON MBED!!!
	 */	 
	switch (control_ticker){
		case true:    //Activate the Channel for Pin 16
		    LPC_ADC->ADCR  = (1UL << 21) | (1UL << 8) | (1UL << 1); 
            break;
		case false:   //Activate the Channel for Pin 20  
		    LPC_ADC->ADCR  = (1UL << 21) | (1UL << 8) | (1UL << 5); 
	        break;
	}
    
    
    /* 
		Every time the program loops the ADC pins are zeroed out
		but depending on the rotation only 1 pin is active for sampling
		MAY HAVE TO ADD ALOT MORE INSIDE THIS CONTROL STRUCTURE
     
		Selecting a Pin
		i)	Reset the Pin by setting the bits to 00
		ii)	Set the Pin for ADC mode
		
		**PIN SELECTION REGISTERS**
		PINSEL1 ## USE 10 TO SET FUNCTION ##
		15:14 -> AD[0] --> Pin 15
		$$$$$17:16 -> AD[1] --> Pin 16
		19:18 -> AD[2] --> Pin 17
		21:20 -> AD[3] --> Pin 18 	!!!THIS IS FOR AOUT ONLY!!!
		PINSEL3 ## USE 11 TO SET FUNCTION ##
		29:28 -> AD[4] --> Pin 19
		$$$$31:30 -> AD[5] --> Pin 20
		
	*/
	LPC_PINCON->PINSEL3 &= ~(3UL << 16);  /* P0.23, Mbed p16. */
	LPC_PINCON->PINSEL1 &= ~(3UL << 30);  /* P0.24, Mbed p20. */

    switch (control_ticker){
		case true:LPC_PINCON->PINSEL1 |=  (1UL << 16); break;
		case false:LPC_PINCON->PINSEL3 |=  (1UL << 30); break;
	}
    
    
    
   
    
    //Configuration for DMA
    config_adc();
    
    //Activation for ADC Burst Mode
    LPC_ADC->ADCR |= (1UL << 16);
		while(1){
        // When transfer complete do this block.
        if (dmaTransferComplete) {
        	led4 = 1;

            //delete conf; // No memory leaks, delete the configuration.
            dmaTransferComplete = false;
            
			//RAW DATA PRINTING BLOCK
            for (int i = 0; i < SAMPLE_BUFFER_LENGTH; i++) {
                   
                int channel = (adcInputBuffer[i] >> 24) & 0x7;
                int iVal = (adcInputBuffer[i] >> 4) & 0xFFF;
                double fVal = 3.3 * (double)((double)iVal) / ((double)0x1000); // scale to 0v to 3.3v
                fprintf(log,"Array index %02d : ADC input channel %d = 0x%03x %01.3f volts\n", i, channel, iVal, fVal);                  
                if(i == SAMPLE_BUFFER_LENGTH) led3 = 1;
            }
                fclose(log);
		
        // Just flash LED1 for something to do.
        led1 = 1;        
    }
	
}
}



// Configuration callback on TC
void TC0_callback(void) {
    
    MODDMA_Config *config = dma.getConfig();
    
    // Disable burst mode and switch off the IRQ flag.
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
     conf.channelNum    ( MODDMA::Channel_0 );
     conf.srcMemAddr    ( 0 );
     conf.dstMemAddr    ( (uint32_t)adcInputBuffer );
     conf.transferSize  ( SAMPLE_BUFFER_LENGTH );
     conf.transferType  ( MODDMA::p2m );
     conf.transferWidth ( MODDMA::word );
     conf.srcConn       ( MODDMA::ADC );
     conf.dstConn       ( 0 );
     conf.dmaLLI        ( 0 );
     conf.attach_tc     ( &TC0_callback );
     conf.attach_err    ( &ERR0_callback );
    ; // end conf.
    
    // Prepare configuration.
    dma.Setup( &conf );
    
    // Enable configuration.
    dma.Enable( &conf );
    
    // Enable ADC irq flag (to DMA).
    // Note, don't set the individual flags,
    // just set the global flag.
    LPC_ADC->ADINTEN = 0x100;
}
	

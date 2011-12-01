/*
	Added in DMA Configurations
 */
 
#include "mbed.h"
#include "MODDMA.h"

#define SAMPLE_BUFFER_LENGTH 1000

DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

MODDMA dma;
MODDMA_Config adc_conf, *dac_conf0, *dac_conf1


//	TRUE  => Pin 16
// 	FALSE => Pin 20
bool control_ticker = false ;

//Method to return 
bool cal_slope(int x, int y);

// ISR set's this when transfer complete.
bool dmaTransferComplete = false;

void config_adc(void);
void config_dac0(void);
void config_dac1(void);

// Function prototypes for IRQ callbacks.
// See definitions following main() below.
void adc_TC0_callback(void);
void adc_ERR0_callback(void);
void dac_TC0_callback(void);
void dac_ERR0_callback(void);
void dac_TC1_callback(void);
void dac_ERR1_callback(void);

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
		case false:LPC_PINCON->PINSEL3 |=  (3UL << 30); break;
	}
    
    
    
   
    
    //Configuration for DMA
    config_adc();
    
    //Activation for ADC Burst Mode
    LPC_ADC->ADCR |= (1UL << 16);
    
    //DAC Activation
     if(!dac_dma.Prepare(conf0)){
            error("Conf0 could not be prepared, check configuration settings");
        }
           LPC_DAC->DACCNTVAL = NoteVal;
    
    
    
    
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






}
void config_adc(void){
	// Prepare an ADC configuration.
     adc_conf.channelNum    ( MODDMA::Channel_2 );
     adc_conf.srcMemAddr    ( 0 );
     adc_conf.dstMemAddr    ( (uint32_t)adcInputBuffer );
     adc_conf.transferSize  ( SAMPLE_BUFFER_LENGTH );
     adc_conf.transferType  ( MODDMA::p2m );
     adc_conf.transferWidth ( MODDMA::word );
     adc_conf.srcConn       ( MODDMA::ADC );
     adc_conf.dstConn       ( 0 );
     adc_conf.dmaLLI        ( 0 );
     adc_conf.attach_tc     ( &adc_TC0_callback );
     adc_conf.attach_err    ( &adc_ERR0_callback );
    ; // end conf.
    
    // Prepare configuration.
    dma.Setup( &adc_conf );
    
    // Enable configuration.
    dma.Enable( &adc_conf );
    
    // Enable ADC irq flag (to DMA).
    // Note, don't set the individual flags,
    // just set the global flag.
    LPC_ADC->ADINTEN = 0x100;
}


void dac_config0(void){
	// Prepare the GPDMA system for buffer0.
    conf0
     ->channelNum    ( MODDMA::Channel_0 )
     ->srcMemAddr    ( (uint32_t) &wave_table[0] )
     ->dstMemAddr    ( MODDMA::DAC )
     ->transferSize  ( OUTPUT_BUFFER_LENGTH )
     ->transferType  ( MODDMA::m2p )
     ->dstConn       ( MODDMA::DAC )
     ->attach_tc     ( &TC0_callback )
     ->attach_err    ( &ERR0_callback )     
    ; // config end
}
void dac_config1(void){
	// Prepare the GPDMA system for buffer1.
    conf1
     ->channelNum    ( MODDMA::Channel_1 )
     ->srcMemAddr    ( (uint32_t) &wave_table[1])
     ->dstMemAddr    ( MODDMA::DAC )
     ->transferSize  ( OUTPUT_BUFFER_LENGTH )
     ->transferType  ( MODDMA::m2p )
     ->dstConn       ( MODDMA::DAC )
     ->attach_tc     ( &TC1_callback )
     ->attach_err    ( &ERR1_callback )     
    ; // config end
}

// Configuration callback on TC
void adc_TC0_callback(void) {
    
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


void dac_TC0_callback(void){
	//Get Configuration Pointer and Shut Down DMA Channel
    MODDMA_Config *config = dac_dma.getConfig();
    dac_dma.Disable( (MODDMA::CHANNELS)config->channelNum());
    
    //Swaps to Buffer 1
    dma.Prepare(dac_conf1);
    
    //Resets IRQ Flags
    if (dma.irqType() == MODDMA::TcIrq) ddma.clearTcIrq(); 
}

void dac_TC1_callback(void){
	//Get Configuration Pointer and Shut Down DMA Channel
    MODDMA_Config *config = dma.getConfig();
    dma.Disable( (MODDMA::CHANNELS)config->channelNum());
    
    //Swaps to Buffer 1
    dma.Prepare(dac_conf0);
    
    //Resets IRQ Flags
    if (dma.irqType() == MODDMA::TcIrq) dma.clearTcIrq();
}


// Configuration callback on Error
void adc_ERR0_callback(void) {
    // Switch off burst conversions.
    LPC_ADC->ADCR |= ~(1UL << 16);
    LPC_ADC->ADINTEN = 0;
    error("Oh no! My Mbed EXPLODED! :( Only kidding, go find the problem");

void dac_ERR0_callback(void){error("DAC_0 Failed");}

void dac_ERR1_callback(void){error("DAC_1 Failed");}





	

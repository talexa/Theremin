/*
 * Sampling with FFT
 * Tim Alexander
 * November 23, 2011
 */


#include 'mbed.h'
#include 'MODDMA.h'


#define MN 256 //This is the number of points for the FFT
#define SAMPLE_RATE 48000	//48kHz is standard sampling rate
#define SAMPLE_BUFFER_LENGTH 4000 //Need to math this out
#define SERIAL_BAUD 115200 // Must be same as Serial Monitor baud

MODDMA dma;	//GPDMA Controller Object

//Configuration Settings for Single Channel ADC
MODDMA_Config conf = new MODDMA_Config; 

Serial pc(USBTX,USBRX);

/*
//Might use this bit later for file output if i need it
LocalFileSystem local("local");	//Setting filesystem so i can write output to the Mbed
FILE *fp; //File Pointer
*/

bool dmaTransferComplete = false;

//ADC Activation, Configuration and Pin Selection Routine
void configure_ADC(void);
//GPDMA Timer Callback Interrupt Request Routine
void TC0_callback(void);
//GPDMA Error Callback Interrupt Request Routine
void ERR0_callback(void);


//FFT Radix 4 Function Declaration
extern "C" void fftR4(short *y,short *x, int N);

int main() {
	pc.baud(SERIAL_BAUD); //Setting Serial Up	

	uint16_t adcInputBuffer[SAMPLE_BUFFER_LENGTH);
	memset(adcInputBuffer, 0 sizeof(adcInputBuffer));
	
	configure_ADC();
	
	// !!! This Activates the A/D Conversions !!!
	LPC->ADCR |= (1UL << 16);
	
	
	if (dmaTransferComplete) { //This Triggers when
		//Do Interesting Stuff Here
		dmaTransferComplete = false;
	}
	
	
	
	/*
		Psuedocode:
		Sample then wait for TC Callback
		Cut 2 Bits from All Samples
		Run FFT with samples
		Find highest value in output array
		Convert that to a frequency
		Print
	 */
	
	
	
	
	
	

}


void configure_ADC(void) {
	
	NVIC_DisableIRQ(ADC_IRQn);

	
	//Bit 12 is ADC Power/Clock Control Bit
	LPC_SC->PCONP    |= (1UL << 12);
	
	//Bits 25:24 are Peripheral Clock Selection Bits
	LPC_SC->PCLKSEL0 &= (3UL << 24);
	
	/*
		Bit 21 is Operational/Power Down Mode Bit
		Bits 15:8 are CLKDIV bits for setting Clock Divisor
		Bits 7:0 are Chip Pins selection bits
		AD0[0] --> p15 --> P0[23]
		AD0[1] --> p16 --> P0[24]
		AD0[2] --> p17 --> P0[25]
		AD0[3] --> p18 --> P0[26]
		AD0[4] --> p19 --> P1[30]
		AD0[5] --> p20 --> P1[31] 
		AD0[6] --> N/A --> P0[26] !!!NOT BROKEN ON MBED!!!
		AD0[7] --> N/A --> P0[26] !!!NOT BROKEN ON MBED!!!
	 */
	LPC_ADC->ADCR = (1UL << 21) | (1UL << 8) | (1UL << 0);
	
	/*
		Selecting a Pin
		i)	Reset the Pin by setting the bits to 00
		ii)	Set the Pin to 10 for ADC mode
	 */
	
	LPC_PINCON->PINSEL1 &= ~(3UL << 14);	//Clears Bits 
	LPC_PINCON->PINSEL1 |= (1UL << 14);		//Sets Bits
	
	//ADC Configuration for DMA Controller
    
    conf.channelNum    ( MODDMA::Channel_0 )
    conf.srcMemAddr    ( 0 )
    conf.dstMemAddr    ( (uint16_t)adcInputBuffer )
    conf.transferSize  ( SAMPLE_BUFFER_LENGTH )
    conf.transferType  ( MODDMA::p2m )
    conf.transferWidth ( MODDMA::word )
    conf.srcConn       ( MODDMA::ADC )
	conf.dstConn       ( 0 )
    conf.dmaLLI        ( 0 )
    conf.attach_tc     ( &TC0_callback )
    conf.attach_err    ( &ERR0_callback )
	
	// !!! Must pass pointer to the following 2 functions !!!
	dma.Setup(&conf);
	dma.Enable(&conf);
	
	
}

/*
	TC0 Callback is made when the DMA transfer is done. 
	Certain Events have to be done before any other control flow is possible
	Disable Burst mode and Turn off IRQ Flag
	Shut Down the DMA Channel
	Set Internal Flags
	Clear DMA IRQ Flags
*/
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
    
    // Clear DMA IRQ flags.
    if (dma.irqType() == MODDMA::TcIrq) dma.clearTcIrq();    
    if (dma.irqType() == MODDMA::ErrIrq) dma.clearErrIrq();
}

// Configuration callback on Error
void ERR0_callback(void) {
    // Switch off burst conversions.
    LPC_ADC->ADCR |= ~(1UL << 16);
    LPC_ADC->ADINTEN = 0;
    error("Epic Failure; there was a DMA ERR!");
}




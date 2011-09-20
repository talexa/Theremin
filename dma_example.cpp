/*
	Timothy Alexander
	MileStone #1 Code
	Objective: Print Output of ADC to Serial Monitor

*/

#include mbed.h
#include MODDMA.h
#include MODSERIAL.h
#define SAMPLE_BUFFER_LENGTH 32
#define NUM_OF_SAMPLES 1


//Will be used for Interrupt Service Routine
bool dmaTransferComplete = false;

//Need to make a buffer for taking in A/D Convertor Samples
uint32_t adBuffer[NUM_OF_SAMPLES * 3];
b
Serial pc(USBTX,USBRX);
MODDMA dma;

//Function Prototypes
void dmaTCCallback(void);
void dmaERRCallback(void);
void TC0_callback(void);
void ERR0_callback(void);

extern "C" void TIMER1_handler(void) __irq {
    led2 = !led2; // Show life
    dma.Setup( conf ); 	// Prepare Transfer
    dma.Enable( conf );
    LPC_ADC->ADCR |= (1UL << 16); // ADC burst mode
    LPC_ADC->ADINTEN = 0x100;     // Do all channels.
    LPC_TIM1->IR = 1; // Clr timer1 irq.
}


int main(){
	uint32_t adcInputBuffer[SAMPLE_BUFFER_LENGTH]; 		//Buffer to hold most recent ADC conversion
	memset(adBuffer, 0, sizeof(adcInputBuffer));		//Clear the Buffer

	NVIC_DisableIRQ(ADC_IRQn);							//ADC will trigger IRQ, so NVIC must be disabled
	
	LPC_SC->PCONP 		|= (1UL << 12);					//Power On ADC
	LPC_SC->PCLKSEL0 	&= ~(3UL << 24);				//PCLK = CCLK/4 96M/4 = ~24Mhz

	//Configuring Pins for ADC
	LPC_PINCON->PINSEL1 &= ~(3UL << 14);	// Mbed Pin 15
	LPC_PINCON->PINSEL1 |= (1UL << 14);
	
	/*
	 * Not Going to use this for Milestone 1
	LPC_PINCON->PINSEL1 &= ~(3UL << 16);	// Mbed Pin 16
	LPC_PINCON->PINSEL1 |= (3UL << 16);
	*/
	
	//Setting the Baud Rate for the Serial Monitor
	pc.baud(115200);
	pc.printf("Getting Values!!!/n");
	pc.printf("========================/n");
	
	
	//Creating a new MODDMA Configuration Object
	MODDMA_Config *config = new MODDMA_config;
	
	config
	->ChannelNum	( MODDMA::Channel_0)//First Channel Available
	->srcMemAddr 	( 0 )				//No Memory Address since Source will be ADC
	->transferType 	( MODDMA::p2m)  	//Peripheral to Memory transfer
	->transferWidth ( MODDMA::word)		//Transfers will be word sized
	->srcConn 		( MODDMA::ADC) 		//Source is ADC
	->dstConn		( 0 )    
	->dmaLLI		( 0 )
	->attach_tc     ( &TC0_callback )	//Attaching Callback Functions...
	->attach_err    ( &ERR0_callback )	//To the DMA Controller
	;
	
	dma.Setup(config);
	dma.Enable(config);	
}

//Configuration callback on TC
void TC0_callback(void){
	MODDMA_Config *config = dma.getConfig();
	
	//Disable Burst Mode and Switch off IRQ Flag
	LPC_ADC->ADCR $= ~(1UL << 16);
	LPC_ADC->ADINTEN = 0;
	
	//Close DMA Channel to Finish the Cycle
	dma.haltAndWaitChannelComplete( (MODDMA::CHANNELS)config->channelNum());
	dma.Disable( (MODDMA::CHANNELS)config->channelNum() );
	
	//Clear DMA IRQ Flags
	dma.clearTcIrq();	
	dma.clearErrIrq();
		
}
//Configuration callback on Error
void ERR0_callback(void){
	//Need to Turn Off Burst Conversions
	LPC_ACD->ADCR |= ~(1UL << 16);
	LPC_ADC->ADINTEN = 0;
	error("Error with DMA!!!")
}

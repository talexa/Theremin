/*
	Timothy Alexander

	These are the configuration settings for the DMA controller using MODDMA library
	This configuration uses a single channel of the controller for the ADC convertor

*/

#include mbed.h
#include MODDMA.h

#define SAMPLE_BUFFER_LENGTH 32

//Will be used for Interrupt Service Routine
bool dmaTransferComplete = false;

Serial pc(USBTX,USBRX);
MODDMA dma;

//Function Prototypes
void dmaTCCallback(void);
void dmaERRCallback(void);
void TC0_callback(void);
void ERR0_callback(void);

int main(){
	//Setting the Baud Rate for the Serial Monitor
	pc.baud(115200);
	pc.printf("Testing out ADC with DMA/n");
	pc.printf("========================/n");

	
	//Buffer to hold most recent ADC conversion
	uint32_t adcInputBuffer[SAMPLE_BUFFER_LENGTH]; 
	
	//Attaching the Callback Functions to the dma object
	dma.attach_tc( &dmaTCCallback );
    dma.attach_err( &dmaERRCallback );
	
	
	//Creating a new MODDMA Configuration Object
	MODDMA_Config *testconfig = new MODDMA_config;
	
	testconfig
	->ChannelNum	( MODDMA::Channel_0)//First Channel Available
	->srcMemAddr 	( 0 )				//No Memory Address since Source will be ADC
	->transferType 	( MODDMA::p2m)  	//Peripheral to Memory transfer
	->transferWidth ( MODDMA::word)		//Transfers will be word sized
	->srcConn 		( MODDMA::ADC) 		//Source is ADC
	->dstConn		( 0 )    
	->dmaLLI		( 0 )
	->attach_tc     ( &TC0_callback )
	->attach_err    ( &ERR0_callback )
	;
	
	dma.Setup(config);
	dma.Enable(config);	
}

//DMA Controller ERR IRQ Callback
void dmaERRCAllback(void){
	error("There be an error");
}
//DMA Controller TC IRQ callback
void dmaERRCallback(void){
	
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
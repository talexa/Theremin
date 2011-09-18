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
	
	config
	->ChannelNum	( MODDMA::Channel_0) 
	->srcMemAddr 	( 0 )
	->transferType 	( MODDMA::p2m)  //Peripheral to Memory transfer
	->transferWidth ( MODDMA::word)	//Transfers will be word sized
	->srcConn 		( MODDMA::ADC)  //Source is ADC
	->dstConn		( 0 )
	->dmaLLI		( 0 )
	->attach_tc     ( &TC0_callback )
	->attach_err    ( &ERR0_callback )
	;
	
	dma.Setup(config);
	dma.Enable(config);
	
	
	
}

	

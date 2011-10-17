/*
Written By: Tim Alexander
Objective: Audio Output on Mbed based on WaveTable

This phase of the program will output audio from Pin 18 (Analog Out)
The Onboard DAC is a 10-bit model, for simplicity we are sticking to an 8 bits of precision waveform
This gives 2^8 or 256 Steps in a Sine Wave having 1 Period

The Output Frequencies are going to be modeled on a traditional Piano, except the 2 Highest Octaves
This gives us a frequency range from 27Hz at the lowest key to ~1kHz at the highest key, in steppings as close as possible to the Piano
Piano Keys are calculated to the 3 decimal place but a Digital Device is restricted to non-fractional frequencies

Since it will take 256 Steps to iterate a wave, what must be calculated is the time between updating the DAC Output with the next step in the Wave Table

For instance to do the lowest key on the Piano(27Hz) we have to cycle the waveform 27 times per second. Since each waveform is 256 updates per second, this is calculated as
    256 * 27   = 6912 updates per second for the DAC
    256 * 1000 = 256000 updates for the highest key in our spectrum
    

    Ideally this is going to be done with the DMA Controller so that the CPU does not have to do so much work
    
    This can be done with Double Buffering:
    
        CNT_ENA bit and DBLBUF_ENA bits are set in the DACCTRL Register
    
        Writes to DACR register are written to a buffer, and then transferred out on the next counter tick
    
    The DMA Counter is configured as Follows:
    
        Rate is set in PCLK_DAC register
        
        The Counter is Decremented from the value in the DACCNTVAL register
    
        When the value in the counter reaches zero, it will reset to the value DACCNTVAL
        
        Need to set up DMA as in the ADC section.....
        
*/


#include mbed.h
#include MODDMA.h
#define OUTPUT_BUFFER_LENGTH 256        

MODDMA dac_dma; //Creating DMA Object for DAC Output

MODDMA_Config *conf0, *conf1

uint8_t dacOutputBuffer[OUTPUT_BUFFER_LENGTH];   // Creating WaveTable 

uint16_t NoteBuffer[] = 
{
	66816,70921,75008,79616,84224,89344,94464,100096
};

int main() {

		*conf0 = new MODDMA_config;
        conf0
        ->ChannelNum ( MODDMA::Channel_0)   	//First Channel Available
        ->srcMemAddr ( (uint32_t) & buffer[0] ) //Buffer Source
        ->dstMemAddr ( MODDMA::DAC)
		->transferSize ( 256 )     				//256 Transfers
		->transferType ( MODDMA::DAC)       	//Peripheral to Memory transfer
        ->dstConn ( MODDMA::DAC )                         
        ->dmaLLI ( 0 )                      	//Linked List 1=Yes, 0=No
        ->attach_tc ( &TC0_callback )       	//Attaching Callback Functions...
        ->attach_err ( &ERR0_callback )     	//To the DMA Controller
        ; // end configuration
		
		*conf1 = new MODDMA_config;
		conf1
		->channelNum    ( MODDMA::Channel_1 )
		->srcMemAddr    ( (uint32_t) &buffer[1] )
		->dstMemAddr    ( MODDMA::DAC )
		->transferSize  ( 256 )
		->transferType  ( MODDMA::m2p )
		->dstConn       ( MODDMA::DAC )
		->attach_tc     ( &TC1_callback )
		->attach_err    ( &ERR1_callback )     
		; // end configuration
		

		
		
		
		//Begin DMA Transfers and Counter
		LPC_DAC->DACCTRL |= (3UL << 2);
		



    while(1) {
            LPC_DAC->DACCNTVAL = NoteBuffer[0];
            LPC_DAC -> 00000000000000010000000000000000 | dacOutputBuffer[step] >> 6;
            
        }
    }
}
void TC0_callback(void){

	//Get Configuration Pointer and Shut Down DMA Channel
	MODDMA_Config *config = dma.getConfig();
	dma.Disable( (MODDMA::CHANNELS)config->channelNum());
	
	//Swaps to Buffer 1
	dma.Prepare(conf1);
	
	//Resets IRQ Flags
	if (dma.irqType() == MODDMA:TcIrq) dma.clearTcIrq(); 
}
void TC1_callback(void){
	//Get Configuration Pointer and Shut Down DMA Channel
	MODDMA_Config *config = dma.getConfig();
	dma.Disable( (MODDMA::CHANNELS)config->channelNum());
	
	//Swaps to Buffer 1
	dma.Prepare(conf0);
	
	//Resets IRQ Flags
	if (dma.irqType() == MODDMA:TcIrq) dma.clearTcIrq();
}
//Configuration 0 Error Callback
void ERR0_callback(void){
	error("DMA_0 Failed");
}
void ERR1_callback(void){
	error("DMA_1 Failed");
}


//Method to Populate Wave Table

//Method to Set the DAC in
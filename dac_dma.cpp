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


uint8_t  wave_table[256] = {
  0x80, 0x83, 0x86, 0x89, 0x8C, 0x90, 0x93, 0x96,
  0x99, 0x9C, 0x9F, 0xA2, 0xA5, 0xA8, 0xAB, 0xAE,
  0xB1, 0xB3, 0xB6, 0xB9, 0xBC, 0xBF, 0xC1, 0xC4,
  0xC7, 0xC9, 0xCC, 0xCE, 0xD1, 0xD3, 0xD5, 0xD8,
  0xDA, 0xDC, 0xDE, 0xE0, 0xE2, 0xE4, 0xE6, 0xE8,
  0xEA, 0xEB, 0xED, 0xEF, 0xF0, 0xF1, 0xF3, 0xF4,
  0xF5, 0xF6, 0xF8, 0xF9, 0xFA, 0xFA, 0xFB, 0xFC,
  0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0xFD,
  0xFD, 0xFC, 0xFB, 0xFA, 0xFA, 0xF9, 0xF8, 0xF6,
  0xF5, 0xF4, 0xF3, 0xF1, 0xF0, 0xEF, 0xED, 0xEB,
  0xEA, 0xE8, 0xE6, 0xE4, 0xE2, 0xE0, 0xDE, 0xDC,
  0xDA, 0xD8, 0xD5, 0xD3, 0xD1, 0xCE, 0xCC, 0xC9,
  0xC7, 0xC4, 0xC1, 0xBF, 0xBC, 0xB9, 0xB6, 0xB3,
  0xB1, 0xAE, 0xAB, 0xA8, 0xA5, 0xA2, 0x9F, 0x9C,
  0x99, 0x96, 0x93, 0x90, 0x8C, 0x89, 0x86, 0x83,
  0x80, 0x7D, 0x7A, 0x77, 0x74, 0x70, 0x6D, 0x6A,
  0x67, 0x64, 0x61, 0x5E, 0x5B, 0x58, 0x55, 0x52,
  0x4F, 0x4D, 0x4A, 0x47, 0x44, 0x41, 0x3F, 0x3C,
  0x39, 0x37, 0x34, 0x32, 0x2F, 0x2D, 0x2B, 0x28,
  0x26, 0x24, 0x22, 0x20, 0x1E, 0x1C, 0x1A, 0x18,
  0x16, 0x15, 0x13, 0x11, 0x10, 0x0F, 0x0D, 0x0C,
  0x0B, 0x0A, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04,
  0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03,
  0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x0A,
  0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x13, 0x15,
  0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24,
  0x26, 0x28, 0x2B, 0x2D, 0x2F, 0x32, 0x34, 0x37,
  0x39, 0x3C, 0x3F, 0x41, 0x44, 0x47, 0x4A, 0x4D,
  0x4F, 0x52, 0x55, 0x58, 0x5B, 0x5E, 0x61, 0x64,
  0x67, 0x6A, 0x6D, 0x70, 0x74, 0x77, 0x7A, 0x7D
};

/* 
 * Determining the value for DACCNTVAL
 * PCLK is set to Oscillate at 24Mhz
 * The formula is in general with f being the intended output frequency
 * DACCNTVAL = 24Mhz/(f * WaveTableSize)
 * These Count values are going to be stored in the NoteBuffer[] Array as defined below
 */

uint16_t NoteBuffer256[] = 
{
	358,	//Middle C
	338,
	319,
	301,
	284,
	268,
	253,
	239,
	226,
	213		//A4
};

uint16_t NoteBuffer360[] = 
{
	255,	//Middle C
	241,
	227,
	214,
	202,
	191,
	180,
	170,
	161,
	152,	//A4
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
void makeWaveTable(void){
	for (i = 0xFE; i++; i < 
}


//Method to Set the DAC in the correct mode

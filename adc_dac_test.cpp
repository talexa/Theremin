/*
Written By: Tim Alexander
Objective: Audio Output on Mbed based on WaveTable

This phase of the program will output audio from Pin 18 (Analog Out)
The DAC is 10-bit, for mathematical simplicity the Output Waveform will have 360 Steps
* 
The Output Frequencies are going to be modeled on a traditional Piano, except the 2 Highest Octaves
This gives us a frequency range from 27Hz at the lowest key to ~1kHz at the highest key, in steppings as close as possible to the Piano
Piano Keys are calculated to the 3 decimal place but a Digital Device is restricted to non-fractional frequencies

Since it will take 256 Steps to iterate a wave, what must be calculated is the time between updating the DAC Output with the next step in the Wave Table

To make a cleaner wave that is not interrupted by CPU activity, a technique called double buffering will be used.
By using 2 DMA channels for the DAC and two buffers the DMA can read output to the DAC continually without interrupt

The DMA Channels:
	Channel 0 : DAC Buffer
	Channel 1 : DAC Buffer
	Channel 2 : ADC Buffer

        
*/

#define OUTPUT_BUFFER_LENGTH 360 
#define SAMPLE_BUFFER_LENGTH 32

#include "mbed.h"
#include "MODDMA.h"

AnalogOut output(p18);       

DigitalOut led1(LED1);

MODDMA dma; //Creating DMA Object 
MODDMA_Config *conf0, *conf1, *conf2;

//Function Prototypes
void TC0_callback(void);
void ERR0_callback(void);
void TC1_callback(void);
void ERR1_callback(void);
void TC2_callback(void);
void TC2_callback(void);

int wave_table[2][OUTPUT_BUFFER_LENGTH];
int NoteVal = 152;
/* 
 * Determining the value for DACCNTVAL
 * PCLK is set to Oscillate at 24Mhz, can be reduced at intervals but not necessary since our lowest note works np
 * The formula is in general with f being the intended output frequency
 * DACCNTVAL = 24Mhz/(f * WaveTableSize)
 * These Count values are going to be stored in the NoteBuffer[] Array as defined below
 * See Piano Key Frequencies.pdf in GitHub Repo for a full list
 */

uint16_t NoteBuffer[] = 
{
    358,    //Middle C
    338,
    319,
    301,
    284,
    268,
    253,
    239,
    226,
    213        //A4
};


int main() {
/*
 * Generation of a Sine Wave of 360 Points
 */
    for (int i =   0; i <=  90; i++) wave_table[0][i] =  (512 * sin(3.14159/180.0 * i)) + 512;                
    for (int i =  91; i <= 180; i++) wave_table[0][i] =  wave_table[0][180 - i];
    for (int i = 181; i <= 270; i++) wave_table[0][i] =  512 - (wave_table[0][i - 180] - 512);
    for (int i = 271; i <  360; i++) wave_table[0][i] =  512 - (wave_table[0][360 - i] - 512);
    
    
/*
 * First Thing to do is modify wave table to work with DAC Hardware. 
 * Since I am double buffering with DMA then I need to duplicate the wavetable. 
 * As specified in LPC17xx user manual(p583), the DACR Register controls the Voltage Output. In our wavetable we only have naive voltage values.
 * Bit 16 of the DACR is the Power Bit, because we are operating well below the update speed we can set this to 1 for Power Mode
 * Bits 15:6 of the DACR is the VALUE field which contains the voltage output.
 * Bits 5:0 of the DACR are reserved, and should not contain any value
 * 
 * Because of the above situation we have to condition our values to accomodate the hardware
 * 
 * wave_table[i] = (1 << 16) | (wavetable[i] << 6) & 0xFFC0
 * 
 * (1<<16) sets the DAC in Power Mode which Gives a settling time of 2.5us and max current of 350uA and a max update rate of 400kHz
 * (wavetable[i] << 6) & 0xFFCO shifts the array data into the VALUE field 
 * 
 */
    for(int i = 0; i < OUTPUT_BUFFER_LENGTH; i++){
        wave_table[0][i] = (1 << 16) | (wave_table[0][i] << 6) & 0xFFC0;
        wave_table[1][i] = wave_table[0][i];
    }




    // Prepare the GPDMA system for buffer0.
    conf0 = new MODDMA_Config;
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
    
    
    // Prepare the GPDMA system for buffer1.
    conf1 = new MODDMA_Config;
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


	//Prepare the GPDMA for ADC Input
	conf2 = new MODDMA_Config;
	conf2	
	 ->ChannelNum	 ( MODDMA::Channel_2)		 //First Channel Available
	 ->srcMemAddr 	 ( 0 )						 //No Memory Address since Source will be ADC
	 ->dstMemAddr	 ( (uint32_t)adcInputBuffer )//This is where the DMA will send the conversion
	 ->transferSize  (SAMPLE_BUFFER_LENGTH)      //Size of the Transfer
	 ->transferType  ( MODDMA::p2m)  		     //Peripheral to Memory transfer
	 ->transferWidth ( MODDMA::word)		     //Transfers will be word sized
	 ->srcConn 		 ( MODDMA::ADC) 			 //Source is ADC
	 ->dstConn		 ( 0 )    					 //Not using Peripheral as Destination
	 ->dmaLLI		 ( 0 )						 //Not using Linked List Functions
	 ->attach_tc     ( &TC2_callback )			 //Attaching Callback Functions...
	 ->attach_err    ( &ERR2_callback )			 //To the DMA Controller
	;	// config end
	
	
        //Load the Configuration Settings into the DMA Controller


/*
 * Digital-to-Analog Converter Configuration Settings
 */         
        if(!dma.Prepare(conf0)){
            error("Conf0 could not be prepared, check configuration settings");
        }
        
        //Set the Output Frequency...
        LPC_DAC->DACCNTVAL = NoteVal;
        //Begin DMA Transfers to the DAC
        LPC_DAC->DACCTRL |= (3UL << 2);
        
        
/*
 * Analog-to-Digital Converter Configuration Settings
 */       
        
		LPC_ADC->ADINTEN = 0x100;		//Setting ADC IRQ Global Flag to the DMA
		LPC_ADC->ADCR |= (1UL << 16);	//Burst Mode
        
        
        

    while(1) {
		//
       }
}

void TC0_callback(void){

    //Get Configuration Pointer and Shut Down DMA Channel
    MODDMA_Config *config = dma.getConfig();
    dma.Disable( (MODDMA::CHANNELS)config->channelNum());
    
    //Swaps to Buffer 1
    dma.Prepare(conf1);
    
    //Resets IRQ Flags
    if (dma.irqType() == MODDMA::TcIrq) dma.clearTcIrq(); 
}
void TC1_callback(void){
    //Get Configuration Pointer and Shut Down DMA Channel
    MODDMA_Config *config = dma.getConfig();
    dma.Disable( (MODDMA::CHANNELS)config->channelNum());
    
    //Swaps to Buffer 1
    dma.Prepare(conf0);
    
    //Resets IRQ Flags
    if (dma.irqType() == MODDMA::TcIrq) dma.clearTcIrq();
}
void TC2_callback(void){
	
	
}
//Configuration 0 Error Callback
void ERR0_callback(void){
    error("DMA_0 Failed");
}
void ERR1_callback(void){
    error("DMA_1 Failed");
}
void ERR2_callback(void){
	error("DMA_2 Failed")
}

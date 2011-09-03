/*
Title: Mbed Hello World
Author: Timothy Alexander
Desc: Lights an LED when Input Voltages crosses a Threshold
*/

#include "mbed.h"

AnalogIn input(p15);
DigitalOut led(LED1);

int main() {
    while(1){
        if(input > .66){
            led = 1;
        } else {
            led = 0;
        }    
    }
}

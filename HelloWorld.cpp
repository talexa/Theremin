/*
Title: Mbed Hello World
Author: Timothy Alexander
Desc: Blinks an LED on the Mbed Platform
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

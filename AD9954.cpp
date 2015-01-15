/*
   AD9954.cpp - AD9954 DDS communication library
   Created by Neal Pisenti, 2013
   JQI - Strontium - UMD

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */


#include "Arduino.h"
#include "SPI.h"
#include <AD9954.h>


/* CONSTRUCTOR */

// Constructor function; initializes communication pinouts
AD9954::AD9954(byte ssPin, byte resetPin, byte updatePin, byte ps0, byte ps1, byte osk)
{
    RESOLUTION  = 4294967296.0;
    _ssPin = ssPin;
    _resetPin = resetPin;
    _updatePin = updatePin;
    _ps0 = ps0;
    _ps1 = ps1;
    _osk = osk;


    // sets up the pinmodes for output
    pinMode(_ssPin, OUTPUT);
    pinMode(_resetPin, OUTPUT);
    pinMode(_updatePin, OUTPUT);
    pinMode(_ps0, OUTPUT);
    pinMode(_ps1, OUTPUT);
    pinMode(_osk, OUTPUT);

    // defaults for pin logic levels
    digitalWrite(_ssPin, HIGH);
    digitalWrite(_resetPin, LOW);
    digitalWrite(_updatePin, LOW);
    digitalWrite(_ps0, LOW);
    digitalWrite(_ps1, LOW);
    digitalWrite(_osk, LOW);

    
}


/* PUBLIC CLASS FUNCTIONS */


// initialize(refClk) - initializes DDS with reference clock frequency refClk
void AD9954::initialize(unsigned long refClk){
    _refIn = refClk;
    _refClk = refClk;

    AD9954::reset();

    byte registerInfo[] = {0x00, 4};
    byte data[] = {0x00, 0x00, 0x00, 0x00};
    AD9954::writeRegister(registerInfo, data);

}

// initialize(refClk, clkMult) -- initializes DDS with input refClk, and activates the
//      on-board PLL multiplier clkMult.
//      clkMult: must be integer between 4 and 20 inclusive.
void AD9954::initialize(unsigned long refClk, byte clkMult){
    _refIn = refClk;
    _refClk = _refIn * clkMult;

    // resets DDS
    AD9954::reset();


    byte multValue = clkMult;
    byte registerInfo[] = {0x01, 3};
    byte data[] = {0x18, 0x00, 0x00};

    // writes value for clock multiplier
    if (_refClk < 25000000){ 
        multValue <<= 1;
        //multValue = multValue << 1;
    }else{
        (multValue <<= 1)++;
        //multValue = (multValue << 1) + 1;

        multValue <<= 2;
        //multValue = multValue << 2;

        data[2] = lowByte(multValue);

        AD9954::writeRegister(registerInfo, data);
    }
}
        



// reset() - takes no arguments; resets DDS
void AD9954::reset(){
    digitalWrite(_resetPin, HIGH);
    delay(1);
    digitalWrite(_resetPin, LOW);
}

// update() - sends a logic pulse to IO UPDATE pin on DDS; updates frequency output to 
//      newly set frequency (FTW0)
void AD9954::update(){
    digitalWrite(_updatePin, HIGH);
    digitalWrite(_updatePin, LOW);
}


// setFreq(freq) -- writes freq to DDS board, in FTW0
void AD9954::setFreq(unsigned long freq){
    // set _freq and _ftw variables
    _freq = freq;
    _ftw = freq * RESOLUTION / _refClk ;

    // divide up ftw into four bytes
    byte ftw[] = { lowByte(_ftw >> 24), lowByte(_ftw >> 16), lowByte(_ftw >> 8), lowByte(_ftw)};
    // register info -- writing four bytes to register 0x04, 
    byte registerInfo[] = {0x04, 4};
    
	byte CFR1[] = { 0x00, 0x00, 0x00, 0x00 };
    byte CFR1Info[] = {0x00, 4};
    
	// actually writes to register
    AD9954::writeRegister(CFR1Info, CFR1);
	AD9954::writeRegister(registerInfo, ftw);
    
	// issues update command
    AD9954::update();
}


// getFreq() - returns current frequency
unsigned long AD9954::getFreq(){
    return _freq;
}

// getFTW() -- returns current FTW
unsigned long AD9954::getFTW(){
    return _ftw;
}

// Function setFTW -- accepts 32-bit frequency tuning word ftw;
//      updates instance variables for FTW and Frequency, and writes ftw to DDS.
void AD9954::setFTW(unsigned long ftw){
    // set freqency and ftw variables
    _ftw = ftw;
    _freq = ftw * _refClk / RESOLUTION;

    // divide up ftw into four bytes
    byte data[] = { lowByte(_ftw >> 24), lowByte(_ftw >> 16), lowByte(_ftw >> 8), lowByte(_ftw)};
    // register info -- writing four bytes to register 0x04, 
    byte registerInfo[] = {0x04, 4};

	
    byte CFR1[] = { 0x00, 0x00, 0x00, 0x00 };
    byte CFR1Info[] = {0x00, 4};
	
    AD9954::writeRegister(CFR1Info, CFR1);
    AD9954::writeRegister(registerInfo, data);
    AD9954::update();

}

// Function linearSweep -- places DDS in linear sweep mode.
//      Behavior is determined by two frequency tuning words, freq0 and freq1 (freq0 < freq1).
//      The PS0 pin HIGH will ramp towards freq1, PS0 LOW will ramp towards freq0.
//      The rate of the ramp is dictated by pos/negDF (positive/negative DeltaFreq), and pos/negRR (positive/negative RampRate).
//      
//      freq0: lower frequency bound (Hz)
//      freq1: upper frequency bound (Hz)
//      posDF: delta frequency for positive ramp (Hz)
//      negDF: delta frequency for negative ramp (Hz)
//      posRR: number between 0 and 255, indicating number of SYNC_CLK cycles spent at each
//              frequency value in the ramp. SYNC_CLK operates at 1/4 of the SYSCLK clock value. Typically SYNC_CLK = 100MHz.
//              Thus, the true "ramp rate" is, eg, posDF/(posRR*10 ns)
//      negRR: same as above, but for negative ramp.
//      
//      As a general rule, round up (not down) in calculating the delta frequency steps.
void AD9954::linearSweep(unsigned long freq0, unsigned long freq1, unsigned long posDF, byte posRR, unsigned long negDF, byte negRR){

    // calculate
    unsigned long ftw0 = freq0*RESOLUTION / _refClk;
    unsigned long ftw1 = freq1*RESOLUTION / _refClk;
    unsigned long posDFW = posDF*RESOLUTION / _refClk;
    unsigned long negDFW = negDF*RESOLUTION / _refClk;


    // construct register values
    //unsigned char CFR1[] = { 0x00, 0x20, 0x00, 0x00 };
	//unsigned char CFR1Info[] = {0x00, 4};
    unsigned char CFR1[] = { 0x00, 0x00, 0x20, 0x00, 0x00 };
	

	
    //unsigned char FTW0[] = {lowByte(ftw0 >> 24), lowByte(ftw0 >> 16), lowByte(ftw0 >> 8), lowByte(ftw0) };
    //unsigned char FTW0Info[] = {0x04, 4};

    unsigned char FTW1[] = {lowByte(ftw1 >> 24), lowByte(ftw1 >> 16), lowByte(ftw1 >> 8), lowByte(ftw1) };
    unsigned char FTW1Info[] = {0x06, 4};

    unsigned char NLSCW[] = { negRR, lowByte(negDFW >> 24), lowByte(negDFW >> 16), lowByte(negDFW >> 8), lowByte(negDFW) };
    unsigned char NLSCWInfo[] = {0x07, 5};

    unsigned char PLSCW[] = { posRR, lowByte(posDFW >> 24), lowByte(posDFW >> 16), lowByte(posDFW >> 8), lowByte(posDFW) };
    unsigned char PLSCWInfo[] = {0x08, 5};
    
    AD9954::writeRegister(CFR1Info, CFR1);
    AD9954::writeRegister(FTW0Info, FTW0);
    AD9954::writeRegister(FTW1Info, FTW1);
    AD9954::writeRegister(NLSCWInfo, NLSCW);
    AD9954::writeRegister(PLSCWInfo, PLSCW);

    AD9954::update();


}





/* PRIVATE CLASS FUNCTIONS */


// Writes SPI to particular register.
//      registerInfo is a 2-element array which contains [register, number of bytes]
void AD9954::writeRegister(unsigned char registerInfo[], unsigned char data[]){

    digitalWrite(_ssPin, LOW);

    // Writes the register value
    SPI.transfer(registerInfo[0]);

    // Writes the data
    for(int i = 0; i < registerInfo[1]; i++){
        SPI.transfer(data[i]);
    }

    digitalWrite(_ssPin, HIGH);

}


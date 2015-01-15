/* 
   AD9954.h - AD9954 DDS communication library
   Created by Neal Pisenti, 2013.
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
   aunsigned long with this program.  If not, see <http://www.gnu.org/licenses/>.


 */

#ifndef AD9954_h
#define AD9954_h

#include "Arduino.h"

// Control Function Register CFR1
#define AD9954_RAM_ENABLE 1 << 31
#define AD9954_RAM_DEST_POW 1 << 30
#define AD9954_INTERNAL_TWOBURST 1 << 27
#define AD9954_INTERNAL_THREEBURST 2 << 27
#define AD9954_INTERNAL_FOURBURST 3 << 27
#define AD9954_INTERNAL_TWOLOOP 4 << 27
#define AD9954_INTERNAL_THREELOOP 5 << 27
#define AD9954_INTERNAL_FOURLOOP 6 << 27
#define AD9954_LOAD_AMPRR 1 << 26
#define AD9954_OSK_ENABLE 1 << 25
#define AD9954_AUTO_OSK_ENABLE 1 << 24

#define AD9954_AUTO_SYNC 1 << 23
#define AD9954_MANUAL_SYNC 1 << 22
#define AD9954_LS_ENABLE 1 << 21

#define AD9954_SRR_LOAD_ENABLE 1 << 15
#define AD9954_AUTOCLR_FREQ_ACCUM 1 << 14
#define AD9954_AUTOCLR_PHASE_ACCUM 1 << 13
#define AD9954_SINE_SELECT 1 << 12
#define AD9954_CLEAR_FREQ_ACCUM 1 << 11
#define AD9954_CLEAR_PHASE_ACCUM 1 << 10
#define AD9954_SDIO_INPUT 1 << 9
#define AD9954_LSB_FIRST 1 << 8

#define AD9954_DIGITAL_POWERDOWN 1 << 7
#define AD9954_COMP_POWERDOWN 1 << 6
#define AD9954_DAC_POWERDOWN 1 << 5
#define AD9954_CLOCK_POWERDOWN 1 << 4
#define AD9954_EXTERNAL_POWERDOWN 1 << 3
#define AD9954_LS_NODWELL 1 << 2
#define AD9954_SYNC_CLK_DISABLE 1 << 1

// CFR2
#define AD9954_HIGHSPEED_SYNC_ENABLE 1 << 11
#define AD9954_HARDWARE_MANUAL_SYNC 1 << 10
#define AD9954_CRYSTAL_OUT_ENABLE 1 << 9
#define AD9954_REFCLK_MULT(M) ((M & 0x1F) << 3)
#define AD9954_VCO_HIGH_RANGE 1 << 2
#define AD9954_SET_CP(M) (M & 0x03)



class AD9954
{
    public: 
        // Constructor function. 
        AD9954( byte, byte, byte, byte, byte, byte);

        // Initialize with refClk frequency
        void initialize(unsigned long);

        // Initialize with refIn frequency, and clock multiplier value
        void initialize(unsigned long, unsigned char);

        // Reset the DDS
        void reset();

        // Update the new frequency tuning word
        void update();
        

        // Gets current frequency
        unsigned long getFreq();
        // Sets frequency
        void setFreq(unsigned long);

        // Gets current frequency tuning word
        unsigned long getFTW();
        // Sets frequency tuning word
        void setFTW(unsigned long);

        // places DDS in linear sweep mode
        void linearSweep(unsigned long, unsigned long, unsigned long, byte, unsigned long, byte);





    private:
        // Instance variables that hold pinout mapping
        // from arduino to DDS pins.
        byte _ssPin, _resetPin, _updatePin, _ps0, _ps1, _osk;

        // Instance variables for frequency _freq, frequency tuning word _ftw,
        // reference clock frequency _refClk, etc.
        unsigned long _ftw, _refClk, _refIn;

		double _freq;
		
		// For easier writing to registers. 
		// Accommodates 4 bytes, which can be accessed either as an unsigned long or
		// as for individual chars.
		union RegVal {
			unsigned long val;
			unsigned char[4] c;
		}

		struct RegData {
			
        // function to write data to register. 
        void writeRegister(byte[2], byte[1024]);


        // DDS frequency resolution
         double RESOLUTION;// = 4294967296; // sets resolution to 2^32 = 32 bits. Using type double to avoid confusion with integer division...



};
 

#endif

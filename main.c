#define BATT_CORRECTION -6
#define digitpause 23 //number of times the ADC is read while displaying a digit, so we're doing something useful
#define MAJOR_VERSION 0b01001000//2 //version number of code, hold start while powering up to view
#define MINOR_VERSION 0b01100001//3
//2.0 first badge flashing
//2.1 fix for external relay in morse code mode
//2.2 more accurate timer, threshold setting bugfix, adjusting thresh/tilt dig[1] display bugfix in timer mode, 
//      soundmode external relay tilt option, loudness threshold roll over fuckup, clap mode flipping, 
//      clap mode config roll over, startup DP bugfix
//2.3   timer display bug when quitting options

/*
 * File:   main.c
 * Author: twitter@see_ess
 *
 * Warning: Dont press the buttons while flashing the pic, probably will short the programmer
 * Battery life estimate with 4 AA's is ~12 hr at good brightness + 5.5 hr before chip reset 
 *
 * Basic code layout:
 * one giant for(;;) loop where digits are lit individually (we can't light them all at once 
 * unless they are all lit with the same value). To keep max display brightness all calculations
 * are done when a digit is lit. During the leftmost digit 0, the display segments are updated.
 * During the rightmost digit 5, most logic is computed (button press/mic input/tilt/etc).
 * Each digit is lit for roughtly 1ms each, during which the microphone ADC is read.
 * 
 * Option settings
 * <not in options mode> option=0
 * Print "oPtion" briefly, option=1
 * count-> switchmode(), option=2, pressing start in that mode switched to count down
 * timer -> switchmode(), option=3
 * shot -> switchmode(), option=4, <default mode>
 * rand -> switchmode(), option=5
 * hype -> switchmode(), option = 6
 * audio -> switchmode(), option = 7
 * setdig ->, option=8
 *      decimal point blink current digit, option = 20-25 for each dig
 *      start increments dig, select moves to next dig. 1st digit exits, uses current mode, write flash if mode=0
 * tilt -> set tiltmode = !tiltmode;, option=9
 * thresh -> mic threshold setting, option=10
 *      DP on current setting, select increments by 20 (mute-1000), start -> sets value and exits option=170-221
 * speed -> refresh speed for countup/down/rand, option=11
 *      decimal point on current setting, select cycles to next value 0-8, start -> sets the speed and exits option=60-68
 * bright -> set brightness backoff, option = 12
 *      select cycles to next value dark/dim/max, option 160-164
 * play -> games submenu, option=13
 *      react, dodge, temp, batt, sound, clap, morse, quit, option=70-78
 *          morse submenu: 80-104, morse custom menu 120-157
 * quit, option=14
 *
 * Flash layout (each address is 14 bits, but only the 8 low bits are high endurance)
 * 0x1FFF   mode
 * 0x1FFE   shot counter least significat digit (right)
 * 0x1FFD   second digit
 * 0x1FFC   third digit
 * 0x1FFB   fourth digit
 * 0x1FFA   fifth digit
 * 0x1FF9   sixth digit
 * 0x1FF8   mic threshold LSB
 * 0x1FF7   mic threshold MSB
 * 0x1FF6   speed
 * 0x1FF5   temp offset
 * 0x1FF4   batt offset
 * 0x1FF3   tiltoption
 * 0x1FF2   loudness level (for sound level mode)
 * 0x1FF1   brightness level (brightness pause)
 * 0x1FF0   Claps required in clap mode
 *
 * to reset all stored configuration in flash, hold both buttons while powering the device on
 * to see the current code version, hold start before powering on the device
 *
 * Header Pinout
 * 1  RA3/MCLR - Button 2:start (Vihh during programming) DONT PRESS DURING PROGRAMMING = short
 * 2  RC5 - segment E (bottom left)
 * 3  RC4 - segment D (bottom)
 * 4  RC3 - segment C (bottom right)
 * 5  RC6 - segment F (top left)
 * 6  RC7 - segment G (middle)
 * 7  RB7 - Digit 4
 * 8  RB6 - Digit 3
 * 9  VDD positive
 * 10 VSS negative
 * 11 RB5 - Digit 2
 * 12 RB4 - Digit 1 (left most)
 * 13 RC2 - segment B (top right)
 * 14 RC1 - segment A (top)
 * 15 RC0 - Decimal point
 * 16 RA2 - tilt sensor (external interrupt)
 * 17 RA1 - Mic input ICSPCLK
 * 18 RA0 - Button 1:select ICSPDAT
 * 19 RA5 - Digit 6 (right most)
 * 20 RA4 - Digit 5
 * 
 * QFN Chip pinout (NOT THE HEADER PINOUT)
 * 1  RA3/MCLR - Button 2:start (Vihh during programming) DONT PRESS DURING PROGRAMMING = short
 * 2  RC5 - segment E (bottom left)
 * 3  RC4 - segment D (bottom)
 * 4  RC3 - segment C (bottom right)
 * 5  RC6 - segment F (top left)
 * 6  RC7 - segment G (middle)
 * 7  RB7 - Digit 4
 * 8  RB6 - Digit 3
 * 9  RB5 - Digit 2
 * 10 RB4 - Digit 1 (left most)
 * 11 RC2 - segment B (top right)
 * 12 RC1 - segment A (top)
 * 13 RC0 - Decimal point
 * 14 RA2 - tilt sensor (external interrupt)
 * 15 RA1 - Mic input ICSPCLK
 * 16 RA0 - Button 1:select ICSPDAT
 * 17 VSS negative
 * 18 VDD positive
 * 19 RA5 - Digit 6 (right most)
 * 20 RA4 - Digit 5
 * 
 * Pickit3 pinout - board pinout
 * 1 VPP MCLR - 1
 * 2 VDD+     - 9
 * 3 VSS-     - 10
 * 4 ICSPDAT  - 18
 * 5 ICSPCLK  - 17
 *
 * after importing to mplab, project properties, xc8 linker, fill flash mem, seq 0x0, range 0x1fe0:0x1fff
 * it stores configuration values in that range, 32 rows is the smallest range to erase/write
 * also exclude that range from program mem, proj properties, xc8 linker, mem model, rom ranges default,-1fe0-1fff
 */
//Pragma config statements should precede project file includes. Use project enums instead of #define for ON and OFF. //CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (Internal Oscillator, I/O function on OSC1)
#pragma config WDTE = ON      // Watchdog Timer Enable (WDT enabled always)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (disabled. I/O or oscillator function on the CLKOUT pin)
// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection ((Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)
#include <xc.h>
#include <stdlib.h>
#include <string.h>
//#define _XTAL_FREQ 31000 // oscillator should be 31khz after OSCCONbits.IRCF = 0b0000;

const char bitcoin_mini_key[36] = {//case sensitive, public key <censored>
    0x0
};
const char digitlookup[32] = { //LED segment selection per digit (index), MSB-LSB: ABCDEFG(DP)
    0b10000001, // 0
    0b11110011, // 1
    0b01001001, // 2
    0b01100001, // 3
    0b00110011, // 4
    0b00100101, // 5
    0b00000101, // 6
    0b11110001, // 7
    0b00000001, // 8
    0b00100001, // 9
    0b00010001, // a
    0b00000111, // b
    0b01001111, // c
    0b01000011, // d
    0b00001101, // e
    0b00011101, // f
    0b10000001, // 0 flipped (+16 from rightside up version)
    0b10011111, // 1 flipped
    0b01001001, // 2 flipped
    0b00001101, // 3 flipped
    0b00010111, // 4 flipped
    0b00100101, // 5 flipped
    0b00100001, // 6 flipped
    0b10001111, // 7 flipped
    0b00000001, // 8 flipped
    0b00000101, // 9 flipped
    0b00000011, // a flipped
    0b00110001, // b flipped
    0b01111001, // c flipped
    0b00011001, // d flipped
    0b01100001, // e flipped
    0b01100011  // f flipped
};
const unsigned char dodgeblocks[6] = { //dodge game blocks that you must avoid
    0b01111111, // middle block
    0b01000111, // middle and bottom block
    0b11101111, // bottom block
    0b11101101, // top and bottom block
    0b11111101, // top block
    0b00111001  // top and middle block
};
const char soundblocks[16] = {
    0b11111110,//[0] dp
    0b11101110,//[1] dp+_
    0b01000110,//[2] dp+_+bottom vertical lines+-
    0b00000000,//[3] all segments lit
    0b01000010,//[4] left [4]
    0b00000110,//[5] right [4]
    0b11100110,//[6] left [2]
    0b11001110,//[7] right [2]
    0b00000000,//[8] inverted [0] for external relay, dp stays on
    0b00010000,//[9] inverted [1] for external relay, dp stays on
    0b10111000,//[10] inverted [2] for external relay, dp stays on
    0b11111110,//[11] inverted [3] for external relay, dp stays on
    0b10111100,//[12] inverted [4] for external relay, dp stays on
    0b11111000,//[13] inverted [5] for external relay, dp stays on
    0b00011000,//[14] inverted [6] for external relay, dp stays on
    0b00110000,//[15] inverted [7] for external relay, dp stays on
};
const char morselookup[36] = {//character / digit to morse code lookup
    //first 3 bits are the number of the elements in morse code dots/dashes (1-5)
    //last 5 bits are the morse code blips 1=dot 0=dash, right aligned. end of string = index 100
    0b01000010,//a 0  .-
    0b10000111,//b 1  -...
    0b10000101,//c 2  -.-.
    0b01100011,//d 3  -..
    0b00100001,//e 4  .
    0b10001101,//f 5  ..-.
    0b01100001,//g 6  --.
    0b10001111,//h 7  ....
    0b01000011,//i 8  ..
    0b10001000,//j 9  .---
    0b01100010,//k 10 -.-
    0b10001011,//l 11 .-..
    0b01000000,//m 12 --
    0b01000001,//n 13 -.
    0b01100000,//o 14 ---
    0b10001001,//p 15 .--.
    0b10000010,//q 16 --.-
    0b01100101,//r 17 .-.
    0b01100111,//s 18 ...
    0b00100000,//t 19 -
    0b01100110,//u 20 ..-
    0b10001110,//v 21 ...-
    0b01100100,//w 22 .--
    0b10000110,//x 23 -..-
    0b10000100,//y 24 -.--
    0b10000011,//z 25 --..
    0b10100000,//0 26 -----
    0b10110000,//1 27 .----
    0b10111000,//2 28 ..---
    0b10111100,//3 29 ...--
    0b10111110,//4 30 ....-
    0b10111111,//5 31 .....
    0b10101111,//6 32 -....
    0b10100111,//7 33 --...
    0b10100011,//8 34 ---..
    0b10100001 //9 35 ----.
};
const char morsestr[555] = { //string to use during morse code output
    //built in morse code strings, char 100 = end of sub-string
    //0:  defcon23
    //1:  SOS
    //2:  hack everything
    //3:  break shit
    //4:  nothing is impossible
    //5:  fuck the NSA
    //6:  i dont play well with others
    //7:  what are you doing dave
    //8:  youre either a one or a zero Alive or dead
    //9:  danger zone
    //10: bros before apparent threats to national security
    //11: im spooning a Barrett 50 cal i could kill a building
    //12: there is no spoon
    //13: never send a human to do a machines job
    //14: guns lots of guns
    //15: its not that im lazy its just that i dont care
    //16: PC load letter
    //17: shall we play a game
    //18: im getting too old for this
    //19: censorship reveals fear
    //20: the right of the people to keep and bear Arms shall not be infringed
    //21: all men having power ought to be mistrusted
    //22: when governments fear the people there is liberty
    //23: custom
    3,4,5,2,14,13,28,29,100,//[0]
    18,14,18,100,//[1]
    7,0,2,10,4,21,4,17,24,19,7,8,13,6,100,//[2]
    1,17,4,0,10,18,7,8,19,100,//[3]
    13,14,19,7,8,13,6,8,18,8,12,15,14,18,18,8,1,11,4,100,//[4]
    5,20,2,10,19,7,4,13,18,0,100,//[5]
    8,3,14,13,19,15,11,0,24,22,4,11,11,22,8,19,7,14,19,7,4,17,18,100,//[6]
    22,7,0,19,0,17,4,24,14,20,3,14,8,13,6,3,0,21,4,100,//[7]
    24,14,20,17,4,4,8,19,7,4,17,0,14,13,4,14,17,0,25,4,17,14,0,11,8,21,4,14,17,3,4,0,3,100,//[8]
    3,0,13,6,4,17,25,14,13,4,100,//[9]
    1,17,14,18,1,4,5,14,17,4,0,15,15,0,17,4,13,19,19,7,17,4,0,19,18,19,14,13,0,19,8,14,13,0,11,18,4,2,20,17,8,19,24,100,//[10]
    8,12,18,15,14,14,13,8,13,6,0,1,0,17,17,4,19,19,31,26,2,0,11,8,2,14,20,11,3,10,8,11,11,0,1,20,8,11,3,8,13,6,100,//[11]
    19,7,4,17,4,8,18,13,14,18,15,14,14,13,100,//[12]
    13,4,21,4,17,18,4,13,3,0,7,20,12,0,13,19,14,3,14,0,12,0,2,7,8,13,4,18,9,14,1,100,//[13]
    6,20,13,18,11,14,19,18,14,5,6,20,13,18,100,//[14]
    8,19,18,13,14,19,19,7,0,19,8,12,11,0,25,24,8,19,18,9,20,18,19,19,7,0,19,8,3,14,13,19,2,0,17,4,100,//[15]
    15,2,11,14,0,3,11,4,19,19,4,17,100,//[16]
    18,7,0,11,11,22,4,15,11,0,24,0,6,0,12,4,100,//[17]
    8,12,6,4,19,19,8,13,6,19,14,14,14,11,3,5,14,17,19,7,8,18,100,//[18]
    2,4,13,18,14,17,18,7,8,15,17,4,21,4,0,11,18,5,4,0,17,100,//[19]
    19,7,4,17,8,6,7,19,14,5,19,7,4,15,4,14,15,11,4,19,14,10,4,4,15,0,13,3,1,4,0,17,0,17,12,18,18,7,0,11,11,13,14,19,1,4,8,13,5,17,8,13,6,4,3,100,//[20]
    0,11,11,12,4,13,7,0,21,8,13,6,15,14,22,4,17,14,20,6,7,19,19,14,1,4,12,8,18,19,17,20,18,19,4,3,100,//[21]
    22,7,4,13,6,14,21,4,17,13,12,4,13,19,18,5,4,0,17,19,7,4,15,4,14,15,11,4,19,7,4,17,4,8,18,11,8,1,4,17,19,24,100//[22]
};//morsestr[]]
const char secretstring[6] = {//probably needs to be at least 6 bytes long, fill with 0xff's
    0b00011101,//fedcon
    0b00001101,
    0b01000011,
    0b01001111,
    0b01000111,
    0b01010111,
    
    //0b01001111,0b00100101,0b00001111,0b01000111,0b01010111,0b00001101,//cstone
    //0xff,0b01000011,0b00001101,0b11000111,0b11110111,0b00010001,0b01010111,0b00001111,//deviant
    //0xff,0b01000111,0b10001111,0b10001111,0b00010001,0b10011001,0b10110001,0xff//ollam
    //0xff,0b01000011,0b00001101,0b00011101,0b01001111,0b01000111,0b01010111,0xff,//gigs
    //0b00100101,0b00010011,0b01000111,0b01000111,0b00001111,0xff,
    //0b00100001,0b11110011,0b00100001,0b00100101,0xff,0xff
};//secretstring[]]
//mode 0=normal, 1=count up , 2=initial count up, -1=count down, 3=rand, 4=defcon/shoot, -2=audio, 
//5=react game, 6=dodge game, 7=temp, 8=batt, 9=sound, 10=clap, 11=timer, 12=btc, 13=morse
signed char mode = 4; //operating mode
short micthresh = 20; //threshold to break the 10-bit ADC, steps of 20
char segmentbuffer[6]; //exactly what segments to light, set to LATC
signed char countbuffer[6] = {0,0,0,0,0,0}; //index for each decimal digit of the counter (avoids math for individual digits)
signed char displaybuffer[6] = {0,0,0,0,0,0}; //digits to display, used because some modes don't write flash
char loop = 0; //used for various counters, usually incremented after lighting each digit
char countshot = 0; //used with lastpeaktopeak to determine when a new shot can be detected
char shotlockout = 0; //don't count another shot in a time window after the last shot
short maxenvelope = 512; //the max adc reading over a group of readings
short minenvelope = 511; //the min adc reading over a group of readings
short maxmindiff = 0; //temp value to save code space
signed short weightedavgpeaktopeak = 0; //used to light the decimal points based on mic value, weight the peak to peak value
char speed = 5; //config setting to slow down countup/down/random modes, range 0-8, stored 1-9, if 0 set to default 5
char tilt = 0; //debounce for tilt, tilt < 75 rightside up, tilt >= 75 (to 150) upside down
char tiltoption = 0; //invert the display from the normal direction
char startup = 10; //>0 if in startup, needed for two modes during startup, 10 because we use this for digit setting 0-9
char optiontimeout = 5; //used to bail out of options menu if no activity is detected, and startup (why it is set to 5)
char gamechar = 0; //used in different game modes
char gamechar2 = 0; //used in different game modes
signed char gamechar3 = 0; //used in different game modes
unsigned short gameshort = 0; //used in different game modes
unsigned short gameshort2 = 0; //used for morse code
char morsechar = 0; //used to set custom morse code strings
char morsecuststr[51]; //custom array for morse code mode
signed short vdd = 0; //battery reading
signed short temp = 0; //temperature reading
signed char tempoffset = 0; //temp offset (0 = +190), use range -99 to 99
signed char battoffset = 0; //0.02 of a voltage off (.02*4.08v), limited range of -20 to +20 (+0.4v to -0.4v)
char option = 0; //if non zero, it means we are in an option menu, different values used for each option (described above)
char debounce1 = 0; //button 1 debounce (so if it is held it doesn't keep thinking it is a new press)
char debounce2 = 0; //button 2 debounce (so if it is held it doesn't keep thinking it is a new press)
char *arraypointer; //pointer to which morse code array we're using
unsigned char timerbuf[64] = {0}; //used to store timer values
char loudnesslevel = 0; //used as a loudness threshold level in the sound level mode
char brightnesspause = 0; // a delay when digits aren't lit to dim display based on the setting
signed char clapnum = 0; //number of claps to trigger state change in clap mode 0 = 2 by default.
char numadcreads = digitpause; //used to save code space, number of adc reads including brightness backoff
char displaytilted = 0; //0 if the display doesn't need to be tilted (no tilt option and normal orientation || tilt option and flipped)
float vddtemp = 0;
void unlock_write(){ //we call this a lot in write_flashmem(), drop it in a function to save space
    PMCON2 = 0x55;  //unlock program memory
    PMCON2 = 0xAA;  //unlock program memory
    WR = 1;         //begin write
    asm ("NOP");    //to give time to write
    asm ("NOP");    //to give time to write
}
void write_flashmem(){
    //high endurance area is last 128 addresses, low byte http://www.microchip.com/forums/m646458.aspx
    GIE = 0;        //disable interupts incase they interfere
    //ERASE SECTION
    PMCON1 = 0;     //not configuration space
    PMADRL = 0xFF;  //least significant bits of address
    PMADRH = 0x1F;  //most significant bits of address (full address is 0x1FFF) this should erase 0x1FE0-0x1FFF
    FREE = 1;       //specify erase operation
    WREN = 1;       //allow write
    unlock_write();
    //END OF ERASE SECTION
    
    //WRITE SECTION
    FREE = 0;       //selecting write operation
    LWLO = 1;       //load write latches only
    PMDATL = mode;  //what to write
    PMDATH = 0;     //each flash addr has 14 bits, but high endurance is low byte only, dont use these bits
    unlock_write();
    
    for(char i=0;i<6;i++){//write digits to flash
        PMADRL=249+i;  //least significant bits of address 0xF9+i
        PMDATL = (char)countbuffer[i];  //what to write
        unlock_write();
    }

    PMADRL = 0xf8;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff8
    PMDATL = ((char)micthresh);  //what to write
    unlock_write();
    
    PMADRL = 0xf7;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff7
    PMDATL = (micthresh>>8);//what to write, micthresh high byte
    unlock_write();
    
    PMADRL = 0xf6;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff6
    PMDATL = speed;  //what to write
    unlock_write();
    
    PMADRL = 0xf5;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff5
    PMDATL = tempoffset;  //what to write
    unlock_write();
    
    PMADRL = 0xf4;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff4
    PMDATL = battoffset;  //what to write
    unlock_write();
    
    PMADRL = 0xf3;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff3
    PMDATL = tiltoption;  //what to write
    unlock_write();
    
    PMADRL = 0xf2;
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff3
    PMDATL = loudnesslevel; //what to write
    unlock_write();
    
    PMADRL = 0xf1;
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff3
    PMDATL = brightnesspause; //what to write
    unlock_write();

    LWLO = 0;       //write latches to flash (done at last word to write)
    PMADRL = 0xf0;  //least significant bits of address
    //PMADRH=0x1f;  //most significant bits of address (full address is 0x1ff3
    PMDATL = clapnum;  //what to write
    unlock_write();
    WREN = 0;        //disallow write
    //END OF WRITE SECTION
}
char generic_read(){ //we call this a lot for reading various things, put in a function to save space
    PMCON1 = 0;//not configuration space
    PMADRH=0x1F; //most significant bits of address (full address is 0x1ff9-0x1ffe)
    RD = 1;      //initiate read operation
    asm ("NOP");//needed otherwise instructions are ignored
    asm ("NOP");
    return PMDATL;
}
void read_count(){
    for(char i=0;i<6;i++){//loop over the 6 addresses and pull each decimal digit (stored in one array position each)
        PMADRL=249+i;//0xf9+i least significant bits of address
        //PMADRH=0x1F; //most significant bits of address (full address is 0x1ff9-0x1ffe)
        countbuffer[i] = (signed char)(generic_read()); //read from flash
        if(countbuffer[i] < 0 || countbuffer[i] > 9){ //bounds check
            countbuffer[0] = 0;countbuffer[1] = 0; //set each digit to 0, something is wrong
            countbuffer[2] = 0;countbuffer[3] = 0;
            countbuffer[4] = 0;countbuffer[5] = 0;
            break;//no need to continue, time to reset the counter
        }
    }
}//read_count()
void read_mode(){ //read the operating mode from flash
    PMADRL=0xFF;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FFF)
    mode = generic_read(); //read from flash
    if(mode < -2 || mode > 14 || mode == 2 || mode == 12){ //bounds check, and startup mode/btc modes shouldn't be written to flash
        mode = 0; //reset mode
    }//end bounds check
}// read_mode()
void read_micthresh(){ //read the microphone threshold from flash
    PMADRL=0xF8;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF8)
    micthresh = generic_read() & 0xff; //read from flash (LSB of threshold)
    PMADRL=0xF7;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF7)
    micthresh = micthresh + ((generic_read() & 0x3f)<<8);
    if(micthresh % 20 != 0 || micthresh < 20 || (micthresh > 1020 && micthresh < 10020) || 
            micthresh > 11020){ //bounds check, 1020 = off, +10k for quickmute
        micthresh = 20; //reset to default mic thresh
    } //end bounds check
}//read_micthresh()
void read_speed(){
    //will fail bounds check on first boot due to == 0, gets reset to 5 (speed 4)
    PMADRL=0xF6;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF6)
    speed = generic_read(); //read from flash
    if(speed == 0 || speed > 9){ //bounds check
        speed = 5; //reset to default
    } //end bounds check
}//readspeed()
void read_temp_offset(){
    PMADRL=0xF5;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF5)
    tempoffset = ((signed char)generic_read()); //read from flash
    if(tempoffset < ((signed char)-99) || tempoffset > ((signed char)99)){ //bounds check
        tempoffset = 0; //reset to default
    } //end bounds check
}//read_temp_offset()
void read_batt_offset(){
    PMADRL=0xF4;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF4)
    battoffset = ((signed char)generic_read()); //read from flash
    if(battoffset < ((signed char)-20) || battoffset > ((signed char)20)){ //bounds check
        battoffset = 0; //reset to default
    } //end bounds check
}//read_batt_offset()
void read_tiltoption(){
    PMADRL=0xF3;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF3)
    tiltoption = generic_read(); //read from flash
    if(tiltoption != 1){//bound check
        tiltoption = 0;
    }//end bounds check
}//read_tiltoption()
void read_loudnesslevel(){
    PMADRL=0xF2;  //least significant bits of address
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF2)
    loudnesslevel = generic_read(); //read from flash
    if(loudnesslevel > 7){//bound check
        loudnesslevel = 0;
    }//end bounds check    
}
void read_brightness(){
    PMADRL=0xF1;
    //PMADRH=0x1F;  //most significant bits of address (full address is 0x1FF1)
    brightnesspause = generic_read(); //read from flash
    if(brightnesspause != 13 && brightnesspause != 20){
        brightnesspause = 0;
    }
    numadcreads = digitpause - brightnesspause; //read the ADC this many times when displaying each digit
}
void readClaps(){
    PMADRL = 0xF0;
    //PMADRH=0x1F; //most significant bits of address (full address is 0x1FF0)
    clapnum = generic_read(); //read from flash
    if(clapnum < -1 || clapnum > 13){ //bounds check, -1 = 1 clap, 0 = 2 claps (default), 13 = 15 claps (max)
        clapnum = 0; //reset to default
    }//end bounds check
}//readClaps())
void reset_clear_flash(){ //reset all settings and clear flash
    mode = 0; //don't write flash for startup mode
    micthresh = 20;
    speed = 0;//reset speed setting
    tiltoption = 0;
    battoffset = 0;
    tempoffset = 0;
    brightnesspause = 0;//max brightness
    option = 0;
    startup = 10;
    optiontimeout = 5;//needed for startup
    loop = 255;
    for(char i=0;i<6;i++){ //reset the counters
        countbuffer[i] = 0; //set each digit to 0
        displaybuffer[i] = 0; //reset each display digit
    }
    write_flashmem();
    mode = 4; //reboot in startup mode
    LATC = 0b01101100; //light horizontal segments and DP to show reset sequence worked
    LATB = 0b00001111; //light up digit 1-4
    LATA = 0b11001111; //light up digit 5-6
    _delay(50000000); //so the user can read the display and know it worked
}
void blackout(){ //clear all digits, used to save space
    segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b11111111;//clear all digits, don't light any segments
    segmentbuffer[2] = 0b11111111;segmentbuffer[3] = 0b11111111;
    segmentbuffer[4] = 0b11111111;segmentbuffer[5] = 0b11111111;
}
void switchmode(signed char modearg){ //change to new operating mode
    //in case we're switching from batt mode
    ADCON0bits.CHS = 0b00001; //select AN1 ADC channel
    FVRCONbits.FVREN = 0; //FVR disabled
    mode = modearg; //switch to new mode
    gamechar2 = 0;//used differently in each mode
    gamechar3 = 0;//used differently in each mode
    gameshort = 0;//used differently in each mode
    gameshort2 = 0;//used differently in each mode
    for(char i=0;i<6;i++){//set display buffer to 000000, segment buffer to 0xff
        displaybuffer[i] = 0;//set to all 0's
    }
    blackout();
    if(mode == 0){ //normal ops mode (shot counter)
        memcpy(displaybuffer, countbuffer, 6);//copy the countbuffer to the display buffer, 6 indexes/chars
    }else if(mode == 13){//morse code mode
        //gamechar2  = bit position in char (+1)
        //gamechar   = string number to use, don't modify here
        //gameshort = 0;  //current position in large array
        //gameshort2 = 0; //string starting point in large array
        char i = 0; //number of end-of-substrings found
        if(gamechar != 23){//using a built in string, not morse code custom string
            while(i < gamechar){//while we haven't found the number of end of strings
                if(morsestr[gameshort] == 100){//check current array position
                    i++;//we found another end of substring
                }
                gameshort++;//move to next position in array
            }//at this point gameshort = array position of substring starting point
            gameshort2 = gameshort;//store first char position of substring
            arraypointer = (char*)(&morsestr);//pointer to built in array
        }else{//custom string mode
            arraypointer = &morsecuststr; //pointer to index 0 in the string
        }
        //pull out num of elements in this char (first 3 bits), shift down to 0-5 (num of elements per char)
        gamechar2 = (((morselookup[arraypointer[gameshort]]) & 0b11100000)>>5);
        loop = 208+((speed-1)*4); //set loop value so first element is right //at end of char, pause before next char (until 255)
    }else if(mode >= 3){ //5=react game mode, 6=dodge, 7=temp, 8=batt, 9=sound, 10=clap, 11=timer, 12=btc, 13=morse
        gamechar = 0;//used differently in each mode
        debounce2 = 100; //so when we enter temp/batt mode we're over the repeat button threshold, and wont enter offset
        if(mode == 6){//dodge game
            //srand( countbuffer[5] ); //seeding the random alg should probably test this but fuck-it
            gamechar = 2; //set char to middle position
        }else if(mode == 7 || mode == 8){//temp mode / batt mode, switch adc junk to read voltage
            FVRCON = 0b10010001; //fvren=1, fvrrdy=0, tsen=0, tsrng=1, cdavfr=00, adfvr=00
            //FVRCONbits.FVREN = 1; //FVR enabled
            //FVRCONbits.TSEN = 0; //disable temp sense
            //FVRCONbits.ADFVR = 0b01; //1x FVR, 1.024v
            ADCON0bits.CHS = 0b11111; //select ADC channel
        }else if(mode == 9){
            gamechar2 = loudnesslevel; //set the temp/config loudness level to the current loudnesslevel
        }else if(mode == 11){//timer mode
            for(char i=0;i<64;i++){//set display buffer to 000000
                timerbuf[i] = 0;//reset timer for all 10 shots
            }
            gameshort = 0x8000 + (rand() & 0x7FFF); //calculate the initial timeout, before starting the counter
        }//end mode 11
    }//end mode conditionals
    loop = 255; //used in various modes
    write_flashmem(); //write flash with new mode setting
}
void read_adc(char numofreads){//read adc value, return the value after reading
    for(char i=0; i < numofreads; i++){
        ADCON0bits.GO_nDONE = 1; //start ADC
        while(ADCON0bits.GO_nDONE){ //while the adc isnt done reading
        }
        //todo check on scope change in digit display time
        //if(mode != 8 && mode != 7){ //if not in temp nor batt mode
            short reading = ((ADRESH << 8) | ADRESL); //store the result in 'reading'
            if(reading > maxenvelope){ maxenvelope = reading; }//if we broke the maximum, set the new max
            if(reading < minenvelope){ minenvelope = reading; }//if we broke the minimum, set the new min
        //}
    } //numreads
} //read_adc()
void updateValue(signed char arrayinput[], signed char value){ //add arrayinput by 'value', limited to 0-9
    signed char rollover = 0; // used to carry a digit to the next place
    arrayinput[5] = arrayinput[5] + value; //add that shit
    for(char k=5; k!=255; k--){ //start at right digit and go left
        arrayinput[k] += rollover;
        if(arrayinput[k] >= 10){ //if this digit is over 10 and needs to be subtract 10, with rollover to next digit
            arrayinput[k] = arrayinput[k] - 10; //subtract 10
            rollover = 1; //we need to add 1 to the digit to the left, can't add more than 10 safely
        }else if(arrayinput[k] <= -1){ //we subtracted >=1 from 0
            arrayinput[k] = 9; //set to 9 doesn't work when subtracting larger values
            rollover = -1; //we need to subtract 1 to the digit to the left
        }else{ //current digit is 9 or less, no need to roll over or reset to zero, we're done
            //rollover = 0;
            break;
        }
    }//for loop, looping through digits
}//updateValue()
void updateDodgeCharacter(){
    if(gamechar == 1){
        segmentbuffer[0] = 0b11111100; //top line
    }else if(gamechar == 2){
        segmentbuffer[0] = 0b01111110; //middle line
    }else{
        segmentbuffer[0] = 0b11101110; //bottom line
    }//end character movement
    if(gamechar2){ //if gamechar2 > 0, we used our powerup, remove dp
        segmentbuffer[0] |= 0b00000001;
    }
}//updatedodgechar()
void flipItAndReverseIt(){//flip each digit when tilted
    char tempflipbuf[6];
    memcpy(tempflipbuf, segmentbuffer, 6);//backup segmentbuffer to tempflipbuf
    for(char i=0;i<6;i++){
        char k = tempflipbuf[5-i];//to save code space
        segmentbuffer[i] = (0b10000001 & k);//segments G and DP don't move when flipped, swap them
        if(0b00000010 & k){ segmentbuffer[i] |= 0b00010000;}//seg A
        if(0b00000100 & k){ segmentbuffer[i] |= 0b00100000;}//seg B
        if(0b00001000 & k){ segmentbuffer[i] |= 0b01000000;}//seg C
        if(0b00010000 & k){ segmentbuffer[i] |= 0b00000010;}//seg D
        if(0b00100000 & k){ segmentbuffer[i] |= 0b00000100;}//seg E
        if(0b01000000 & k){ segmentbuffer[i] |= 0b00001000;}//seg F
    }
}//nerfebinipfyneyapnap
void lightDigit(char dig){//digit 0-5
    LATC = segmentbuffer[dig]; //copy what segments to diplay to output
    if(segmentbuffer[dig] != 255){ //if this digit has something to display
        read_adc(brightnesspause); //dim display based on brightness setting
        if(dig < 4){//digits 0-3
            LATB = (0b11101111 << dig); //dig 0 0b11101111 1 0b11011111 2 0b10111111 3 0b01111111
        }else {
            LATA = 0b11101111<<(dig-4); //light up digit 5-6
        }
        read_adc(numadcreads-1); //read the mic while we're waiting
        if(dig != 0){ read_adc(1); } //read one less for digit 0 due to computation logic
        if(displaytilted){ //display tilted
            if(option == 20 + dig ){
                read_adc(numadcreads * 6); //make the current option digit brighter
            }
        }else if(option == 25 - dig){
            read_adc(numadcreads * 6); //make the current option digit brighter
        }
    }else if(mode == 13){//morse code mode
        read_adc(numadcreads); //added delay so morsecode mode is timed right
    }
}//lightDigit())
void decimalDisplay(signed short val){//used in to save code space
    if(val < 0){//negative value, need positive values for digitlookup[]
        val *= -1;//negate
    }
    segmentbuffer[3] = digitlookup[(val / 100) % 10];//val 100's place
    segmentbuffer[4] = digitlookup[(val / 10) % 10]; //val 10's place
    segmentbuffer[5] = digitlookup[val % 10]; //val 1's place;
}
void main(void) {
    TRISC = 0b00000000; //set rc0-rc7 pins as output
    TRISB = 0b00000000; //set rb4-7 pins as output
    TRISA = 0b11001111; //set ra4-5 pins as output/ ra0-3 pins as input
    OSCCON = 0b01111010; //SPLLEN=0, IRCF=1111, 0, SCS1=1
    //OSCCONbits.SCS1 = 1; //SCS to internal clock source
    //OSCCONbits.IRCF = 0b1111; //set clock freq, max 16mhz
    ANSELA = 0b00000010; //RA1 as analog input for mic, RA0,2,3 digital input
    //INLVLA = 0b00000000; //TTL logic on all input pins
    OPTION_REG = 0b01010111; //nWPUEN=0, INTEDG=1, TMR0CS=0, PSA=0, PS=111
    //OPTION_REGbits.nWPUEN = 0; //weak pullups are enabled globally
    //OPTION_REGbits.TMR0CS = 0; //timer users internal clock
    //OPTION_REGbits.PSA = 0; //prescaller on
    //OPTION_REGbits.PS = 0b111; //256 prescaller
    WPUA = 0b00001101; //weak pull up enabled for RA0,2-3
    ADCON1 = 0b11100000;//ADC right justified, ADC clock supplied from internal oscillator, vref+ is VDD
    //ADCON1bits.ADFM = 1; //ADC is right justified
    //ADCON1bits.ADCS = 0b110; //ADC clock supplied from internal oscillator
    //ADCON1bits.ADPREF = 0b00; //VREF+ is connected to VDD for ADC bounding
    ADCON2bits.TRIGSEL = 0b0000; //no auto-conversion trigger selection bits
    //FVRCONbits.TSRNG = 1; //low temp range
    ADCON0 = 0b00000101; //-<CHS4:0=00001><GO/DONE=0><ADON=1>
    //ADCON0bits.CHS = 0b00001; //select AN1 ADC channel
    //ADCON0bits.ADON = 1; //enable ADC for mic input
    WDTCONbits.WDTPS; //2 second watchdog timer reset period
    short audiomodemax; //max adc reading while in audio mode
    short audiomodemin;//min adc reading while in audio mode
    signed char decimal = 0xff; // LSB = right most digit decimal point, 0=on
    char entersleep = 0; //this tracks buttons being held to enter sleep
    for (;;){//main loop
        //first digit, left most
        lightDigit(0);
        
        if(option == 0){ //not in any option menu
            if(mode <= 2){ //if we're in count up/down, normal, audio, or startup modes, dump displaybuffer to display
                if(gameshort2 > 0){//in timeout window for shot mode mute on/off display
                    gameshort2--;//decrement sub menu timeout
                    blackout();//clear all digits
                    segmentbuffer[2] = 0b01000111;//o
                    if(micthresh > 9000){ //already muted, display on
                        segmentbuffer[3] = 0b01010111;//n
                    }else{ //display off
                        segmentbuffer[3] = 0b00011101;//f
                        segmentbuffer[4] = 0b00011101;//f
                    }//end muted/not muted check
                }else{
                    //if you're just looking to crudely modify the display to what you want
                    //set segmentbuffer[0]-[5] here (modes <= 2), [0]=left digit, [5]=right digit
                    //using the 7-segment notation A-G, A=top segment (continues clockwise to F) and G=middle segment
                    //plus the decimal point (DP), you set each segmentbuffer[] char array to 0bGFEDCBA(DP)
                    //bit 1 is that segment off, bit 0 is that segment on, due to current sinking
                    for(char i=0;i<6;i++){//optimized to save code space
                        //pull out decimal bit, apply it to which segments to light
                        segmentbuffer[i] = (0xFE | decimal>>(5-i)) & digitlookup[displaybuffer[i]];
                    }
                }
                if(displaytilted){ //display tilted
                    flipItAndReverseIt();
                }
            }else if(mode == 3){ //rand mode
                if( 0 == (loop & (0xFF >> (speed-1)))){ //used to slow down display refresh, if n LSBs of loop are all 0's
                    if(gamechar == 0){//gamechar 0 is scroll right
                        for(char i = 5;i != 0;i--){//move digits over left
                            segmentbuffer[i] = segmentbuffer[i-1];
                        }
                        segmentbuffer[gamechar] = rand();//update the one new digit on the end (depending on scroll direction)
                    }else if(gamechar == 5){
                        for(char i = 0;i < 5;i++){//move digits over right
                            segmentbuffer[i] = segmentbuffer[i+1];
                        }
                        segmentbuffer[gamechar] = rand();//update the one new digit on the end (depending on scroll direction)
                    }else{//gamechar == 10, refresh all digits
                        for(char i=0;i<6;i++){//set every segment randomly
                            segmentbuffer[i] = rand();
                        }//end for loop
                    }//end random sub-modes
                }//end random mode display refresh
                for(char i=0;i<6;i++){ //code space optimization
                    segmentbuffer[i] = (0b1 & (decimal>>(5-i))) | (0xFE & segmentbuffer[i]);
                }
                //end rand mode decimal point display update loop
            }else if(mode == 4){ //hype mode, display dEFcon, Shoot
                //gamechar submodes
                //gamechar2 array position in secret menu
                if(startup && gamechar < 255 && !PORTAbits.RA3){//show version if start button is pressed during startup
                    blackout();//clear screen
                    segmentbuffer[0] = MAJOR_VERSION;//show major version on left digit
                    //segmentbuffer[0] &= 0b11111110;//add a DP
                    segmentbuffer[1] = MINOR_VERSION;//show minor version after DP
                    loop = 0;//reset this to prevent moving to next mode
                    gamechar++;
                }else if(gamechar == 22){//pressed start 21 times, one over special mode
                    gamechar = 0;//reset to normal hype mode
                }else if(gamechar == 20){ //custom string for users (manually programmed), press start 20x
                    blackout(); //so people know something happened right away
                    gamechar++;//show the custom string
                }else if(gamechar == 21){
                    if(loop == 0){
                        loop = speed*28;//reset loop value
                        for(char i=0;i<6;i++){//set each digit from the secretstring[]
                            segmentbuffer[i] = secretstring[gamechar2+i];
                        }
                        gamechar2++;//go to next array position
                        if(gamechar2 > sizeof(secretstring)-6){//over the max array size?
                            gamechar2 = 0;//go back to the start of the array
                        }
                        if(displaytilted){ //display tilted
                            flipItAndReverseIt();
                        }
                    }//display update conditional
                }else if(loop < 117){ //display "dEFcon" (not 128 due to space in shoot messing up timing)
                    segmentbuffer[0] = 0b01000011;segmentbuffer[1] = 0b00001101;
                    segmentbuffer[2] = 0b00011101;segmentbuffer[3] = 0b01001111;
                    segmentbuffer[4] = 0b01000111;segmentbuffer[5] = 0b01010111;
                    if(displaytilted){ //display tilted
                        flipItAndReverseIt();
                    }
                }else{ //display " shoot"
                    //no dp in this mode since "shoot" is 5 chars and digit 0 is skipped
                    segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b00100101;
                    segmentbuffer[2] = 0b00010011;segmentbuffer[3] = 0b01000111;
                    segmentbuffer[4] = 0b01000111;segmentbuffer[5] = 0b00001111;
                    if(displaytilted){ //display tilted
                        flipItAndReverseIt();
                    }
                }//end display conditional of hype mode
                //mode == 4, hype mode
            }else if(mode == 5){ //react game mode
                //displaybuffer[], gamechar, gameshort, and loop have been cleared by switchmode()
                //gamechar = different sub modes of the game, "ready", blackout, fail, count up, and score display
                //gameshort is the random blackout time,
                if(gamechar == 250 || gamechar == 251){ //start the counter
                    //dump the counter to the display
                    //blackout(); //don't light any segments should already be blacked out
                    segmentbuffer[2] = digitlookup[displaybuffer[3]]; //dig3, lookup pattern
                    segmentbuffer[3] = digitlookup[displaybuffer[4]]; //dig4, lookup pattern
                    segmentbuffer[4] = digitlookup[displaybuffer[5]]; //right digit, lookup pattern
                    if(gamechar == 250){//counting up
                        if(displaybuffer[3] == 5){ //if the user didn't press start and we're at 500 end the game
                            gamechar = 2; //fail
                        }
                        updateValue(displaybuffer, 1);//increment the display buffer
                    }else{//in scoreboard viewing mode
                        segmentbuffer[1] = 0b11111111;//so when flipped it doesn't fuck up
                        if(displaytilted){ //display tilted
                            flipItAndReverseIt();
                        }
                    }
                }else if(gamechar == 0){ //initial display mode, "ready"
                    if(loop < 220){ //first 220 interations, display " ready"
                        segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b01011111;
                        segmentbuffer[2] = 0b00001101;segmentbuffer[3] = 0b00010001;
                        segmentbuffer[4] = 0b01000011;segmentbuffer[5] = 0b00100011;
                        if(displaytilted){ //display tilted
                            flipItAndReverseIt();
                        }
                    }else{ //begin game
                        gamechar = 1; //move to blackout period
                    }
                    gameshort = gameshort = 0xffe + (rand() & 0x7FFF); //calculate the initial timeout, before starting the counter
                    //out of initial react game startup
                }else if(gamechar == 1){ //in blackout period
                    blackout();//blank screen
                    if(gameshort != 0){ //if time left during blackout period
                        gameshort--; //don't do anything until the timeout expires
                    }else{ //gameshort == 0, blackout timer expired
                        gamechar = 250; //start counter, gamechar = 2 is fail, start increments gamechar, so build in buffer
                    }
                }else if(gamechar < 250){ //pressed start during blackout, before counter started
                    //display " fail "
                    //blackout(); //don't light any segments, should already be blacked out
                    segmentbuffer[1] = 0b00011101;segmentbuffer[2] = 0b00010001;//fa
                    segmentbuffer[3] = 0b11110111;segmentbuffer[4] = 0b10001111;//il
                    if(displaytilted){ //display tilted
                        flipItAndReverseIt();
                    }
                    gamechar++;
                    if(gamechar > 246){ //we've showed them how bad they suck, start over, gap to prevent pressing start
                        switchmode(5);
                    }
                }else if(gamechar == 252){ //user pressed start while viewing their score
                    switchmode(5); //restart game
                }
                //end mode == 5
            }else if(mode == 6){ //dodge game
                //gameshort, game sub modes, number of blocks dodged
                //gamechar character position, 1-3
                //gamechar2 used for special power use, >1 = used, higher numbers used for position of "8" display
                if(gameshort == 0){ //startup display mode
                    updateDodgeCharacter(); //display character
                    if(loop == 0){ //update countdown number
                        loop = 100;//reset loop
                        if(segmentbuffer[3] == 0xFF && segmentbuffer[4] == 0xFF && segmentbuffer[5] == 0xFF){ //all off
                            segmentbuffer[3] = 0b01100001; // display 3
                        }else if(segmentbuffer[3] == 0b01100001){ // if 3 is displayed
                            segmentbuffer[3] = 0b11111111; //shut off 3
                            segmentbuffer[4] = 0b01001001; // display 2
                        }else if(segmentbuffer[4] == 0b01001001){ //if 2 is displayed
                            segmentbuffer[4] = 0b11111111; //shut off 2
                            segmentbuffer[5] = 0b11110011; // display 1
                        }else{ //done with count down
                            segmentbuffer[5] = 0b11111111; //shut off 1
                            gameshort = 1; //start game
                        }//end "1"/"2"/"3" conditional
                    }//end loop == 255
                //end startup 3/2/1
                }else if(gameshort > 0 && gameshort < 500){ //main game mode
                    if(!gamechar2 && (maxenvelope - minenvelope) > 40){ //if we have a special, and we hit the mic, use special
                        gamechar2 = 1; //turn off DP indicator
                        updateDodgeCharacter(); //write updated char without dp
                        loop = 75; //don't advance blocks for a bit
                    }else if(gamechar2 && gamechar2 < 6 && loop == 89){ //if we just used our special, display "8"s
                        segmentbuffer[gamechar2] = 0b00000001; // "8"
                        loop = 75; //reset loop value
                        gamechar2++; //move to next digit
                    }else if(gamechar2 && gamechar2 < 11 && loop == 89){ //if we used our special, and displayed "8"s, remove them
                        segmentbuffer[gamechar2 - 5] = 0b11111111; //clear block, gamechar = 6-10
                        loop = 75; //reset loop value
                        gamechar2++; //move to next digit
                    }
                    if(loop == 90){ //if we should move blocks/check for collisions, loop is set to 1-86, then incremented to 90
                        if((gamechar == 1 && !(segmentbuffer[1] & 0b00000010) || //collision detection
                                gamechar == 2 && !(segmentbuffer[1] & 0b10000000) ||
                                gamechar == 3 && !(segmentbuffer[1] & 0b00010000)) && gamechar3 < 20){ 
                            gameshort = 500; //go to fail mode
                            loop = 0;
                            //end collision detection
                        }else{ //no collision detected
                            //set new scroll speed by resetting loop value
                            if(gameshort <= 10){ //first 10 blocks, 4x multiplier for speed
                                loop = gameshort * 4; //bump speed by 4, gameshort 1-10 = loop 4-40
                            }else if(gameshort <= 30){ //blocks 10-40, 1x multiplier
                                loop = 30 + gameshort; //gameshort 11-30 = loop 41-60
                            }else if(gameshort <= 75){ //blocks 40-75, 0.25 multiplier
                                loop = 54 + (gameshort / 4); //gameshort 31-75 = loop 61-72
                            }else{ //blocks 75+, 1/16th multiplier
                                loop = 69 + (gameshort / 16); //gameshort 76-272 = loop 73-86
                            }
                            if(segmentbuffer[1] != 255){ //if the block closest to the game character wasn't blank
                                updateValue(displaybuffer, 1); //increment score if we avoided a block
                            }
                            segmentbuffer[1] = segmentbuffer[2];segmentbuffer[2] = segmentbuffer[3];//move blocks left
                            segmentbuffer[3] = segmentbuffer[4];segmentbuffer[4] = segmentbuffer[5];
                            if(segmentbuffer[4] != 255){ //if last block wasn't blank, add a blank block
                                segmentbuffer[5] = 0b11111111; //blank block in right position
                            }else{ //add a random block
                                segmentbuffer[5] = dodgeblocks[(rand() % 6)]; //random index 0-5, lookup block from dodgeblocks[]
                                if(gameshort <= 272){ gameshort++; } //make it harder, used in scroll speed calculations
                            }//end adding new block
                        }//end no collision detected condition
                    }//end loop=90
                    read_adc(2 * numadcreads); //make first digit (player segment) brighter in game
                //end in game, gameshort == 1-499 max
                }else if(gameshort == 500){ //show fail for a bit
                    blackout(); //don't light any segments
                    segmentbuffer[1] = 0b00011101;segmentbuffer[2] = 0b00010001;//display " fail "
                    segmentbuffer[3] = 0b11110111;segmentbuffer[4] = 0b10001111;
                    if(loop == 255){ gameshort = 501; } //move on to displaying score
                }else if(gameshort == 501){ //display score
                    blackout(); //don't light any segments
                    for(char i=2;i<6;i++){//code optimization, segmentbuffer[2]-[5]
                        segmentbuffer[i] = digitlookup[displaybuffer[i]]; //display score
                    }
                    if(displaytilted){ //display tilted
                        flipItAndReverseIt();
                    }
                }//end dodge game sub-modes
                //end dodge game conditional
            }else if(mode == 7 || mode == 8){
                //each batt offset increment is 0.02v, we can adjust -20 to 20 or -.4 to +.4 of reading
                if(gamechar2 > 0){ //in offset window
                    gamechar2--; //decrement for offset timeout
                        if(gamechar2 == 0){ //timeout over, write flash
                            write_flashmem(); //store the offset
                        }
                    signed char calcoffset = 0;//used for code optimization
                    if(mode == 8){ calcoffset = battoffset; //if we're in batt mode 
                    }else{ calcoffset = tempoffset; } //else we're in temp mode
                    decimalDisplay(calcoffset);//saves codespace, populates segbuf[3-5]
                    //segmentbuffer[4] = digitlookup[(calcoffset/10) % 10];
                    //segmentbuffer[5] = digitlookup[calcoffset % 10];
                    if(calcoffset < 0){ //if we're negative
                        segmentbuffer[3] = 0b01111111; //"-" negative
                    }else{ //positive values of offset
                        segmentbuffer[3] = 0b11111111; // " " (no minus sign)
                    }//negative display logic
                    segmentbuffer[0] = 0b01000111; //o
                    segmentbuffer[1] = 0b00011101; //f
                    segmentbuffer[2] = 0b00011101; //f
                }else if(mode == 7){ //temp reading / display
                    //this thing is total shit http://www.microchip.com/forums/tm.aspx?high=&m=552887
                    //even their app note has an error on it http://ww1.microchip.com/downloads/en/AppNotes/01333A.pdf
                    if(gamechar == 0){//first time, read batt level for offset
                        gamechar++; //don't do this next time
                        ADCON0bits.GO_nDONE = 1; //start ADC
                        while(ADCON0bits.GO_nDONE){ //while the adc isnt done reading
                        }
                        //1023 (max adc) * 1.024 (FVR)* some power of 10 since decimals suck
                        vdd = (short)(104755.2 / ((ADRESH << 8) | ADRESL))+(battoffset*2)+BATT_CORRECTION; //read
                        vddtemp = ((float)vdd) / 400; //divide by 4 for temp calculation below (high range))
                        FVRCON = 0b10110011; //switch to temp reading, fvren=1, fvrrdy=0, tsen=1, tsrng=1, cdavfr=00, adfvr=00 
                        ADCON0bits.CHS = 0b11101; //select temp ADC channel
                    }else if( 0 == (loop & (0xFF >> (speed-1))) ){//time to read temp
                        blackout();
                        ADCON0bits.GO_nDONE = 1; //start ADC
                        while(ADCON0bits.GO_nDONE){ //while the adc isnt done reading
                        }
                        temp = ((ADRESH << 8) | ADRESL); //read temp, adjust due to offset/cal
                        //offset: .95*vdd+offset
                        //temp = 529;//debug
                        //vddtemp = 5.5/4;
                        temp = (short)(((0.71214233 - vddtemp * (1-(0.95*temp+tempoffset)/1023) )/.000132-400));
                        
                        if(temp <= -1000){ //too low to display
                            //todo, is this in the manual i wrote?
                            segmentbuffer[1] = 0b10001111;segmentbuffer[2] = 0b01000111; //Lo
                            segmentbuffer[3] = 0b10000111;segmentbuffer[4] = 0b11000011; //both halves of W
                        }else if(temp >= 1000){//too high to display
                            segmentbuffer[1] = 0b00010011;segmentbuffer[2] = 0b11011111; //Hi
                            segmentbuffer[3] = 0b00100001;segmentbuffer[4] = 0b00010111; //gh
                        }else {//not high or low, display temperature
                            if(temp < 0){ //if we're negative
                                //temp = temp * ((short)-1);//stupid bytes being too small
                                segmentbuffer[1] = 0b01111111; //"-" negative, one of these will be overwritten later
                            }//else{ //todo test, since we have blackout
                            //    segmentbuffer[1] = 0b11111111;//positive values
                            //}
                            decimalDisplay(temp);
                            segmentbuffer[2] = segmentbuffer[3];segmentbuffer[3] = segmentbuffer[4];
                            segmentbuffer[4] = segmentbuffer[5];
                            //segmentbuffer[2] = digitlookup[(temp / 100) % 10];//temp 10's place
                            //segmentbuffer[3] = digitlookup[(temp / 10) % 10]; //temp 1's place
                            //segmentbuffer[4] = digitlookup[temp % 10]; //temp .1's place;
                            segmentbuffer[5] = 0b00111001;//degrees symbol
                        }//end positive/negative conditionals
                        segmentbuffer[0] = 0b11111111; //readings get jacked up if we use this digit
                        if(displaytilted){ //display tilted
                            if(temp < 1000 && temp > -1000){//display actual temp
                                //move things over so we don't use [0]
                                segmentbuffer[0] = segmentbuffer[1];segmentbuffer[1] = segmentbuffer[2];
                                segmentbuffer[2] = segmentbuffer[3];segmentbuffer[3] = segmentbuffer[4];
                                segmentbuffer[4] = segmentbuffer[5];segmentbuffer[5] = 0b11111111;
                                segmentbuffer[3] &= 0b11111110; //turn on DP on 3's voltage digit
                            }
                            flipItAndReverseIt();
                        }else if(temp < 1000 && temp > -1000){//if not tilted and in valid temp range
                            segmentbuffer[3] &= 0b11111110; //turn on DP on 1's voltage digit
                        }
                    }//in offset timeout window / not in timeout window
                }else if(mode == 8){ //battery voltage display mode
                    //not in the offset timeout window
                    if( 0 == (loop & (0xFF >> (speed-1))) ){//time to read / update display
                        ADCON0bits.GO_nDONE = 1; //start ADC
                        while(ADCON0bits.GO_nDONE){ //while the adc isnt done reading
                        }
                        //1023 (max adc) * 1.024 (FVR)* some power of 10 since decimals suck
                        vdd = ((short)(104755.2 / ((ADRESH << 8) | ADRESL))) + (battoffset*2)+BATT_CORRECTION; //read, adjust by offset
                        blackout();
                        decimalDisplay(vdd);
                        segmentbuffer[2] = segmentbuffer[3];segmentbuffer[3] = segmentbuffer[4];
                        segmentbuffer[4] = segmentbuffer[5];segmentbuffer[5] = 0xff;
                        if(displaytilted){ //display tilted
                            flipItAndReverseIt();
                        }//end tilt
                        segmentbuffer[2] &= 0b11111110; //turn on DP on 1's voltage digit
                    }//end reading/updating display
                }//end mode 8
            }else if(mode == 13){ //morse game mode
                //speed 0: dot = 72 (loop var), speed 1: dot = 64, speed 2: dot = 56...
                //first time bucket  = on for both . and -
                //second time bucket = skipped for . and on for -
                //third time bucket  = off for . and -
                //gameshort  = current charposition in array
                //gameshort2 = where sub-string starts
                //gamechar2  = bit position in char (+1)
                //gamechar   = string number to use
                //pull off next char from char[] read first 3 bits & 0xe0
                if(gamechar2 == 0){ //need new character?
                    gameshort++; //next character
                    if(arraypointer[gameshort] == 100){//at end of this string
                        gameshort = gameshort2;//go back to first char in substring
                    }
                    //pull out num of elements in this char (first 3 bits), shift down to dec1-5 (num of elements per char)
                    gamechar2 = (((morselookup[arraypointer[gameshort]]) & 0b11100000)>>5);//finish operation described above
                    loop = 208+((speed-1)*4); //at end of char, pause before next char (until 255)
                }
                char tempspeedcalc = 72-((speed-1)*8);//used to save code space
                if(loop == 0){//update segments, turn on (for the length of '.')
                    if(!tiltoption){//not set to tilted
                       segmentbuffer[0] = 0b11101101;segmentbuffer[1] = 0b11101101; //light segments 
                       segmentbuffer[2] = 0b11101101;segmentbuffer[3] = 0b11101101;
                       segmentbuffer[4] = 0b11101101;segmentbuffer[5] = 0b11101101;
                    }else{//set to tilted, invert lighting, don't sink on A/D segment for relay use
                       //blackout(); //for some reason this would add 2 bytes to the code, wtf compiler
                       segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b11111111; //don't light segments
                       segmentbuffer[2] = 0b11111111;segmentbuffer[3] = 0b11111111;
                       segmentbuffer[4] = 0b11111111;segmentbuffer[5] = 0b11111111;
                    }
                }else if(loop == tempspeedcalc){ //we've lit the display for the timeframe of a . next determine if element is a -
                    //grab current char & the bit position of current .- element : if that bit = 1 we're done with this element
                    if(((morselookup[arraypointer[gameshort]]) & (0x1<<(gamechar2-1))) > 0){ // .
                        loop = tempspeedcalc*2; //done with element, skip to shut off display
                    }
                }
                //done lighting the display
                if(loop == tempspeedcalc*2){//shut off the display, in pause window, one . in length
                    if(!tiltoption){//not set to tilted
                        blackout(); //don't light any segments
                    }else{//set to tilted, invert lighting, don't sink on A/D/G segment for relay use
                       segmentbuffer[0] = 0b01101101;segmentbuffer[1] = 0b01101101; //light segments
                       segmentbuffer[2] = 0b01101101;segmentbuffer[3] = 0b01101101;
                       segmentbuffer[4] = 0b01101101;segmentbuffer[5] = 0b01101101;
                    }
                }else if(loop == tempspeedcalc*3){ //done with this element and pause, grab next element
                    loop = 255;
                    gamechar2--; //next bit lower
                }
            }else if(mode == 12){ //btc
                if(gamechar2 == 42){ //displayed the whole thing, reboot, gamechar=6*7                   
                    startup = 10; //needed for startup
                    loop = 0; //reset loop
                    optiontimeout = 5; //needed for startup
                    gamechar2 = 0; //probably should reset this
                    mode = 4; //reboot
                }else if( loop == 0 ){//|| loop == 127){//move to next block
                    for(char i = 0; i < 6; i++){
                        segmentbuffer[i] = bitcoin_mini_key[i + gamechar2];
                    }
                    loop = 180; //don't wait a full 255 to go to next one. 
                    gamechar2 = gamechar2 + 6; //next block of 6
                }
            }else if(mode == 11){//timer mode
                //gamechar = mode, 0=Press, 1=Start, 2=blackout, 9=shoot!, 10-25=counting mode, 30-45=review mode
                //gamechar2 = used to step down to 0-9 shot number
                //gameshort = blackout timer
                //gameshort2 = number of shots recorded
                //optiontimeout = temp value used for calc
                if(gamechar == 0){//display press
                    segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b00011001;
                    segmentbuffer[2] = 0b01011111;segmentbuffer[3] = 0b00001101;
                    segmentbuffer[4] = 0b00100101;segmentbuffer[5] = 0b00100101;
                    if(loop == 190){gamechar = 1;} //cycle to "start"
                }else if(gamechar == 1){//display start
                    //segmentbuffer[0] = 0b11111111;
                    segmentbuffer[1] = 0b00100101;
                    segmentbuffer[2] = 0b00001111;segmentbuffer[3] = 0b00010001;
                    segmentbuffer[4] = 0b01011111;segmentbuffer[5] = 0b00001111;
                    if(loop == 190){gamechar = 0;} //cycle to "press"
                }else if(gamechar == 2){//blackout period
                    gameshort--;//when this hits zero, blackout period over
                    if(gameshort == 0){//end of blackout, go to "shoot!" mode / count shots
                        gamechar = 10;//move to count shots mode
                        TMR0 = 0; //reset timer
                    }
                }//end gamechar 0-2
                if(gamechar >= 10){//in counter or review mode
                    if(gamechar > 29){//in reivew mode
                        char i = (gamechar-30)*4;//starting position in the array for this shot
                        displaybuffer[1] = timerbuf[i];//load each display buffer
                        displaybuffer[2] = timerbuf[i+1];
                        displaybuffer[3] = timerbuf[i+2];
                        displaybuffer[4] = timerbuf[i+3];
                    }else{//not in review mode, active timer
                        if(displaybuffer[1] != 9 || displaybuffer[2] != 9){//if we're under 99.00 seconds, keep counting
                            //16mhz clock = 4mhz instruction cycle .00000025
                            //.00000025 * 256 (max counter) = .000064 before timer rollover
                            //256 prescaller = .000064 steps, .016384 before roll over
                            //256/125 8ms steps close, too slow over time, 124 too fast
                            //256 scaller, 78 steps = 5ms, closest I can get
                            while(TMR0 >= 78){//if the timer has ticked over 5ms
                                TMR0 = TMR0 - 78;//subtract 5ms from timer
                                updateValue(displaybuffer, 5);//increment display by 5ms, something something nyquist?shannon sampling
                            }//end timer loop
                            if(loop == 40){//small offset to make timer more accurate
                                loop = 0;//reset loop, it only goes between 0-40
                                updateValue(displaybuffer, 1);//adjust clock by .001
                            }
                        }//end, stop timer at 99 seconds
                    }
                    segmentbuffer[0] = digitlookup[gamechar-10-gamechar2]; //show shot number 0-9, gamechar2 = 20 in review mode
                    segmentbuffer[0] &= 0b11111110; //turn on dp
                    //use [1]-[4] since we're counting ms, but we want to display 10's of ms
                    segmentbuffer[2] = digitlookup[displaybuffer[1]];//10's of seconds
                    segmentbuffer[3] = digitlookup[displaybuffer[2]];//seconds
                    segmentbuffer[3] &= 0b11111110; //turn on decimal point
                    segmentbuffer[4] = digitlookup[displaybuffer[3]];//tenths of seconds
                    segmentbuffer[5] = digitlookup[displaybuffer[4]];//hundredths of seconds
                }//end gamechar checks
            }else if(mode == 9){//sound meter mode
                //Different displays for sound levels stored in soundblocks[]
                //normal mode adds new characters to the rightmost digit, and scrolls left with slow fall off
                //if tiltoption is set, it is a live display, the middle two digits show the level with slow fall off
                //gamechar is used for rolloff of display
                //gamechar2 is for loudnesslevel setting
                //gamechar3 is used for temp value of segbuf[2] during external relay mode
                //gameshort is for loudnesslevel configuration timeout
                //speed 8=loop4, 7=10, 6=16, 5=24, 4=30, 3=36, 2=42, 1=48, 0=54
                if(gameshort > 0){//in loudness level configuration, and havent timed out yet
                    gameshort--; //decrement timeout
                    if(gameshort == 0){//timeout period over, set value
                        loudnesslevel = gamechar2;//set temp value to loudness level
                        write_flashmem(); //store value
                    }else{//not in timeout value
                        segmentbuffer[0] = 0b10001111;segmentbuffer[1] = 0b01000111;//Lo
                        segmentbuffer[2] = 0b11000111;segmentbuffer[3] = 0b01000011;//ud
                        segmentbuffer[4] = 0b11111111;segmentbuffer[5] = digitlookup[gamechar2];//" #" for loudness level
                        if(gamechar2 == loudnesslevel){segmentbuffer[5] &= 0b11111110;}//set dp if temp value == current value
                    }
                }else if( 60-(speed*6) == loop){//time to update the display
                    loop = 0;//reset since we're not counting to 255
                    maxmindiff = maxenvelope - minenvelope; //temp value to avoid calculating this everytime
                    maxenvelope = 0;minenvelope = 1023; //reset values
                    char tempcalc = loudnesslevel+1;//need range 1-8 not 0-7
                    //======== loudness values
                    //loudness 7:7=800 6=656 5=512 4=368 3=224 2=80 1=32
                    //loudness 6:7=700 6=574 5=448 4=322 3=196 2=70 1=28
                    //loudness 5:7=600 6=492 5=384 4=276 3=168 2=60 1=24
                    //loudness 4:7=500 6=410 5=320 4=230 3=140 2=50 1=20
                    //loudness 3:7=400 6=328 5=256 4=184 3=112 2=40 1=16
                    //loudness 2:7=300 6=246 5=192 4=138 3=84  2=30 1=12
                    //loudness 1:7=200 6=164 5=128 4=92  3=56  2=20 1=8
                    //loudness 0:7=100 6=82  5=64  4=46  3=28  2=10 1=4
                    //8 total display possibilities: using index 0-5 of soundblocks[]:
                    //555555, 455554, 345543, 234432, 123321, 012210, 001100, 000000
                    for(char i=0;i<6;i++){segmentbuffer[i] = 0;}//set all segments to zero
                    if(maxmindiff > 100*tempcalc){//0,1,3,5,6,7,8,9
                        segmentbuffer[0] = 3;segmentbuffer[1] = 3;segmentbuffer[2] = 3;//if loudess, light all segments
                        segmentbuffer[3] = 3;segmentbuffer[4] = 3;segmentbuffer[5] = 3;
                        gamechar = 7;//used to roll off next time
                    }else if(maxmindiff > 82*tempcalc || gamechar == 7){
                        segmentbuffer[0] = 4;segmentbuffer[1] = 3;segmentbuffer[2] = 3;//if second loudess, light 455554
                        segmentbuffer[3] = 3;segmentbuffer[4] = 3;segmentbuffer[5] = 5;
                        gamechar = 6;//used to roll off next time
                    }else if(maxmindiff > 64*tempcalc || gamechar == 6){
                        segmentbuffer[0] = 2;segmentbuffer[1] = 4;segmentbuffer[2] = 3;//if third loudess, light 345543
                        segmentbuffer[3] = 3;segmentbuffer[4] = 5;segmentbuffer[5] = 2;
                        gamechar = 5;//used to roll off next time
                    }else if(maxmindiff > 46*tempcalc || gamechar == 5){
                        segmentbuffer[0] = 6;segmentbuffer[1] = 2;segmentbuffer[2] = 4;//if 4th loudess, light 234432
                        segmentbuffer[3] = 5;segmentbuffer[4] = 2;segmentbuffer[5] = 7;
                        gamechar = 4;//used to roll off next time
                    }else if(maxmindiff > 28*tempcalc || gamechar == 4){
                        segmentbuffer[0] = 1;segmentbuffer[1] = 6;segmentbuffer[2] = 2;//if 3rd loudess, light 123321
                        segmentbuffer[3] = 2;segmentbuffer[4] = 7;segmentbuffer[5] = 1;
                        gamechar = 3;//used to roll off next time
                    }else if(maxmindiff > 10*tempcalc || gamechar == 3){
                        segmentbuffer[1] = 1;segmentbuffer[2] = 6;//if 2nd loudess, light 012210
                        segmentbuffer[3] = 7;segmentbuffer[4] = 1;
                        gamechar = 2;//used to roll off next time
                    }else if(maxmindiff > 4*tempcalc || gamechar == 2){
                        segmentbuffer[2] = 1;segmentbuffer[3] = 1;//if 1st loudess, light 001100
                        gamechar = 1;//used to roll off next time
                    }
                    gamechar--; //roll off one level for next time
                    gamechar3 = segmentbuffer[2]+8;//temp value for external relay mode
                    for(char i=0;i<6;i++){//convert index values to soundblock[] segments to light
                        if(tiltoption){//if tilt option set, external relay mode
                            segmentbuffer[i] = gamechar3; //set every digit the same for relay control (2nd dig)
                        }
                        segmentbuffer[i] = soundblocks[segmentbuffer[i]];//set actual soundblocks value to each digit
                    }
                }//end update new character conditional in sound meter mode
            }else if(mode == 10){ //clap mode
                //gamechar = on/off state
                //gamechar2 = number of claps
                //gamechar3 = temp num of claps during config
                //gameshort = main timeout period
                //gameshort2 = clap number config timeout period
                blackout();
                if(gameshort2 == 0){//not in clapnum config timeout
                    if(gameshort == 0){//not in main timeout
                        if(gamechar2 == clapnum+2){//clapped the right amount, and timeout expired
                            gamechar2 = 0;//reset claps to the default 0
                            gamechar = (gamechar+1)%2;//change light state, 0->1, 1->0
                        }else if(gamechar2 > 0 && gamechar2 < clapnum+2){//if there was not enough claps, don't toggle state
                            gamechar2 = 0;//reset claps to the default 0
                        }
                    }else{//in timer
                        gameshort--;//decrement timer
                        if(gamechar2 > clapnum+2){//clapped too many times, reset to 0, don't change light state
                            gamechar2 = 0; //reset claps to 0
                            gameshort = 0; //reset timer
                        }
                    }//end timeout checks
                    segmentbuffer[0] = digitlookup[gamechar2];//display num claps on first digit
                    if(gamechar == 0){//light off
                        segmentbuffer[1] = 0b11111110;segmentbuffer[2] = 0b11111110;//dp/dp
                        segmentbuffer[3] = 0b01000110;segmentbuffer[4] = 0b00011100;//of
                        segmentbuffer[5] = 0b00011100;//f
                        segmentbuffer[0] = segmentbuffer[0] & 0b11111110;
                    }else{
                        //segmentbuffer[1] = 0b11111111;
                        //segmentbuffer[2] = 0b11111111;segmentbuffer[3] = 0b11111111;
                        segmentbuffer[4] = 0b01000111;segmentbuffer[5] = 0b01010111;//on
                    }
                }else{//in clapnum config timeout
                    segmentbuffer[0] = 0b10001101;segmentbuffer[1] = 0b10001111;//Cl
                    segmentbuffer[2] = 0b00010001;segmentbuffer[3] = 0b00011001;//AP
                    segmentbuffer[5] = digitlookup[(gamechar3+2)];//S#
                    if(gamechar3 == clapnum){//highlight DP on current setting
                        segmentbuffer[5] = segmentbuffer[5] & 0b11111110;//light dp
                    }
                    gameshort2--;//decrement timeout
                    if(gameshort2 == 0){//save new clap num
                        clapnum = gamechar3; //update clapnum from temp config value
                        write_flashmem();//write new clapnum
                    }
                }//end in clap num config timeout
                if(displaytilted){ //display tilted
                        flipItAndReverseIt();
                }//end tilt
            }//end different mode checks, while option == 0
        }else{//end option == 0 (not in option menu), else condition == in option menu
            blackout(); //don't light any segments
            if(option == 1){//in option menu, display "oPtion"
                segmentbuffer[0] = 0b01000111;segmentbuffer[1] = 0b00011001;
                segmentbuffer[2] = 0b00001111;segmentbuffer[3] = 0b11110111;
                segmentbuffer[4] = 0b01000111;segmentbuffer[5] = 0b01010111;
            }else if(option == 2){//display "count "
                segmentbuffer[0] = 0b01001110;segmentbuffer[1] = 0b01000111;
                segmentbuffer[2] = 0b11000111;segmentbuffer[3] = 0b01010111;
                segmentbuffer[4] = 0b00001111;//segmentbuffer[5] = 0b11111111;
            }else if(option == 3){//display "tiMMer"
                segmentbuffer[0] = 0b00001110;segmentbuffer[1] = 0b11110110;
                segmentbuffer[2] = 0b10011001;segmentbuffer[3] = 0b10110001;
                segmentbuffer[4] = 0b00001101;segmentbuffer[5] = 0b01011111;
            }else if(option == 4){//display " shot "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b00100100;
                segmentbuffer[2] = 0b00010111;segmentbuffer[3] = 0b01000111;
                segmentbuffer[4] = 0b00001111;//segmentbuffer[5] = 0b11111111;
            }else if(option == 5){//display " rand "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b01011110;segmentbuffer[2] = 0b00010000;
                segmentbuffer[3] = 0b01010111;segmentbuffer[4] = 0b01000011;
                //segmentbuffer[5] = 0b11111111;
            }else if(option == 6){//display " hype "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b00010011;segmentbuffer[2] = 0b00100010;
                segmentbuffer[3] = 0b00011001;segmentbuffer[4] = 0b00001101;
                //segmentbuffer[5] = 0b11111111;
            }else if(option == 7){//display "audio "
                segmentbuffer[0] = 0b00010001;segmentbuffer[1] = 0b10000011;
                segmentbuffer[2] = 0b01000010;segmentbuffer[3] = 0b11110110;
                segmentbuffer[4] = 0b01000111;//segmentbuffer[5] = 0b11111111;
            }else if(option == 8){//display "setdig"
                segmentbuffer[0] = 0b00100101;segmentbuffer[1] = 0b00001101;
                segmentbuffer[2] = 0b00001111;segmentbuffer[3] = 0b01000010;
                segmentbuffer[4] = 0b11110111;segmentbuffer[5] = 0b00100001;
            }else if(option == 9){//display " tilt "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b00001111;
                segmentbuffer[2] = 0b11110111;segmentbuffer[3] = 0b10001110;
                segmentbuffer[4] = 0b00001110;//segmentbuffer[5] = 0b11111111;
            }else if(option == 10){//display "thresh"
                segmentbuffer[0] = 0b00001111;segmentbuffer[1] = 0b00010111;
                segmentbuffer[2] = 0b01011111;segmentbuffer[3] = 0b00001101;
                segmentbuffer[4] = 0b00100100;segmentbuffer[5] = 0b00010111;
            }else if(option == 11){//display "speed "
                segmentbuffer[0] = 0b00100101;segmentbuffer[1] = 0b00011001;
                segmentbuffer[2] = 0b00001101;segmentbuffer[3] = 0b00001101;
                segmentbuffer[4] = 0b01000010;segmentbuffer[5] = 0b11111110;
            }else if(option == 12){//display "briGht"
                segmentbuffer[0] = 0b00000111;segmentbuffer[1] = 0b01011111;//br
                segmentbuffer[2] = 0b11110111;segmentbuffer[3] = 0b00100001;//ig
                segmentbuffer[4] = 0b00010111;segmentbuffer[5] = 0b00001110;//ht
            }else if(option == 13){ //display " play "
                segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b00011001;
                segmentbuffer[2] = 0b10001111;segmentbuffer[3] = 0b00010001;
                segmentbuffer[4] = 0b00100011;segmentbuffer[5] = 0b11111111;
            }else if(option == 14 || option == 77){//display " quit "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b00110000;
                segmentbuffer[2] = 0b11000110;segmentbuffer[3] = 0b11110110;
                segmentbuffer[4] = 0b00001110;//segmentbuffer[5] = 0b11111111;
            }else if(option >= 20 && option < 26){ //in set digit sub menu
                for(char i = 0;i<6;i++){//convert each decimal digit into segments
                    segmentbuffer[i] = digitlookup[displaybuffer[i]];
                }
                //blink the DP of current dig, mask off the dig, xor the low DP bit (which is masked/shifted to LSB)
                segmentbuffer[25-option] = (0b11111110 & segmentbuffer[25-option])^((loop & 0b00010000)>>4);
            }else if(option >= 170 && option < 221){ //in mic threshold submenu
                if (option < 220) {//mic threshold submenu
                    //segmentbuffer[0] = 0b11111111; //all segments off
                    decimalDisplay((option-169)*2);
                    segmentbuffer[1] = segmentbuffer[3];segmentbuffer[2] = segmentbuffer[4];
                    segmentbuffer[3] = segmentbuffer[5];segmentbuffer[5] = 0xff;
                    segmentbuffer[4] = 0b10000001; //zero (always in steps of 20
                } else {//option == 220
                    //segmentbuffer[0] = 0b11111111; //mic thresh submenu, "off"
                    //segmentbuffer[1] = 0b11111111;
                    segmentbuffer[2] = 0b01000111;
                    segmentbuffer[3] = 0b00011101;
                    segmentbuffer[4] = 0b00011101;
                    //segmentbuffer[5] = 0b11111111;
                }
                if( ((option - 169) * 20) == micthresh ){ //if current mic threshold is shown, light decimal point
                    segmentbuffer[4] &= 0b11111110; //light FIF decimal point
                }
                //end mic thresh conditional
            }else if(option >= 60 && option < 69){ //count speed submenu
                //segmentbuffer[0] = 0b11111111;segmentbuffer[1] = 0b11111111;//dig off
                //segmentbuffer[2] = 0b11111111;segmentbuffer[3] = 0b11111111;//dig off
                segmentbuffer[4] = 0b10000001;segmentbuffer[5] = digitlookup[option - 60]; //"0" and the speed 0-8
                if( (option - 59) == speed ){ //if current count speed is shown, light decimal point
                    segmentbuffer[5] &= 0b11111110; //light the FIF decimal point
                }
            }else if(option == 70){ //"react " game
                segmentbuffer[0] = 0b01011110;segmentbuffer[1] = 0b00001101;
                segmentbuffer[2] = 0b00010001;segmentbuffer[3] = 0b10001101;
                segmentbuffer[4] = 0b00001111;//segmentbuffer[5] = 0b11111111;
            }else if(option == 71){ //"dodge " game
                segmentbuffer[0] = 0b01000011;segmentbuffer[1] = 0b01000110;
                segmentbuffer[2] = 0b01000011;segmentbuffer[3] = 0b00100001;
                segmentbuffer[4] = 0b00001101;//segmentbuffer[5] = 0b11111111;
            }else if(option == 72){ //"teMMp "
                segmentbuffer[0] = 0b00001111;segmentbuffer[1] = 0b00001101;
                segmentbuffer[2] = 0b10011000;segmentbuffer[3] = 0b10110001;//mm
                segmentbuffer[4] = 0b00011001;//segmentbuffer[5] = 0b11111111;
            }else if(option == 73){ //" batt " mode
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b00000111;
                segmentbuffer[2] = 0b00010001;segmentbuffer[3] = 0b00001110;
                segmentbuffer[4] = 0b00001111;//segmentbuffer[5] = 0b11111111;
            }else if(option == 74){ //"Sound" meter mode
                segmentbuffer[0] = 0b00100101;segmentbuffer[1] = 0b01000111;
                segmentbuffer[2] = 0b11000111;segmentbuffer[3] = 0b01010111;
                segmentbuffer[4] = 0b01000010;//segmentbuffer[5] = 0b11111111;
            }else if(option == 75){ //"clap" mode
                segmentbuffer[1] = 0b10001101;segmentbuffer[2] = 0b10001111;//cl
                segmentbuffer[3] = 0b00010001;segmentbuffer[4] = 0b00011001;//ap
                segmentbuffer[5] = 0b11111110;//dp on
            }else if(option == 76){ //"Morse" game
                segmentbuffer[0] = 0b10011001;segmentbuffer[1] = 0b10110001;
                segmentbuffer[2] = 0b01000111;segmentbuffer[3] = 0b01011111;
                segmentbuffer[4] = 0b00100101;segmentbuffer[5] = 0b00001101;
            }else if(option >= 80 && option < 103){ //morse code sub menu "Str ##"
                decimalDisplay(option-80);//saves code space, populates segbuf[2]-[4]
                //segmentbuffer[4] = digitlookup[((option-80)/10)];
                //segmentbuffer[5] = digitlookup[((option-80)%10)];
                segmentbuffer[0] = 0b00100101;segmentbuffer[1] = 0b00001111;//st
                segmentbuffer[2] = 0b01011111;
                segmentbuffer[3] = 0b11111111;//r
            }else if(option == 103){ //" cust "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b10001101;
                segmentbuffer[2] = 0b10000011;segmentbuffer[3] = 0b00100101;
                segmentbuffer[4] = 0b00001111;//segmentbuffer[5] = 0b11111111;
            }else if(option >= 120 && option <= 155){//morse code custom sub sub menu
                //120=a, 135=z, 136=0, 146=9
                //Display: PCC.CDD where CC=current character(morsechar) DD=configurable character(option)
                segmentbuffer[0] = 0b00011001; //"P"
                decimalDisplay(option-120);//saves code space
                segmentbuffer[3] = 0b10001101;//"C"
                segmentbuffer[1] = digitlookup[morsechar/10];//0-49 character being configured
                segmentbuffer[2] = digitlookup[morsechar%10];//do this after battTempDisplay
                segmentbuffer[2] &= 0b11111110; //turn on DP
                //segmentbuffer[4] = digitlookup[(option-120)/10];//numerical value of character to select
                //segmentbuffer[5] = digitlookup[(option-120)%10];
            }else if(option == 156){//morse custom sub sub menu " done "
                //segmentbuffer[0] = 0b11111111;
                segmentbuffer[1] = 0b01000011;
                segmentbuffer[2] = 0b10000001;segmentbuffer[3] = 0b10010001;
                segmentbuffer[4] = 0b00001101;//segmentbuffer[5] = 0b11111111;
            }else if(option >= 160 && option < 163){//display brightness sub menu
                if(option == 160){
                    //segmentbuffer[0] = 0b11111111;
                    segmentbuffer[1] = 0b00010011;segmentbuffer[2] = 0b11110111;//hi
                    segmentbuffer[3] = 0b00100001;segmentbuffer[4] = 0b00010111;//gh
                    //segmentbuffer[5] = 0b00001111;
                    brightnesspause = 0;//new brightness value
                }else if(option == 161){
                    //segmentbuffer[0] = 0b11111111;
                    segmentbuffer[1] = 0b10011001;segmentbuffer[2] = 0b10110001;//mm
                    segmentbuffer[3] = 0b00001101;segmentbuffer[4] = 0b01000011;//ed
                    //segmentbuffer[5] = 0b11111111;
                    brightnesspause = 13;//new brightness value
                }else if(option == 162){
                    //segmentbuffer[0] = 0b11111111;
                    segmentbuffer[1] = 0b01000011;segmentbuffer[2] = 0b11110111;//di
                    segmentbuffer[3] = 0b10011001;segmentbuffer[4] = 0b10110001;//mm
                    //segmentbuffer[5] = 0b11111111;
                    brightnesspause = 20;//new brightness value
                }
                write_flashmem();
                numadcreads = digitpause - brightnesspause; //read the ADC this many times when displaying each digit
            }//end options check
            if(displaytilted){ //display tilted
                flipItAndReverseIt();
            }//done tilt
        }//end option conditional
        LATB = 0b11111111; //turn off digits 1-4
        
        //second digit
        lightDigit(1); //light digit 2;
        LATB = 0b11111111;//turn off digits 1-4
        if(mode == 6 && gameshort < 500 && segmentbuffer[1] == 255){//added delay so dodge game is same speed (2 blocks vs 3 blocks))
            read_adc(numadcreads);
        }
        //third digit
        lightDigit(2); //light third digit
        LATB = 0b11111111;//turn off digits 1-4
        //fourth digit
        lightDigit(3); //light fourth digit
        LATB = 0b11111111;//turn off digits 1-4
        //fifth digit
        lightDigit(4);
        LATA = 0b11111111; //turn off digit 5-6
        //sixth digit, right most
        lightDigit(5);
                
        if(startup){ //if in startup mode
            if(mode == 2){ //are we in the initial count up mode?
                if(loop < 240){
                    //start at 000000 and increment one digit to 9, right to left optiontimeout will be digit selection
                    //startup will be value to write to digit, 10-19 subtract 10 to stay above 0, 0=not startup mode
                    if( (loop & 0b00000011) == 0 ){ //only do this every 4th loop value
                        if(startup == 19){ //if we're done incrementing this digit
                            startup = 10; //roll over to next digit, start at 0 (startup-10 is used below)
                            optiontimeout--; //move to the next digit
                        }else{
                            startup++; //otherwise just increment this digit, up to 9
                        }
                        displaybuffer[optiontimeout] = startup-10; //set the selected digit value
                    }//end every 4th loop during startup, incrementing digit
                }else if(loop == 255){ //done with startup
                    //this sequence is important, first time coming up read_micthresh should fail bounds check (==0)
                    //causing current values to be written to flash (such as temp offset, and battoffset)
                    read_count(); //read the counter value from flash, populate the decimal array
                    read_mode(); //read stored mode from flash
                    read_tiltoption();//read stored tilt setting from flash
                    read_temp_offset(); //read stored temp offset from flash
                    read_batt_offset(); //read stored batt offset from flash
                    read_speed(); //read stored speed from flash
                    read_loudnesslevel(); //read stored loudness level from flash for sound level mode
                    read_brightness(); //read brightness level from flash
                    readClaps();
                    read_micthresh(); //read stored mic threshold from flash
                    switchmode(mode); //switch to stored mode, normal ops by default, writes flash in case bounds check failed
                    startup = 0; //not in startup mode anymore
                }
            }else if(mode == 4){ //defcon,shoot display mode
                //loop is incremented above, <128=defcon, 128-254=shoot, 255=switchmode
                if(loop == 255){ //switch to initial count up mode
                    mode = 2; //don't use switchmode() because it'll write the mode in flash and get stuck in startup
                }// end loop==255
                if(!PORTAbits.RA0 && !PORTAbits.RA3){ //Button 1 and 2 pressed select + start
                    reset_clear_flash(); //reset everything
                }
            }//end mode == 2/4 (in startup)
        }else{//if not in startup mode
            if(PORTAbits.RA2 && tilt != 0){ //0-74 right side up, min 0
                tilt--;//rightside up
            }else if(!PORTAbits.RA2 && tilt != 150){ //75-150 is upside down, max 150
                tilt++;//upside down
            }
            if(tilt >= 75 && !tiltoption || tilt < 75 && tiltoption){ displaytilted = 1; //if we should tilt display, set displaytilted
            }else{ displaytilted = 0; } //else clear it
            if(option){ //option > 0, in options menu
                if(loop == 255){ //when timer rolls over
                    optiontimeout++; //secondary timer used for actual timeout
                    if(optiontimeout == 9){ //if timeout is reached (timeout will vary based on number of digits lit, oh well)
                        blackout(); //clear screen
                        option = 0; //exit options when timeout reached
                    }
                //if we've hit the timeout on the option screen, jump to option 2
                }else if(option == 1 && loop == 210){ //freeze initial "oPtion" on screen, cycle to option 2 after loop increments
                    option++; //move off of "option", go to first item
                    optiontimeout = 0; //reset the larger timeout value
                }//initial option timeout
            }else{ //not in options (option == 0)
                if(mode == -1 || mode == 1){ //end option > 0, if option == 0 and in countup/down mode
                    if( 0 == (loop & (0xFF >> (speed-1)))){ //used to slow down count up/down, if n LSBs of loop are all 0's
                        updateValue(displaybuffer, mode); // count up/down mode, 0=normal
                    }
                }else if(mode == 0 || mode == 11 || mode == 10){ //option==0 and mode == 0, 11, or 10 (shot, timer, clap modes)
                    if(countshot == 0){//if we're not allowed to count shots (reading hans't fallen below 0.5 * micthresh
                        if((maxenvelope - minenvelope) < micthresh / 2 ){//only count shots if the last reading was half the threshold
                            countshot = 1; //ready to count shots
                        }
                    }//cant use elseif because shotlockout should be a static time value, either lockout can be cleared at any time
                    if(shotlockout){
                        shotlockout--; //if we recently detected a shot, don't do that again for a while
                    //conditions for shot being counted, break threshold, countshot flag, not in shotlockout time
                    }else if((maxenvelope - minenvelope) > micthresh && countshot && !shotlockout){//mic read over threshold, count a shot
                        if(mode == 0){//not in timer mode, count shots (mode == 0) //todo test, was else{}
                            updateValue(countbuffer, 1); //increment shot counterwrite
                            memcpy(displaybuffer, countbuffer, 6); //copy the counter to the display, 6 chars long for each digit
                            write_flashmem(); //store the shot
                            shotlockout = 4; //don't count shots until it equals 0 (decrements each display loop), equates to 22.7 shots/sec
                        }else if(mode == 11){//in timer mode
                            if(gamechar >= 10 && gamechar <= 25){//if we're in the first 16 submodes to count a shot
                                TMR0 = 0;//reset timer
                                for(char i=0;i<4;i++){//store the shot in timerbuf
                                    timerbuf[((gamechar-10)*4)+i] = displaybuffer[1+i];//store current shot
                                }
                                gameshort2++; //count the shot
                                if(gamechar == 25){//last shot, go to review mode, keep value on screen
                                    gamechar = 45;//reviewing last shot
                                    gamechar2 = 20;//step down to 10-19 when setting segmentbuffers
                                }else{//not on last shot, reset counters and lockouts
                                    gamechar++;//move to next shot
                                    displaybuffer[1] = 0;displaybuffer[2] = 0;//reset displayed counter
                                    displaybuffer[3] = 0;displaybuffer[4] = 0; displaybuffer[5] = 0;
                                    shotlockout = 7; //don't count shots until 0 (decrements each display loop), longer since no flash write
                                }
                            }//end gamechar 10-19
                        }else if(mode == 10){
                            if(gamechar == 0){//light off
                                gameshort = 200; //set timeout
                            }else{//light on
                                gameshort = 400; //set longer timeout (only using 3 digits)
                            }
                            gamechar2++; //one more clap detected
                            shotlockout = 20; //can be longer than others
                        }
                        countshot = 0; //don't count shots again until the adc value drops to half the threshold
                    }
                }else if(mode == -2){ //if option == 0 and in audio test mode or sound meter mode
                    //audiomode* are longer term min/max based on "speed" value, update them from min/maxenvelope
                    if(audiomodemax < maxenvelope){ audiomodemax = maxenvelope; } //set new maximum for audio display
                    if(audiomodemin > minenvelope){ audiomodemin = minenvelope; } //set new minimum for audio display
                    maxmindiff = audiomodemax - audiomodemin;
                    if( 0 == (loop & (0xFF >> (speed-1))) ){ //used to slow down display refresh
                        displaybuffer[0] = 0;//set to 0, needed incase setdig is used in this mode
                        displaybuffer[1] = 0;//set to 0, needed incase setdig is used in this mode
                        displaybuffer[2] = 0;
                        if(maxmindiff > 1000){displaybuffer[2] = 1;}//if over 1000, set to 1, faster than calculating, but prob never used
                        
                        displaybuffer[3] = (maxmindiff / 100) % 10;
                        displaybuffer[4] = (maxmindiff / 10) % 10;
                        displaybuffer[5] = maxmindiff % 10;
                        audiomodemax = 0; //reset the min/max for a new set of readings after displaying it
                        audiomodemin = 1023;
                    }// speed conditional to update display in audio mode
                }//end mode conditionals
            }//end option == 0
            if(!PORTAbits.RA0){ //Button 1 pressed (select)
                if(!debounce1){ //and it wasnt before
                    debounce1 = 1; //next time we'll know it is just being held down
                    //conditionals for modes where select button shouldn't send you to options
                    if(mode == 6 && gameshort < 501){ //dodge game, during startup, gameplay, or fail
                        if(gamechar3 < 20){
                            gamechar3 = 0;//reset cheat, has to be start 20x in a row
                        }
                        if(gameshort < 500 && gamechar != 1){ //move guy up if not in startup mode, and not at top already
                            gamechar--; //move displayed character up
                            updateDodgeCharacter(); //update displayed char
                        }
                    }else if(mode == 7 && gamechar2 > 0){//if we need to do something in temp mode in offset
                    }else if(mode == 8 && gamechar2 > 0){//if we need to do something in batt mode in offset
                    }else if(mode == 9 && gameshort){//in sound mode, and loudness sub mode 
                        gameshort = 200; //reset timeout value
                        gamechar2--; //decrement sound meter loudness level if in this sub config menu
                        if(gamechar2 == 255){ gamechar2 = 7; } //roll over
                    }else if(mode == 10 && gameshort2 > 0){ //in clap mode, config num of claps
                        gameshort2 = 300;//reset timeout
                        gamechar3--;
                        if(gamechar3 == -2){ gamechar3 = 13; } //decrement temp clapnum config value if above -1 = '1'
                    }else{ //go to options: could be in dodge game  but not count down / in game
                        option++; //increment option number
                        optiontimeout = 0; //reset the timeout
                        loop = 255; //reset the loop counter
                        if(option == 15){ option = 2; } //roll over
                        else if(option == 26){ //done setting digit submenu
                            option = 0; //exit option menu
                            if(0){mode=12;} //dwai
                            else if(mode == 0){ //if we set the digit while in normal mode
                                memcpy(countbuffer, displaybuffer, 6); //copy the display to the counter
                                write_flashmem(); //write flash
                            }
                        }else if(option == 221){ option = 170;//roll over mic thresh submenu
                        }else if(option == 69){ option = 60;//roll over speed submenu
                        }else if(option == 78){ option = 70;//roll over play submenu
                        }else if(option == 104){ option = 80;//roll over morse sub menu
                        }else if(option == 157){ option = 120;//roll over morse custom sub sub menu
                        }else if(option == 163){ option = 160;//roll over brightness setting
                        }
                    }
                }else{//it is being held down
                    if(!PORTAbits.RA3){ //before going into sleep, make sure both buttons are held down for a bit
                        entersleep++; //increment up to 255
                        if(entersleep == 255){
                            //WDT is always off
                            LATC = 0b00000000; //sink all segments (even though each anode is off)
                            LATA = 0b11111111; //turn off digit 5-6
                            SLEEP();//wake on watchdog, ~2sec
                            entersleep = 0;//reset
                            //if the start button is pressed and select isnt pressed exit sleep, otherwise go back to sleep
                            while(PORTAbits.RA3 || !PORTAbits.RA0){
                                CLRWDT();//reset watchdog timer
                                SLEEP();//sleep
                            }//end sleep while loop
                            option = 0; //get out of the options menu
                            blackout();
                            debounce2 = 100; //after exiting sleep, don't jump right into batt/temp offset menus
                        }//end enter/exit sleep
                    }//end both buttons pressed, increment sleep
                    if(debounce1 != 255){debounce1++;}//increment up to 255
                    if(!option){ //not in option menus
                        if(mode == 7 && gamechar2 > 0){//in temp mode, in offset submode, decrement offset
                            if(debounce1 == 95){//been holding the button
                                tempoffset--;//decrease the offset
                                debounce1 = 85;//so we increment faster up to 95 after initial press
                            }else if(debounce1 == 2){ //just pressed the button down (cant be 1 due to "held down" else{})
                                tempoffset--; //decrease the offset
                            }
                            if(tempoffset == ((signed char)-100)){//two digit limit
                                tempoffset = 99;//roll over
                            }
                            gamechar2 = 255; //set the offset timeout to the max
                        }else if(mode == 8 && gamechar2 > 0){//in batt mode, in offset submode, decrement offset
                            if(debounce1 == 95){//been holding the button
                                battoffset--;//decrease the offset
                                debounce1 = 80;
                            }else if(debounce1 == 2){ //just pressed the button down (cant be 1 due to "held down" else{})
                                battoffset--;//decrease the offset
                            }
                            if(battoffset == ((signed char)-21)){ // -21, roll over to -20
                                battoffset = 20;
                            }
                            gamechar2 = 255; //set the offset timeout to the max
                        }//end mode checks
                    }else if(option >= 170 && option <= 221){//end not in option menus, in micthresh option menu
                        if(debounce1 == 90){//after initial hold down pause
                            option++; //next threshold up
                            debounce1 = 75; //increment faster if we keep holding
                            if(option == 221){//past max
                                option = 170;//roll over
                            }
                        }
                    }//end options conditional
                }//end button being held down
            }else{//button 1 not pressed
                debounce1 = 0;
                entersleep = 0; //reset sleep entering counter
            }//end button1
            if(!PORTAbits.RA3){ //Button 2 pressed (start)
                if(!debounce2){ //and it isnt being held down
                    debounce2 = 1; //note that the button is held down
                    if(option == 0){ //not in options menu
                        if(mode == 0){//if in shot mode, mute on/off
                            if(micthresh > 9000){ //already muted
                                micthresh = micthresh - 10000;//restore mic threshold
                                gameshort2 = 600; //timeout to display this message
                            }else{ //not muted, mute that crap
                                gameshort2 = 400; //timeout to display this message
                                if(micthresh < 1020){//if we're "off" due to threshold setting, don't allow turning it "on"
                                    micthresh = micthresh + 10000;//mute mic threshold
                                }
                            }//end muted/not muted check
                            write_flashmem(); //write flash
                        }else if(mode == 3){//rand mode
                            gamechar = gamechar + 5;//move to next rand sub-mode, scroll left right and refresh all
                            if(gamechar > 10){gamechar = 0;}//roll over, three modes 0/5/10
                        }else if(mode == 4 || mode == 5){ //in react game
                            gamechar++; //move to next step of game
                        }else if(mode == 6){ //in dodge game
                            gamechar++; //move character or advance modes I guess?
                            gamechar3++; //cheat if you press start 20 times in a row
                            if(gameshort < 500){ //character is displayed (active game submode)
                                if(gamechar == 4){//can't go over 3
                                    gamechar = 3;
                                }else{
                                    updateDodgeCharacter(); //move the displayed characters
                                }
                            }else if(gameshort == 501){//score is displayed, restart game on start button press
                                switchmode(6); //restart dodge game
                            }
                        }else if(mode == 1 || mode == -1){//in count up mode
                            switchmode(-(mode));//switch to count down or count up (whatever mode you weren't already in
                        }else if(mode == 11){//timer mode
                            if(gamechar == 0 || gamechar == 1){//press/start sub modes
                                gamechar = 2;//move to blackout period
                                blackout(); //don't light any segments
                            }else if(gamechar == 2){//blackout period
                                gamechar = 10; //skip blackout, jump to starting timer
                            }else if(gamechar == 10){//pressed start without counting any shots, clear current shot time
                                gamechar = 10;//move to count shots mode
                                displaybuffer[1] = 0;displaybuffer[2] = 0;
                                displaybuffer[3] = 0;displaybuffer[4] = 0;displaybuffer[5] = 0;
                                TMR0 = 0;//start timer
                            }else if(gamechar <= 25){//gamechar=10-25, button pressed during shot counting mode, go to review
                                gamechar = 30;//review first shot
                                gamechar2 = 20;//step down to 10-25 when setting segmentbuffers
                            }else if(gamechar >= 30){//button pressed during review mode, move to next shot
                                if(gameshort2 > (gamechar-29)){//if next shot was recorded
                                    gamechar++;//move to next shot
                                    if(gamechar == 46){gamechar = 30;}//roll over
                                }else{//roll over to first shot
                                    gamechar = 30;
                                }//end next shot recorded check
                            }//end review mode
                        }else if(mode == 9){//sound mode, start adjusts loudness level
                            if(gameshort){ gamechar2++; }//increase temp loudnesslevel if we're in this sub-config menu
                            if(gamechar2 == 8){ gamechar2 = 0; }//roll over
                            gameshort = 200; //reset timeout value
                        }else if(mode == 10){
                            if(gameshort2 == 0){//just entering clapnum config
                                gamechar3 = clapnum;
                            }else{//already in clapnum config
                                gamechar3++;
                                if(gamechar3 == 14){ gamechar3 = -1; } //increment clapnum config if not maxed
                            }
                            gameshort2 = 300;//reset timer
                        }//end modes outside of options
                    }else{ //option != 0, in options menu
                        optiontimeout = 0; //reset option timeout
                        loop = 255;
                        if(option == 1){ //hit start on initial options menu?
                            option = 2; //jump past initial option screen to first value
                        }else if(option == 2){ //switch to count mode
                            option = 0; //quit options
                            switchmode(1); //switch to count mode
                        }else if(option == 3){ //switch to timer mode
                            option = 0; //quit options
                            switchmode(11); //switch to timer mode
                        }else if(option == 4){ //switch to normal shot mode
                            option = 0; //quit options
                            switchmode(0); //switch to normal shot counter mode
                        }else if(option == 5 || option == 6){ //switch to rand or hype mode
                            switchmode(option-2);//3 or 4 //switch to rand or hype mode
                            option = 0; //quit options
                        }else if(option == 7){ //switch to audio mode
                            option = 0; //quit options
                            switchmode(-2); //switch to audio mode
                        }else if(option == 8){ //set digit
                            option = 20; //switch to set digit submenu, option 20-25 for dig 1-6
                        }else if(option == 9){ //switch tilt sensor
                            option = 0; //quit options
                            blackout();
                            debounce2 = 100; //exit options over the repeat threshold, and wont enter offset in temp/batt
                            tiltoption = !tiltoption;
                            write_flashmem();//store new tilt setting
                        }else if(option == 10){ //set mic threshold
                            if(micthresh > 9000){//locally muted
                                option = (micthresh - 10000)/20;//remove the 10,000 added during quickmuting
                            }else{
                                option = micthresh/20;//divide by 20 since option is a char
                            }
                            option += 169;//jump to the current threshold
                        }else if(option == 11){ //set the speed
                            option = speed + 59; //go to the current speed value submenu
                            optiontimeout = 240; //reset timeout (normally 0 but 4 digits are skipped when displaying speed)
                        }else if(option == 12){ //bright mode setting
                            option = 160; //jump to brightness sub menu, on the high setting
                        }else if(option == 13){ //play mode selected
                            option = 70; //jump to play submenu
                        }else if(option == 14 || option == 77 || option > 159 && option < 163){ //quit options
                            option = 0; //quit
                            blackout();
                            debounce2 = 100; //exit options over the repeat threshold, and wont enter offset in temp/batt
                        }else if(option >= 20 && option < 26){ //set digit sub menu, start increments value
                            //increment selected digit in set digit mode
                            if(displaybuffer[25-option] == 9) //if we need to roll over this digit
                                displaybuffer[25-option] = 0; //roll over, set it to 0
                            else
                                displaybuffer[25-option] += 1; //otherwise just increment
                        }else if(option >= 170 && option < 221){//micthres submenu, start pressed
                            micthresh = (option-169) * 20; //set the new mic threshold
                            write_flashmem(); //store the new mic thresh
                            blackout();//needed for modes that don't use digits lit by threshold
                            option = 0; //quit options
                           debounce2 = 100; //exit options over the repeat threshold, and wont enter offset in temp/batt
                        }else if(option >= 60 && option < 69){ //start pushed while selecting speed
                            speed = option - 59; //set the new speed
                            write_flashmem(); //store the new speed
                            option = 0; //quit options
                            debounce2 = 100; //exit options over the repeat threshold, and wont enter offset in temp/batt
                        }else if(option >= 70 && option < 76){ //game selected
                            switchmode(option - 65); //start game (react/dodge/temp/batt)
                            option = 0; //quit options
                        }else if(option == 76){//morse code, go to sub menu
                            option = 80;
                        }else if(option >= 80 && option < 103){//morse built in strings
                            gamechar = option-80; //set which string to use, don't need a short but we have it
                            switchmode(13);//start morse code mode
                            option = 0; //quit options
                        }else if(option == 103){//"custom" string chosen in morse code sub menu
                            morsechar = 0; //character position when setting custom string
                            option = 120;//go to custom string sub^3 menu
                        }else if(option >= 120 && option <= 155){//morse code custom sub menu, one char selected (not done))
                            morsecuststr[morsechar] = option - 120; //set current character
                            morsechar++;//move to next character
                            option = 120;//reset to "a" char
                            if(morsechar == 50){ //no more space, treat it as "done"
                                option = 0;//quit options
                                gamechar = 23; //designate custom string chosen
                                morsecuststr[50] = 100; //set end or string value
                                switchmode(13); //start morese code
                            }
                        }else if(option == 156){//morse code custom string sub menu, selected "done"
                            option = 0;//quit options
                            if(morsechar){//if these idiots at least selected one+ character
                                gamechar = 23; //designate custom string chosen
                                morsecuststr[morsechar] = 100;//set a end of string delim
                                switchmode(13);//start morse code
                            }
                        }
                    }//end option != 0
                }else{//it is beingheld down
                    if(debounce2 != 255){debounce2++;}
                    if(!option){ //not in option menus
                        if(mode == 7){ //temp mode, start button adjusts offset (regardless if the button is held)
                            if(gamechar2 > 0){//don't increment unless we've already in the offset menu
                                if(debounce2 == 95){//been holding the button
                                    tempoffset++; //increase the offset
                                    debounce2 = 85; //so we increment faster up to 95 after initial press
                                }else if(debounce2 == 2){ //just pressed the button down (cant be 1 due to else{})
                                    tempoffset++;//increase the offset
                                }
                                if(tempoffset > ((signed char)99)){//two digit limit
                                    tempoffset = -99;//roll over
                                }
                            }//end in offset menu
                            if(debounce2 < 96){//as long as we didn't just enter this mode (set to 100 initially)
                                gamechar2 = 255; //set the offset timeout to the max
                            }
                        }else if(mode == 8){ //batt mode, start button adjusts offset if in offset mode
                            if(gamechar2 > 0){ //don't increment unless we've already in the offset menu
                                if(debounce2 == 95){//been holding the button
                                    battoffset++;//increase the offset
                                    debounce2 = 80;//so we increment faster up to 95 after initial press
                                }else if(debounce2 == 2){ //just pressed the button down (cant be 1 due to else{})
                                    battoffset++;
                                }
                                if(battoffset == ((signed char)21)){ // +21, roll over to -20
                                    battoffset = -20;
                                }
                            }//end in offset menu
                            if(debounce2 < 96){//as long as we didn't just enter this mode (set to 100 initially)
                                gamechar2 = 255; //set the offset timeout to the max
                            }
                        }//end mode checks
                    }//end not in option menus
                }//end button being held down
            }else{ //button 2 not pressed
                debounce2 = 0; //button isnt being held down
                entersleep = 0; //buttons not pressed, reset sleep button press indicator
            }//end button 2
            //set the decimal points based on mic input
            maxmindiff = maxenvelope - minenvelope;
            if((maxmindiff) > weightedavgpeaktopeak){//ADC louder than current display
                //light the DP VU meter, at max value for this DP bucket (so the top DP isn't lit for a short period)
                if(maxmindiff > 420){weightedavgpeaktopeak = 540;
                }else if(maxmindiff > 300){weightedavgpeaktopeak = 420;
                }else if(maxmindiff > 200){weightedavgpeaktopeak = 300;
                }else if(maxmindiff > 120){weightedavgpeaktopeak = 200;
                }else if(maxmindiff > 60){weightedavgpeaktopeak = 120;
                }else if(maxmindiff > 20){weightedavgpeaktopeak = 60;
                }
            }else{//ADC softer than current display, slowly decrement
                weightedavgpeaktopeak = weightedavgpeaktopeak - 4; //how quickly the VU meter DP's roll off
                if(weightedavgpeaktopeak < 0){ weightedavgpeaktopeak = 0; } //keep in bounds
            }
            //not in dodge game (mic for specials) && not in soundlevel && not in audio mode while start button pressed
            if(mode != 7 && mode != 9 && (mode != -2 || PORTAbits.RA3)){
                maxenvelope = 0; //reset the max for enveloping
                minenvelope = 1023; //reset the min for enveloping
            }
            //set the "decimal" value for which DPs should be lit
            if(weightedavgpeaktopeak <= 20){ decimal = 0b11111111; //all 6 DPs off
            }else if(weightedavgpeaktopeak < 60){ decimal = 0b11011111; //DP 1 on 21-149
            }else if(weightedavgpeaktopeak < 120){ decimal = 0b11001111; //DP 1-2 on 150-279
            }else if(weightedavgpeaktopeak < 200){ decimal = 0b11000111; //DP 1-3 on 280-409
            }else if(weightedavgpeaktopeak < 300){ decimal = 0b11000011; //DP 1-4 on 410-539
            }else if(weightedavgpeaktopeak < 420){ decimal = 0b11000001; //DP 1-5 on 540-669
            }else{ decimal = 0b11000000; }//all 6 DPs on 669+

        }//end not in startup mode
        loop++; //used for all kinds of counters and junk
        CLRWDT(); //clear watchdog timer
        LATA = 0b11111111; //turn off digit 5-6
    }//end for(;;) loop
}//end main()
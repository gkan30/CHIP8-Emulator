/*
	||ABOUT||

================================================================

Author : GKan
https://github.com/gkan30
Licensetype : GNU General Public License(GPL) v2
http ://www.gnu.org/licenses/old-licenses/gpl-2.0.html

	||OVERVIEW||
===================================================================
This is a basic CHIP8 Emulator.The code is not super optimized,
it is written for the purpose of being easy to understand and to learn from it.

	||USAGE||
===================================================================

Use the command line to run it,or drag the ROM onto the CHIP8EMULATOR.exe
NOTE::Super Chip8 opcodes are yet to be implemented
*/
#include<iostream>		//You Know Why,If you don't then you are too early for CHIP8
#include"chip8.h"		//Header containing function declaration.Makes the code cleaner in my opinion
#include<Windows.h>		//Used for controlling the console window parameters
#include<chrono>		//Used for keeping track of time, hence the clock speed of the CHIP8

using namespace std;

Chip8 chip8;


void main(int argc, char** argv) {
	
	int cycles = 0;			//Used to keep track of cycles 
	chip8.Initialize();		//Initialize all the variables to their starting value
	chip8.LoadROM(argv[1]);	//Loading to ROM onto the CHIP8 memory
	COORD c;	
	c.X = 64;				//Width of Display window.Better left unchanged
	c.Y = 32;				//Height of Display window.Better left unchanged
	auto tp1 = chrono::system_clock::now();	//Used to keep track of time and hence frequency
	auto tp2 = chrono::system_clock::now();
	HWND console = GetConsoleWindow();			//Get Handle of the display window
	SetConsoleScreenBufferSize(console, c);		//Set the display size
	while (1) {
		SetConsoleTextAttribute(console, 255);	//Set display color to white
		SetConsoleScreenBufferSize(console, c);
		if (cycles <= 60) {						//The CHIP8 has frequency of 60Hz
			chip8.emulateCycle();				//Run 1 Opcode at a time
			chip8.UserInput();					//Detect User input
			if (chip8.drawFlag) {				//Draw to console only when needed instead of everyframe

				chip8.Render();					//Call the actual console render function
				chip8.drawFlag = false;			//Set flag to false until enabled again
			}
			cycles++;							//Inceremnt cycles count
			tp2 = chrono::system_clock::now();	//Current Time
		}
		else {
			auto duration = chrono::duration_cast<chrono::milliseconds>(tp2 - tp1);//Time taken for completing 60Hz
			Sleep(duration.count()*1.6);		//^0 cycles aare completed,so sleep for the rest of the remaining second
			cycles = 0;							//reset cycles to zero
			tp1 = chrono::system_clock::now();	//Update time for the next round
		}
	}

}

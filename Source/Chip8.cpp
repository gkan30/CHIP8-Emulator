#include"chip8.h"	//Just Contains Declarations for the chip8 class
#include<fstream>	//Contains functions used for reading the ROM file into CHIP8 Memory
#include<iostream>	//You Know Why,If you don't then you are too early for CHIP8
#include<time.h>	//Used for getting time.now() in this case
#include<Windows.h>	//Used For drawing to console in this case

using namespace std;

ushort opcode;

uchar memory[4096];				//The Chip8 has 4K bytes of memory
uchar V[16];					//Has 16 registers
ushort I;						//Has I register to store address 
ushort pc;						// Program counter
uchar RenderBuffer[64 * 32+1];	//Chip8 has 64*32 black and white graphics.Used for rendering.Since we use command prompt to render,1 extra byte for/0
uchar DelayTimer;				//The Chip8 has a DelayTimer used for timing events
uchar SoundTimer;				//The Chip8 has a sound timer used for audio timing
ushort stack[16];				//The Stack used for jumping in and out of subroutines
ushort sp;						//The Stack Pointer used for keeping track of top of stack
uchar key[16];					//Chip8 has 16 keys.This array keeps track of key inputs.Keys are in HEX format 1,2,3,4,5,6,7,8,9,A,B,C,D,E,F

uchar chip8_fontset[80] =		//The Chip8 has this fontset for it to show numbers.By specification,the fontset is loaded from address 0-80
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};


//The scary functions below are unrelated to CHIP8 logic,they are for drawing directly to cwindows console

char* screen = new char[64 * 32+1];			//The buffer for drawing directly to console,rather than using graphic functions
HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);	//Creates the windows screen buffer
DWORD dwBytesWritten = 0;


void Chip8::Initialize()
{
	
	pc = 0x200;	//By specifications from the wiki,ROM is loaded from address 512 i.e 0x200 in hex
	opcode = 0;	//Clear Opcode
	I = 0;		
	sp = 0;		//Empty stack
	// Clear display
	for (int i = 0; i < 2048; ++i)
		RenderBuffer[i] = 0;

	// Clear stack
	for (int i = 0; i < 16; ++i)
		stack[i] = 0;
	//Clear Key States
	for (int i = 0; i < 16; ++i)
		key[i] = V[i] = 0;

	// Clear memory
	for (int i = 0; i < 4096; ++i)
		memory[i] = 0;

	// Load font
	for (int i = 0; i < 80; ++i)
		memory[i] = chip8_fontset[i];

	// Reset timers
	DelayTimer = 0;
	SoundTimer = 0;

	// Clear screen once
	drawFlag = true;
	//A random number generator standard function needed by one of CHIP8 opcode
	srand(time(NULL));
}


void Chip8::LoadROM(const char* ROM) // ROM = file path
{
	
		if (ROM == NULL) {		//When no file is given to exe
			cout << "No ROM supplied, Please run via command prompt or drag the ROM onto exe\n";
			Sleep(5000);	//Give the user some time to actually read the screen before closing 
			exit(0);	//Exit the program
		}
		printf("Loading ROM : %s\n", ROM);
		// Open file
		FILE* pFile = fopen(ROM, "rb");

		if (pFile == NULL)	//When file reading fails
		{
			cout << "Cannot Open ROM!.Maybe be corrupt or no permission to open\n";
			Sleep(5000);
			fputs("File error", stderr);
			exit(0);
		}

		SetConsoleActiveScreenBuffer(hConsole);												//Used for displaying in console.print to console is slow,so we output directly to windows buffer

		screen[64 * 32] = '\0';																//Since we send out a string to buffer,last byte in null character no denote the end of string
		WriteConsoleOutputCharacter(hConsole, screen, 64 * 32+1, { 0,0 }, &dwBytesWritten);	//Denote the number of bytes we are sending out to buffer
		// Check file size
		fseek(pFile, 0, SEEK_END);	
		long lSize = ftell(pFile);															//Size of file,number of bytes
		rewind(pFile);
		printf("Filesize: %d\n", (int)lSize);

		// Allocate memory to contain the whole file
		char* buffer = (char*)malloc(sizeof(char) * lSize);
		if (buffer == NULL)
		{
			fputs("Memory error", stderr);

		}

		// Copy the file into the buffer
		size_t result = fread(buffer, 1, lSize, pFile);
		if (result != lSize)
		{
			fputs("Reading error", stderr);

		}

		// Copy buffer to Chip8 memory
		if ((4096 - 512) > lSize)	//Chip8 has 4k bytes of memory only,A ROM exceeding the space limit of Chip8 is not accepted
		{
			for (int i = 0; i < lSize; ++i)
				memory[i + 512] = buffer[i];
		}
		else
			printf("Error: ROM too big for memory");

		// Close file, free buffer
		fclose(pFile);
		free(buffer);

	}
	
void Chip8::emulateCycle()
{
	// Fetch opcode
	opcode = memory[pc] << 8 | memory[pc + 1];

	// Process opcode
	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x00E0: Clears the screen
			for (int i = 0; i < 2048; ++i)
				RenderBuffer[i] = 0x0;
			drawFlag = true;
			pc += 2;
			break;

		case 0x000E: // 0x00EE: Returns from subroutine
			--sp;			// 16 levels of stack, decrease stack pointer to prevent overwrite
			pc = stack[sp];	// Put the stored return address from the stack back into the program counter					
			pc += 2;		// Don't forget to increase the program counter!
			break;

		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
		}
		break;

	case 0x1000: // 0x1NNN: Jumps to address NNN
		pc = opcode & 0x0FFF;
		break;

	case 0x2000: // 0x2NNN: Calls subroutine at NNN.
		stack[sp] = pc;			// Store current address in stack
		++sp;					// Increment stack pointer
		pc = opcode & 0x0FFF;	// Set the program counter to the address at NNN
		break;

	case 0x3000: // 0x3XNN: Skips the next instruction if VX equals NN
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x4000: // 0x4XNN: Skips the next instruction if VX doesn't equal NN
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x5000: // 0x5XY0: Skips the next instruction if VX equals VY.
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0x6000: // 0x6XNN: Sets VX to NN.
		V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
		pc += 2;
		break;

	case 0x7000: // 0x7XNN: Adds NN to VX.
		V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
		pc += 2;
		break;

	case 0x8000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x8XY0: Sets VX to the value of VY
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0001: // 0x8XY1: Sets VX to "VX OR VY"
			V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0002: // 0x8XY2: Sets VX to "VX AND VY"
			V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0003: // 0x8XY3: Sets VX to "VX XOR VY"
			V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0004: // 0x8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't					
			if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8]))
				V[0xF] = 1; //carry
			else
				V[0xF] = 0;
			V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0005: // 0x8XY5: VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't
			if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
				V[0xF] = 0; // there is a borrow
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0006: // 0x8XY6: Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
			V[(opcode & 0x0F00) >> 8] >>= 1;
			pc += 2;
			break;

		case 0x0007: // 0x8XY7: Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't
			if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])	// VY-VX
				V[0xF] = 0; // there is a borrow
			else
				V[0xF] = 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x000E: // 0x8XYE: Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift
			V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
			V[(opcode & 0x0F00) >> 8] <<= 1;
			pc += 2;
			break;

		default:
			printf("Unknown opcode in series [0x8000]: 0x%X\n", opcode);
		}
		break;

	case 0x9000: // 0x9XY0: Skips the next instruction if VX doesn't equal VY
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0xA000: // ANNN: Sets I to the address NNN
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	case 0xB000: // BNNN: Jumps to the address NNN plus V0
		pc = (opcode & 0x0FFF) + V[0];
		break;

	case 0xC000: // CXNN: Sets VX to a random number and NN
		V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
		pc += 2;
		break;

	case 0xD000: // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. 
				 // Each row of 8 pixels is read as bit-coded starting from memory location I; 
				 // I value doesn't change after the execution of this instruction. 
				 // VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
				 // and to 0 if that doesn't happen
	{
		ushort x = V[(opcode & 0x0F00) >> 8];
		ushort y = V[(opcode & 0x00F0) >> 4];
		ushort height = opcode & 0x000F;
		ushort pixel;

		V[0xF] = 0;
		for (int yline = 0; yline < height; yline++)
		{
			pixel = memory[I + yline];
			for (int xline = 0; xline < 8; xline++)		//loop through each bit from sprite stored at the given memory location and store it in the display array
			{
				if ((pixel & (0x80 >> xline)) != 0)
				{
					if (RenderBuffer[(x + xline + ((y + yline) * 64))] == 1)
					{
						V[0xF] = 1;
					}
					RenderBuffer[x + xline + ((y + yline) * 64)] ^= 1;
				}
			}
		}

		drawFlag = true;  //We are using this bool to draw the frame only when required instead of drawing every time
		pc += 2;			//increment the program counter to next opcode postion.Every opcode is 2 bytes long
	}
	break;

	case 0xE000:
		switch (opcode & 0x00FF)
		{
		case 0x009E: // EX9E: Skips the next instruction if the key stored in VX is pressed
			if (key[V[(opcode & 0x0F00) >> 8]] != 0)
				pc += 4;
			else
				pc += 2;
			break;

		case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed
			if (key[V[(opcode & 0x0F00) >> 8]] == 0)
				pc += 4;
			else
				pc += 2;
			break;

		default:
			printf("Unknown opcode [0xE000]: 0x%X\n", opcode);
		}
		break;

	case 0xF000:
		switch (opcode & 0x00FF)
		{
		case 0x0007: // FX07: Sets VX to the value of the delay timer
			V[(opcode & 0x0F00) >> 8] = DelayTimer;
			pc += 2;
			break;

		case 0x000A: // FX0A: A key press is awaited, and then stored in VX		
		{
			bool keyPress = false;

			for (int i = 0; i < 16; ++i)
			{
				if (key[i] != 0)
				{
					V[(opcode & 0x0F00) >> 8] = i;
					keyPress = true;
				}
			}

			// If we didn't received a keypress, skip this cycle and try again.
			if (!keyPress)
				return;

			pc += 2;
		}
		break;

		case 0x0015: // FX15: Sets the delay timer to VX
			DelayTimer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0018: // FX18: Sets the sound timer to VX
			SoundTimer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x001E: // FX1E: Adds VX to I
			if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF)	// VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
				V[0xF] = 1;
			else
				V[0xF] = 0;
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0029: // FX29: Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
			I = V[(opcode & 0x0F00) >> 8] * 0x5;
			pc += 2;
			break;

		case 0x0033: // FX33: Stores the Binary-coded decimal representation of VX at the addresses I, I plus 1, and I plus 2
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
			pc += 2;
			break;

		case 0x0055: // FX55: Stores V0 to VX in memory starting at address I					
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				memory[I + i] = V[i];

			// On the original interpreter, when the operation is done, I = I + X + 1.
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;

		case 0x0065: // FX65: Fills V0 to VX with values from memory starting at address I					
			for (int i = 0; i <= ((opcode & 0x0F00) >> 8); ++i)
				V[i] = memory[I + i];

			// On the original interpreter, when the operation is done, I = I + X + 1.
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			break;
		}
		
		break;

	default:
		printf("Unknown opcode encountered 0x%X\n", opcode);
	}

	// Update timers
	if (DelayTimer > 0)
		--DelayTimer;

	if (SoundTimer > 0)
	{
		if (SoundTimer == 1)
			//Add Sound Output here
		--SoundTimer;
	}
}	//For more information ,refer wikipedia page.It's really good.
void Chip8::UserInput() 
{
/*		Keymapping:
===============================

		The original keypad :
			|1|2|3|C|
			|4|5|6|D|
			|7|8|9|G|
			|A|0|B|F|

		Keyboard mapping :
			|1|2|3|4|
			|q|w|e|r|
			|a|s|d|f|
			|z|x|c|v|						*/

	if (GetAsyncKeyState((ushort)'A') & 0x8000) {	
		key[7] = 1;												//Key state is 1 if pressed
	}
	else {
		key[7] = 0;												//Key state 0 if not pressed
	}
	if (GetAsyncKeyState((ushort)'S') & 0x8000) {
		key[8] = 1;
	}
	else {
		key[8] = 0;
	}
	if (GetAsyncKeyState((ushort)'D') & 0x8000) {
		key[9] = 1;
	}
	else {
		key[9] = 0;
	}
	if (GetAsyncKeyState((ushort)'F') & 0x8000) {
		key[0xE] = 1;
	}
	else {
		key[0xE] = 0;
	}
	if (GetAsyncKeyState((ushort)'3') & 0x8000) {
		key[3] = 1;
	}
	else {
		key[3] = 0;
	}
	if (GetAsyncKeyState((ushort)'Q') & 0x8000) {
		key[4] = 1;
	}
	else {
		key[4] = 0;
	}
	if (GetAsyncKeyState((ushort)'W') & 0x8000) {
		key[5] = 1;
	}
	else {
		key[5] = 0;
	}
	if (GetAsyncKeyState((ushort)'E') & 0x8000) {
		key[6] = 1;
	}
	else {
		key[6] = 0;
	}
	if (GetAsyncKeyState((ushort)'R') & 0x8000) {
		key[0xD] = 1;
	}
	else {
		key[0xD] = 0;
	}
	if (GetAsyncKeyState((ushort)'Z') & 0x8000) {
		key[0xA] = 1;
	}
	else {
		key[0xA] = 0;
	}
	if (GetAsyncKeyState((ushort)'X') & 0x8000) {
		key[0] = 1;
	}
	else {
		key[0] = 0;
	}
	if (GetAsyncKeyState((ushort)'C') & 0x8000) {
		key[0xB] = 1;
	}
	else {
		key[0xB] = 0;
	}
	if (GetAsyncKeyState((ushort)'V') & 0x8000) {
		key[0xF] = 1;
	}
	else {
		key[0xF] = 0;
	}
	if (GetAsyncKeyState((ushort)'1') & 0x8000) {
		key[1] = 1;
	}
	else {
		key[1] = 0;
	}
	if (GetAsyncKeyState((ushort)'2') & 0x8000) {
		key[2] = 1;
	}
	else {
		key[2] = 0;
	}
	if (GetAsyncKeyState((ushort)'4') & 0x8000) {
		key[0xC] = 1;
	}
	else {
		key[0xC] = 0;
	}
}
void Chip8::Render(){
	//Translate from renderBuffer to ScreenBuffer
	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 64; ++x)
		{
			if (RenderBuffer[(y * 64) + x] == 0)
				screen[(y * 64) + x] = ' ';	//Place a space where pixel is off
			else
				screen[(y * 64) + x] = 219; //Place a block where pixel is ON,219 is ASCII code for the SQUARE BLOCK
		}
		
	}
	
	WriteConsoleOutputCharacter(hConsole, screen, 64 * 32, { 0,0 }, &dwBytesWritten);		//Write to windows buffer we created eariler
}
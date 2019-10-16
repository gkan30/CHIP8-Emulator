#pragma once
typedef unsigned short ushort;
typedef unsigned char uchar;


class Chip8 {
public :
	bool drawFlag;
	void Initialize();
	
	void emulateCycle();
	void Render();
	void UserInput();
	void LoadROM(const char* ROM);
};


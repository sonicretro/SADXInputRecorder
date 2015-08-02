// inputrecorder.cpp : Defines the exported functions for the DLL application.
//

#include <vector>
#include <fstream>
#include <SADXModLoader.h>

using namespace std;

const unsigned int magic = 'OMED';

void Init(const char *path, const HelperFunctions &helperFunctions)
{
	CreateDirectory(L"savedata\\input", nullptr);
}

unsigned int currentframe = 0;
unsigned int savedframe = 0;
vector<ControllerData> recording;
bool levelstarted = false;
bool levelcomplete = false;
bool isrecording;

void ResetGhost()
{
	recording.clear();
	isrecording = false;
	currentframe = 0;
	savedframe = 0;
}

void LoadGhost()
{
	levelcomplete = false;
	switch (CurrentLevel)
	{
	case LevelIDs_HedgehogHammer:
	case LevelIDs_StationSquare:
	case LevelIDs_EggCarrierOutside:
	case LevelIDs_EggCarrierInside:
	case LevelIDs_MysticRuins:
	case LevelIDs_Past:
	case LevelIDs_SkyChase1:
	case LevelIDs_SkyChase2:
	case LevelIDs_SSGarden:
	case LevelIDs_ECGarden:
	case LevelIDs_MRGarden:
	case LevelIDs_ChaoRace:
		return;
	}
	levelstarted = true;
	if (recording.size() == 0)
	{
		isrecording = true;
		char path[MAX_PATH];
		_snprintf_s(path, MAX_PATH, "savedata\\input\\%02d%s.gst", CurrentLevel, CharIDStrings[CurrentCharacter]);
		ifstream str(path, ifstream::binary);
		if (str.is_open())
		{
			unsigned int m;
			str.read((char *)&m, sizeof(m));
			if (m == magic)
			{
				unsigned int size;
				str.read((char *)&size, sizeof(size));
				recording.resize(size);
				str.read((char *)recording.data(), sizeof(ControllerData) * size);
				isrecording = false;
			}
			str.close();
		}
		currentframe = 0;
	}
}

FunctionPointer(void, sub_40EFA0, (unsigned __int8), 0x40EFA0);
void __cdecl SaveGhost(unsigned __int8 a1)
{
	sub_40EFA0(a1);
	char path[MAX_PATH];
	_snprintf_s(path, MAX_PATH, "savedata\\input\\%02d%s.gst", CurrentLevel, CharIDStrings[CurrentCharacter]);
	ofstream str(path, ifstream::binary);
	if (str.is_open())
	{
		str.write((char *)&magic, sizeof(magic));
		unsigned int size = recording.size();
		str.write((char *)&size, sizeof(size));
		str.write((char *)recording.data(), sizeof(ControllerData) * size);
		str.close();
	}
	levelcomplete = true;
	levelstarted = false;
}

void Checkpoint()
{
	savedframe = currentframe;
}

void Restart()
{
	if (isrecording)
		recording.resize(savedframe);
	currentframe = savedframe;
}

DataPointer(ControllerData, Controller, 0x3B0E7F0);
void UpdateController()
{
	if (!levelstarted || levelcomplete || !GetCharacterObject(0) || IsGamePaused())
		return;
	if (isrecording)
		recording.push_back(Controller);
	else
	{
		bool pressedA = (Controller.PressedButtons & Buttons_A) == Buttons_A;
		Controller = recording[currentframe++];
		if (pressedA)
		{
			isrecording = true;
			recording.resize(currentframe);
		}
	}
}

const int sub_40F170 = 0x40F170;
__declspec(naked) void ControllerHax()
{
	__asm
	{
		call UpdateController
		xor ebx, ebx
		jmp sub_40F170
	}
}

PointerInfo jumps[] = {
	ptrdecl(0x41597B, LoadGhost),
	ptrdecl(0x44EEE1, Checkpoint),
	ptrdecl(0x44EE67, Restart),
	ptrdecl(0x44ED60, ResetGhost)
};

PointerInfo calls[] = {
	ptrdecl(0x415545, SaveGhost),
	ptrdecl(0x40FEE6, ControllerHax)
};

extern "C" __declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, Init, NULL, 0, arrayptrandlength(jumps), arrayptrandlength(calls) };
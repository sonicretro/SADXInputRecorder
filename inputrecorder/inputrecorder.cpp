// inputrecorder.cpp : Defines the exported functions for the DLL application.
//

#include <vector>
#include <fstream>
#include <SADXModLoader.h>

using namespace std;

struct AnalogThing
{
	int		angle;
	float	magnitude;
};

DataArray(AnalogThing, NormalizedAnalogs, 0x03B0E7A0, 8);
FunctionPointer(void, DisableController, (unsigned __int8), 0x40EFA0);

const unsigned int magic = 'OMED';
ControllerData* Controller = nullptr;
unsigned int currentframe = 0;
unsigned int savedframe = 0;
bool levelstarted = false;
bool levelcomplete = false;
bool isrecording;

vector<ControllerData> recording;
vector<AnalogThing> analogs;

extern "C"
{
	__declspec(dllexport) void Init(const char *path, const HelperFunctions &helperFunctions)
	{
		CreateDirectory(L"savedata\\input", nullptr);
		Controller = &ControllersRaw[0];
	}

	__declspec(dllexport) void OnInput()
	{
		if (!levelstarted || levelcomplete || !GetCharacterObject(0) || IsGamePaused())
			return;

		if (isrecording)
			recording.push_back(*Controller);
		else
		{
			bool pressedA = (Controller->PressedButtons & Buttons_A) == Buttons_A;
			*Controller = recording[currentframe++];
			if (pressedA)
			{
				isrecording = true;
				recording.resize(currentframe);
				analogs.resize(currentframe);
			}
		}
	}
}

void OnControl_c()
{
	if (!levelstarted || levelcomplete || !GetCharacterObject(0) || IsGamePaused())
		return;

	if (isrecording)
	{
		analogs.push_back(NormalizedAnalogs[0]);
	}
	else
	{
		// CurrentFrame is incremented in OnInput which happens before OnControl.
		NormalizedAnalogs[0] = analogs[currentframe - 1];
	}
}

void __declspec(naked) OnControl_asm()
{
	__asm
	{
		call OnControl_c
		ret
	}
}


#pragma region File I/O
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
		ifstream file(path, ifstream::binary);

		if (file.is_open())
		{
			unsigned int m;
			file.read((char *)&m, sizeof(m));
			if (m == magic)
			{
				unsigned int size;
				file.read((char *)&size, sizeof(size));
				recording.resize(size);
				file.read((char *)recording.data(), sizeof(ControllerData) * size);

				file.read((char*)&size, sizeof(size));
				analogs.resize(size);
				file.read((char*)analogs.data(), sizeof(AnalogThing) * size);

				isrecording = false;
			}
			file.close();
		}

		currentframe = 0;
	}
}

void __cdecl SaveGhost(unsigned __int8 a1)
{
	DisableController(a1);
	char path[MAX_PATH];
	_snprintf_s(path, MAX_PATH, "savedata\\input\\%02d%s.gst", CurrentLevel, CharIDStrings[CurrentCharacter]);
	ofstream file(path, ifstream::binary);

	if (file.is_open())
	{
		// Writes DEMO header
		file.write((char *)&magic, sizeof(magic));

		// Writes controller data
		unsigned int size = recording.size();
		file.write((char *)&size, sizeof(size));
		file.write((char *)recording.data(), sizeof(ControllerData) * size);

		// Writes analog data
		size = analogs.size();
		file.write((char*)&size, sizeof(size));
		file.write((char*)analogs.data(), sizeof(AnalogThing) * size);

		file.close();
	}

	levelcomplete = true;
	levelstarted = false;
}
#pragma endregion

void ResetGhost()
{
	recording.clear();
	isrecording = false;
	currentframe = 0;
	savedframe = 0;
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

PointerInfo jumps[] = {
	ptrdecl(0x41597B, LoadGhost),
	ptrdecl(0x44EEE1, Checkpoint),
	ptrdecl(0x44EE67, Restart),
	ptrdecl(0x44ED60, ResetGhost),
	ptrdecl(0x40FF00, OnControl_asm)
};

PointerInfo calls[] = {
	ptrdecl(0x415545, SaveGhost),
};

extern "C" __declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer, Init, NULL, 0, arrayptrandlength(jumps), arrayptrandlength(calls) };
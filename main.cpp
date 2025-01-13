#include <windows.h>
#include <string>
#include <fstream>
#include <filesystem>
#include <format>
#include <d3d9.h>
#include <d3dx9.h>
#include "toml++/toml.hpp"
#include "nya_commonhooklib.h"

#include "fouc.h"
#include "fo2versioncheck.h"

void WriteLog(const std::string& str) {
	static auto file = std::ofstream("FlatOutUCSkinSwapper_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

PDIRECT3DTEXTURE9 LoadTextureWithDDSCheck(const char* filename) {
	std::ifstream fin(filename, std::ios::in | std::ios::binary );
	if (!fin.is_open()) return nullptr;

	fin.seekg(0, std::ios::end);

	size_t size = fin.tellg();
	if (size <= 0x4C) return nullptr;

	auto data = new char[size];
	fin.seekg(0, std::ios::beg);
	fin.read(data, size);

	if (data[0] != 'D' || data[1] != 'D' || data[2] != 'S') {
		delete[] data;
		WriteLog("Failed to load " + (std::string)filename);
		return nullptr;
	}

	if (data[0x4C] == 0x18) {
		data[0x4C] = 0x20;
		WriteLog("Loading " + (std::string)filename + " with DDS pixelformat hack");
	}

	PDIRECT3DTEXTURE9 texture;
	auto hr = D3DXCreateTextureFromFileInMemory(pDeviceD3d->pD3DDevice, data, size, &texture);
	delete[] data;
	if (hr != S_OK) {
		WriteLog("Failed to load " + (std::string)filename);
		return nullptr;
	}
	return texture;
}

// Simple helper function to load an image into a DX9 texture with common settings
PDIRECT3DTEXTURE9 LoadTexture(const char* filename) {
	if (!std::filesystem::exists(filename)) return nullptr;

	// Load texture from disk
	PDIRECT3DTEXTURE9 texture;
	auto hr = D3DXCreateTextureFromFileA(pDeviceD3d->pD3DDevice, filename, &texture);
	if (hr != S_OK) {
		return LoadTextureWithDDSCheck(filename);
	}
	return texture;
}

bool bReplaceAllCarsTextures = false;
int nRefreshKey = VK_INSERT;

IDirect3DTexture9* TryLoadCustomTexture(std::string path) {
	// load tga first, game initially has tga in the path
	if (auto tex = LoadTexture(path.c_str())) return tex;

	// then load dds if tga doesn't exist
	for (int i = 0; i < 3; i++) path.pop_back();
	path += "dds";
	if (auto tex = LoadTexture(path.c_str())) return tex;

	// load png as a last resort
	for (int i = 0; i < 3; i++) path.pop_back();
	path += "png";
	if (auto tex = LoadTexture(path.c_str())) return tex;
	return nullptr;
}

void ReplaceTextureWithCustom(DevTexture* pTexture, const char* path) {
	if (auto texture = TryLoadCustomTexture(path)) {
		if (pTexture->pD3DTexture) pTexture->pD3DTexture->Release();
		pTexture->pD3DTexture = texture;
		WriteLog("Replaced texture " + (std::string)path + " with loose files");
	}
}

void ReplaceTextureWithCustom(DevTexture* pTexture) {
	return ReplaceTextureWithCustom(pTexture, pTexture->sPath.Get());
}

void SetCarTexturesToCustom(Car* car) {
	if (!car) return;

	for (int i = 0; i < car->aTextureNodes.GetSize(); i++) {
		auto node = car->aTextureNodes[i];
		if (!node) continue;
		if (!node->sPath.Get()) continue;
		if (node->sPath.length <= 3) continue;
		ReplaceTextureWithCustom(node);
	}

	ReplaceTextureWithCustom(car->pSkin);
	ReplaceTextureWithCustom(car->pSkinDamaged);
	ReplaceTextureWithCustom(car->pSkinSpecular);
	ReplaceTextureWithCustom(car->pSkinBurned);
	ReplaceTextureWithCustom(car->pLightsDamaged);
	ReplaceTextureWithCustom(car->pLightsGlow);
	ReplaceTextureWithCustom(car->pLightsGlowLit);
	ReplaceTextureWithCustom(car->pLightsDamagedGlow);
}

bool bKeyPressed;
bool bKeyPressedLast;
bool IsKeyJustPressed(int key) {
	if (key <= 0) return false;
	if (key >= 255) return false;

	bKeyPressedLast = bKeyPressed;
	bKeyPressed = (GetAsyncKeyState(key) & 0x8000) != 0;
	return bKeyPressed && !bKeyPressedLast;
}

auto RenderRace = (void(__stdcall*)(void*, int))0x48AAF0;
void __stdcall RenderRaceHooked(void* a1, int a2) {
	static Player* pLocalPlayer = nullptr;
	static Car* pLocalPlayerCar = nullptr;
	static bool bFirstFrameToReplaceTextures = false;

	auto localPlayer = GetPlayer(0);
	auto localPlayerCar = GetPlayer(0)->pCar;
	if (localPlayer != pLocalPlayer || localPlayerCar != pLocalPlayerCar) {
		bFirstFrameToReplaceTextures = true;
	}
	pLocalPlayer = localPlayer;
	pLocalPlayerCar = localPlayerCar;

	if (pLocalPlayer && (bFirstFrameToReplaceTextures || IsKeyJustPressed(nRefreshKey))) {
		if (bReplaceAllCarsTextures) {
			for (int i = 0; i < pPlayerHost->GetNumPlayers(); i++) {
				SetCarTexturesToCustom(GetPlayer(i)->pCar);
			}
		}
		else SetCarTexturesToCustom(pLocalPlayer->pCar);
		bFirstFrameToReplaceTextures = false;
	}
	RenderRace(a1, a2);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			DoFlatOutVersionCheck(FO2Version::FOUC_GFWL);

			auto config = toml::parse_file("FlatOutUCSkinSwapper_gcp.toml");
			bReplaceAllCarsTextures = config["main"]["replace_all_cars"].value_or(false);
			nRefreshKey = config["main"]["reload_key"].value_or(VK_INSERT);

			RenderRace = (void(__stdcall*)(void*, int))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x46BA7F, &RenderRaceHooked);
		} break;
		default:
			break;
	}
	return TRUE;
}
#include "stdafx.h"
#include "Hooks.h"
#include "Offset.h"
#include "Tools/Menu/Menu.h"
#include "Tools/Hacks/Misc.h"
#include "Tools/Utils/Playerlist.h"
#include "Tools/Hacks/Esp.h"
#include "Tools/Hacks/Antiaim.h"
#include "Tools/Hacks/Aimbot.h"
#include "Tools/Hacks/Legit.h"
#include "Tools/Hacks/CreateMoveETC.h"
#include "Tools/Menu/SettingsManager.h"
#include "Tools/Hacks/UI/UI.h"
#include "Tools/Hacks/KnifeBot.h"
#include "imGui\imgui.h"
#include "imGui\imgui_internal.h"
#include "imGui\dx9\imgui_dx9.h"
#include "Resolver.h"
#include "rresolver.h"
//#include "aresolver.h"
#include "Header.h"
#include "Chams.h"
#include "Nospread.h"
#include "aresolver.h"
Resolver2* Yaw;
Resolver3* Yaw2;
//ayyResolver* AResolver;
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment(lib, "Winmm.lib")

#define DEV


#define PI_2 PI/2
#define PI_4 PI/4
CHackManager Hacks;

float FakeAngle;

VTHookManager VMTPanel;
VTHookManager VMTClient;
VTHookManager VMTEngine;
VTHookManager VMTModelRender;
VTHookManager VMTClientMode;
VTHookManager VMTRenderView;
VTHookManager VMTGameEvent;
VTHookManager VMTSurface;
VTHookManager VMTFindMDL;
VTHookManager VMTDirectx;
VTHookManager VMTSendMove;

class Hitbox;

std::vector< int > AutoStraferConstants = {};
std::vector< const char* > smoke_materials =
{
	"particle/beam_smoke_01",
	"particle/particle_smokegrenade",
	"particle/particle_smokegrenade1",
	"particle/particle_smokegrenade2",
	"particle/particle_smokegrenade3",
	"particle/particle_smokegrenade_sc",
	"particle/smoke1/smoke1",
	"particle/smoke1/smoke1_ash",
	"particle/smoke1/smoke1_nearcull",
	"particle/smoke1/smoke1_nearcull2",
	"particle/smoke1/smoke1_snow",
	"particle/smokesprites_0001",
	"particle/smokestack",
	"particle/vistasmokev1/vistasmokev1",
	"particle/vistasmokev1/vistasmokev1_emods",
	"particle/vistasmokev1/vistasmokev1_emods_impactdust",
	"particle/vistasmokev1/vistasmokev1_fire",
	"particle/vistasmokev1/vistasmokev1_nearcull",
	"particle/vistasmokev1/vistasmokev1_nearcull_fog",
	"particle/vistasmokev1/vistasmokev1_nearcull_nodepth",
	"particle/vistasmokev1/vistasmokev1_smokegrenade",
	"particle/vistasmokev1/vistasmokev4_emods_nocull",
	"particle/vistasmokev1/vistasmokev4_nearcull",
	"particle/vistasmokev1/vistasmokev4_nocull"
};

typedef void(*Fucntion)(IGameEvent* event);


class trace_info
{
public:
	trace_info(Vector starts, Vector positions, float times, int userids)
	{
		this->start = starts;
		this->position = positions;
		this->time = times;
		this->userid = userids;
	}

	Vector position;
	Vector start;
	float time;
	int userid;
};
std::vector<trace_info> trace_logs;

struct SoundLog
{
	SoundLog(const Vector* SoundPos, int EntIndex, float SoundTime)
	{
		this->m_vSoundPosition = *SoundPos;
		this->m_iEntIndex = EntIndex;
		this->m_flSoundTime = SoundTime;
	}

	Vector m_vSoundPosition;
	int m_iEntIndex;
	float m_flSoundTime;
};

vector<SoundLog> SoundLogs;

/*void FnImpact(IGameEvent* event)
{
	auto shooter = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetPlayerForUserID(event->GetInt("userid")));

	Vector position(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));
	if (shooter)
		BulletLogs.push_back(BulletImpactLog(event->GetInt("userid"), shooter->GetEyePosition(), position, Interfaces.pGlobalVars->curtime));

}*/

 //InjectProcessApp = ("csgo.exe"); GetPlayerForUserID(event->GetInt("client_panorama.dll"))].shotCount++; [Процесс инжекта нетрогай]

void Hurt(IGameEvent* event)
{
	Resolver::FireGameEvent(event);

	auto attacker = event->GetInt("attacker");

	if (Interfaces.pEngine->GetPlayerForUserID(attacker) == Interfaces.pEngine->GetLocalPlayer())
	{
		g_PlayerSettings[Interfaces.pEngine->GetPlayerForUserID(event->GetInt("userid"))].shotCount++;

		Global::playerhurtcalled = true;

		if (Settings.Visuals.Sound == 1)
			Interfaces.pEngine->ClientCmd_Unrestricted("play buttons\\arena_switch_press_02.wav", '0');
	}
}


void WeaponFire(IGameEvent* event)
{
	if (Interfaces.pEngine->GetPlayerForUserID(event->GetInt("userid")) == Interfaces.pEngine->GetLocalPlayer())
	{
		Global::weaponfirecalled = true;

		
	}
}

void Impact(IGameEvent* event)
{
	auto* index = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetPlayerForUserID(event->GetInt("userid")));

	//return if the userid is not valid or we werent the entity who was firing
	//if (G::LocalPlayer)
	//{
	//get the bullet impact's position
	Vector position(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));

	//	if (VisualElements.Visual_Player_EnemyOnly->Checked && index->GetTeamNum() == G::LocalPlayer->GetTeamNum())
	//		return;

	//	Msg("pos = %f, %f, %f", event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));
	if (index)
		trace_logs.push_back(trace_info(index->GetEyePosition(), position, Interfaces.pGlobalVars->curtime, event->GetInt("userid")));
	//	}
}

class CGameEventListener : public IGameEventListener2
{
private:
	std::string eventName;
	Fucntion Call;
	bool server;
public:
	CGameEventListener() : eventName(""), Call(nullptr), server(false)
	{
	}

	CGameEventListener(const std::string& gameEventName, const Fucntion Func, bool Server) : eventName(gameEventName), Call(Func), server(Server)
	{
		this->RegListener();
	}

	virtual void FireGameEvent(IGameEvent* event)
	{
		this->Call(event);
	}

	virtual int IndicateEventHandling(void)
	{
		return 0x2A;
	}

	int GetEventDebugID() override
	{
		return 0x2A;
	}

	void RegListener(void)
	{
		static bool first = true;
		if (!first)
			return;
		if (Interfaces.g_GameEventMgr->AddListener(this, this->eventName.c_str(), server))
			Hacks.listeners.push_back(this);
	}
};



// Hitmarker

#define REG_EVENT_LISTENER(p, e, n, s) p = new CGameEventListener(n, e, s)

CGameEventListener* ResolverPlayerHurt;
CGameEventListener* ResolverWeaponFire;
CGameEventListener* BTImpact;

void Init()
{
	for (int i = 0; i < 255; i++)
		g_PlayerSettings[i] = PlayerSettings();

	//AResolver->Init();

	REG_EVENT_LISTENER(ResolverPlayerHurt, &Hurt, "player_hurt", false);
	REG_EVENT_LISTENER(ResolverWeaponFire, &WeaponFire, "weapon_fire", false);
	REG_EVENT_LISTENER(BTImpact, &Impact, "bullet_impact", false);
}

void DrawBeam(Vector src, Vector end, CColor color)
{
	int r, g, b, a;
	color.GetColor(r, g, b, a);
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_pszModelName = "sprites/physbeam.vmt";
	beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 1.5f;
	beamInfo.m_flWidth = 1.5f;
	beamInfo.m_flEndWidth = 1.0f;
	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 2.0f;
	beamInfo.m_flBrightness = a;
	beamInfo.m_flSpeed = 0.2f;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = r;
	beamInfo.m_flGreen = g;
	beamInfo.m_flBlue = b;
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = 0;

	beamInfo.m_vecStart = src;
	beamInfo.m_vecEnd = end;

	Beam_t* myBeam = Interfaces.g_pViewRenderBeams->CreateBeamPoints(beamInfo);

	if (myBeam)
		Interfaces.g_pViewRenderBeams->DrawBeam(myBeam);
}

void PositionAdjustment()
{

		//static ConVar* cvar_cl_interp = Interfaces.g_ICVars->FindVar("cl_interp");
		//static ConVar* cvar_cl_updaterate = Interfaces.g_ICVars->FindVar("cl_updaterate");
		//static ConVar* cvar_sv_maxupdaterate = Interfaces.g_ICVars->FindVar("sv_maxupdaterate");
		//static ConVar* cvar_sv_minupdaterate = Interfaces.g_ICVars->FindVar("sv_minupdaterate");
		//static ConVar* cvar_cl_interp_ratio = Interfaces.g_ICVars->FindVar("cl_interp_ratio");


		//float cl_interp = cvar_cl_interp->GetFloat();
		//int cl_updaterate = cvar_cl_updaterate->GetInt();
		//int sv_maxupdaterate = cvar_sv_maxupdaterate->GetInt();
		//int sv_minupdaterate = cvar_sv_minupdaterate->GetInt();
		//int cl_interp_ratio = cvar_cl_interp_ratio->GetInt();

		//if (sv_maxupdaterate <= cl_updaterate)
		//	cl_updaterate = sv_maxupdaterate;

		//if (sv_minupdaterate > cl_updaterate)
		//	cl_updaterate = sv_minupdaterate;

		//float new_interp = (float)cl_interp_ratio / (float)cl_updaterate;

		//if (new_interp > cl_interp)
		//	cl_interp = new_interp;

		//float flSimTime = Hacks.LocalPlayer->GetSimulationTime();
		//float flOldAnimTime = Hacks.LocalPlayer->GetAnimTime();

		//int iTargetTickDiff = (int)(0.5f + (flSimTime - flOldAnimTime) / Interfaces.pGlobalVars->interval_per_tick);

		//int result = (int)floorf(Misc::TIME_TO_TICKS(cl_interp)) + (int)floorf(Misc::TIME_TO_TICKS(Hacks.LocalPlayer->GetSimulationTime()));

		//if ((result - Hacks.CurrentCmd->tick_count) >= -50)
		//{
		//	Hacks.CurrentCmd->tick_count = result;
		//}
	
}


void DoNightMode()
{
	static bool bPerformed = false, bLastSetting;

	Hacks.LocalPlayer = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());

	ConVar* r_drawspecificstaticprop = Interfaces.g_ICVars->FindVar("r_drawspecificstaticprop");
	SpoofedConvar* r_drawspecificstaticprop_s = new SpoofedConvar(r_drawspecificstaticprop);
	r_drawspecificstaticprop_s->SetInt(0);

	ConVar* fog_override = Interfaces.g_ICVars->FindVar("fog_override");
	SpoofedConvar* fog_override_spoofed = new SpoofedConvar(fog_override);
	fog_override_spoofed->SetInt(1);

	ConVar* fog_enable = Interfaces.g_ICVars->FindVar("fog_enable");
	SpoofedConvar* fog_enable_spoofed = new SpoofedConvar(fog_enable);
	fog_enable_spoofed->SetInt(0);

	static ConVar* sv_skyname = Interfaces.g_ICVars->FindVar("sv_skyname");
	*(float*)((DWORD)&sv_skyname->fnChangeCallback + 0xC) = NULL;
	sv_skyname->nFlags &= ~FCVAR_CHEAT; // something something dont force convars

	if (!Hacks.LocalPlayer || !Interfaces.pEngine->IsConnected() || !Interfaces.pEngine->IsInGame())
		return;

	if (!bPerformed)
	{
		for (auto i = Interfaces.pMaterialSystem->FirstMaterial(); i != Interfaces.pMaterialSystem->InvalidMaterial(); i = Interfaces.pMaterialSystem->NextMaterial(i))
		{
			IMaterial* pMaterial = Interfaces.pMaterialSystem->GetMaterial(i);

			if (!pMaterial || pMaterial->IsErrorMaterial())
				continue;

			if (bLastSetting)
			{
				if (strstr(pMaterial->GetTextureGroupName(), "Model")) {
					pMaterial->ColorModulate(0.60, 0.60, 0.60);
				}

				if (strstr(pMaterial->GetTextureGroupName(), "World"))
				{
					//pMaterial->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false);
					sv_skyname->SetValue("sky_csgo_night02");
					pMaterial->ColorModulate(0.1, 0.1, 0.1);
				}

				if (strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
				{
					pMaterial->ColorModulate(0.3, 0.3, 0.3);
				}
			}
			else
			{
				if (strstr(pMaterial->GetTextureGroupName(), "Model")) {
					pMaterial->ColorModulate(1, 1, 1);
				}
				if ((strstr(pMaterial->GetTextureGroupName(), "World")) || strstr(pMaterial->GetTextureGroupName(), "StaticProp"))
				{
					sv_skyname->SetValue("vertigoblue_hdr"); // braucht fix fьr default value vonner skybox
															 //pMaterial->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, false);
					pMaterial->ColorModulate(1, 1, 1);
				}
			}

		}
		bPerformed = true;
	}
	if (bLastSetting != Settings.Visuals.Nightmode)
	{
		bLastSetting = Settings.Visuals.Nightmode;
		bPerformed = false;
	}
}

void AutoResolver(Vector* & Angle, PlayerList::CPlayer* Player)
{
	static int iLastUpdatedTick = 0;

	//	Player->reset.y = Angle->y;

	static Vector orginalview = Vector(0, 0, 0);
	/*if(orginalview.x != Angle->x)
	orginalview.x = Angle->x;*/
	if (Angle->y != Player->resolved)
	{
		//orginalview.y = Angle->y;

		float flResolve = 0.f;
		float flLowerBodyYaw = Player->entity->pelvisangs();

		int difference = orginalview.y - flLowerBodyYaw;

		iLastUpdatedTick++;

		/*if (flLowerBodyYaw != Player->flLastPelvisAng)
		{
		if (Player->entity->GetVecVelocity().Length2D() == 0)
		{
		int temp = static_cast<int>(floor(Player->flEyeAng - Player->flLastPelvisAng));
		while (temp < 0)
		temp += 360;
		while (temp > 360)
		temp -= 360;
		Player->Backtrack[temp] = flLowerBodyYaw - Player->flEyeAng;
		}

		iLastUpdatedTick = 0;
		Player->flLastPelvisAng = flLowerBodyYaw;
		Player->flEyeAng = orginalview.y;
		}*/

		if (Player->entity->GetVecVelocity().Length2D() >= 1)
		{
			flResolve = flLowerBodyYaw;
		}
		/*	else
		{
		int temp = static_cast<int>(floor(orginalview.y - flLowerBodyYaw));
		while (temp < 0)
		temp += 360;
		while (temp > 360)
		temp -= 360;
		flResolve = Player->Backtrack[temp] + orginalview.y;
		}*/
		Angle->y = flResolve;
		Player->resolved = Angle->y;
	}
}

void StartPrediction()
{
	static bool done = false;


	class iXXqZdjcSHnQwPNiLHHHajQYhDkidkmNGnNUnLAFXsOibGVFxBzXlTXYhYjySaYIuCjgbTVZhbKIxHXDHHNENoCNqONoCbNWuf
	{
		void BwPEEFzPjjpDBbIRknuUeZxQmfHzfoNnpSxgSdjZoBqEallmUjCIZlLBZYpyYZJUxclJFCZhewGFJTQaUtFVOeneg()
		{
			float wUdpRkTbYYeYlkotHwqolVicMPeICGtwXjYxKVUHhpMmGxpzjIxGthWBMa = 1.12435E-37F;
		}

		void kAtGJablTKLBLhWiVxpAGzUiNcPmyKJqTBWQtHXwnmpXMeIaZqQFnCB()
		{
			float tYKaSFAxFcPXQSabfnNWUbWKEmFLHYqwhdWYGpsyxkVgUOdhYdgJmyLqcZxGZOQGMtQNWcwPVoAKfPQ = 2.442928E-25F;
		}

		void NDSJhKsFVHTNzJwpuBYpkWIqNYXalmjIxCbmOdkfusgPViRRoPuoRSmQUCVjNssIxlpWHhDq()
		{
			long WTPSwEgfMxTUQEokslhatcZztKSUjgmdZZWtbcfqdHGWPDwNIxQpinzTUqAMLmVFiPURHVjpgQXMxcKRnflOqXULBdAL = 21002035753981798;
		}

		void PjpxIPFKwxiOequjcGhftiupXpcNKMGEsSIYTotMtQsjsSsgYFAsFpBHPaAsMRZKQFxlKwqOlxNEQjYeYVPflnZbsaiSQb()
		{
		}

		void yRqsVAXGzTSaiEkgVVbqIuExMwuTaLyZlcJdszdMheoscTDolqbZJkddnVGAfeGOxSPfUgOxSojDekuSTeHJJ()
		{
			float aZGQyTEGdACWjdxufEFhubCZcBOHayWeYCPgoqpEnJfdDokOyLCfdoXYyHNXfXGHDy = 2.441375E+11F;
			long fOmbULVyxOoZipVRaJFQJJFtzpdACfsEAgFOScwlcZjZIfOoLLxnczHyiTDEHxWqw = 33515175692976358;
		}
	};
}

void OpenMenu()
{
	static bool is_down = false;
	static bool is_clicked = false;

	if (GetAsyncKeyState(VK_INSERT))
	{
		is_clicked = false;
		is_down = true;
	}
	else if (!GetAsyncKeyState(VK_INSERT) && is_down)
	{
		is_clicked = true;
		is_down = false;
	}
	else
	{
		is_clicked = false;
		is_down = false;
	}

	if (is_clicked)
	{
		Settings.Global.menu_open = !Settings.Global.menu_open;
		std::string msg = "cl_mouseenable " + std::to_string(!Settings.Global.menu_open);
		Interfaces.pEngine->ClientCmd_Unrestricted(msg.c_str(), 0);
	}
}
long __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
{
	D3DCOLOR rectColor = D3DCOLOR_XRGB(255, 0, 0);
	D3DRECT BarRect = { 1, 1, 1, 1 };

	pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, rectColor, 0, 0);

	if (!Settings.Global.d3d_init)
		GUI_Init(pDevice);

	VMTDirectx.ReHook();

	//ImGui::GetIO().MouseDrawCursor = Settings.Global.menu_open;

	ImGui_ImplDX9_NewFrame();

	POINT mp;
	
	GetCursorPos(&mp);

	ImGuiIO& io = ImGui::GetIO();

	io.MousePos.x = mp.x;
	io.MousePos.y = mp.y;

	if (Settings.Global.menu_open)
	{

		DrawMenu();
	}
	ImGui::Render();

	return Hacks.oEndScene(pDevice);
}

long __stdcall Hooked_EndScene_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (!Settings.Global.d3d_init)
		return Hacks.oEndSceneReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_InvalidateDeviceObjects();

	auto hr = Hacks.oEndSceneReset(pDevice, pPresentationParameters);

	ImGui_ImplDX9_CreateDeviceObjects();

	return hr;
}

bool getFont(const std::string& name, std::string& path) {
	char buffer[MAX_PATH];
	HKEY registryKey;

	GetWindowsDirectoryA(buffer, MAX_PATH);
	std::string fontsFolder = buffer + std::string("\\Fonts\\");

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &registryKey)) {
		return false;
	}

	uint32_t valueIndex = 0;
	char valueName[MAX_PATH];
	uint8_t valueData[MAX_PATH];
	std::wstring wsFontFile;

	do {
		uint32_t valueNameSize = MAX_PATH;
		uint32_t valueDataSize = MAX_PATH;
		uint32_t valueType;

		auto error = RegEnumValueA(
			registryKey,
			valueIndex,
			valueName,
			reinterpret_cast<DWORD*>(&valueNameSize),
			0,
			reinterpret_cast<DWORD*>(&valueType),
			valueData,
			reinterpret_cast<DWORD*>(&valueDataSize));

		valueIndex++;

		if (error == ERROR_NO_MORE_ITEMS) {
			RegCloseKey(registryKey);
			return false;
		}

		if (error || valueType != REG_SZ) {
			continue;
		}

		if (_strnicmp(name.data(), valueName, name.size()) == 0) {
			path = fontsFolder + std::string((char*)valueData, valueDataSize);
			RegCloseKey(registryKey);
			return true;
		}
	} while (true);

	return false;
}

#define RGBA_TO_FLOAT(r,g,b,a) (float)r/255.0f, (float)g/255.0f, (float)b/255.0f, (float)a/255.0f

void GUI_Init(IDirect3DDevice9* pDevice)
{
	ImGui_ImplDX9_Init(INIT::Window, pDevice);


	ImGuiIO& io = ImGui::GetIO();


	ImGuiStyle& style = ImGui::GetStyle();

	ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(
		Avalon_compressed_data, Avalon_compressed_size, 14.f);

	style.Colors[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.58f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.73f, 0.73f, 0.73f, 0.65f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.18f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.58f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.53f, 0.53f, 0.53f, 0.34f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.68f, 0.68f, 0.68f, 0.83f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.20f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.75f, 0.75f, 0.75f, 0.87f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.30f, 0.29f, 0.29f, 0.60f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.67f, 0.64f, 0.64f, 0.40f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.61f, 0.61f, 0.61f, 0.40f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.77f, 0.76f, 0.76f, 0.50f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.69f, 0.68f, 0.68f, 0.30f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.49f, 0.48f, 0.48f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.96f, 0.96f, 0.96f, 0.60f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.00f, 0.68f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.68f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.68f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_Column] = ImVec4(0.15f, 0.00f, 0.00f, 0.35f);
	style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.59f);
	style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
	style.Colors[ImGuiCol_CloseButton] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);

	style.Alpha = .0f;
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.FramePadding = ImVec2(4, 1);
	style.ScrollbarSize = 10.f;
	style.ScrollbarRounding = 0.f;
	style.GrabMinSize = 5.f;

	Settings.Global.d3d_init = true;
}

int __stdcall Hooked_DoPostScreenEffects(int a1)
{
	if (Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame()) {
		if (Hacks.LocalPlayer) {
			Esp::CallGlow();
		}
	}

	return Hacks.oDoPostScreenEffects(Interfaces.pClientMode, a1);
}

void __stdcall Hooked_PaintTraverse(unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{
	if (!strcmp("HudZoom", Interfaces.pPanel->GetName(vguiPanel)) && Settings.Visuals.RemoveScope)
		return;

	Hacks.oPaintTraverse(Interfaces.pPanel, vguiPanel, forceRepaint, allowForce);

	if (strcmp("FocusOverlayPanel", Interfaces.pPanel->GetName(vguiPanel)))
	{
		return;
	}

	if (Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame())
	{
		Hacks.LocalPlayer = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
		Hacks.LocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();

		if (Hacks.LocalPlayer)
		{
			Misc::DrawScope();
			Esp::DoEsp();
			DoNightMode();
			GUI.Draw();
		}
	}
	Esp::Watermark();
	//Esp::StatusBar();

}
void NameSpam(const char* name, const char* name2) {
	static bool steal = false;

	ConVar* nameConvar = Interfaces.g_ICVars->FindVar("name");
	*(int*)((DWORD)&nameConvar->fnChangeCallback + 0xC) = 0;

	if (nameConvar)
	{
		if (steal)
		{
			nameConvar->SetValue(name);
		}
		else
		{
			nameConvar->SetValue(name2);
		}
		steal = !steal;
	}
}

struct CIncomingSequence
{
	CIncomingSequence::CIncomingSequence(int instate, int outstate, int seqnr, float time)
	{
		inreliablestate = instate;
		outreliablestate = outstate;
		sequencenr = seqnr;
		curtime = time;
	}
	int inreliablestate;
	int outreliablestate;
	int sequencenr;
	float curtime;
};


void New_CreateMove(CInput* thisptr, void* _EAX, int sequence_number, float input_sample_frametime, bool active)
{
	CInput::CVerifiedUserCmd* VerifiedCmd;
	AA.ShouldAA = false;
	CreateMoveETC::GetCmds(sequence_number, Hacks.CurrentCmd, VerifiedCmd);
	Misc::ServerRankReveal();
	if (Hacks.CurrentCmd && VerifiedCmd && Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame() && Hacks.LocalPlayer)
	{
		MEMCHECK;
		Misc::NameSpammer();
		Misc::Clan_Tag();
		Hacks.LocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();
		Vector OrigAng = Hacks.CurrentCmd->viewangles;
		CreateMoveETC::Bhop(OrigAng);
		Misc::Normalize(OrigAng);
		DoAutoStrafe();
		if (Hacks.LocalWeapon)
		{
			if (Hacks.LocalPlayer->isAlive())
			{
				CreateMoveETC::LocalPrediction();
				StartPrediction();
				Misc::AutoZues();
				KnifeBot::Run();
				if (Hacks.LocalWeapon->HasAmmo())
				{
					if (Settings.Aimbot.Enabled)
						Legitbot.Run();
					Aimbot.Aim(Hacks.CurrentCmd);
					if (Misc::Shooting())
					{

						if (Settings.Ragebot.SilentAim && !Hacks.LocalWeapon->IsMiscWeapon())
							Hacks.SendPacket = false;

						if (!Settings.Ragebot.SilentAim && Settings.Ragebot.EnableAim)
							Interfaces.pEngine->SetViewAngles(Hacks.CurrentCmd->viewangles);

						if (Settings.Ragebot.Accuracy.RemoveRecoil)
							Hacks.CurrentCmd->viewangles -= Hacks.LocalPlayer->GetPunchAngle() * 2.f;

						if (Settings.Ragebot.EnableAim && Settings.Ragebot.Accuracy.RemoveSpread)
							Hacks.CurrentCmd->viewangles = NoSpread->SpreadFactor(Hacks.CurrentCmd->random_seed);

					}
					else if (!Hacks.LocalWeapon->IsNade())
						Hacks.CurrentCmd->buttons &= ~IN_ATTACK;
					if (*Hacks.LocalWeapon->GetItemDefinitionIndex() == weapon_revolver && Settings.Ragebot.EnableAim)
					{
						if (Hacks.LocalWeapon->GetPostponeFireReadyTime() - Misc::GetServerTime() > 0.045 && Settings.Ragebot.AutoRevoler)
							Hacks.CurrentCmd->buttons |= IN_ATTACK;
					}
				

					
				}
				else
				{
					Hacks.CurrentCmd->buttons &= ~IN_ATTACK;
					Hacks.CurrentCmd->buttons |= IN_RELOAD;
				}
				if (!Misc::Shooting())
					AA.Run();
			}
		}

		if (Hacks.CurrentCmd->viewangles.y != Hacks.LocalPlayer->pelvisangs())
			faked = true;
		else
			faked = false;

		lineLBY = Hacks.LocalPlayer->pelvisangs();
		if (Hacks.SendPacket == true) {
			lineFakeAngle = Hacks.CurrentCmd->viewangles.y;
		}
		else if (Hacks.SendPacket == false) {
			lineRealAngle = Hacks.CurrentCmd->viewangles.y;
		}

		CreateMoveETC::CleanUp(OrigAng);
		LocalInfo.Choked = Hacks.SendPacket ? 0 : LocalInfo.Choked++;
		CreateMoveETC::VerifyCmd(VerifiedCmd);
	}
}

void __declspec(naked) __fastcall Hooked_Createmove(CInput* thisptr, void* _EAX, int sequence_number, float input_sample_frametime, bool active)
{
	Hacks.SendPacket = true;
	__asm
	{
		MOV Hacks.SendPacket, BL
		PUSH EBP
		MOV EBP, ESP
		SUB ESP, 8
		PUSHAD
		PUSH active
		PUSH input_sample_frametime
		PUSH sequence_number
		CALL Hacks.oCreateMove
	}
	Hacks.LocalPlayer = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
	New_CreateMove(thisptr, _EAX, sequence_number, input_sample_frametime, active);
	__asm
	{
		POPAD
		MOV BL, Hacks.SendPacket
		MOV ESP, EBP
		POP EBP
		RETN 0x0C
	}
}

void __stdcall Hooked_OverrideView(CViewSetup* pSetup)
{
	if (Settings.Misc.FovChanger)
	{
		ConVar* vFOV = Interfaces.g_ICVars->FindVar("viewmodel_fov");
		*(int*)((DWORD)&vFOV->fnChangeCallback + 0xC) = 0;
		vFOV->SetValue(Settings.Misc.ViewmodelFov);

		if (Interfaces.pEngine->IsInGame() && Interfaces.pEngine->IsConnected() && Hacks.LocalPlayer && Hacks.LocalPlayer->GetScope() && Hacks.LocalPlayer->GetHealth() > 0)
		{
			pSetup->fov = Settings.Misc.WorldFov;
			//CHECKMEM;
		}
	}
	Hacks.FOV = pSetup->fov;
	Hacks.oOverrideView(pSetup);
}

void __stdcall Hooked_PlaySound(const char* pSample)
{
	Hacks.oPlaySound(pSample);

	if (strstr(pSample, "weapons/hegrenade/beep.wav"))
	{
		if (!Settings.Misc.AutoAccept)
			return;

		Interfaces.pEngine->ClientCmd_Unrestricted("echo SOUND_FILE_FOUND", 0);
		DWORD dwIsReady = (Utils.PFindPattern("client_panorama.dll", "55 8B EC 83 E4 F8 83 EC 08 56 8B 35 ? ? ? ? 57 8B 8E"));
		reinterpret_cast< void(*)() >(dwIsReady)();
	}
}

void __stdcall Hooked_Frame_Stage_Notify(ClientFrameStage_t curStage)
{
	Hacks.LocalPlayer = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
	if (Hacks.LocalPlayer)
	{
		LocalInfo.PunchAns = *Hacks.LocalPlayer->GetPunchAnglePtr();
		LocalInfo.Flags = Hacks.LocalPlayer->GetFlags();
	}
	Vector*pPunchAngle = nullptr, *pViewPunchAngle = nullptr, vPunchAngle, vViewPunchAngle;
	if (Interfaces.pEngine->IsInGame() && Interfaces.pEngine->IsConnected())
	{
		if (Hacks.LocalPlayer)
		{
			Hacks.LocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();

			if (curStage == FRAME_RENDER_START)
			{
			//	MEMCHECK;
				if (Settings.Visuals.NoVisRecoil)
				{
					pPunchAngle = Hacks.LocalPlayer->GetPunchAnglePtr();
					pViewPunchAngle = Hacks.LocalPlayer->GetViewPunchAnglePtr();

					if (pPunchAngle && pViewPunchAngle)
					{
						vPunchAngle = *pPunchAngle;
						pPunchAngle->Init();
						vViewPunchAngle = *pViewPunchAngle;
						pViewPunchAngle->Init();
					}
				}

				for (auto material : smoke_materials)
				{
					IMaterial* mat = Interfaces.pMaterialSystem->FindMaterial(material, "Other textures");
					mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, Settings.Visuals.NoSmoke);
				}

			}

			if (curStage == FRAME_NET_UPDATE_START)
			{
				if (Settings.Visuals.BulletTracer)
				{
					float Red, Green, Blue;

					Red = Settings.Visuals.Colors.BulletTracer[0] * 255;
					Green = Settings.Visuals.Colors.BulletTracer[1] * 255;
					Blue = Settings.Visuals.Colors.BulletTracer[2] * 255;

					for (unsigned int i = 0; i < trace_logs.size(); i++) {

						auto *shooter = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetPlayerForUserID(trace_logs[i].userid));

						if (!shooter) return;

						CColor color = CColor(Red, Green, Blue, 210);

						DrawBeam(trace_logs[i].start, trace_logs[i].position, color);

						trace_logs.erase(trace_logs.begin() + i);
					}
				}
			}

			if (Settings.Ragebot.Resolver == 1 && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_END)
				Yaw->AntiAimResolver();

			if (Settings.Ragebot.Resolver == 3)
				Resolver::FrameStageNotify(curStage);

			if (Settings.Ragebot.Resolver == 2 && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_END)
				Yaw2->AntiAimResolver();
		}

		if (*reinterpret_cast<bool*>(reinterpret_cast<DWORD>(Interfaces.pInput) + 0xA5))
		{
			static Vector last;
			static Vector last_angle2;

			if (!Hacks.SendPacket)
			{
				*reinterpret_cast<Vector*>(reinterpret_cast<DWORD>(Hacks.LocalPlayer) + 0x31C8) = Hacks.CurrentCmd->viewangles;
				last_angle2 = Hacks.CurrentCmd->viewangles;
			}
			else
				*reinterpret_cast<Vector*>(reinterpret_cast<DWORD>(Hacks.LocalPlayer) + 0x31C8) = last_angle2;

		}

	}

	Hacks.oFrameStageNotify(curStage);

	if (pPunchAngle && pViewPunchAngle)
	{
		*pPunchAngle = vPunchAngle;
		*pViewPunchAngle = vViewPunchAngle;
	}
}

void InitKeyValues(KeyValues* keyValues, char* name)
{
	static uint8_t* sig1;
	if (!sig1)
	{
		sig1 = Utils.pattern_scan(GetModuleHandleW(L"client_panorama.dll"), "68 ? ? ? ? 8B C8 E8 ? ? ? ? 89 45 FC EB 07 C7 45 ? ? ? ? ? 8B 03 56");
		sig1 += 7;
		sig1 = sig1 + *reinterpret_cast<PDWORD_PTR>(sig1 + 1) + 5;
	}

	static auto function = (void(__thiscall*)(KeyValues*, const char*))sig1;
	function(keyValues, name);

}
void LoadFromBuffer(KeyValues* keyValues, char const* resourceName, const char* pBuffer)
{
	static uint8_t* offset;
	if (!offset) offset = Utils.pattern_scan(GetModuleHandleW(L"client_panorama.dll"), "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89 4C 24 04"); //фикс понорамы [client_panorama.dll]
	static auto function = (void(__thiscall*)(KeyValues*, char const*, const char*, void*, const char*, void*))offset;
	function(keyValues, resourceName, pBuffer, 0, 0, 0);
}

IMaterial* GetCurrentMaterial(int mati)
{
	float R = (Settings.Visuals.Colors.ChamsVisible[0]);
	float G = (Settings.Visuals.Colors.ChamsVisible[1]);
	float B = (Settings.Visuals.Colors.ChamsVisible[2]);

	static IMaterial* mat;

	switch (mati)
	{
	case 3:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", nullptr);
		mat->ColorModulate(R, G, B);
		break;
	case 4:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/gold", nullptr);
		mat->ColorModulate(R, G, B);
		break;
	case 5:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/player/ct_fbi/ct_fbi_glass", nullptr);
		mat->ColorModulate(R, G, B);
		break;
	case 6:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/gloss", nullptr);
		mat->ColorModulate(R, G, B);
		break;
	case 7:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_blue", nullptr);
		mat->ColorModulate(R, G, B);
		break;
	default:
		mat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", nullptr);
		mat->ColorModulate(R, G, B);
		return mat;
		break;
	}
	return mat;
}

IMaterial* GetCurrentZMaterial(int mati)
{
	static IMaterial* zmat;

	float RI = (Settings.Visuals.Colors.ChamsVisible[0]);
	float GI = (Settings.Visuals.Colors.ChamsVisible[1]);
	float BI = (Settings.Visuals.Colors.ChamsVisible[2]);
	switch (mati)
	{
	case 3:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", nullptr);
		zmat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, !Settings.Visuals.Chams.ThrowWalls);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	case 4:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/gold", nullptr);
		zmat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, !Settings.Visuals.Chams.ThrowWalls);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	case 5:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/player/ct_fbi/ct_fbi_glass", nullptr);
		zmat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, !Settings.Visuals.Chams.ThrowWalls);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	case 6:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/gloss", nullptr);
		zmat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, !Settings.Visuals.Chams.ThrowWalls);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	case 7:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_blue", nullptr);
		zmat->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, !Settings.Visuals.Chams.ThrowWalls);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	default:
		zmat = Interfaces.pMaterialSystem->FindMaterial("models/inventory_items/trophy_majors/crystal_clear", nullptr);
		zmat->ColorModulate(RI, GI, BI);
		return zmat;
		break;
	}
}

IMaterial* Create_Material(bool Ignore, bool Lit, bool Wireframe)
{
	static int created = 0;
	static const char tmp[] =
	{
		"\"%s\"\
		\n{\
		\n\t\"$basetexture\" \"vgui/white_additive\"\
		\n\t\"$envmap\" \"\"\
		\n\t\"$ignorez\" \"%i\"\
		\n\t\"$model\" \"1\"\
		\n\t\"$flat\" \"1\"\
		\n\t\"$nocull\"  \"0\"\
		\n\t\"$selfillum\" \"1\"\
		\n\t\"$halflambert\" \"1\"\
		\n\t\"$nofog\"  \"0\"\
		\n\t\"$znearer\" \"0\"\
		\n\t\"$wireframe\" \"%i\"\
        \n}\n"
	};
	char* baseType = (Lit == true ? "VertexLitGeneric" : "UnlitGeneric");
	char material[512];
	char name[512];
	sprintf_s(material, sizeof(material), tmp, baseType, (Ignore) ? 1 : 0, (Wireframe) ? 1 : 0);
	sprintf_s(name, sizeof(name), "#Aimpulse_Chams_%i.vmt", created);
	++created;
	KeyValues* keyValues = static_cast< KeyValues* >(malloc(sizeof(KeyValues)));
	InitKeyValues(keyValues, baseType);
	LoadFromBuffer(keyValues, name, material);
	IMaterial* createdMaterial = Interfaces.pMaterialSystem->CreateMaterial(name, keyValues);
	createdMaterial->IncrementReferenceCount();
	return createdMaterial;
}

bool Do_Chams(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t& pInfo, matrix3x4* pCustomBoneToWorld)
{
	static IMaterial* Covered_Lit = Create_Material(true, false, false);
	static IMaterial* Visable_Lit = Create_Material(false, false, false);
	//Colors
	float R = (Settings.Visuals.Colors.ChamsVisible[0]);
	float G = (Settings.Visuals.Colors.ChamsVisible[1]);
	float B = (Settings.Visuals.Colors.ChamsVisible[2]);

	float RI = (Settings.Visuals.Colors.ChamsInvisible[0]);
	float GI = (Settings.Visuals.Colors.ChamsInvisible[1]);
	float BI = (Settings.Visuals.Colors.ChamsInvisible[2]);
	//Colors-End
	CBaseEntity* Model_Entity = Interfaces.pEntList->GetClientEntity(pInfo.entity_index);
	auto Model_Name = Interfaces.g_pModelInfo->GetModelName(const_cast< model_t* >(pInfo.pModel));
	if (Settings.Visuals.Chams.Enable)
	{
		if (Model_Entity->IsPlayer())
		{
			if (Model_Entity->GetTeam() == Hacks.LocalPlayer->GetTeam() && ! Settings.Visuals.Chams.Team)
				return false;

			Covered_Lit->ColorModulate(RI, GI, BI);
			Visable_Lit->ColorModulate(R, G, B);

			if (!Model_Entity->isAlive())
				return false;

			if (Model_Entity->HasGunGameImmunity())
				Covered_Lit->AlphaModulate(0.75f);
			else
				Covered_Lit->AlphaModulate(1.f);

			if (!Settings.Visuals.Chams.ThrowWalls)
			{
				Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
				Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

				//return true;
			}

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			return true;
		}
		
		if (Model_Entity->IsWeapon() && Settings.Visuals.Chams.Guns)
		{
			//Colors
			float R = (Settings.Visuals.Colors.ChamsGunsVisible[0]);
			float G = (Settings.Visuals.Colors.ChamsGunsVisible[1]);
			float B = (Settings.Visuals.Colors.ChamsGunsVisible[2]);

			float RI = (Settings.Visuals.Colors.ChamsGunsInvisible[0]);
			float GI = (Settings.Visuals.Colors.ChamsGunsInvisible[1]);
			float BI = (Settings.Visuals.Colors.ChamsGunsInvisible[2]);
			//Colors-End
			Covered_Lit->ColorModulate(RI, GI, BI);
			Visable_Lit->ColorModulate(R, G, B);
			Visable_Lit->AlphaModulate(1.0f);
			Covered_Lit->AlphaModulate(1.0f);

			if (!Settings.Visuals.Chams.ThrowWalls)
			{
				Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
				Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
				return true;
			}

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			return true;
		}
	}
	if (Settings.Visuals.NoFlash)
	{
		IMaterial* Flash = Interfaces.pMaterialSystem->FindMaterial("effects\\flashbang", "ClientEffect textures");
		IMaterial* FlashWhite = Interfaces.pMaterialSystem->FindMaterial("effects\\flashbang_white", "ClientEffect textures");
		Flash->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
		FlashWhite->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
		return false;
	}

	return false;
}

void __fastcall Hooked_SceneEnd(void* thisptr, void* edx)
{
	if (Settings.Visuals.ChamsNew.Enable)
	{
		//removed bcuz antipasta
	}
	if (Hacks.LocalPlayer->isAlive()) {

		if (Settings.Visuals.FakeChams)
		{
			if (Hacks.LocalPlayer)
			{
				IMaterial* mat = Create_Material(false, true, false);

				Vector OrigAng;
				OrigAng = Hacks.LocalPlayer->GetEyeAngles();
				Hacks.LocalPlayer->SetAngle(Vector(0, lineFakeAngle, 0));
				float lbycolor[3] = { 1.f, 1.f, 1.f };
				float badcolor[3] = { 1.f, 0.f, 0.f };
				Interfaces.g_pRenderView->SetColorModulation(lbycolor);
				Interfaces.g_pModelRender->ForcedMaterialOverride(mat);
				Hacks.LocalPlayer->draw_model(STUDIO_RENDER, 255);
				Interfaces.g_pModelRender->ForcedMaterialOverride(nullptr);
				Hacks.LocalPlayer->SetAngle(OrigAng);
			}
		}
	}
	Hacks.oSceneEnd(thisptr, edx);
}

void __fastcall Hooked_DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t& pInfo, matrix3x4* pCustomBoneToWorld)
{
	if (Settings.Visuals.Chams.Enable && Do_Chams(thisptr, edx, ctx, state, pInfo, pCustomBoneToWorld))
		return;

	Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

	Interfaces.g_pModelRender->ForcedMaterialOverride(nullptr, OVERRIDE_NORMAL);
}

void __stdcall Hooked_ClientCmdUnrestricted(const char* szCmdString, char flag)
{
	/*string str(szCmdString);
	std::string prefix;
	if (!str.compare(0, string("Clantag ").size(), "Clantag "))
	{
		str.replace(0, string("Clantag ").size(), "");
		prefix = string("\\n");
		std::size_t found = str.find(prefix);
		if (found != std::string::npos)
			str.replace(found, found + prefix.size(), "\n");
		Misc::SetClanTag(str.c_str());
	}
	else if (!str.compare(0, string("ReleaseName").size(), "ReleaseName"))
	{
		Misc::setName("\n\xAD\xAD\xAD­­­");
	}
	else if (!str.compare(0, string("Name ").size(), "Name "))
	{
		str.replace(0, string("Name ").size(), "");
		prefix = string("\\n");
		std::size_t found = str.find(prefix);
		if (found != std::string::npos)
			str.replace(found, found + prefix.size(), "\n");
		Misc::setName(str.c_str());
	}
	else*/
	Hacks.oClientCmdUnresticted(szCmdString, flag);
}



LRESULT __stdcall Hooked_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		Hacks.PressedKeys[VK_LBUTTON] = true;
		break;
	case WM_LBUTTONUP:
		Hacks.PressedKeys[VK_LBUTTON] = false;
		break;
	case WM_RBUTTONDOWN:
		Hacks.PressedKeys[VK_RBUTTON] = true;
		break;
	case WM_RBUTTONUP:
		Hacks.PressedKeys[VK_RBUTTON] = false;
		break;
	case WM_MBUTTONDOWN:
		Hacks.PressedKeys[VK_MBUTTON] = true;
		break;
	case WM_MBUTTONUP:
		Hacks.PressedKeys[VK_MBUTTON] = false;
		break;
	case WM_XBUTTONDOWN:
	{
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
			Hacks.PressedKeys[VK_XBUTTON1] = true;
		}
		else if (button == XBUTTON2)
		{
			Hacks.PressedKeys[VK_XBUTTON2] = true;
		}
		break;
	}
	case WM_XBUTTONUP:
	{
		UINT button = GET_XBUTTON_WPARAM(wParam);
		if (button == XBUTTON1)
		{
			Hacks.PressedKeys[VK_XBUTTON1] = false;
		}
		else if (button == XBUTTON2)
		{
			Hacks.PressedKeys[VK_XBUTTON2] = false;
		}
		break;
	}
	case WM_KEYDOWN:
		Hacks.PressedKeys[wParam] = true;
		break;
	case WM_KEYUP:
		Hacks.PressedKeys[wParam] = false;
		break;
	default: break;
	}

	OpenMenu();

	if (Settings.Global.d3d_init && Settings.Global.menu_open && ImGui_ImplDX9_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(INIT::OldWindow, hWnd, uMsg, wParam, lParam);
}

// Junk Code By Troll Face & Thaisen's Gen
void zLFlwBxSHDIolQJsiKtm9115134() {     int QkbcyRYdeAcRbhslmapB43787643 = -832749386;    int QkbcyRYdeAcRbhslmapB35964109 = -846682602;    int QkbcyRYdeAcRbhslmapB74057356 = 79993394;    int QkbcyRYdeAcRbhslmapB26256751 = -428713681;    int QkbcyRYdeAcRbhslmapB40638030 = -590906673;    int QkbcyRYdeAcRbhslmapB10197150 = -146818139;    int QkbcyRYdeAcRbhslmapB42360679 = -330491943;    int QkbcyRYdeAcRbhslmapB8622028 = -300578437;    int QkbcyRYdeAcRbhslmapB48558978 = -305579246;    int QkbcyRYdeAcRbhslmapB54802116 = -767783955;    int QkbcyRYdeAcRbhslmapB44403048 = -810129701;    int QkbcyRYdeAcRbhslmapB9153269 = -521622583;    int QkbcyRYdeAcRbhslmapB37624121 = -134009181;    int QkbcyRYdeAcRbhslmapB91207087 = 80253083;    int QkbcyRYdeAcRbhslmapB5206560 = -987121267;    int QkbcyRYdeAcRbhslmapB43781465 = -666084234;    int QkbcyRYdeAcRbhslmapB83376225 = -629211013;    int QkbcyRYdeAcRbhslmapB77913823 = -316504822;    int QkbcyRYdeAcRbhslmapB18412540 = -845414788;    int QkbcyRYdeAcRbhslmapB80266550 = -81479465;    int QkbcyRYdeAcRbhslmapB27292839 = -129788320;    int QkbcyRYdeAcRbhslmapB5940590 = -163058919;    int QkbcyRYdeAcRbhslmapB42920219 = -508947160;    int QkbcyRYdeAcRbhslmapB94452052 = -805854329;    int QkbcyRYdeAcRbhslmapB48541467 = -984467236;    int QkbcyRYdeAcRbhslmapB76813339 = -292749060;    int QkbcyRYdeAcRbhslmapB81763604 = -662933223;    int QkbcyRYdeAcRbhslmapB83687539 = -445633593;    int QkbcyRYdeAcRbhslmapB48956128 = -599177661;    int QkbcyRYdeAcRbhslmapB40109726 = -919476900;    int QkbcyRYdeAcRbhslmapB57203828 = -408237925;    int QkbcyRYdeAcRbhslmapB21042684 = -519510120;    int QkbcyRYdeAcRbhslmapB55326861 = -408330501;    int QkbcyRYdeAcRbhslmapB37402624 = -662673519;    int QkbcyRYdeAcRbhslmapB5702268 = -292635577;    int QkbcyRYdeAcRbhslmapB98419190 = -464124531;    int QkbcyRYdeAcRbhslmapB29545615 = 98859836;    int QkbcyRYdeAcRbhslmapB15224538 = -196670772;    int QkbcyRYdeAcRbhslmapB95670784 = -336263606;    int QkbcyRYdeAcRbhslmapB5203845 = -814825303;    int QkbcyRYdeAcRbhslmapB65809704 = -414447431;    int QkbcyRYdeAcRbhslmapB66581093 = -112669242;    int QkbcyRYdeAcRbhslmapB85354471 = -839271347;    int QkbcyRYdeAcRbhslmapB93365185 = -620832426;    int QkbcyRYdeAcRbhslmapB93573186 = -443462953;    int QkbcyRYdeAcRbhslmapB69031009 = -319249116;    int QkbcyRYdeAcRbhslmapB14159442 = -216647404;    int QkbcyRYdeAcRbhslmapB83686037 = -567651294;    int QkbcyRYdeAcRbhslmapB58722490 = -662571711;    int QkbcyRYdeAcRbhslmapB51426153 = -489442470;    int QkbcyRYdeAcRbhslmapB45713675 = -192286781;    int QkbcyRYdeAcRbhslmapB43107408 = -507428054;    int QkbcyRYdeAcRbhslmapB76792472 = -308627798;    int QkbcyRYdeAcRbhslmapB77586005 = -865084663;    int QkbcyRYdeAcRbhslmapB81822295 = 2129365;    int QkbcyRYdeAcRbhslmapB37847054 = -569690468;    int QkbcyRYdeAcRbhslmapB93043890 = -237735442;    int QkbcyRYdeAcRbhslmapB79605304 = -114152278;    int QkbcyRYdeAcRbhslmapB77715283 = -444246445;    int QkbcyRYdeAcRbhslmapB63824691 = -198157614;    int QkbcyRYdeAcRbhslmapB28433545 = -483884917;    int QkbcyRYdeAcRbhslmapB58673140 = -884858351;    int QkbcyRYdeAcRbhslmapB59665900 = -701400777;    int QkbcyRYdeAcRbhslmapB8449252 = -386102346;    int QkbcyRYdeAcRbhslmapB97598287 = -259546030;    int QkbcyRYdeAcRbhslmapB23360364 = -190619581;    int QkbcyRYdeAcRbhslmapB53826408 = -13292082;    int QkbcyRYdeAcRbhslmapB221497 = -471335662;    int QkbcyRYdeAcRbhslmapB85504820 = -627111340;    int QkbcyRYdeAcRbhslmapB6787370 = -422996737;    int QkbcyRYdeAcRbhslmapB14235851 = -664944070;    int QkbcyRYdeAcRbhslmapB68151688 = -332540242;    int QkbcyRYdeAcRbhslmapB82243039 = -980241216;    int QkbcyRYdeAcRbhslmapB13208695 = 69410515;    int QkbcyRYdeAcRbhslmapB14456847 = -667032035;    int QkbcyRYdeAcRbhslmapB60711745 = 82880921;    int QkbcyRYdeAcRbhslmapB20586118 = -323787573;    int QkbcyRYdeAcRbhslmapB49555034 = -888114735;    int QkbcyRYdeAcRbhslmapB878867 = -262391376;    int QkbcyRYdeAcRbhslmapB79510458 = -565218121;    int QkbcyRYdeAcRbhslmapB62653897 = 23898344;    int QkbcyRYdeAcRbhslmapB98077566 = 4718071;    int QkbcyRYdeAcRbhslmapB24965050 = -783061882;    int QkbcyRYdeAcRbhslmapB97529974 = -9735191;    int QkbcyRYdeAcRbhslmapB94396050 = -627190120;    int QkbcyRYdeAcRbhslmapB14096421 = -900809871;    int QkbcyRYdeAcRbhslmapB44250211 = -110882323;    int QkbcyRYdeAcRbhslmapB77740855 = -543245839;    int QkbcyRYdeAcRbhslmapB55580329 = -564802885;    int QkbcyRYdeAcRbhslmapB67855213 = -722945110;    int QkbcyRYdeAcRbhslmapB5375301 = -126389089;    int QkbcyRYdeAcRbhslmapB49940311 = -786987887;    int QkbcyRYdeAcRbhslmapB37509254 = -752424327;    int QkbcyRYdeAcRbhslmapB31846093 = -38105993;    int QkbcyRYdeAcRbhslmapB76770299 = -230940387;    int QkbcyRYdeAcRbhslmapB7136565 = -529589081;    int QkbcyRYdeAcRbhslmapB6915194 = -411268466;    int QkbcyRYdeAcRbhslmapB76905219 = -353169001;    int QkbcyRYdeAcRbhslmapB95766897 = -261286396;    int QkbcyRYdeAcRbhslmapB70212822 = -832749386;     QkbcyRYdeAcRbhslmapB43787643 = QkbcyRYdeAcRbhslmapB35964109;     QkbcyRYdeAcRbhslmapB35964109 = QkbcyRYdeAcRbhslmapB74057356;     QkbcyRYdeAcRbhslmapB74057356 = QkbcyRYdeAcRbhslmapB26256751;     QkbcyRYdeAcRbhslmapB26256751 = QkbcyRYdeAcRbhslmapB40638030;     QkbcyRYdeAcRbhslmapB40638030 = QkbcyRYdeAcRbhslmapB10197150;     QkbcyRYdeAcRbhslmapB10197150 = QkbcyRYdeAcRbhslmapB42360679;     QkbcyRYdeAcRbhslmapB42360679 = QkbcyRYdeAcRbhslmapB8622028;     QkbcyRYdeAcRbhslmapB8622028 = QkbcyRYdeAcRbhslmapB48558978;     QkbcyRYdeAcRbhslmapB48558978 = QkbcyRYdeAcRbhslmapB54802116;     QkbcyRYdeAcRbhslmapB54802116 = QkbcyRYdeAcRbhslmapB44403048;     QkbcyRYdeAcRbhslmapB44403048 = QkbcyRYdeAcRbhslmapB9153269;     QkbcyRYdeAcRbhslmapB9153269 = QkbcyRYdeAcRbhslmapB37624121;     QkbcyRYdeAcRbhslmapB37624121 = QkbcyRYdeAcRbhslmapB91207087;     QkbcyRYdeAcRbhslmapB91207087 = QkbcyRYdeAcRbhslmapB5206560;     QkbcyRYdeAcRbhslmapB5206560 = QkbcyRYdeAcRbhslmapB43781465;     QkbcyRYdeAcRbhslmapB43781465 = QkbcyRYdeAcRbhslmapB83376225;     QkbcyRYdeAcRbhslmapB83376225 = QkbcyRYdeAcRbhslmapB77913823;     QkbcyRYdeAcRbhslmapB77913823 = QkbcyRYdeAcRbhslmapB18412540;     QkbcyRYdeAcRbhslmapB18412540 = QkbcyRYdeAcRbhslmapB80266550;     QkbcyRYdeAcRbhslmapB80266550 = QkbcyRYdeAcRbhslmapB27292839;     QkbcyRYdeAcRbhslmapB27292839 = QkbcyRYdeAcRbhslmapB5940590;     QkbcyRYdeAcRbhslmapB5940590 = QkbcyRYdeAcRbhslmapB42920219;     QkbcyRYdeAcRbhslmapB42920219 = QkbcyRYdeAcRbhslmapB94452052;     QkbcyRYdeAcRbhslmapB94452052 = QkbcyRYdeAcRbhslmapB48541467;     QkbcyRYdeAcRbhslmapB48541467 = QkbcyRYdeAcRbhslmapB76813339;     QkbcyRYdeAcRbhslmapB76813339 = QkbcyRYdeAcRbhslmapB81763604;     QkbcyRYdeAcRbhslmapB81763604 = QkbcyRYdeAcRbhslmapB83687539;     QkbcyRYdeAcRbhslmapB83687539 = QkbcyRYdeAcRbhslmapB48956128;     QkbcyRYdeAcRbhslmapB48956128 = QkbcyRYdeAcRbhslmapB40109726;     QkbcyRYdeAcRbhslmapB40109726 = QkbcyRYdeAcRbhslmapB57203828;     QkbcyRYdeAcRbhslmapB57203828 = QkbcyRYdeAcRbhslmapB21042684;     QkbcyRYdeAcRbhslmapB21042684 = QkbcyRYdeAcRbhslmapB55326861;     QkbcyRYdeAcRbhslmapB55326861 = QkbcyRYdeAcRbhslmapB37402624;     QkbcyRYdeAcRbhslmapB37402624 = QkbcyRYdeAcRbhslmapB5702268;     QkbcyRYdeAcRbhslmapB5702268 = QkbcyRYdeAcRbhslmapB98419190;     QkbcyRYdeAcRbhslmapB98419190 = QkbcyRYdeAcRbhslmapB29545615;     QkbcyRYdeAcRbhslmapB29545615 = QkbcyRYdeAcRbhslmapB15224538;     QkbcyRYdeAcRbhslmapB15224538 = QkbcyRYdeAcRbhslmapB95670784;     QkbcyRYdeAcRbhslmapB95670784 = QkbcyRYdeAcRbhslmapB5203845;     QkbcyRYdeAcRbhslmapB5203845 = QkbcyRYdeAcRbhslmapB65809704;     QkbcyRYdeAcRbhslmapB65809704 = QkbcyRYdeAcRbhslmapB66581093;     QkbcyRYdeAcRbhslmapB66581093 = QkbcyRYdeAcRbhslmapB85354471;     QkbcyRYdeAcRbhslmapB85354471 = QkbcyRYdeAcRbhslmapB93365185;     QkbcyRYdeAcRbhslmapB93365185 = QkbcyRYdeAcRbhslmapB93573186;     QkbcyRYdeAcRbhslmapB93573186 = QkbcyRYdeAcRbhslmapB69031009;     QkbcyRYdeAcRbhslmapB69031009 = QkbcyRYdeAcRbhslmapB14159442;     QkbcyRYdeAcRbhslmapB14159442 = QkbcyRYdeAcRbhslmapB83686037;     QkbcyRYdeAcRbhslmapB83686037 = QkbcyRYdeAcRbhslmapB58722490;     QkbcyRYdeAcRbhslmapB58722490 = QkbcyRYdeAcRbhslmapB51426153;     QkbcyRYdeAcRbhslmapB51426153 = QkbcyRYdeAcRbhslmapB45713675;     QkbcyRYdeAcRbhslmapB45713675 = QkbcyRYdeAcRbhslmapB43107408;     QkbcyRYdeAcRbhslmapB43107408 = QkbcyRYdeAcRbhslmapB76792472;     QkbcyRYdeAcRbhslmapB76792472 = QkbcyRYdeAcRbhslmapB77586005;     QkbcyRYdeAcRbhslmapB77586005 = QkbcyRYdeAcRbhslmapB81822295;     QkbcyRYdeAcRbhslmapB81822295 = QkbcyRYdeAcRbhslmapB37847054;     QkbcyRYdeAcRbhslmapB37847054 = QkbcyRYdeAcRbhslmapB93043890;     QkbcyRYdeAcRbhslmapB93043890 = QkbcyRYdeAcRbhslmapB79605304;     QkbcyRYdeAcRbhslmapB79605304 = QkbcyRYdeAcRbhslmapB77715283;     QkbcyRYdeAcRbhslmapB77715283 = QkbcyRYdeAcRbhslmapB63824691;     QkbcyRYdeAcRbhslmapB63824691 = QkbcyRYdeAcRbhslmapB28433545;     QkbcyRYdeAcRbhslmapB28433545 = QkbcyRYdeAcRbhslmapB58673140;     QkbcyRYdeAcRbhslmapB58673140 = QkbcyRYdeAcRbhslmapB59665900;     QkbcyRYdeAcRbhslmapB59665900 = QkbcyRYdeAcRbhslmapB8449252;     QkbcyRYdeAcRbhslmapB8449252 = QkbcyRYdeAcRbhslmapB97598287;     QkbcyRYdeAcRbhslmapB97598287 = QkbcyRYdeAcRbhslmapB23360364;     QkbcyRYdeAcRbhslmapB23360364 = QkbcyRYdeAcRbhslmapB53826408;     QkbcyRYdeAcRbhslmapB53826408 = QkbcyRYdeAcRbhslmapB221497;     QkbcyRYdeAcRbhslmapB221497 = QkbcyRYdeAcRbhslmapB85504820;     QkbcyRYdeAcRbhslmapB85504820 = QkbcyRYdeAcRbhslmapB6787370;     QkbcyRYdeAcRbhslmapB6787370 = QkbcyRYdeAcRbhslmapB14235851;     QkbcyRYdeAcRbhslmapB14235851 = QkbcyRYdeAcRbhslmapB68151688;     QkbcyRYdeAcRbhslmapB68151688 = QkbcyRYdeAcRbhslmapB82243039;     QkbcyRYdeAcRbhslmapB82243039 = QkbcyRYdeAcRbhslmapB13208695;     QkbcyRYdeAcRbhslmapB13208695 = QkbcyRYdeAcRbhslmapB14456847;     QkbcyRYdeAcRbhslmapB14456847 = QkbcyRYdeAcRbhslmapB60711745;     QkbcyRYdeAcRbhslmapB60711745 = QkbcyRYdeAcRbhslmapB20586118;     QkbcyRYdeAcRbhslmapB20586118 = QkbcyRYdeAcRbhslmapB49555034;     QkbcyRYdeAcRbhslmapB49555034 = QkbcyRYdeAcRbhslmapB878867;     QkbcyRYdeAcRbhslmapB878867 = QkbcyRYdeAcRbhslmapB79510458;     QkbcyRYdeAcRbhslmapB79510458 = QkbcyRYdeAcRbhslmapB62653897;     QkbcyRYdeAcRbhslmapB62653897 = QkbcyRYdeAcRbhslmapB98077566;     QkbcyRYdeAcRbhslmapB98077566 = QkbcyRYdeAcRbhslmapB24965050;     QkbcyRYdeAcRbhslmapB24965050 = QkbcyRYdeAcRbhslmapB97529974;     QkbcyRYdeAcRbhslmapB97529974 = QkbcyRYdeAcRbhslmapB94396050;     QkbcyRYdeAcRbhslmapB94396050 = QkbcyRYdeAcRbhslmapB14096421;     QkbcyRYdeAcRbhslmapB14096421 = QkbcyRYdeAcRbhslmapB44250211;     QkbcyRYdeAcRbhslmapB44250211 = QkbcyRYdeAcRbhslmapB77740855;     QkbcyRYdeAcRbhslmapB77740855 = QkbcyRYdeAcRbhslmapB55580329;     QkbcyRYdeAcRbhslmapB55580329 = QkbcyRYdeAcRbhslmapB67855213;     QkbcyRYdeAcRbhslmapB67855213 = QkbcyRYdeAcRbhslmapB5375301;     QkbcyRYdeAcRbhslmapB5375301 = QkbcyRYdeAcRbhslmapB49940311;     QkbcyRYdeAcRbhslmapB49940311 = QkbcyRYdeAcRbhslmapB37509254;     QkbcyRYdeAcRbhslmapB37509254 = QkbcyRYdeAcRbhslmapB31846093;     QkbcyRYdeAcRbhslmapB31846093 = QkbcyRYdeAcRbhslmapB76770299;     QkbcyRYdeAcRbhslmapB76770299 = QkbcyRYdeAcRbhslmapB7136565;     QkbcyRYdeAcRbhslmapB7136565 = QkbcyRYdeAcRbhslmapB6915194;     QkbcyRYdeAcRbhslmapB6915194 = QkbcyRYdeAcRbhslmapB76905219;     QkbcyRYdeAcRbhslmapB76905219 = QkbcyRYdeAcRbhslmapB95766897;     QkbcyRYdeAcRbhslmapB95766897 = QkbcyRYdeAcRbhslmapB70212822;     QkbcyRYdeAcRbhslmapB70212822 = QkbcyRYdeAcRbhslmapB43787643;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void vhGKlXuQTsrrtKfgXrjb15524274() {     int CsqrjohEkQcNojCpGGfN77876434 = -62147789;    int CsqrjohEkQcNojCpGGfN91316889 = -435505390;    int CsqrjohEkQcNojCpGGfN45302934 = -483246912;    int CsqrjohEkQcNojCpGGfN86382155 = -812290966;    int CsqrjohEkQcNojCpGGfN94362827 = -267680730;    int CsqrjohEkQcNojCpGGfN43022708 = 9810292;    int CsqrjohEkQcNojCpGGfN49622516 = 50942314;    int CsqrjohEkQcNojCpGGfN48934186 = -161620526;    int CsqrjohEkQcNojCpGGfN60666788 = -159209646;    int CsqrjohEkQcNojCpGGfN15347093 = -911918952;    int CsqrjohEkQcNojCpGGfN33936870 = -323402217;    int CsqrjohEkQcNojCpGGfN17522852 = -636224780;    int CsqrjohEkQcNojCpGGfN83766730 = -343532775;    int CsqrjohEkQcNojCpGGfN55239807 = -108535593;    int CsqrjohEkQcNojCpGGfN66088664 = -281323260;    int CsqrjohEkQcNojCpGGfN1940243 = -490471608;    int CsqrjohEkQcNojCpGGfN85233493 = -950053107;    int CsqrjohEkQcNojCpGGfN19542768 = -883912916;    int CsqrjohEkQcNojCpGGfN44568097 = -840177305;    int CsqrjohEkQcNojCpGGfN62107707 = -426408103;    int CsqrjohEkQcNojCpGGfN63206389 = 79489935;    int CsqrjohEkQcNojCpGGfN51903286 = -799710100;    int CsqrjohEkQcNojCpGGfN39181224 = -563220278;    int CsqrjohEkQcNojCpGGfN81463880 = -55960557;    int CsqrjohEkQcNojCpGGfN67657987 = -608287008;    int CsqrjohEkQcNojCpGGfN32962159 = -836215593;    int CsqrjohEkQcNojCpGGfN611856 = -687063109;    int CsqrjohEkQcNojCpGGfN68463991 = -697104791;    int CsqrjohEkQcNojCpGGfN65725317 = -780537413;    int CsqrjohEkQcNojCpGGfN42708727 = -353475961;    int CsqrjohEkQcNojCpGGfN25790799 = -905141376;    int CsqrjohEkQcNojCpGGfN11535966 = 44816225;    int CsqrjohEkQcNojCpGGfN72806106 = -26440045;    int CsqrjohEkQcNojCpGGfN98188683 = -458168014;    int CsqrjohEkQcNojCpGGfN97695594 = -423350767;    int CsqrjohEkQcNojCpGGfN91307028 = -507763995;    int CsqrjohEkQcNojCpGGfN75444902 = -459762109;    int CsqrjohEkQcNojCpGGfN37754427 = -344912153;    int CsqrjohEkQcNojCpGGfN26703754 = -933108799;    int CsqrjohEkQcNojCpGGfN33513903 = -872094319;    int CsqrjohEkQcNojCpGGfN92245659 = -539414985;    int CsqrjohEkQcNojCpGGfN18895874 = -471459464;    int CsqrjohEkQcNojCpGGfN87439026 = -449196059;    int CsqrjohEkQcNojCpGGfN20116034 = -583502254;    int CsqrjohEkQcNojCpGGfN77303665 = -887650883;    int CsqrjohEkQcNojCpGGfN29976037 = -932890225;    int CsqrjohEkQcNojCpGGfN36895739 = -428802889;    int CsqrjohEkQcNojCpGGfN4521767 = -582131567;    int CsqrjohEkQcNojCpGGfN91846093 = 99657196;    int CsqrjohEkQcNojCpGGfN93069819 = -507394839;    int CsqrjohEkQcNojCpGGfN39280002 = -927988412;    int CsqrjohEkQcNojCpGGfN88379223 = -723530034;    int CsqrjohEkQcNojCpGGfN87270510 = -395532072;    int CsqrjohEkQcNojCpGGfN46297534 = -137434757;    int CsqrjohEkQcNojCpGGfN52317884 = -375559560;    int CsqrjohEkQcNojCpGGfN25973149 = -262437689;    int CsqrjohEkQcNojCpGGfN52135665 = -872285112;    int CsqrjohEkQcNojCpGGfN63839053 = -327286355;    int CsqrjohEkQcNojCpGGfN18724169 = -104003958;    int CsqrjohEkQcNojCpGGfN61400669 = -431465138;    int CsqrjohEkQcNojCpGGfN42410852 = -303126600;    int CsqrjohEkQcNojCpGGfN81158525 = -251952896;    int CsqrjohEkQcNojCpGGfN83208868 = -381083113;    int CsqrjohEkQcNojCpGGfN17958061 = -805733686;    int CsqrjohEkQcNojCpGGfN89556293 = 93222424;    int CsqrjohEkQcNojCpGGfN22400905 = -268218443;    int CsqrjohEkQcNojCpGGfN44716745 = -509784735;    int CsqrjohEkQcNojCpGGfN85578047 = -885364762;    int CsqrjohEkQcNojCpGGfN57544213 = -685184827;    int CsqrjohEkQcNojCpGGfN74781635 = -773559265;    int CsqrjohEkQcNojCpGGfN26495341 = 69290500;    int CsqrjohEkQcNojCpGGfN47479067 = -505140955;    int CsqrjohEkQcNojCpGGfN92839013 = -950804117;    int CsqrjohEkQcNojCpGGfN11054195 = -968082987;    int CsqrjohEkQcNojCpGGfN69862047 = -886993119;    int CsqrjohEkQcNojCpGGfN44310516 = -449050602;    int CsqrjohEkQcNojCpGGfN64464259 = -250514042;    int CsqrjohEkQcNojCpGGfN19065191 = -979718025;    int CsqrjohEkQcNojCpGGfN4160215 = -168309675;    int CsqrjohEkQcNojCpGGfN37681950 = -675396784;    int CsqrjohEkQcNojCpGGfN96066419 = -307412705;    int CsqrjohEkQcNojCpGGfN96090089 = -4931543;    int CsqrjohEkQcNojCpGGfN76617898 = -696761988;    int CsqrjohEkQcNojCpGGfN72655497 = -173142574;    int CsqrjohEkQcNojCpGGfN3428725 = -425487549;    int CsqrjohEkQcNojCpGGfN37411576 = -81611342;    int CsqrjohEkQcNojCpGGfN24265456 = -559651704;    int CsqrjohEkQcNojCpGGfN26508573 = -889005289;    int CsqrjohEkQcNojCpGGfN45870800 = 17391545;    int CsqrjohEkQcNojCpGGfN71722445 = -60913078;    int CsqrjohEkQcNojCpGGfN39171363 = -635478884;    int CsqrjohEkQcNojCpGGfN11605849 = -32475754;    int CsqrjohEkQcNojCpGGfN19030259 = -140908195;    int CsqrjohEkQcNojCpGGfN65303085 = -401643662;    int CsqrjohEkQcNojCpGGfN91103050 = -468967719;    int CsqrjohEkQcNojCpGGfN11087135 = -187462090;    int CsqrjohEkQcNojCpGGfN35687005 = 9623648;    int CsqrjohEkQcNojCpGGfN69480966 = -643462374;    int CsqrjohEkQcNojCpGGfN30559740 = -576724678;    int CsqrjohEkQcNojCpGGfN54902761 = -62147789;     CsqrjohEkQcNojCpGGfN77876434 = CsqrjohEkQcNojCpGGfN91316889;     CsqrjohEkQcNojCpGGfN91316889 = CsqrjohEkQcNojCpGGfN45302934;     CsqrjohEkQcNojCpGGfN45302934 = CsqrjohEkQcNojCpGGfN86382155;     CsqrjohEkQcNojCpGGfN86382155 = CsqrjohEkQcNojCpGGfN94362827;     CsqrjohEkQcNojCpGGfN94362827 = CsqrjohEkQcNojCpGGfN43022708;     CsqrjohEkQcNojCpGGfN43022708 = CsqrjohEkQcNojCpGGfN49622516;     CsqrjohEkQcNojCpGGfN49622516 = CsqrjohEkQcNojCpGGfN48934186;     CsqrjohEkQcNojCpGGfN48934186 = CsqrjohEkQcNojCpGGfN60666788;     CsqrjohEkQcNojCpGGfN60666788 = CsqrjohEkQcNojCpGGfN15347093;     CsqrjohEkQcNojCpGGfN15347093 = CsqrjohEkQcNojCpGGfN33936870;     CsqrjohEkQcNojCpGGfN33936870 = CsqrjohEkQcNojCpGGfN17522852;     CsqrjohEkQcNojCpGGfN17522852 = CsqrjohEkQcNojCpGGfN83766730;     CsqrjohEkQcNojCpGGfN83766730 = CsqrjohEkQcNojCpGGfN55239807;     CsqrjohEkQcNojCpGGfN55239807 = CsqrjohEkQcNojCpGGfN66088664;     CsqrjohEkQcNojCpGGfN66088664 = CsqrjohEkQcNojCpGGfN1940243;     CsqrjohEkQcNojCpGGfN1940243 = CsqrjohEkQcNojCpGGfN85233493;     CsqrjohEkQcNojCpGGfN85233493 = CsqrjohEkQcNojCpGGfN19542768;     CsqrjohEkQcNojCpGGfN19542768 = CsqrjohEkQcNojCpGGfN44568097;     CsqrjohEkQcNojCpGGfN44568097 = CsqrjohEkQcNojCpGGfN62107707;     CsqrjohEkQcNojCpGGfN62107707 = CsqrjohEkQcNojCpGGfN63206389;     CsqrjohEkQcNojCpGGfN63206389 = CsqrjohEkQcNojCpGGfN51903286;     CsqrjohEkQcNojCpGGfN51903286 = CsqrjohEkQcNojCpGGfN39181224;     CsqrjohEkQcNojCpGGfN39181224 = CsqrjohEkQcNojCpGGfN81463880;     CsqrjohEkQcNojCpGGfN81463880 = CsqrjohEkQcNojCpGGfN67657987;     CsqrjohEkQcNojCpGGfN67657987 = CsqrjohEkQcNojCpGGfN32962159;     CsqrjohEkQcNojCpGGfN32962159 = CsqrjohEkQcNojCpGGfN611856;     CsqrjohEkQcNojCpGGfN611856 = CsqrjohEkQcNojCpGGfN68463991;     CsqrjohEkQcNojCpGGfN68463991 = CsqrjohEkQcNojCpGGfN65725317;     CsqrjohEkQcNojCpGGfN65725317 = CsqrjohEkQcNojCpGGfN42708727;     CsqrjohEkQcNojCpGGfN42708727 = CsqrjohEkQcNojCpGGfN25790799;     CsqrjohEkQcNojCpGGfN25790799 = CsqrjohEkQcNojCpGGfN11535966;     CsqrjohEkQcNojCpGGfN11535966 = CsqrjohEkQcNojCpGGfN72806106;     CsqrjohEkQcNojCpGGfN72806106 = CsqrjohEkQcNojCpGGfN98188683;     CsqrjohEkQcNojCpGGfN98188683 = CsqrjohEkQcNojCpGGfN97695594;     CsqrjohEkQcNojCpGGfN97695594 = CsqrjohEkQcNojCpGGfN91307028;     CsqrjohEkQcNojCpGGfN91307028 = CsqrjohEkQcNojCpGGfN75444902;     CsqrjohEkQcNojCpGGfN75444902 = CsqrjohEkQcNojCpGGfN37754427;     CsqrjohEkQcNojCpGGfN37754427 = CsqrjohEkQcNojCpGGfN26703754;     CsqrjohEkQcNojCpGGfN26703754 = CsqrjohEkQcNojCpGGfN33513903;     CsqrjohEkQcNojCpGGfN33513903 = CsqrjohEkQcNojCpGGfN92245659;     CsqrjohEkQcNojCpGGfN92245659 = CsqrjohEkQcNojCpGGfN18895874;     CsqrjohEkQcNojCpGGfN18895874 = CsqrjohEkQcNojCpGGfN87439026;     CsqrjohEkQcNojCpGGfN87439026 = CsqrjohEkQcNojCpGGfN20116034;     CsqrjohEkQcNojCpGGfN20116034 = CsqrjohEkQcNojCpGGfN77303665;     CsqrjohEkQcNojCpGGfN77303665 = CsqrjohEkQcNojCpGGfN29976037;     CsqrjohEkQcNojCpGGfN29976037 = CsqrjohEkQcNojCpGGfN36895739;     CsqrjohEkQcNojCpGGfN36895739 = CsqrjohEkQcNojCpGGfN4521767;     CsqrjohEkQcNojCpGGfN4521767 = CsqrjohEkQcNojCpGGfN91846093;     CsqrjohEkQcNojCpGGfN91846093 = CsqrjohEkQcNojCpGGfN93069819;     CsqrjohEkQcNojCpGGfN93069819 = CsqrjohEkQcNojCpGGfN39280002;     CsqrjohEkQcNojCpGGfN39280002 = CsqrjohEkQcNojCpGGfN88379223;     CsqrjohEkQcNojCpGGfN88379223 = CsqrjohEkQcNojCpGGfN87270510;     CsqrjohEkQcNojCpGGfN87270510 = CsqrjohEkQcNojCpGGfN46297534;     CsqrjohEkQcNojCpGGfN46297534 = CsqrjohEkQcNojCpGGfN52317884;     CsqrjohEkQcNojCpGGfN52317884 = CsqrjohEkQcNojCpGGfN25973149;     CsqrjohEkQcNojCpGGfN25973149 = CsqrjohEkQcNojCpGGfN52135665;     CsqrjohEkQcNojCpGGfN52135665 = CsqrjohEkQcNojCpGGfN63839053;     CsqrjohEkQcNojCpGGfN63839053 = CsqrjohEkQcNojCpGGfN18724169;     CsqrjohEkQcNojCpGGfN18724169 = CsqrjohEkQcNojCpGGfN61400669;     CsqrjohEkQcNojCpGGfN61400669 = CsqrjohEkQcNojCpGGfN42410852;     CsqrjohEkQcNojCpGGfN42410852 = CsqrjohEkQcNojCpGGfN81158525;     CsqrjohEkQcNojCpGGfN81158525 = CsqrjohEkQcNojCpGGfN83208868;     CsqrjohEkQcNojCpGGfN83208868 = CsqrjohEkQcNojCpGGfN17958061;     CsqrjohEkQcNojCpGGfN17958061 = CsqrjohEkQcNojCpGGfN89556293;     CsqrjohEkQcNojCpGGfN89556293 = CsqrjohEkQcNojCpGGfN22400905;     CsqrjohEkQcNojCpGGfN22400905 = CsqrjohEkQcNojCpGGfN44716745;     CsqrjohEkQcNojCpGGfN44716745 = CsqrjohEkQcNojCpGGfN85578047;     CsqrjohEkQcNojCpGGfN85578047 = CsqrjohEkQcNojCpGGfN57544213;     CsqrjohEkQcNojCpGGfN57544213 = CsqrjohEkQcNojCpGGfN74781635;     CsqrjohEkQcNojCpGGfN74781635 = CsqrjohEkQcNojCpGGfN26495341;     CsqrjohEkQcNojCpGGfN26495341 = CsqrjohEkQcNojCpGGfN47479067;     CsqrjohEkQcNojCpGGfN47479067 = CsqrjohEkQcNojCpGGfN92839013;     CsqrjohEkQcNojCpGGfN92839013 = CsqrjohEkQcNojCpGGfN11054195;     CsqrjohEkQcNojCpGGfN11054195 = CsqrjohEkQcNojCpGGfN69862047;     CsqrjohEkQcNojCpGGfN69862047 = CsqrjohEkQcNojCpGGfN44310516;     CsqrjohEkQcNojCpGGfN44310516 = CsqrjohEkQcNojCpGGfN64464259;     CsqrjohEkQcNojCpGGfN64464259 = CsqrjohEkQcNojCpGGfN19065191;     CsqrjohEkQcNojCpGGfN19065191 = CsqrjohEkQcNojCpGGfN4160215;     CsqrjohEkQcNojCpGGfN4160215 = CsqrjohEkQcNojCpGGfN37681950;     CsqrjohEkQcNojCpGGfN37681950 = CsqrjohEkQcNojCpGGfN96066419;     CsqrjohEkQcNojCpGGfN96066419 = CsqrjohEkQcNojCpGGfN96090089;     CsqrjohEkQcNojCpGGfN96090089 = CsqrjohEkQcNojCpGGfN76617898;     CsqrjohEkQcNojCpGGfN76617898 = CsqrjohEkQcNojCpGGfN72655497;     CsqrjohEkQcNojCpGGfN72655497 = CsqrjohEkQcNojCpGGfN3428725;     CsqrjohEkQcNojCpGGfN3428725 = CsqrjohEkQcNojCpGGfN37411576;     CsqrjohEkQcNojCpGGfN37411576 = CsqrjohEkQcNojCpGGfN24265456;     CsqrjohEkQcNojCpGGfN24265456 = CsqrjohEkQcNojCpGGfN26508573;     CsqrjohEkQcNojCpGGfN26508573 = CsqrjohEkQcNojCpGGfN45870800;     CsqrjohEkQcNojCpGGfN45870800 = CsqrjohEkQcNojCpGGfN71722445;     CsqrjohEkQcNojCpGGfN71722445 = CsqrjohEkQcNojCpGGfN39171363;     CsqrjohEkQcNojCpGGfN39171363 = CsqrjohEkQcNojCpGGfN11605849;     CsqrjohEkQcNojCpGGfN11605849 = CsqrjohEkQcNojCpGGfN19030259;     CsqrjohEkQcNojCpGGfN19030259 = CsqrjohEkQcNojCpGGfN65303085;     CsqrjohEkQcNojCpGGfN65303085 = CsqrjohEkQcNojCpGGfN91103050;     CsqrjohEkQcNojCpGGfN91103050 = CsqrjohEkQcNojCpGGfN11087135;     CsqrjohEkQcNojCpGGfN11087135 = CsqrjohEkQcNojCpGGfN35687005;     CsqrjohEkQcNojCpGGfN35687005 = CsqrjohEkQcNojCpGGfN69480966;     CsqrjohEkQcNojCpGGfN69480966 = CsqrjohEkQcNojCpGGfN30559740;     CsqrjohEkQcNojCpGGfN30559740 = CsqrjohEkQcNojCpGGfN54902761;     CsqrjohEkQcNojCpGGfN54902761 = CsqrjohEkQcNojCpGGfN77876434;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void stsZLLSjmUxdFQWnwFJW13212893() {     int cHzgVMwAfbUbpYwgMRuN60502516 = -717171886;    int cHzgVMwAfbUbpYwgMRuN50294009 = -894293211;    int cHzgVMwAfbUbpYwgMRuN61238974 = -181038910;    int cHzgVMwAfbUbpYwgMRuN90867720 = 88777968;    int cHzgVMwAfbUbpYwgMRuN53643807 = -196240155;    int cHzgVMwAfbUbpYwgMRuN98605461 = -798215879;    int cHzgVMwAfbUbpYwgMRuN17496091 = -830551125;    int cHzgVMwAfbUbpYwgMRuN68661556 = -236356934;    int cHzgVMwAfbUbpYwgMRuN95906513 = -291660751;    int cHzgVMwAfbUbpYwgMRuN17294395 = 94120259;    int cHzgVMwAfbUbpYwgMRuN41971698 = -584593718;    int cHzgVMwAfbUbpYwgMRuN56834105 = -492844864;    int cHzgVMwAfbUbpYwgMRuN7517537 = 83294415;    int cHzgVMwAfbUbpYwgMRuN29095281 = -477189278;    int cHzgVMwAfbUbpYwgMRuN61452414 = -992184717;    int cHzgVMwAfbUbpYwgMRuN8361162 = 27980997;    int cHzgVMwAfbUbpYwgMRuN93086522 = -27949917;    int cHzgVMwAfbUbpYwgMRuN31715146 = -749362834;    int cHzgVMwAfbUbpYwgMRuN10114414 = -714982274;    int cHzgVMwAfbUbpYwgMRuN81033254 = -759799969;    int cHzgVMwAfbUbpYwgMRuN44741135 = -423827506;    int cHzgVMwAfbUbpYwgMRuN62597547 = -594875673;    int cHzgVMwAfbUbpYwgMRuN36321269 = -357719331;    int cHzgVMwAfbUbpYwgMRuN85911703 = -982802615;    int cHzgVMwAfbUbpYwgMRuN38827472 = -231824200;    int cHzgVMwAfbUbpYwgMRuN5630250 = -404735587;    int cHzgVMwAfbUbpYwgMRuN22990254 = -265375071;    int cHzgVMwAfbUbpYwgMRuN93382317 = -890698897;    int cHzgVMwAfbUbpYwgMRuN56259927 = -19759336;    int cHzgVMwAfbUbpYwgMRuN71127500 = -260762122;    int cHzgVMwAfbUbpYwgMRuN53840749 = -937596414;    int cHzgVMwAfbUbpYwgMRuN35410237 = -92907142;    int cHzgVMwAfbUbpYwgMRuN48230280 = -308453833;    int cHzgVMwAfbUbpYwgMRuN58800071 = -182241553;    int cHzgVMwAfbUbpYwgMRuN62718425 = -133196110;    int cHzgVMwAfbUbpYwgMRuN2795496 = -585372948;    int cHzgVMwAfbUbpYwgMRuN14786743 = -915174804;    int cHzgVMwAfbUbpYwgMRuN39171343 = -595576546;    int cHzgVMwAfbUbpYwgMRuN34896119 = -817979098;    int cHzgVMwAfbUbpYwgMRuN98308416 = -114232711;    int cHzgVMwAfbUbpYwgMRuN68971792 = -711658803;    int cHzgVMwAfbUbpYwgMRuN75734855 = -67639495;    int cHzgVMwAfbUbpYwgMRuN2456787 = -918821504;    int cHzgVMwAfbUbpYwgMRuN68453540 = -508810;    int cHzgVMwAfbUbpYwgMRuN9065744 = -702458256;    int cHzgVMwAfbUbpYwgMRuN34315472 = -530104832;    int cHzgVMwAfbUbpYwgMRuN58327173 = -549240219;    int cHzgVMwAfbUbpYwgMRuN40827864 = -564368679;    int cHzgVMwAfbUbpYwgMRuN57636734 = -52107097;    int cHzgVMwAfbUbpYwgMRuN66643496 = -678355695;    int cHzgVMwAfbUbpYwgMRuN72259190 = -378611875;    int cHzgVMwAfbUbpYwgMRuN12223470 = -553971677;    int cHzgVMwAfbUbpYwgMRuN81228094 = -430562215;    int cHzgVMwAfbUbpYwgMRuN76455940 = -85201189;    int cHzgVMwAfbUbpYwgMRuN68471758 = -448625384;    int cHzgVMwAfbUbpYwgMRuN97904968 = -22296213;    int cHzgVMwAfbUbpYwgMRuN13972741 = -436573881;    int cHzgVMwAfbUbpYwgMRuN75327270 = -198236296;    int cHzgVMwAfbUbpYwgMRuN52040248 = -679397833;    int cHzgVMwAfbUbpYwgMRuN48013557 = -791504569;    int cHzgVMwAfbUbpYwgMRuN75615207 = -432840808;    int cHzgVMwAfbUbpYwgMRuN24113774 = -939852229;    int cHzgVMwAfbUbpYwgMRuN12401629 = -116597598;    int cHzgVMwAfbUbpYwgMRuN24779013 = 69101371;    int cHzgVMwAfbUbpYwgMRuN63453646 = 31716673;    int cHzgVMwAfbUbpYwgMRuN6561461 = -391686577;    int cHzgVMwAfbUbpYwgMRuN8603825 = -84391032;    int cHzgVMwAfbUbpYwgMRuN48717465 = -734464032;    int cHzgVMwAfbUbpYwgMRuN66376856 = -243993169;    int cHzgVMwAfbUbpYwgMRuN58656919 = -306811769;    int cHzgVMwAfbUbpYwgMRuN93574419 = -56844199;    int cHzgVMwAfbUbpYwgMRuN53915179 = -432373372;    int cHzgVMwAfbUbpYwgMRuN96819026 = -931383737;    int cHzgVMwAfbUbpYwgMRuN11805997 = -500749563;    int cHzgVMwAfbUbpYwgMRuN12061463 = 51858834;    int cHzgVMwAfbUbpYwgMRuN69006279 = -256188011;    int cHzgVMwAfbUbpYwgMRuN60140761 = -676054169;    int cHzgVMwAfbUbpYwgMRuN67867728 = -257210521;    int cHzgVMwAfbUbpYwgMRuN76845960 = -180344359;    int cHzgVMwAfbUbpYwgMRuN4512001 = -701719368;    int cHzgVMwAfbUbpYwgMRuN47303077 = -855495369;    int cHzgVMwAfbUbpYwgMRuN82162389 = -701006392;    int cHzgVMwAfbUbpYwgMRuN35745584 = -738591801;    int cHzgVMwAfbUbpYwgMRuN89616431 = -341403642;    int cHzgVMwAfbUbpYwgMRuN98868310 = -882150247;    int cHzgVMwAfbUbpYwgMRuN41617279 = -283624738;    int cHzgVMwAfbUbpYwgMRuN54182143 = -662344927;    int cHzgVMwAfbUbpYwgMRuN71774340 = -123252644;    int cHzgVMwAfbUbpYwgMRuN90328313 = -733616170;    int cHzgVMwAfbUbpYwgMRuN64813457 = -10899897;    int cHzgVMwAfbUbpYwgMRuN88822755 = -48799068;    int cHzgVMwAfbUbpYwgMRuN39459473 = -616938508;    int cHzgVMwAfbUbpYwgMRuN87131095 = -916178713;    int cHzgVMwAfbUbpYwgMRuN86882562 = 73525470;    int cHzgVMwAfbUbpYwgMRuN22693210 = -681391903;    int cHzgVMwAfbUbpYwgMRuN44858019 = -771806575;    int cHzgVMwAfbUbpYwgMRuN63333226 = -951041898;    int cHzgVMwAfbUbpYwgMRuN77677774 = -887922876;    int cHzgVMwAfbUbpYwgMRuN4999895 = 67774516;    int cHzgVMwAfbUbpYwgMRuN2504283 = -717171886;     cHzgVMwAfbUbpYwgMRuN60502516 = cHzgVMwAfbUbpYwgMRuN50294009;     cHzgVMwAfbUbpYwgMRuN50294009 = cHzgVMwAfbUbpYwgMRuN61238974;     cHzgVMwAfbUbpYwgMRuN61238974 = cHzgVMwAfbUbpYwgMRuN90867720;     cHzgVMwAfbUbpYwgMRuN90867720 = cHzgVMwAfbUbpYwgMRuN53643807;     cHzgVMwAfbUbpYwgMRuN53643807 = cHzgVMwAfbUbpYwgMRuN98605461;     cHzgVMwAfbUbpYwgMRuN98605461 = cHzgVMwAfbUbpYwgMRuN17496091;     cHzgVMwAfbUbpYwgMRuN17496091 = cHzgVMwAfbUbpYwgMRuN68661556;     cHzgVMwAfbUbpYwgMRuN68661556 = cHzgVMwAfbUbpYwgMRuN95906513;     cHzgVMwAfbUbpYwgMRuN95906513 = cHzgVMwAfbUbpYwgMRuN17294395;     cHzgVMwAfbUbpYwgMRuN17294395 = cHzgVMwAfbUbpYwgMRuN41971698;     cHzgVMwAfbUbpYwgMRuN41971698 = cHzgVMwAfbUbpYwgMRuN56834105;     cHzgVMwAfbUbpYwgMRuN56834105 = cHzgVMwAfbUbpYwgMRuN7517537;     cHzgVMwAfbUbpYwgMRuN7517537 = cHzgVMwAfbUbpYwgMRuN29095281;     cHzgVMwAfbUbpYwgMRuN29095281 = cHzgVMwAfbUbpYwgMRuN61452414;     cHzgVMwAfbUbpYwgMRuN61452414 = cHzgVMwAfbUbpYwgMRuN8361162;     cHzgVMwAfbUbpYwgMRuN8361162 = cHzgVMwAfbUbpYwgMRuN93086522;     cHzgVMwAfbUbpYwgMRuN93086522 = cHzgVMwAfbUbpYwgMRuN31715146;     cHzgVMwAfbUbpYwgMRuN31715146 = cHzgVMwAfbUbpYwgMRuN10114414;     cHzgVMwAfbUbpYwgMRuN10114414 = cHzgVMwAfbUbpYwgMRuN81033254;     cHzgVMwAfbUbpYwgMRuN81033254 = cHzgVMwAfbUbpYwgMRuN44741135;     cHzgVMwAfbUbpYwgMRuN44741135 = cHzgVMwAfbUbpYwgMRuN62597547;     cHzgVMwAfbUbpYwgMRuN62597547 = cHzgVMwAfbUbpYwgMRuN36321269;     cHzgVMwAfbUbpYwgMRuN36321269 = cHzgVMwAfbUbpYwgMRuN85911703;     cHzgVMwAfbUbpYwgMRuN85911703 = cHzgVMwAfbUbpYwgMRuN38827472;     cHzgVMwAfbUbpYwgMRuN38827472 = cHzgVMwAfbUbpYwgMRuN5630250;     cHzgVMwAfbUbpYwgMRuN5630250 = cHzgVMwAfbUbpYwgMRuN22990254;     cHzgVMwAfbUbpYwgMRuN22990254 = cHzgVMwAfbUbpYwgMRuN93382317;     cHzgVMwAfbUbpYwgMRuN93382317 = cHzgVMwAfbUbpYwgMRuN56259927;     cHzgVMwAfbUbpYwgMRuN56259927 = cHzgVMwAfbUbpYwgMRuN71127500;     cHzgVMwAfbUbpYwgMRuN71127500 = cHzgVMwAfbUbpYwgMRuN53840749;     cHzgVMwAfbUbpYwgMRuN53840749 = cHzgVMwAfbUbpYwgMRuN35410237;     cHzgVMwAfbUbpYwgMRuN35410237 = cHzgVMwAfbUbpYwgMRuN48230280;     cHzgVMwAfbUbpYwgMRuN48230280 = cHzgVMwAfbUbpYwgMRuN58800071;     cHzgVMwAfbUbpYwgMRuN58800071 = cHzgVMwAfbUbpYwgMRuN62718425;     cHzgVMwAfbUbpYwgMRuN62718425 = cHzgVMwAfbUbpYwgMRuN2795496;     cHzgVMwAfbUbpYwgMRuN2795496 = cHzgVMwAfbUbpYwgMRuN14786743;     cHzgVMwAfbUbpYwgMRuN14786743 = cHzgVMwAfbUbpYwgMRuN39171343;     cHzgVMwAfbUbpYwgMRuN39171343 = cHzgVMwAfbUbpYwgMRuN34896119;     cHzgVMwAfbUbpYwgMRuN34896119 = cHzgVMwAfbUbpYwgMRuN98308416;     cHzgVMwAfbUbpYwgMRuN98308416 = cHzgVMwAfbUbpYwgMRuN68971792;     cHzgVMwAfbUbpYwgMRuN68971792 = cHzgVMwAfbUbpYwgMRuN75734855;     cHzgVMwAfbUbpYwgMRuN75734855 = cHzgVMwAfbUbpYwgMRuN2456787;     cHzgVMwAfbUbpYwgMRuN2456787 = cHzgVMwAfbUbpYwgMRuN68453540;     cHzgVMwAfbUbpYwgMRuN68453540 = cHzgVMwAfbUbpYwgMRuN9065744;     cHzgVMwAfbUbpYwgMRuN9065744 = cHzgVMwAfbUbpYwgMRuN34315472;     cHzgVMwAfbUbpYwgMRuN34315472 = cHzgVMwAfbUbpYwgMRuN58327173;     cHzgVMwAfbUbpYwgMRuN58327173 = cHzgVMwAfbUbpYwgMRuN40827864;     cHzgVMwAfbUbpYwgMRuN40827864 = cHzgVMwAfbUbpYwgMRuN57636734;     cHzgVMwAfbUbpYwgMRuN57636734 = cHzgVMwAfbUbpYwgMRuN66643496;     cHzgVMwAfbUbpYwgMRuN66643496 = cHzgVMwAfbUbpYwgMRuN72259190;     cHzgVMwAfbUbpYwgMRuN72259190 = cHzgVMwAfbUbpYwgMRuN12223470;     cHzgVMwAfbUbpYwgMRuN12223470 = cHzgVMwAfbUbpYwgMRuN81228094;     cHzgVMwAfbUbpYwgMRuN81228094 = cHzgVMwAfbUbpYwgMRuN76455940;     cHzgVMwAfbUbpYwgMRuN76455940 = cHzgVMwAfbUbpYwgMRuN68471758;     cHzgVMwAfbUbpYwgMRuN68471758 = cHzgVMwAfbUbpYwgMRuN97904968;     cHzgVMwAfbUbpYwgMRuN97904968 = cHzgVMwAfbUbpYwgMRuN13972741;     cHzgVMwAfbUbpYwgMRuN13972741 = cHzgVMwAfbUbpYwgMRuN75327270;     cHzgVMwAfbUbpYwgMRuN75327270 = cHzgVMwAfbUbpYwgMRuN52040248;     cHzgVMwAfbUbpYwgMRuN52040248 = cHzgVMwAfbUbpYwgMRuN48013557;     cHzgVMwAfbUbpYwgMRuN48013557 = cHzgVMwAfbUbpYwgMRuN75615207;     cHzgVMwAfbUbpYwgMRuN75615207 = cHzgVMwAfbUbpYwgMRuN24113774;     cHzgVMwAfbUbpYwgMRuN24113774 = cHzgVMwAfbUbpYwgMRuN12401629;     cHzgVMwAfbUbpYwgMRuN12401629 = cHzgVMwAfbUbpYwgMRuN24779013;     cHzgVMwAfbUbpYwgMRuN24779013 = cHzgVMwAfbUbpYwgMRuN63453646;     cHzgVMwAfbUbpYwgMRuN63453646 = cHzgVMwAfbUbpYwgMRuN6561461;     cHzgVMwAfbUbpYwgMRuN6561461 = cHzgVMwAfbUbpYwgMRuN8603825;     cHzgVMwAfbUbpYwgMRuN8603825 = cHzgVMwAfbUbpYwgMRuN48717465;     cHzgVMwAfbUbpYwgMRuN48717465 = cHzgVMwAfbUbpYwgMRuN66376856;     cHzgVMwAfbUbpYwgMRuN66376856 = cHzgVMwAfbUbpYwgMRuN58656919;     cHzgVMwAfbUbpYwgMRuN58656919 = cHzgVMwAfbUbpYwgMRuN93574419;     cHzgVMwAfbUbpYwgMRuN93574419 = cHzgVMwAfbUbpYwgMRuN53915179;     cHzgVMwAfbUbpYwgMRuN53915179 = cHzgVMwAfbUbpYwgMRuN96819026;     cHzgVMwAfbUbpYwgMRuN96819026 = cHzgVMwAfbUbpYwgMRuN11805997;     cHzgVMwAfbUbpYwgMRuN11805997 = cHzgVMwAfbUbpYwgMRuN12061463;     cHzgVMwAfbUbpYwgMRuN12061463 = cHzgVMwAfbUbpYwgMRuN69006279;     cHzgVMwAfbUbpYwgMRuN69006279 = cHzgVMwAfbUbpYwgMRuN60140761;     cHzgVMwAfbUbpYwgMRuN60140761 = cHzgVMwAfbUbpYwgMRuN67867728;     cHzgVMwAfbUbpYwgMRuN67867728 = cHzgVMwAfbUbpYwgMRuN76845960;     cHzgVMwAfbUbpYwgMRuN76845960 = cHzgVMwAfbUbpYwgMRuN4512001;     cHzgVMwAfbUbpYwgMRuN4512001 = cHzgVMwAfbUbpYwgMRuN47303077;     cHzgVMwAfbUbpYwgMRuN47303077 = cHzgVMwAfbUbpYwgMRuN82162389;     cHzgVMwAfbUbpYwgMRuN82162389 = cHzgVMwAfbUbpYwgMRuN35745584;     cHzgVMwAfbUbpYwgMRuN35745584 = cHzgVMwAfbUbpYwgMRuN89616431;     cHzgVMwAfbUbpYwgMRuN89616431 = cHzgVMwAfbUbpYwgMRuN98868310;     cHzgVMwAfbUbpYwgMRuN98868310 = cHzgVMwAfbUbpYwgMRuN41617279;     cHzgVMwAfbUbpYwgMRuN41617279 = cHzgVMwAfbUbpYwgMRuN54182143;     cHzgVMwAfbUbpYwgMRuN54182143 = cHzgVMwAfbUbpYwgMRuN71774340;     cHzgVMwAfbUbpYwgMRuN71774340 = cHzgVMwAfbUbpYwgMRuN90328313;     cHzgVMwAfbUbpYwgMRuN90328313 = cHzgVMwAfbUbpYwgMRuN64813457;     cHzgVMwAfbUbpYwgMRuN64813457 = cHzgVMwAfbUbpYwgMRuN88822755;     cHzgVMwAfbUbpYwgMRuN88822755 = cHzgVMwAfbUbpYwgMRuN39459473;     cHzgVMwAfbUbpYwgMRuN39459473 = cHzgVMwAfbUbpYwgMRuN87131095;     cHzgVMwAfbUbpYwgMRuN87131095 = cHzgVMwAfbUbpYwgMRuN86882562;     cHzgVMwAfbUbpYwgMRuN86882562 = cHzgVMwAfbUbpYwgMRuN22693210;     cHzgVMwAfbUbpYwgMRuN22693210 = cHzgVMwAfbUbpYwgMRuN44858019;     cHzgVMwAfbUbpYwgMRuN44858019 = cHzgVMwAfbUbpYwgMRuN63333226;     cHzgVMwAfbUbpYwgMRuN63333226 = cHzgVMwAfbUbpYwgMRuN77677774;     cHzgVMwAfbUbpYwgMRuN77677774 = cHzgVMwAfbUbpYwgMRuN4999895;     cHzgVMwAfbUbpYwgMRuN4999895 = cHzgVMwAfbUbpYwgMRuN2504283;     cHzgVMwAfbUbpYwgMRuN2504283 = cHzgVMwAfbUbpYwgMRuN60502516;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void McCmvwvdbadZKfxIHfsg71864563() {     int fzfeukRAIjcHRmTZTJqC23933169 = -300945299;    int fzfeukRAIjcHRmTZTJqC75545589 = -654067825;    int fzfeukRAIjcHRmTZTJqC53828844 = -259113370;    int fzfeukRAIjcHRmTZTJqC96141698 = 74798241;    int fzfeukRAIjcHRmTZTJqC94758384 = -557963186;    int fzfeukRAIjcHRmTZTJqC76236689 = -639439108;    int fzfeukRAIjcHRmTZTJqC74926368 = -154877626;    int fzfeukRAIjcHRmTZTJqC21345384 = -554759250;    int fzfeukRAIjcHRmTZTJqC44889433 = -865589891;    int fzfeukRAIjcHRmTZTJqC51512424 = -485726258;    int fzfeukRAIjcHRmTZTJqC9576965 = -22436377;    int fzfeukRAIjcHRmTZTJqC33948794 = -27161631;    int fzfeukRAIjcHRmTZTJqC5685674 = -238350596;    int fzfeukRAIjcHRmTZTJqC63125187 = -110102795;    int fzfeukRAIjcHRmTZTJqC84786607 = -99349558;    int fzfeukRAIjcHRmTZTJqC8555847 = -559218441;    int fzfeukRAIjcHRmTZTJqC56943723 = -259221452;    int fzfeukRAIjcHRmTZTJqC76051129 = -624676354;    int fzfeukRAIjcHRmTZTJqC74990460 = -620539115;    int fzfeukRAIjcHRmTZTJqC76161465 = -74386754;    int fzfeukRAIjcHRmTZTJqC5188355 = -16336484;    int fzfeukRAIjcHRmTZTJqC25460745 = -310233432;    int fzfeukRAIjcHRmTZTJqC67516862 = -303107147;    int fzfeukRAIjcHRmTZTJqC82724102 = -521428459;    int fzfeukRAIjcHRmTZTJqC22262293 = -606433138;    int fzfeukRAIjcHRmTZTJqC47755566 = -758123604;    int fzfeukRAIjcHRmTZTJqC3799302 = -195126895;    int fzfeukRAIjcHRmTZTJqC5780070 = -146013316;    int fzfeukRAIjcHRmTZTJqC53565076 = -552831187;    int fzfeukRAIjcHRmTZTJqC10644276 = 88202515;    int fzfeukRAIjcHRmTZTJqC48768402 = -973159414;    int fzfeukRAIjcHRmTZTJqC66115916 = -692105395;    int fzfeukRAIjcHRmTZTJqC68216588 = -181545984;    int fzfeukRAIjcHRmTZTJqC54503993 = -290888096;    int fzfeukRAIjcHRmTZTJqC59045554 = -676278445;    int fzfeukRAIjcHRmTZTJqC78518264 = -629551611;    int fzfeukRAIjcHRmTZTJqC28835912 = -284856920;    int fzfeukRAIjcHRmTZTJqC78826423 = -954967499;    int fzfeukRAIjcHRmTZTJqC22709398 = 28222929;    int fzfeukRAIjcHRmTZTJqC3388069 = -263325051;    int fzfeukRAIjcHRmTZTJqC23190132 = -540551377;    int fzfeukRAIjcHRmTZTJqC85264305 = -850832730;    int fzfeukRAIjcHRmTZTJqC15857742 = -1122418;    int fzfeukRAIjcHRmTZTJqC47927987 = -548168111;    int fzfeukRAIjcHRmTZTJqC80860216 = -211259193;    int fzfeukRAIjcHRmTZTJqC29282633 = -97453457;    int fzfeukRAIjcHRmTZTJqC82630060 = -26606526;    int fzfeukRAIjcHRmTZTJqC71432964 = -735480012;    int fzfeukRAIjcHRmTZTJqC3977275 = 32121919;    int fzfeukRAIjcHRmTZTJqC19905099 = -324072;    int fzfeukRAIjcHRmTZTJqC10895563 = -46296393;    int fzfeukRAIjcHRmTZTJqC30171039 = -82999683;    int fzfeukRAIjcHRmTZTJqC12078823 = -481722455;    int fzfeukRAIjcHRmTZTJqC65190434 = -328589638;    int fzfeukRAIjcHRmTZTJqC66676208 = -472272672;    int fzfeukRAIjcHRmTZTJqC98472423 = -990711868;    int fzfeukRAIjcHRmTZTJqC8028727 = -250960679;    int fzfeukRAIjcHRmTZTJqC71104741 = -737684911;    int fzfeukRAIjcHRmTZTJqC73879405 = -318768621;    int fzfeukRAIjcHRmTZTJqC47002819 = -799839583;    int fzfeukRAIjcHRmTZTJqC72437388 = -344312214;    int fzfeukRAIjcHRmTZTJqC69146299 = 91135690;    int fzfeukRAIjcHRmTZTJqC67780308 = 98071937;    int fzfeukRAIjcHRmTZTJqC34245157 = -853792407;    int fzfeukRAIjcHRmTZTJqC2744023 = -512566845;    int fzfeukRAIjcHRmTZTJqC43461049 = -330330982;    int fzfeukRAIjcHRmTZTJqC65732205 = -845615647;    int fzfeukRAIjcHRmTZTJqC51181681 = -947462500;    int fzfeukRAIjcHRmTZTJqC4079634 = -433824351;    int fzfeukRAIjcHRmTZTJqC6268344 = -469797947;    int fzfeukRAIjcHRmTZTJqC79719934 = -174361522;    int fzfeukRAIjcHRmTZTJqC78117300 = -304253954;    int fzfeukRAIjcHRmTZTJqC53341732 = -552899284;    int fzfeukRAIjcHRmTZTJqC71602391 = -257214065;    int fzfeukRAIjcHRmTZTJqC52971334 = -533835377;    int fzfeukRAIjcHRmTZTJqC19924050 = -165503755;    int fzfeukRAIjcHRmTZTJqC9603004 = -209111014;    int fzfeukRAIjcHRmTZTJqC19588875 = -754939037;    int fzfeukRAIjcHRmTZTJqC1863887 = -210169267;    int fzfeukRAIjcHRmTZTJqC92979659 = -408979681;    int fzfeukRAIjcHRmTZTJqC65125506 = -631517078;    int fzfeukRAIjcHRmTZTJqC32366338 = -459646883;    int fzfeukRAIjcHRmTZTJqC1802795 = -78135236;    int fzfeukRAIjcHRmTZTJqC33659977 = -452507115;    int fzfeukRAIjcHRmTZTJqC99748712 = -865501093;    int fzfeukRAIjcHRmTZTJqC18597364 = -790159731;    int fzfeukRAIjcHRmTZTJqC54037093 = -110382940;    int fzfeukRAIjcHRmTZTJqC3026154 = -852956347;    int fzfeukRAIjcHRmTZTJqC87827784 = -818615425;    int fzfeukRAIjcHRmTZTJqC60573130 = -685566577;    int fzfeukRAIjcHRmTZTJqC70489537 = -278590932;    int fzfeukRAIjcHRmTZTJqC57731171 = -547172009;    int fzfeukRAIjcHRmTZTJqC4947018 = -536198878;    int fzfeukRAIjcHRmTZTJqC75706578 = -171937488;    int fzfeukRAIjcHRmTZTJqC30950681 = -919012837;    int fzfeukRAIjcHRmTZTJqC54043832 = -531687068;    int fzfeukRAIjcHRmTZTJqC17483997 = -848904667;    int fzfeukRAIjcHRmTZTJqC81612584 = -147330012;    int fzfeukRAIjcHRmTZTJqC45183965 = 64398734;    int fzfeukRAIjcHRmTZTJqC37399168 = -300945299;     fzfeukRAIjcHRmTZTJqC23933169 = fzfeukRAIjcHRmTZTJqC75545589;     fzfeukRAIjcHRmTZTJqC75545589 = fzfeukRAIjcHRmTZTJqC53828844;     fzfeukRAIjcHRmTZTJqC53828844 = fzfeukRAIjcHRmTZTJqC96141698;     fzfeukRAIjcHRmTZTJqC96141698 = fzfeukRAIjcHRmTZTJqC94758384;     fzfeukRAIjcHRmTZTJqC94758384 = fzfeukRAIjcHRmTZTJqC76236689;     fzfeukRAIjcHRmTZTJqC76236689 = fzfeukRAIjcHRmTZTJqC74926368;     fzfeukRAIjcHRmTZTJqC74926368 = fzfeukRAIjcHRmTZTJqC21345384;     fzfeukRAIjcHRmTZTJqC21345384 = fzfeukRAIjcHRmTZTJqC44889433;     fzfeukRAIjcHRmTZTJqC44889433 = fzfeukRAIjcHRmTZTJqC51512424;     fzfeukRAIjcHRmTZTJqC51512424 = fzfeukRAIjcHRmTZTJqC9576965;     fzfeukRAIjcHRmTZTJqC9576965 = fzfeukRAIjcHRmTZTJqC33948794;     fzfeukRAIjcHRmTZTJqC33948794 = fzfeukRAIjcHRmTZTJqC5685674;     fzfeukRAIjcHRmTZTJqC5685674 = fzfeukRAIjcHRmTZTJqC63125187;     fzfeukRAIjcHRmTZTJqC63125187 = fzfeukRAIjcHRmTZTJqC84786607;     fzfeukRAIjcHRmTZTJqC84786607 = fzfeukRAIjcHRmTZTJqC8555847;     fzfeukRAIjcHRmTZTJqC8555847 = fzfeukRAIjcHRmTZTJqC56943723;     fzfeukRAIjcHRmTZTJqC56943723 = fzfeukRAIjcHRmTZTJqC76051129;     fzfeukRAIjcHRmTZTJqC76051129 = fzfeukRAIjcHRmTZTJqC74990460;     fzfeukRAIjcHRmTZTJqC74990460 = fzfeukRAIjcHRmTZTJqC76161465;     fzfeukRAIjcHRmTZTJqC76161465 = fzfeukRAIjcHRmTZTJqC5188355;     fzfeukRAIjcHRmTZTJqC5188355 = fzfeukRAIjcHRmTZTJqC25460745;     fzfeukRAIjcHRmTZTJqC25460745 = fzfeukRAIjcHRmTZTJqC67516862;     fzfeukRAIjcHRmTZTJqC67516862 = fzfeukRAIjcHRmTZTJqC82724102;     fzfeukRAIjcHRmTZTJqC82724102 = fzfeukRAIjcHRmTZTJqC22262293;     fzfeukRAIjcHRmTZTJqC22262293 = fzfeukRAIjcHRmTZTJqC47755566;     fzfeukRAIjcHRmTZTJqC47755566 = fzfeukRAIjcHRmTZTJqC3799302;     fzfeukRAIjcHRmTZTJqC3799302 = fzfeukRAIjcHRmTZTJqC5780070;     fzfeukRAIjcHRmTZTJqC5780070 = fzfeukRAIjcHRmTZTJqC53565076;     fzfeukRAIjcHRmTZTJqC53565076 = fzfeukRAIjcHRmTZTJqC10644276;     fzfeukRAIjcHRmTZTJqC10644276 = fzfeukRAIjcHRmTZTJqC48768402;     fzfeukRAIjcHRmTZTJqC48768402 = fzfeukRAIjcHRmTZTJqC66115916;     fzfeukRAIjcHRmTZTJqC66115916 = fzfeukRAIjcHRmTZTJqC68216588;     fzfeukRAIjcHRmTZTJqC68216588 = fzfeukRAIjcHRmTZTJqC54503993;     fzfeukRAIjcHRmTZTJqC54503993 = fzfeukRAIjcHRmTZTJqC59045554;     fzfeukRAIjcHRmTZTJqC59045554 = fzfeukRAIjcHRmTZTJqC78518264;     fzfeukRAIjcHRmTZTJqC78518264 = fzfeukRAIjcHRmTZTJqC28835912;     fzfeukRAIjcHRmTZTJqC28835912 = fzfeukRAIjcHRmTZTJqC78826423;     fzfeukRAIjcHRmTZTJqC78826423 = fzfeukRAIjcHRmTZTJqC22709398;     fzfeukRAIjcHRmTZTJqC22709398 = fzfeukRAIjcHRmTZTJqC3388069;     fzfeukRAIjcHRmTZTJqC3388069 = fzfeukRAIjcHRmTZTJqC23190132;     fzfeukRAIjcHRmTZTJqC23190132 = fzfeukRAIjcHRmTZTJqC85264305;     fzfeukRAIjcHRmTZTJqC85264305 = fzfeukRAIjcHRmTZTJqC15857742;     fzfeukRAIjcHRmTZTJqC15857742 = fzfeukRAIjcHRmTZTJqC47927987;     fzfeukRAIjcHRmTZTJqC47927987 = fzfeukRAIjcHRmTZTJqC80860216;     fzfeukRAIjcHRmTZTJqC80860216 = fzfeukRAIjcHRmTZTJqC29282633;     fzfeukRAIjcHRmTZTJqC29282633 = fzfeukRAIjcHRmTZTJqC82630060;     fzfeukRAIjcHRmTZTJqC82630060 = fzfeukRAIjcHRmTZTJqC71432964;     fzfeukRAIjcHRmTZTJqC71432964 = fzfeukRAIjcHRmTZTJqC3977275;     fzfeukRAIjcHRmTZTJqC3977275 = fzfeukRAIjcHRmTZTJqC19905099;     fzfeukRAIjcHRmTZTJqC19905099 = fzfeukRAIjcHRmTZTJqC10895563;     fzfeukRAIjcHRmTZTJqC10895563 = fzfeukRAIjcHRmTZTJqC30171039;     fzfeukRAIjcHRmTZTJqC30171039 = fzfeukRAIjcHRmTZTJqC12078823;     fzfeukRAIjcHRmTZTJqC12078823 = fzfeukRAIjcHRmTZTJqC65190434;     fzfeukRAIjcHRmTZTJqC65190434 = fzfeukRAIjcHRmTZTJqC66676208;     fzfeukRAIjcHRmTZTJqC66676208 = fzfeukRAIjcHRmTZTJqC98472423;     fzfeukRAIjcHRmTZTJqC98472423 = fzfeukRAIjcHRmTZTJqC8028727;     fzfeukRAIjcHRmTZTJqC8028727 = fzfeukRAIjcHRmTZTJqC71104741;     fzfeukRAIjcHRmTZTJqC71104741 = fzfeukRAIjcHRmTZTJqC73879405;     fzfeukRAIjcHRmTZTJqC73879405 = fzfeukRAIjcHRmTZTJqC47002819;     fzfeukRAIjcHRmTZTJqC47002819 = fzfeukRAIjcHRmTZTJqC72437388;     fzfeukRAIjcHRmTZTJqC72437388 = fzfeukRAIjcHRmTZTJqC69146299;     fzfeukRAIjcHRmTZTJqC69146299 = fzfeukRAIjcHRmTZTJqC67780308;     fzfeukRAIjcHRmTZTJqC67780308 = fzfeukRAIjcHRmTZTJqC34245157;     fzfeukRAIjcHRmTZTJqC34245157 = fzfeukRAIjcHRmTZTJqC2744023;     fzfeukRAIjcHRmTZTJqC2744023 = fzfeukRAIjcHRmTZTJqC43461049;     fzfeukRAIjcHRmTZTJqC43461049 = fzfeukRAIjcHRmTZTJqC65732205;     fzfeukRAIjcHRmTZTJqC65732205 = fzfeukRAIjcHRmTZTJqC51181681;     fzfeukRAIjcHRmTZTJqC51181681 = fzfeukRAIjcHRmTZTJqC4079634;     fzfeukRAIjcHRmTZTJqC4079634 = fzfeukRAIjcHRmTZTJqC6268344;     fzfeukRAIjcHRmTZTJqC6268344 = fzfeukRAIjcHRmTZTJqC79719934;     fzfeukRAIjcHRmTZTJqC79719934 = fzfeukRAIjcHRmTZTJqC78117300;     fzfeukRAIjcHRmTZTJqC78117300 = fzfeukRAIjcHRmTZTJqC53341732;     fzfeukRAIjcHRmTZTJqC53341732 = fzfeukRAIjcHRmTZTJqC71602391;     fzfeukRAIjcHRmTZTJqC71602391 = fzfeukRAIjcHRmTZTJqC52971334;     fzfeukRAIjcHRmTZTJqC52971334 = fzfeukRAIjcHRmTZTJqC19924050;     fzfeukRAIjcHRmTZTJqC19924050 = fzfeukRAIjcHRmTZTJqC9603004;     fzfeukRAIjcHRmTZTJqC9603004 = fzfeukRAIjcHRmTZTJqC19588875;     fzfeukRAIjcHRmTZTJqC19588875 = fzfeukRAIjcHRmTZTJqC1863887;     fzfeukRAIjcHRmTZTJqC1863887 = fzfeukRAIjcHRmTZTJqC92979659;     fzfeukRAIjcHRmTZTJqC92979659 = fzfeukRAIjcHRmTZTJqC65125506;     fzfeukRAIjcHRmTZTJqC65125506 = fzfeukRAIjcHRmTZTJqC32366338;     fzfeukRAIjcHRmTZTJqC32366338 = fzfeukRAIjcHRmTZTJqC1802795;     fzfeukRAIjcHRmTZTJqC1802795 = fzfeukRAIjcHRmTZTJqC33659977;     fzfeukRAIjcHRmTZTJqC33659977 = fzfeukRAIjcHRmTZTJqC99748712;     fzfeukRAIjcHRmTZTJqC99748712 = fzfeukRAIjcHRmTZTJqC18597364;     fzfeukRAIjcHRmTZTJqC18597364 = fzfeukRAIjcHRmTZTJqC54037093;     fzfeukRAIjcHRmTZTJqC54037093 = fzfeukRAIjcHRmTZTJqC3026154;     fzfeukRAIjcHRmTZTJqC3026154 = fzfeukRAIjcHRmTZTJqC87827784;     fzfeukRAIjcHRmTZTJqC87827784 = fzfeukRAIjcHRmTZTJqC60573130;     fzfeukRAIjcHRmTZTJqC60573130 = fzfeukRAIjcHRmTZTJqC70489537;     fzfeukRAIjcHRmTZTJqC70489537 = fzfeukRAIjcHRmTZTJqC57731171;     fzfeukRAIjcHRmTZTJqC57731171 = fzfeukRAIjcHRmTZTJqC4947018;     fzfeukRAIjcHRmTZTJqC4947018 = fzfeukRAIjcHRmTZTJqC75706578;     fzfeukRAIjcHRmTZTJqC75706578 = fzfeukRAIjcHRmTZTJqC30950681;     fzfeukRAIjcHRmTZTJqC30950681 = fzfeukRAIjcHRmTZTJqC54043832;     fzfeukRAIjcHRmTZTJqC54043832 = fzfeukRAIjcHRmTZTJqC17483997;     fzfeukRAIjcHRmTZTJqC17483997 = fzfeukRAIjcHRmTZTJqC81612584;     fzfeukRAIjcHRmTZTJqC81612584 = fzfeukRAIjcHRmTZTJqC45183965;     fzfeukRAIjcHRmTZTJqC45183965 = fzfeukRAIjcHRmTZTJqC37399168;     fzfeukRAIjcHRmTZTJqC37399168 = fzfeukRAIjcHRmTZTJqC23933169;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void XfzAuseFwZnYNkoKMukq30516234() {     int ABmuzaTsRrfsuxowtWeX87363821 = -984718712;    int ABmuzaTsRrfsuxowtWeX797169 = -413842440;    int ABmuzaTsRrfsuxowtWeX46418714 = -337187830;    int ABmuzaTsRrfsuxowtWeX1415677 = 60818515;    int ABmuzaTsRrfsuxowtWeX35872963 = -919686218;    int ABmuzaTsRrfsuxowtWeX53867918 = -480662338;    int ABmuzaTsRrfsuxowtWeX32356646 = -579204126;    int ABmuzaTsRrfsuxowtWeX74029212 = -873161565;    int ABmuzaTsRrfsuxowtWeX93872352 = -339519032;    int ABmuzaTsRrfsuxowtWeX85730453 = 34427224;    int ABmuzaTsRrfsuxowtWeX77182232 = -560279035;    int ABmuzaTsRrfsuxowtWeX11063483 = -661478397;    int ABmuzaTsRrfsuxowtWeX3853811 = -559995606;    int ABmuzaTsRrfsuxowtWeX97155093 = -843016312;    int ABmuzaTsRrfsuxowtWeX8120802 = -306514398;    int ABmuzaTsRrfsuxowtWeX8750531 = -46417879;    int ABmuzaTsRrfsuxowtWeX20800924 = -490492986;    int ABmuzaTsRrfsuxowtWeX20387112 = -499989874;    int ABmuzaTsRrfsuxowtWeX39866506 = -526095957;    int ABmuzaTsRrfsuxowtWeX71289676 = -488973539;    int ABmuzaTsRrfsuxowtWeX65635575 = -708845463;    int ABmuzaTsRrfsuxowtWeX88323942 = -25591191;    int ABmuzaTsRrfsuxowtWeX98712454 = -248494962;    int ABmuzaTsRrfsuxowtWeX79536502 = -60054304;    int ABmuzaTsRrfsuxowtWeX5697114 = -981042076;    int ABmuzaTsRrfsuxowtWeX89880882 = -11511620;    int ABmuzaTsRrfsuxowtWeX84608349 = -124878719;    int ABmuzaTsRrfsuxowtWeX18177821 = -501327735;    int ABmuzaTsRrfsuxowtWeX50870224 = 14096963;    int ABmuzaTsRrfsuxowtWeX50161051 = -662832848;    int ABmuzaTsRrfsuxowtWeX43696055 = 91277586;    int ABmuzaTsRrfsuxowtWeX96821594 = -191303648;    int ABmuzaTsRrfsuxowtWeX88202896 = -54638135;    int ABmuzaTsRrfsuxowtWeX50207915 = -399534640;    int ABmuzaTsRrfsuxowtWeX55372682 = -119360779;    int ABmuzaTsRrfsuxowtWeX54241033 = -673730273;    int ABmuzaTsRrfsuxowtWeX42885081 = -754539035;    int ABmuzaTsRrfsuxowtWeX18481504 = -214358451;    int ABmuzaTsRrfsuxowtWeX10522676 = -225575044;    int ABmuzaTsRrfsuxowtWeX8467721 = -412417390;    int ABmuzaTsRrfsuxowtWeX77408471 = -369443952;    int ABmuzaTsRrfsuxowtWeX94793754 = -534025965;    int ABmuzaTsRrfsuxowtWeX29258697 = -183423332;    int ABmuzaTsRrfsuxowtWeX27402433 = 4172589;    int ABmuzaTsRrfsuxowtWeX52654690 = -820060130;    int ABmuzaTsRrfsuxowtWeX24249794 = -764802082;    int ABmuzaTsRrfsuxowtWeX6932947 = -603972833;    int ABmuzaTsRrfsuxowtWeX2038064 = -906591346;    int ABmuzaTsRrfsuxowtWeX50317815 = -983649065;    int ABmuzaTsRrfsuxowtWeX73166702 = -422292449;    int ABmuzaTsRrfsuxowtWeX49531935 = -813980911;    int ABmuzaTsRrfsuxowtWeX48118607 = -712027689;    int ABmuzaTsRrfsuxowtWeX42929552 = -532882695;    int ABmuzaTsRrfsuxowtWeX53924929 = -571978086;    int ABmuzaTsRrfsuxowtWeX64880659 = -495919961;    int ABmuzaTsRrfsuxowtWeX99039878 = -859127522;    int ABmuzaTsRrfsuxowtWeX2084714 = -65347478;    int ABmuzaTsRrfsuxowtWeX66882212 = -177133526;    int ABmuzaTsRrfsuxowtWeX95718563 = 41860590;    int ABmuzaTsRrfsuxowtWeX45992081 = -808174598;    int ABmuzaTsRrfsuxowtWeX69259569 = -255783620;    int ABmuzaTsRrfsuxowtWeX14178826 = 22123609;    int ABmuzaTsRrfsuxowtWeX23158988 = -787258529;    int ABmuzaTsRrfsuxowtWeX43711302 = -676686184;    int ABmuzaTsRrfsuxowtWeX42034399 = 43149638;    int ABmuzaTsRrfsuxowtWeX80360637 = -268975388;    int ABmuzaTsRrfsuxowtWeX22860586 = -506840263;    int ABmuzaTsRrfsuxowtWeX53645896 = -60460967;    int ABmuzaTsRrfsuxowtWeX41782412 = -623655534;    int ABmuzaTsRrfsuxowtWeX53879768 = -632784126;    int ABmuzaTsRrfsuxowtWeX65865449 = -291878845;    int ABmuzaTsRrfsuxowtWeX2319421 = -176134535;    int ABmuzaTsRrfsuxowtWeX9864437 = -174414830;    int ABmuzaTsRrfsuxowtWeX31398785 = -13678568;    int ABmuzaTsRrfsuxowtWeX93881205 = -19529588;    int ABmuzaTsRrfsuxowtWeX70841820 = -74819498;    int ABmuzaTsRrfsuxowtWeX59065245 = -842167859;    int ABmuzaTsRrfsuxowtWeX71310022 = -152667552;    int ABmuzaTsRrfsuxowtWeX26881813 = -239994175;    int ABmuzaTsRrfsuxowtWeX81447319 = -116239994;    int ABmuzaTsRrfsuxowtWeX82947935 = -407538787;    int ABmuzaTsRrfsuxowtWeX82570285 = -218287373;    int ABmuzaTsRrfsuxowtWeX67860006 = -517678670;    int ABmuzaTsRrfsuxowtWeX77703521 = -563610588;    int ABmuzaTsRrfsuxowtWeX629116 = -848851938;    int ABmuzaTsRrfsuxowtWeX95577447 = -196694725;    int ABmuzaTsRrfsuxowtWeX53892043 = -658420953;    int ABmuzaTsRrfsuxowtWeX34277967 = -482660050;    int ABmuzaTsRrfsuxowtWeX85327256 = -903614679;    int ABmuzaTsRrfsuxowtWeX56332803 = -260233258;    int ABmuzaTsRrfsuxowtWeX52156320 = -508382796;    int ABmuzaTsRrfsuxowtWeX76002869 = -477405510;    int ABmuzaTsRrfsuxowtWeX22762941 = -156219042;    int ABmuzaTsRrfsuxowtWeX64530594 = -417400447;    int ABmuzaTsRrfsuxowtWeX39208152 = -56633770;    int ABmuzaTsRrfsuxowtWeX63229646 = -291567561;    int ABmuzaTsRrfsuxowtWeX71634767 = -746767437;    int ABmuzaTsRrfsuxowtWeX85547395 = -506737148;    int ABmuzaTsRrfsuxowtWeX85368034 = 61022951;    int ABmuzaTsRrfsuxowtWeX72294052 = -984718712;     ABmuzaTsRrfsuxowtWeX87363821 = ABmuzaTsRrfsuxowtWeX797169;     ABmuzaTsRrfsuxowtWeX797169 = ABmuzaTsRrfsuxowtWeX46418714;     ABmuzaTsRrfsuxowtWeX46418714 = ABmuzaTsRrfsuxowtWeX1415677;     ABmuzaTsRrfsuxowtWeX1415677 = ABmuzaTsRrfsuxowtWeX35872963;     ABmuzaTsRrfsuxowtWeX35872963 = ABmuzaTsRrfsuxowtWeX53867918;     ABmuzaTsRrfsuxowtWeX53867918 = ABmuzaTsRrfsuxowtWeX32356646;     ABmuzaTsRrfsuxowtWeX32356646 = ABmuzaTsRrfsuxowtWeX74029212;     ABmuzaTsRrfsuxowtWeX74029212 = ABmuzaTsRrfsuxowtWeX93872352;     ABmuzaTsRrfsuxowtWeX93872352 = ABmuzaTsRrfsuxowtWeX85730453;     ABmuzaTsRrfsuxowtWeX85730453 = ABmuzaTsRrfsuxowtWeX77182232;     ABmuzaTsRrfsuxowtWeX77182232 = ABmuzaTsRrfsuxowtWeX11063483;     ABmuzaTsRrfsuxowtWeX11063483 = ABmuzaTsRrfsuxowtWeX3853811;     ABmuzaTsRrfsuxowtWeX3853811 = ABmuzaTsRrfsuxowtWeX97155093;     ABmuzaTsRrfsuxowtWeX97155093 = ABmuzaTsRrfsuxowtWeX8120802;     ABmuzaTsRrfsuxowtWeX8120802 = ABmuzaTsRrfsuxowtWeX8750531;     ABmuzaTsRrfsuxowtWeX8750531 = ABmuzaTsRrfsuxowtWeX20800924;     ABmuzaTsRrfsuxowtWeX20800924 = ABmuzaTsRrfsuxowtWeX20387112;     ABmuzaTsRrfsuxowtWeX20387112 = ABmuzaTsRrfsuxowtWeX39866506;     ABmuzaTsRrfsuxowtWeX39866506 = ABmuzaTsRrfsuxowtWeX71289676;     ABmuzaTsRrfsuxowtWeX71289676 = ABmuzaTsRrfsuxowtWeX65635575;     ABmuzaTsRrfsuxowtWeX65635575 = ABmuzaTsRrfsuxowtWeX88323942;     ABmuzaTsRrfsuxowtWeX88323942 = ABmuzaTsRrfsuxowtWeX98712454;     ABmuzaTsRrfsuxowtWeX98712454 = ABmuzaTsRrfsuxowtWeX79536502;     ABmuzaTsRrfsuxowtWeX79536502 = ABmuzaTsRrfsuxowtWeX5697114;     ABmuzaTsRrfsuxowtWeX5697114 = ABmuzaTsRrfsuxowtWeX89880882;     ABmuzaTsRrfsuxowtWeX89880882 = ABmuzaTsRrfsuxowtWeX84608349;     ABmuzaTsRrfsuxowtWeX84608349 = ABmuzaTsRrfsuxowtWeX18177821;     ABmuzaTsRrfsuxowtWeX18177821 = ABmuzaTsRrfsuxowtWeX50870224;     ABmuzaTsRrfsuxowtWeX50870224 = ABmuzaTsRrfsuxowtWeX50161051;     ABmuzaTsRrfsuxowtWeX50161051 = ABmuzaTsRrfsuxowtWeX43696055;     ABmuzaTsRrfsuxowtWeX43696055 = ABmuzaTsRrfsuxowtWeX96821594;     ABmuzaTsRrfsuxowtWeX96821594 = ABmuzaTsRrfsuxowtWeX88202896;     ABmuzaTsRrfsuxowtWeX88202896 = ABmuzaTsRrfsuxowtWeX50207915;     ABmuzaTsRrfsuxowtWeX50207915 = ABmuzaTsRrfsuxowtWeX55372682;     ABmuzaTsRrfsuxowtWeX55372682 = ABmuzaTsRrfsuxowtWeX54241033;     ABmuzaTsRrfsuxowtWeX54241033 = ABmuzaTsRrfsuxowtWeX42885081;     ABmuzaTsRrfsuxowtWeX42885081 = ABmuzaTsRrfsuxowtWeX18481504;     ABmuzaTsRrfsuxowtWeX18481504 = ABmuzaTsRrfsuxowtWeX10522676;     ABmuzaTsRrfsuxowtWeX10522676 = ABmuzaTsRrfsuxowtWeX8467721;     ABmuzaTsRrfsuxowtWeX8467721 = ABmuzaTsRrfsuxowtWeX77408471;     ABmuzaTsRrfsuxowtWeX77408471 = ABmuzaTsRrfsuxowtWeX94793754;     ABmuzaTsRrfsuxowtWeX94793754 = ABmuzaTsRrfsuxowtWeX29258697;     ABmuzaTsRrfsuxowtWeX29258697 = ABmuzaTsRrfsuxowtWeX27402433;     ABmuzaTsRrfsuxowtWeX27402433 = ABmuzaTsRrfsuxowtWeX52654690;     ABmuzaTsRrfsuxowtWeX52654690 = ABmuzaTsRrfsuxowtWeX24249794;     ABmuzaTsRrfsuxowtWeX24249794 = ABmuzaTsRrfsuxowtWeX6932947;     ABmuzaTsRrfsuxowtWeX6932947 = ABmuzaTsRrfsuxowtWeX2038064;     ABmuzaTsRrfsuxowtWeX2038064 = ABmuzaTsRrfsuxowtWeX50317815;     ABmuzaTsRrfsuxowtWeX50317815 = ABmuzaTsRrfsuxowtWeX73166702;     ABmuzaTsRrfsuxowtWeX73166702 = ABmuzaTsRrfsuxowtWeX49531935;     ABmuzaTsRrfsuxowtWeX49531935 = ABmuzaTsRrfsuxowtWeX48118607;     ABmuzaTsRrfsuxowtWeX48118607 = ABmuzaTsRrfsuxowtWeX42929552;     ABmuzaTsRrfsuxowtWeX42929552 = ABmuzaTsRrfsuxowtWeX53924929;     ABmuzaTsRrfsuxowtWeX53924929 = ABmuzaTsRrfsuxowtWeX64880659;     ABmuzaTsRrfsuxowtWeX64880659 = ABmuzaTsRrfsuxowtWeX99039878;     ABmuzaTsRrfsuxowtWeX99039878 = ABmuzaTsRrfsuxowtWeX2084714;     ABmuzaTsRrfsuxowtWeX2084714 = ABmuzaTsRrfsuxowtWeX66882212;     ABmuzaTsRrfsuxowtWeX66882212 = ABmuzaTsRrfsuxowtWeX95718563;     ABmuzaTsRrfsuxowtWeX95718563 = ABmuzaTsRrfsuxowtWeX45992081;     ABmuzaTsRrfsuxowtWeX45992081 = ABmuzaTsRrfsuxowtWeX69259569;     ABmuzaTsRrfsuxowtWeX69259569 = ABmuzaTsRrfsuxowtWeX14178826;     ABmuzaTsRrfsuxowtWeX14178826 = ABmuzaTsRrfsuxowtWeX23158988;     ABmuzaTsRrfsuxowtWeX23158988 = ABmuzaTsRrfsuxowtWeX43711302;     ABmuzaTsRrfsuxowtWeX43711302 = ABmuzaTsRrfsuxowtWeX42034399;     ABmuzaTsRrfsuxowtWeX42034399 = ABmuzaTsRrfsuxowtWeX80360637;     ABmuzaTsRrfsuxowtWeX80360637 = ABmuzaTsRrfsuxowtWeX22860586;     ABmuzaTsRrfsuxowtWeX22860586 = ABmuzaTsRrfsuxowtWeX53645896;     ABmuzaTsRrfsuxowtWeX53645896 = ABmuzaTsRrfsuxowtWeX41782412;     ABmuzaTsRrfsuxowtWeX41782412 = ABmuzaTsRrfsuxowtWeX53879768;     ABmuzaTsRrfsuxowtWeX53879768 = ABmuzaTsRrfsuxowtWeX65865449;     ABmuzaTsRrfsuxowtWeX65865449 = ABmuzaTsRrfsuxowtWeX2319421;     ABmuzaTsRrfsuxowtWeX2319421 = ABmuzaTsRrfsuxowtWeX9864437;     ABmuzaTsRrfsuxowtWeX9864437 = ABmuzaTsRrfsuxowtWeX31398785;     ABmuzaTsRrfsuxowtWeX31398785 = ABmuzaTsRrfsuxowtWeX93881205;     ABmuzaTsRrfsuxowtWeX93881205 = ABmuzaTsRrfsuxowtWeX70841820;     ABmuzaTsRrfsuxowtWeX70841820 = ABmuzaTsRrfsuxowtWeX59065245;     ABmuzaTsRrfsuxowtWeX59065245 = ABmuzaTsRrfsuxowtWeX71310022;     ABmuzaTsRrfsuxowtWeX71310022 = ABmuzaTsRrfsuxowtWeX26881813;     ABmuzaTsRrfsuxowtWeX26881813 = ABmuzaTsRrfsuxowtWeX81447319;     ABmuzaTsRrfsuxowtWeX81447319 = ABmuzaTsRrfsuxowtWeX82947935;     ABmuzaTsRrfsuxowtWeX82947935 = ABmuzaTsRrfsuxowtWeX82570285;     ABmuzaTsRrfsuxowtWeX82570285 = ABmuzaTsRrfsuxowtWeX67860006;     ABmuzaTsRrfsuxowtWeX67860006 = ABmuzaTsRrfsuxowtWeX77703521;     ABmuzaTsRrfsuxowtWeX77703521 = ABmuzaTsRrfsuxowtWeX629116;     ABmuzaTsRrfsuxowtWeX629116 = ABmuzaTsRrfsuxowtWeX95577447;     ABmuzaTsRrfsuxowtWeX95577447 = ABmuzaTsRrfsuxowtWeX53892043;     ABmuzaTsRrfsuxowtWeX53892043 = ABmuzaTsRrfsuxowtWeX34277967;     ABmuzaTsRrfsuxowtWeX34277967 = ABmuzaTsRrfsuxowtWeX85327256;     ABmuzaTsRrfsuxowtWeX85327256 = ABmuzaTsRrfsuxowtWeX56332803;     ABmuzaTsRrfsuxowtWeX56332803 = ABmuzaTsRrfsuxowtWeX52156320;     ABmuzaTsRrfsuxowtWeX52156320 = ABmuzaTsRrfsuxowtWeX76002869;     ABmuzaTsRrfsuxowtWeX76002869 = ABmuzaTsRrfsuxowtWeX22762941;     ABmuzaTsRrfsuxowtWeX22762941 = ABmuzaTsRrfsuxowtWeX64530594;     ABmuzaTsRrfsuxowtWeX64530594 = ABmuzaTsRrfsuxowtWeX39208152;     ABmuzaTsRrfsuxowtWeX39208152 = ABmuzaTsRrfsuxowtWeX63229646;     ABmuzaTsRrfsuxowtWeX63229646 = ABmuzaTsRrfsuxowtWeX71634767;     ABmuzaTsRrfsuxowtWeX71634767 = ABmuzaTsRrfsuxowtWeX85547395;     ABmuzaTsRrfsuxowtWeX85547395 = ABmuzaTsRrfsuxowtWeX85368034;     ABmuzaTsRrfsuxowtWeX85368034 = ABmuzaTsRrfsuxowtWeX72294052;     ABmuzaTsRrfsuxowtWeX72294052 = ABmuzaTsRrfsuxowtWeX87363821;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void afgPBapqSysfSLJFfTsN36925374() {     int FnHlxmFTvfSfiTZPCbxS21452612 = -214117114;    int FnHlxmFTvfSfiTZPCbxS56149948 = -2665228;    int FnHlxmFTvfSfiTZPCbxS17664292 = -900428136;    int FnHlxmFTvfSfiTZPCbxS61541081 = -322758771;    int FnHlxmFTvfSfiTZPCbxS89597760 = -596460275;    int FnHlxmFTvfSfiTZPCbxS86693476 = -324033907;    int FnHlxmFTvfSfiTZPCbxS39618483 = -197769869;    int FnHlxmFTvfSfiTZPCbxS14341370 = -734203654;    int FnHlxmFTvfSfiTZPCbxS5980163 = -193149432;    int FnHlxmFTvfSfiTZPCbxS46275430 = -109707773;    int FnHlxmFTvfSfiTZPCbxS66716054 = -73551551;    int FnHlxmFTvfSfiTZPCbxS19433066 = -776080594;    int FnHlxmFTvfSfiTZPCbxS49996420 = -769519200;    int FnHlxmFTvfSfiTZPCbxS61187814 = 68195012;    int FnHlxmFTvfSfiTZPCbxS69002905 = -700716391;    int FnHlxmFTvfSfiTZPCbxS66909308 = -970805254;    int FnHlxmFTvfSfiTZPCbxS22658193 = -811335081;    int FnHlxmFTvfSfiTZPCbxS62016056 = 32602032;    int FnHlxmFTvfSfiTZPCbxS66022064 = -520858474;    int FnHlxmFTvfSfiTZPCbxS53130833 = -833902177;    int FnHlxmFTvfSfiTZPCbxS1549126 = -499567208;    int FnHlxmFTvfSfiTZPCbxS34286639 = -662242372;    int FnHlxmFTvfSfiTZPCbxS94973460 = -302768081;    int FnHlxmFTvfSfiTZPCbxS66548330 = -410160533;    int FnHlxmFTvfSfiTZPCbxS24813633 = -604861849;    int FnHlxmFTvfSfiTZPCbxS46029702 = -554978153;    int FnHlxmFTvfSfiTZPCbxS3456601 = -149008605;    int FnHlxmFTvfSfiTZPCbxS2954273 = -752798934;    int FnHlxmFTvfSfiTZPCbxS67639413 = -167262789;    int FnHlxmFTvfSfiTZPCbxS52760052 = -96831909;    int FnHlxmFTvfSfiTZPCbxS12283026 = -405625865;    int FnHlxmFTvfSfiTZPCbxS87314877 = -726977303;    int FnHlxmFTvfSfiTZPCbxS5682143 = -772747679;    int FnHlxmFTvfSfiTZPCbxS10993975 = -195029134;    int FnHlxmFTvfSfiTZPCbxS47366009 = -250075969;    int FnHlxmFTvfSfiTZPCbxS47128872 = -717369737;    int FnHlxmFTvfSfiTZPCbxS88784368 = -213160980;    int FnHlxmFTvfSfiTZPCbxS41011393 = -362599833;    int FnHlxmFTvfSfiTZPCbxS41555645 = -822420238;    int FnHlxmFTvfSfiTZPCbxS36777778 = -469686405;    int FnHlxmFTvfSfiTZPCbxS3844427 = -494411506;    int FnHlxmFTvfSfiTZPCbxS47108535 = -892816188;    int FnHlxmFTvfSfiTZPCbxS31343252 = -893348044;    int FnHlxmFTvfSfiTZPCbxS54153281 = 41502761;    int FnHlxmFTvfSfiTZPCbxS36385169 = -164248060;    int FnHlxmFTvfSfiTZPCbxS85194821 = -278443191;    int FnHlxmFTvfSfiTZPCbxS29669244 = -816128318;    int FnHlxmFTvfSfiTZPCbxS22873793 = -921071619;    int FnHlxmFTvfSfiTZPCbxS83441418 = -221420157;    int FnHlxmFTvfSfiTZPCbxS14810369 = -440244818;    int FnHlxmFTvfSfiTZPCbxS43098262 = -449682543;    int FnHlxmFTvfSfiTZPCbxS93390422 = -928129669;    int FnHlxmFTvfSfiTZPCbxS53407590 = -619786969;    int FnHlxmFTvfSfiTZPCbxS22636458 = -944328179;    int FnHlxmFTvfSfiTZPCbxS35376248 = -873608886;    int FnHlxmFTvfSfiTZPCbxS87165973 = -551874743;    int FnHlxmFTvfSfiTZPCbxS61176488 = -699897147;    int FnHlxmFTvfSfiTZPCbxS51115961 = -390267604;    int FnHlxmFTvfSfiTZPCbxS36727448 = -717896923;    int FnHlxmFTvfSfiTZPCbxS43568059 = 58517878;    int FnHlxmFTvfSfiTZPCbxS83236875 = -75025303;    int FnHlxmFTvfSfiTZPCbxS36664211 = -444970936;    int FnHlxmFTvfSfiTZPCbxS46701957 = -466940865;    int FnHlxmFTvfSfiTZPCbxS53220110 = 3682476;    int FnHlxmFTvfSfiTZPCbxS33992405 = -704081909;    int FnHlxmFTvfSfiTZPCbxS79401177 = -346574249;    int FnHlxmFTvfSfiTZPCbxS13750923 = 96667084;    int FnHlxmFTvfSfiTZPCbxS39002446 = -474490066;    int FnHlxmFTvfSfiTZPCbxS13821806 = -681729020;    int FnHlxmFTvfSfiTZPCbxS21874034 = -983346654;    int FnHlxmFTvfSfiTZPCbxS78124939 = -657644274;    int FnHlxmFTvfSfiTZPCbxS81646799 = -348735248;    int FnHlxmFTvfSfiTZPCbxS20460411 = -144977731;    int FnHlxmFTvfSfiTZPCbxS29244286 = 48827930;    int FnHlxmFTvfSfiTZPCbxS49286406 = -239490672;    int FnHlxmFTvfSfiTZPCbxS54440591 = -606751021;    int FnHlxmFTvfSfiTZPCbxS2943387 = -768894328;    int FnHlxmFTvfSfiTZPCbxS40820179 = -244270842;    int FnHlxmFTvfSfiTZPCbxS30163161 = -145912473;    int FnHlxmFTvfSfiTZPCbxS39618811 = -226418658;    int FnHlxmFTvfSfiTZPCbxS16360458 = -738849836;    int FnHlxmFTvfSfiTZPCbxS80582807 = -227936987;    int FnHlxmFTvfSfiTZPCbxS19512855 = -431378777;    int FnHlxmFTvfSfiTZPCbxS52829044 = -727017972;    int FnHlxmFTvfSfiTZPCbxS9661790 = -647149366;    int FnHlxmFTvfSfiTZPCbxS18892603 = -477496196;    int FnHlxmFTvfSfiTZPCbxS33907287 = -7190334;    int FnHlxmFTvfSfiTZPCbxS83045684 = -828419500;    int FnHlxmFTvfSfiTZPCbxS75617726 = -321420249;    int FnHlxmFTvfSfiTZPCbxS60200035 = -698201226;    int FnHlxmFTvfSfiTZPCbxS85952383 = 82527410;    int FnHlxmFTvfSfiTZPCbxS37668408 = -822893377;    int FnHlxmFTvfSfiTZPCbxS4283945 = -644702910;    int FnHlxmFTvfSfiTZPCbxS97987586 = -780938116;    int FnHlxmFTvfSfiTZPCbxS53540903 = -294661103;    int FnHlxmFTvfSfiTZPCbxS67180216 = 50559430;    int FnHlxmFTvfSfiTZPCbxS406579 = -325875323;    int FnHlxmFTvfSfiTZPCbxS78123141 = -797030520;    int FnHlxmFTvfSfiTZPCbxS20160877 = -254415331;    int FnHlxmFTvfSfiTZPCbxS56983991 = -214117114;     FnHlxmFTvfSfiTZPCbxS21452612 = FnHlxmFTvfSfiTZPCbxS56149948;     FnHlxmFTvfSfiTZPCbxS56149948 = FnHlxmFTvfSfiTZPCbxS17664292;     FnHlxmFTvfSfiTZPCbxS17664292 = FnHlxmFTvfSfiTZPCbxS61541081;     FnHlxmFTvfSfiTZPCbxS61541081 = FnHlxmFTvfSfiTZPCbxS89597760;     FnHlxmFTvfSfiTZPCbxS89597760 = FnHlxmFTvfSfiTZPCbxS86693476;     FnHlxmFTvfSfiTZPCbxS86693476 = FnHlxmFTvfSfiTZPCbxS39618483;     FnHlxmFTvfSfiTZPCbxS39618483 = FnHlxmFTvfSfiTZPCbxS14341370;     FnHlxmFTvfSfiTZPCbxS14341370 = FnHlxmFTvfSfiTZPCbxS5980163;     FnHlxmFTvfSfiTZPCbxS5980163 = FnHlxmFTvfSfiTZPCbxS46275430;     FnHlxmFTvfSfiTZPCbxS46275430 = FnHlxmFTvfSfiTZPCbxS66716054;     FnHlxmFTvfSfiTZPCbxS66716054 = FnHlxmFTvfSfiTZPCbxS19433066;     FnHlxmFTvfSfiTZPCbxS19433066 = FnHlxmFTvfSfiTZPCbxS49996420;     FnHlxmFTvfSfiTZPCbxS49996420 = FnHlxmFTvfSfiTZPCbxS61187814;     FnHlxmFTvfSfiTZPCbxS61187814 = FnHlxmFTvfSfiTZPCbxS69002905;     FnHlxmFTvfSfiTZPCbxS69002905 = FnHlxmFTvfSfiTZPCbxS66909308;     FnHlxmFTvfSfiTZPCbxS66909308 = FnHlxmFTvfSfiTZPCbxS22658193;     FnHlxmFTvfSfiTZPCbxS22658193 = FnHlxmFTvfSfiTZPCbxS62016056;     FnHlxmFTvfSfiTZPCbxS62016056 = FnHlxmFTvfSfiTZPCbxS66022064;     FnHlxmFTvfSfiTZPCbxS66022064 = FnHlxmFTvfSfiTZPCbxS53130833;     FnHlxmFTvfSfiTZPCbxS53130833 = FnHlxmFTvfSfiTZPCbxS1549126;     FnHlxmFTvfSfiTZPCbxS1549126 = FnHlxmFTvfSfiTZPCbxS34286639;     FnHlxmFTvfSfiTZPCbxS34286639 = FnHlxmFTvfSfiTZPCbxS94973460;     FnHlxmFTvfSfiTZPCbxS94973460 = FnHlxmFTvfSfiTZPCbxS66548330;     FnHlxmFTvfSfiTZPCbxS66548330 = FnHlxmFTvfSfiTZPCbxS24813633;     FnHlxmFTvfSfiTZPCbxS24813633 = FnHlxmFTvfSfiTZPCbxS46029702;     FnHlxmFTvfSfiTZPCbxS46029702 = FnHlxmFTvfSfiTZPCbxS3456601;     FnHlxmFTvfSfiTZPCbxS3456601 = FnHlxmFTvfSfiTZPCbxS2954273;     FnHlxmFTvfSfiTZPCbxS2954273 = FnHlxmFTvfSfiTZPCbxS67639413;     FnHlxmFTvfSfiTZPCbxS67639413 = FnHlxmFTvfSfiTZPCbxS52760052;     FnHlxmFTvfSfiTZPCbxS52760052 = FnHlxmFTvfSfiTZPCbxS12283026;     FnHlxmFTvfSfiTZPCbxS12283026 = FnHlxmFTvfSfiTZPCbxS87314877;     FnHlxmFTvfSfiTZPCbxS87314877 = FnHlxmFTvfSfiTZPCbxS5682143;     FnHlxmFTvfSfiTZPCbxS5682143 = FnHlxmFTvfSfiTZPCbxS10993975;     FnHlxmFTvfSfiTZPCbxS10993975 = FnHlxmFTvfSfiTZPCbxS47366009;     FnHlxmFTvfSfiTZPCbxS47366009 = FnHlxmFTvfSfiTZPCbxS47128872;     FnHlxmFTvfSfiTZPCbxS47128872 = FnHlxmFTvfSfiTZPCbxS88784368;     FnHlxmFTvfSfiTZPCbxS88784368 = FnHlxmFTvfSfiTZPCbxS41011393;     FnHlxmFTvfSfiTZPCbxS41011393 = FnHlxmFTvfSfiTZPCbxS41555645;     FnHlxmFTvfSfiTZPCbxS41555645 = FnHlxmFTvfSfiTZPCbxS36777778;     FnHlxmFTvfSfiTZPCbxS36777778 = FnHlxmFTvfSfiTZPCbxS3844427;     FnHlxmFTvfSfiTZPCbxS3844427 = FnHlxmFTvfSfiTZPCbxS47108535;     FnHlxmFTvfSfiTZPCbxS47108535 = FnHlxmFTvfSfiTZPCbxS31343252;     FnHlxmFTvfSfiTZPCbxS31343252 = FnHlxmFTvfSfiTZPCbxS54153281;     FnHlxmFTvfSfiTZPCbxS54153281 = FnHlxmFTvfSfiTZPCbxS36385169;     FnHlxmFTvfSfiTZPCbxS36385169 = FnHlxmFTvfSfiTZPCbxS85194821;     FnHlxmFTvfSfiTZPCbxS85194821 = FnHlxmFTvfSfiTZPCbxS29669244;     FnHlxmFTvfSfiTZPCbxS29669244 = FnHlxmFTvfSfiTZPCbxS22873793;     FnHlxmFTvfSfiTZPCbxS22873793 = FnHlxmFTvfSfiTZPCbxS83441418;     FnHlxmFTvfSfiTZPCbxS83441418 = FnHlxmFTvfSfiTZPCbxS14810369;     FnHlxmFTvfSfiTZPCbxS14810369 = FnHlxmFTvfSfiTZPCbxS43098262;     FnHlxmFTvfSfiTZPCbxS43098262 = FnHlxmFTvfSfiTZPCbxS93390422;     FnHlxmFTvfSfiTZPCbxS93390422 = FnHlxmFTvfSfiTZPCbxS53407590;     FnHlxmFTvfSfiTZPCbxS53407590 = FnHlxmFTvfSfiTZPCbxS22636458;     FnHlxmFTvfSfiTZPCbxS22636458 = FnHlxmFTvfSfiTZPCbxS35376248;     FnHlxmFTvfSfiTZPCbxS35376248 = FnHlxmFTvfSfiTZPCbxS87165973;     FnHlxmFTvfSfiTZPCbxS87165973 = FnHlxmFTvfSfiTZPCbxS61176488;     FnHlxmFTvfSfiTZPCbxS61176488 = FnHlxmFTvfSfiTZPCbxS51115961;     FnHlxmFTvfSfiTZPCbxS51115961 = FnHlxmFTvfSfiTZPCbxS36727448;     FnHlxmFTvfSfiTZPCbxS36727448 = FnHlxmFTvfSfiTZPCbxS43568059;     FnHlxmFTvfSfiTZPCbxS43568059 = FnHlxmFTvfSfiTZPCbxS83236875;     FnHlxmFTvfSfiTZPCbxS83236875 = FnHlxmFTvfSfiTZPCbxS36664211;     FnHlxmFTvfSfiTZPCbxS36664211 = FnHlxmFTvfSfiTZPCbxS46701957;     FnHlxmFTvfSfiTZPCbxS46701957 = FnHlxmFTvfSfiTZPCbxS53220110;     FnHlxmFTvfSfiTZPCbxS53220110 = FnHlxmFTvfSfiTZPCbxS33992405;     FnHlxmFTvfSfiTZPCbxS33992405 = FnHlxmFTvfSfiTZPCbxS79401177;     FnHlxmFTvfSfiTZPCbxS79401177 = FnHlxmFTvfSfiTZPCbxS13750923;     FnHlxmFTvfSfiTZPCbxS13750923 = FnHlxmFTvfSfiTZPCbxS39002446;     FnHlxmFTvfSfiTZPCbxS39002446 = FnHlxmFTvfSfiTZPCbxS13821806;     FnHlxmFTvfSfiTZPCbxS13821806 = FnHlxmFTvfSfiTZPCbxS21874034;     FnHlxmFTvfSfiTZPCbxS21874034 = FnHlxmFTvfSfiTZPCbxS78124939;     FnHlxmFTvfSfiTZPCbxS78124939 = FnHlxmFTvfSfiTZPCbxS81646799;     FnHlxmFTvfSfiTZPCbxS81646799 = FnHlxmFTvfSfiTZPCbxS20460411;     FnHlxmFTvfSfiTZPCbxS20460411 = FnHlxmFTvfSfiTZPCbxS29244286;     FnHlxmFTvfSfiTZPCbxS29244286 = FnHlxmFTvfSfiTZPCbxS49286406;     FnHlxmFTvfSfiTZPCbxS49286406 = FnHlxmFTvfSfiTZPCbxS54440591;     FnHlxmFTvfSfiTZPCbxS54440591 = FnHlxmFTvfSfiTZPCbxS2943387;     FnHlxmFTvfSfiTZPCbxS2943387 = FnHlxmFTvfSfiTZPCbxS40820179;     FnHlxmFTvfSfiTZPCbxS40820179 = FnHlxmFTvfSfiTZPCbxS30163161;     FnHlxmFTvfSfiTZPCbxS30163161 = FnHlxmFTvfSfiTZPCbxS39618811;     FnHlxmFTvfSfiTZPCbxS39618811 = FnHlxmFTvfSfiTZPCbxS16360458;     FnHlxmFTvfSfiTZPCbxS16360458 = FnHlxmFTvfSfiTZPCbxS80582807;     FnHlxmFTvfSfiTZPCbxS80582807 = FnHlxmFTvfSfiTZPCbxS19512855;     FnHlxmFTvfSfiTZPCbxS19512855 = FnHlxmFTvfSfiTZPCbxS52829044;     FnHlxmFTvfSfiTZPCbxS52829044 = FnHlxmFTvfSfiTZPCbxS9661790;     FnHlxmFTvfSfiTZPCbxS9661790 = FnHlxmFTvfSfiTZPCbxS18892603;     FnHlxmFTvfSfiTZPCbxS18892603 = FnHlxmFTvfSfiTZPCbxS33907287;     FnHlxmFTvfSfiTZPCbxS33907287 = FnHlxmFTvfSfiTZPCbxS83045684;     FnHlxmFTvfSfiTZPCbxS83045684 = FnHlxmFTvfSfiTZPCbxS75617726;     FnHlxmFTvfSfiTZPCbxS75617726 = FnHlxmFTvfSfiTZPCbxS60200035;     FnHlxmFTvfSfiTZPCbxS60200035 = FnHlxmFTvfSfiTZPCbxS85952383;     FnHlxmFTvfSfiTZPCbxS85952383 = FnHlxmFTvfSfiTZPCbxS37668408;     FnHlxmFTvfSfiTZPCbxS37668408 = FnHlxmFTvfSfiTZPCbxS4283945;     FnHlxmFTvfSfiTZPCbxS4283945 = FnHlxmFTvfSfiTZPCbxS97987586;     FnHlxmFTvfSfiTZPCbxS97987586 = FnHlxmFTvfSfiTZPCbxS53540903;     FnHlxmFTvfSfiTZPCbxS53540903 = FnHlxmFTvfSfiTZPCbxS67180216;     FnHlxmFTvfSfiTZPCbxS67180216 = FnHlxmFTvfSfiTZPCbxS406579;     FnHlxmFTvfSfiTZPCbxS406579 = FnHlxmFTvfSfiTZPCbxS78123141;     FnHlxmFTvfSfiTZPCbxS78123141 = FnHlxmFTvfSfiTZPCbxS20160877;     FnHlxmFTvfSfiTZPCbxS20160877 = FnHlxmFTvfSfiTZPCbxS56983991;     FnHlxmFTvfSfiTZPCbxS56983991 = FnHlxmFTvfSfiTZPCbxS21452612;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OnUjwjWJDFPHXdeGhspL95577044() {     int YQVxLMWXKAmfykzSGKGd84883264 = -897890528;    int YQVxLMWXKAmfykzSGKGd81401527 = -862439842;    int YQVxLMWXKAmfykzSGKGd10254162 = -978502596;    int YQVxLMWXKAmfykzSGKGd66815059 = -336738498;    int YQVxLMWXKAmfykzSGKGd30712339 = -958183306;    int YQVxLMWXKAmfykzSGKGd64324704 = -165257137;    int YQVxLMWXKAmfykzSGKGd97048760 = -622096370;    int YQVxLMWXKAmfykzSGKGd67025197 = 47394030;    int YQVxLMWXKAmfykzSGKGd54963082 = -767078573;    int YQVxLMWXKAmfykzSGKGd80493459 = -689554290;    int YQVxLMWXKAmfykzSGKGd34321322 = -611394210;    int YQVxLMWXKAmfykzSGKGd96547753 = -310397361;    int YQVxLMWXKAmfykzSGKGd48164557 = 8835789;    int YQVxLMWXKAmfykzSGKGd95217720 = -664718505;    int YQVxLMWXKAmfykzSGKGd92337098 = -907881231;    int YQVxLMWXKAmfykzSGKGd67103992 = -458004692;    int YQVxLMWXKAmfykzSGKGd86515393 = 57393385;    int YQVxLMWXKAmfykzSGKGd6352040 = -942711488;    int YQVxLMWXKAmfykzSGKGd30898110 = -426415316;    int YQVxLMWXKAmfykzSGKGd48259044 = -148488962;    int YQVxLMWXKAmfykzSGKGd61996346 = -92076186;    int YQVxLMWXKAmfykzSGKGd97149836 = -377600130;    int YQVxLMWXKAmfykzSGKGd26169053 = -248155897;    int YQVxLMWXKAmfykzSGKGd63360729 = 51213622;    int YQVxLMWXKAmfykzSGKGd8248453 = -979470787;    int YQVxLMWXKAmfykzSGKGd88155017 = -908366169;    int YQVxLMWXKAmfykzSGKGd84265648 = -78760429;    int YQVxLMWXKAmfykzSGKGd15352024 = -8113353;    int YQVxLMWXKAmfykzSGKGd64944561 = -700334639;    int YQVxLMWXKAmfykzSGKGd92276827 = -847867272;    int YQVxLMWXKAmfykzSGKGd7210678 = -441188865;    int YQVxLMWXKAmfykzSGKGd18020556 = -226175556;    int YQVxLMWXKAmfykzSGKGd25668450 = -645839829;    int YQVxLMWXKAmfykzSGKGd6697896 = -303675678;    int YQVxLMWXKAmfykzSGKGd43693137 = -793158303;    int YQVxLMWXKAmfykzSGKGd22851641 = -761548400;    int YQVxLMWXKAmfykzSGKGd2833538 = -682843095;    int YQVxLMWXKAmfykzSGKGd80666473 = -721990786;    int YQVxLMWXKAmfykzSGKGd29368923 = 23781789;    int YQVxLMWXKAmfykzSGKGd41857430 = -618778744;    int YQVxLMWXKAmfykzSGKGd58062766 = -323304080;    int YQVxLMWXKAmfykzSGKGd56637985 = -576009423;    int YQVxLMWXKAmfykzSGKGd44744207 = 24351042;    int YQVxLMWXKAmfykzSGKGd33627728 = -506156539;    int YQVxLMWXKAmfykzSGKGd8179642 = -773048997;    int YQVxLMWXKAmfykzSGKGd80161983 = -945791816;    int YQVxLMWXKAmfykzSGKGd53972130 = -293494625;    int YQVxLMWXKAmfykzSGKGd53478893 = 7817048;    int YQVxLMWXKAmfykzSGKGd29781958 = -137191142;    int YQVxLMWXKAmfykzSGKGd68071972 = -862213195;    int YQVxLMWXKAmfykzSGKGd81734634 = -117367061;    int YQVxLMWXKAmfykzSGKGd11337992 = -457157676;    int YQVxLMWXKAmfykzSGKGd84258318 = -670947209;    int YQVxLMWXKAmfykzSGKGd11370953 = -87716627;    int YQVxLMWXKAmfykzSGKGd33580699 = -897256174;    int YQVxLMWXKAmfykzSGKGd87733428 = -420290398;    int YQVxLMWXKAmfykzSGKGd55232475 = -514283946;    int YQVxLMWXKAmfykzSGKGd46893432 = -929716218;    int YQVxLMWXKAmfykzSGKGd58566606 = -357267711;    int YQVxLMWXKAmfykzSGKGd42557321 = 50182863;    int YQVxLMWXKAmfykzSGKGd80059056 = 13503291;    int YQVxLMWXKAmfykzSGKGd81696736 = -513983017;    int YQVxLMWXKAmfykzSGKGd2080637 = -252271331;    int YQVxLMWXKAmfykzSGKGd62686255 = -919211301;    int YQVxLMWXKAmfykzSGKGd73282781 = -148365426;    int YQVxLMWXKAmfykzSGKGd16300766 = -285218654;    int YQVxLMWXKAmfykzSGKGd70879303 = -664557532;    int YQVxLMWXKAmfykzSGKGd41466661 = -687488534;    int YQVxLMWXKAmfykzSGKGd51524583 = -871560202;    int YQVxLMWXKAmfykzSGKGd69485458 = -46332832;    int YQVxLMWXKAmfykzSGKGd64270454 = -775161597;    int YQVxLMWXKAmfykzSGKGd5848920 = -220615830;    int YQVxLMWXKAmfykzSGKGd76983116 = -866493278;    int YQVxLMWXKAmfykzSGKGd89040679 = -807636572;    int YQVxLMWXKAmfykzSGKGd90196277 = -825184883;    int YQVxLMWXKAmfykzSGKGd5358362 = -516066764;    int YQVxLMWXKAmfykzSGKGd52405629 = -301951173;    int YQVxLMWXKAmfykzSGKGd92541325 = -741999358;    int YQVxLMWXKAmfykzSGKGd55181087 = -175737381;    int YQVxLMWXKAmfykzSGKGd28086470 = 66321029;    int YQVxLMWXKAmfykzSGKGd34182887 = -514871545;    int YQVxLMWXKAmfykzSGKGd30786756 = 13422523;    int YQVxLMWXKAmfykzSGKGd85570065 = -870922212;    int YQVxLMWXKAmfykzSGKGd96872589 = -838121445;    int YQVxLMWXKAmfykzSGKGd10542193 = -630500212;    int YQVxLMWXKAmfykzSGKGd95872686 = -984031189;    int YQVxLMWXKAmfykzSGKGd33762237 = -555228347;    int YQVxLMWXKAmfykzSGKGd14297498 = -458123203;    int YQVxLMWXKAmfykzSGKGd73117197 = -406419504;    int YQVxLMWXKAmfykzSGKGd55959708 = -272867906;    int YQVxLMWXKAmfykzSGKGd67619166 = -147264454;    int YQVxLMWXKAmfykzSGKGd55940106 = -753126878;    int YQVxLMWXKAmfykzSGKGd22099868 = -264723075;    int YQVxLMWXKAmfykzSGKGd86811602 = 73598926;    int YQVxLMWXKAmfykzSGKGd61798374 = -532282036;    int YQVxLMWXKAmfykzSGKGd76366029 = -809321063;    int YQVxLMWXKAmfykzSGKGd54557349 = -223738093;    int YQVxLMWXKAmfykzSGKGd82057952 = -56437657;    int YQVxLMWXKAmfykzSGKGd60344946 = -257791114;    int YQVxLMWXKAmfykzSGKGd91878876 = -897890528;     YQVxLMWXKAmfykzSGKGd84883264 = YQVxLMWXKAmfykzSGKGd81401527;     YQVxLMWXKAmfykzSGKGd81401527 = YQVxLMWXKAmfykzSGKGd10254162;     YQVxLMWXKAmfykzSGKGd10254162 = YQVxLMWXKAmfykzSGKGd66815059;     YQVxLMWXKAmfykzSGKGd66815059 = YQVxLMWXKAmfykzSGKGd30712339;     YQVxLMWXKAmfykzSGKGd30712339 = YQVxLMWXKAmfykzSGKGd64324704;     YQVxLMWXKAmfykzSGKGd64324704 = YQVxLMWXKAmfykzSGKGd97048760;     YQVxLMWXKAmfykzSGKGd97048760 = YQVxLMWXKAmfykzSGKGd67025197;     YQVxLMWXKAmfykzSGKGd67025197 = YQVxLMWXKAmfykzSGKGd54963082;     YQVxLMWXKAmfykzSGKGd54963082 = YQVxLMWXKAmfykzSGKGd80493459;     YQVxLMWXKAmfykzSGKGd80493459 = YQVxLMWXKAmfykzSGKGd34321322;     YQVxLMWXKAmfykzSGKGd34321322 = YQVxLMWXKAmfykzSGKGd96547753;     YQVxLMWXKAmfykzSGKGd96547753 = YQVxLMWXKAmfykzSGKGd48164557;     YQVxLMWXKAmfykzSGKGd48164557 = YQVxLMWXKAmfykzSGKGd95217720;     YQVxLMWXKAmfykzSGKGd95217720 = YQVxLMWXKAmfykzSGKGd92337098;     YQVxLMWXKAmfykzSGKGd92337098 = YQVxLMWXKAmfykzSGKGd67103992;     YQVxLMWXKAmfykzSGKGd67103992 = YQVxLMWXKAmfykzSGKGd86515393;     YQVxLMWXKAmfykzSGKGd86515393 = YQVxLMWXKAmfykzSGKGd6352040;     YQVxLMWXKAmfykzSGKGd6352040 = YQVxLMWXKAmfykzSGKGd30898110;     YQVxLMWXKAmfykzSGKGd30898110 = YQVxLMWXKAmfykzSGKGd48259044;     YQVxLMWXKAmfykzSGKGd48259044 = YQVxLMWXKAmfykzSGKGd61996346;     YQVxLMWXKAmfykzSGKGd61996346 = YQVxLMWXKAmfykzSGKGd97149836;     YQVxLMWXKAmfykzSGKGd97149836 = YQVxLMWXKAmfykzSGKGd26169053;     YQVxLMWXKAmfykzSGKGd26169053 = YQVxLMWXKAmfykzSGKGd63360729;     YQVxLMWXKAmfykzSGKGd63360729 = YQVxLMWXKAmfykzSGKGd8248453;     YQVxLMWXKAmfykzSGKGd8248453 = YQVxLMWXKAmfykzSGKGd88155017;     YQVxLMWXKAmfykzSGKGd88155017 = YQVxLMWXKAmfykzSGKGd84265648;     YQVxLMWXKAmfykzSGKGd84265648 = YQVxLMWXKAmfykzSGKGd15352024;     YQVxLMWXKAmfykzSGKGd15352024 = YQVxLMWXKAmfykzSGKGd64944561;     YQVxLMWXKAmfykzSGKGd64944561 = YQVxLMWXKAmfykzSGKGd92276827;     YQVxLMWXKAmfykzSGKGd92276827 = YQVxLMWXKAmfykzSGKGd7210678;     YQVxLMWXKAmfykzSGKGd7210678 = YQVxLMWXKAmfykzSGKGd18020556;     YQVxLMWXKAmfykzSGKGd18020556 = YQVxLMWXKAmfykzSGKGd25668450;     YQVxLMWXKAmfykzSGKGd25668450 = YQVxLMWXKAmfykzSGKGd6697896;     YQVxLMWXKAmfykzSGKGd6697896 = YQVxLMWXKAmfykzSGKGd43693137;     YQVxLMWXKAmfykzSGKGd43693137 = YQVxLMWXKAmfykzSGKGd22851641;     YQVxLMWXKAmfykzSGKGd22851641 = YQVxLMWXKAmfykzSGKGd2833538;     YQVxLMWXKAmfykzSGKGd2833538 = YQVxLMWXKAmfykzSGKGd80666473;     YQVxLMWXKAmfykzSGKGd80666473 = YQVxLMWXKAmfykzSGKGd29368923;     YQVxLMWXKAmfykzSGKGd29368923 = YQVxLMWXKAmfykzSGKGd41857430;     YQVxLMWXKAmfykzSGKGd41857430 = YQVxLMWXKAmfykzSGKGd58062766;     YQVxLMWXKAmfykzSGKGd58062766 = YQVxLMWXKAmfykzSGKGd56637985;     YQVxLMWXKAmfykzSGKGd56637985 = YQVxLMWXKAmfykzSGKGd44744207;     YQVxLMWXKAmfykzSGKGd44744207 = YQVxLMWXKAmfykzSGKGd33627728;     YQVxLMWXKAmfykzSGKGd33627728 = YQVxLMWXKAmfykzSGKGd8179642;     YQVxLMWXKAmfykzSGKGd8179642 = YQVxLMWXKAmfykzSGKGd80161983;     YQVxLMWXKAmfykzSGKGd80161983 = YQVxLMWXKAmfykzSGKGd53972130;     YQVxLMWXKAmfykzSGKGd53972130 = YQVxLMWXKAmfykzSGKGd53478893;     YQVxLMWXKAmfykzSGKGd53478893 = YQVxLMWXKAmfykzSGKGd29781958;     YQVxLMWXKAmfykzSGKGd29781958 = YQVxLMWXKAmfykzSGKGd68071972;     YQVxLMWXKAmfykzSGKGd68071972 = YQVxLMWXKAmfykzSGKGd81734634;     YQVxLMWXKAmfykzSGKGd81734634 = YQVxLMWXKAmfykzSGKGd11337992;     YQVxLMWXKAmfykzSGKGd11337992 = YQVxLMWXKAmfykzSGKGd84258318;     YQVxLMWXKAmfykzSGKGd84258318 = YQVxLMWXKAmfykzSGKGd11370953;     YQVxLMWXKAmfykzSGKGd11370953 = YQVxLMWXKAmfykzSGKGd33580699;     YQVxLMWXKAmfykzSGKGd33580699 = YQVxLMWXKAmfykzSGKGd87733428;     YQVxLMWXKAmfykzSGKGd87733428 = YQVxLMWXKAmfykzSGKGd55232475;     YQVxLMWXKAmfykzSGKGd55232475 = YQVxLMWXKAmfykzSGKGd46893432;     YQVxLMWXKAmfykzSGKGd46893432 = YQVxLMWXKAmfykzSGKGd58566606;     YQVxLMWXKAmfykzSGKGd58566606 = YQVxLMWXKAmfykzSGKGd42557321;     YQVxLMWXKAmfykzSGKGd42557321 = YQVxLMWXKAmfykzSGKGd80059056;     YQVxLMWXKAmfykzSGKGd80059056 = YQVxLMWXKAmfykzSGKGd81696736;     YQVxLMWXKAmfykzSGKGd81696736 = YQVxLMWXKAmfykzSGKGd2080637;     YQVxLMWXKAmfykzSGKGd2080637 = YQVxLMWXKAmfykzSGKGd62686255;     YQVxLMWXKAmfykzSGKGd62686255 = YQVxLMWXKAmfykzSGKGd73282781;     YQVxLMWXKAmfykzSGKGd73282781 = YQVxLMWXKAmfykzSGKGd16300766;     YQVxLMWXKAmfykzSGKGd16300766 = YQVxLMWXKAmfykzSGKGd70879303;     YQVxLMWXKAmfykzSGKGd70879303 = YQVxLMWXKAmfykzSGKGd41466661;     YQVxLMWXKAmfykzSGKGd41466661 = YQVxLMWXKAmfykzSGKGd51524583;     YQVxLMWXKAmfykzSGKGd51524583 = YQVxLMWXKAmfykzSGKGd69485458;     YQVxLMWXKAmfykzSGKGd69485458 = YQVxLMWXKAmfykzSGKGd64270454;     YQVxLMWXKAmfykzSGKGd64270454 = YQVxLMWXKAmfykzSGKGd5848920;     YQVxLMWXKAmfykzSGKGd5848920 = YQVxLMWXKAmfykzSGKGd76983116;     YQVxLMWXKAmfykzSGKGd76983116 = YQVxLMWXKAmfykzSGKGd89040679;     YQVxLMWXKAmfykzSGKGd89040679 = YQVxLMWXKAmfykzSGKGd90196277;     YQVxLMWXKAmfykzSGKGd90196277 = YQVxLMWXKAmfykzSGKGd5358362;     YQVxLMWXKAmfykzSGKGd5358362 = YQVxLMWXKAmfykzSGKGd52405629;     YQVxLMWXKAmfykzSGKGd52405629 = YQVxLMWXKAmfykzSGKGd92541325;     YQVxLMWXKAmfykzSGKGd92541325 = YQVxLMWXKAmfykzSGKGd55181087;     YQVxLMWXKAmfykzSGKGd55181087 = YQVxLMWXKAmfykzSGKGd28086470;     YQVxLMWXKAmfykzSGKGd28086470 = YQVxLMWXKAmfykzSGKGd34182887;     YQVxLMWXKAmfykzSGKGd34182887 = YQVxLMWXKAmfykzSGKGd30786756;     YQVxLMWXKAmfykzSGKGd30786756 = YQVxLMWXKAmfykzSGKGd85570065;     YQVxLMWXKAmfykzSGKGd85570065 = YQVxLMWXKAmfykzSGKGd96872589;     YQVxLMWXKAmfykzSGKGd96872589 = YQVxLMWXKAmfykzSGKGd10542193;     YQVxLMWXKAmfykzSGKGd10542193 = YQVxLMWXKAmfykzSGKGd95872686;     YQVxLMWXKAmfykzSGKGd95872686 = YQVxLMWXKAmfykzSGKGd33762237;     YQVxLMWXKAmfykzSGKGd33762237 = YQVxLMWXKAmfykzSGKGd14297498;     YQVxLMWXKAmfykzSGKGd14297498 = YQVxLMWXKAmfykzSGKGd73117197;     YQVxLMWXKAmfykzSGKGd73117197 = YQVxLMWXKAmfykzSGKGd55959708;     YQVxLMWXKAmfykzSGKGd55959708 = YQVxLMWXKAmfykzSGKGd67619166;     YQVxLMWXKAmfykzSGKGd67619166 = YQVxLMWXKAmfykzSGKGd55940106;     YQVxLMWXKAmfykzSGKGd55940106 = YQVxLMWXKAmfykzSGKGd22099868;     YQVxLMWXKAmfykzSGKGd22099868 = YQVxLMWXKAmfykzSGKGd86811602;     YQVxLMWXKAmfykzSGKGd86811602 = YQVxLMWXKAmfykzSGKGd61798374;     YQVxLMWXKAmfykzSGKGd61798374 = YQVxLMWXKAmfykzSGKGd76366029;     YQVxLMWXKAmfykzSGKGd76366029 = YQVxLMWXKAmfykzSGKGd54557349;     YQVxLMWXKAmfykzSGKGd54557349 = YQVxLMWXKAmfykzSGKGd82057952;     YQVxLMWXKAmfykzSGKGd82057952 = YQVxLMWXKAmfykzSGKGd60344946;     YQVxLMWXKAmfykzSGKGd60344946 = YQVxLMWXKAmfykzSGKGd91878876;     YQVxLMWXKAmfykzSGKGd91878876 = YQVxLMWXKAmfykzSGKGd84883264;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void cjhkAqWtSOucViTCigOE1986185() {     int MogFKNzAMMqCcfvqxasM18972056 = -127288930;    int MogFKNzAMMqCcfvqxasM36754308 = -451262630;    int MogFKNzAMMqCcfvqxasM81499738 = -441742902;    int MogFKNzAMMqCcfvqxasM26940463 = -720315783;    int MogFKNzAMMqCcfvqxasM84437135 = -634957363;    int MogFKNzAMMqCcfvqxasM97150262 = -8628706;    int MogFKNzAMMqCcfvqxasM4310598 = -240662113;    int MogFKNzAMMqCcfvqxasM7337356 = -913648058;    int MogFKNzAMMqCcfvqxasM67070892 = -620708972;    int MogFKNzAMMqCcfvqxasM41038435 = -833689287;    int MogFKNzAMMqCcfvqxasM23855144 = -124666726;    int MogFKNzAMMqCcfvqxasM4917337 = -424999558;    int MogFKNzAMMqCcfvqxasM94307167 = -200687805;    int MogFKNzAMMqCcfvqxasM59250440 = -853507181;    int MogFKNzAMMqCcfvqxasM53219202 = -202083224;    int MogFKNzAMMqCcfvqxasM25262770 = -282392066;    int MogFKNzAMMqCcfvqxasM88372661 = -263448709;    int MogFKNzAMMqCcfvqxasM47980984 = -410119582;    int MogFKNzAMMqCcfvqxasM57053667 = -421177833;    int MogFKNzAMMqCcfvqxasM30100201 = -493417601;    int MogFKNzAMMqCcfvqxasM97909896 = -982797932;    int MogFKNzAMMqCcfvqxasM43112533 = 85748688;    int MogFKNzAMMqCcfvqxasM22430058 = -302429015;    int MogFKNzAMMqCcfvqxasM50372558 = -298892607;    int MogFKNzAMMqCcfvqxasM27364973 = -603290559;    int MogFKNzAMMqCcfvqxasM44303837 = -351832702;    int MogFKNzAMMqCcfvqxasM3113900 = -102890315;    int MogFKNzAMMqCcfvqxasM128476 = -259584551;    int MogFKNzAMMqCcfvqxasM81713750 = -881694392;    int MogFKNzAMMqCcfvqxasM94875828 = -281866332;    int MogFKNzAMMqCcfvqxasM75797648 = -938092315;    int MogFKNzAMMqCcfvqxasM8513839 = -761849210;    int MogFKNzAMMqCcfvqxasM43147696 = -263949373;    int MogFKNzAMMqCcfvqxasM67483955 = -99170173;    int MogFKNzAMMqCcfvqxasM35686464 = -923873493;    int MogFKNzAMMqCcfvqxasM15739479 = -805187864;    int MogFKNzAMMqCcfvqxasM48732825 = -141465040;    int MogFKNzAMMqCcfvqxasM3196364 = -870232167;    int MogFKNzAMMqCcfvqxasM60401893 = -573063404;    int MogFKNzAMMqCcfvqxasM70167487 = -676047760;    int MogFKNzAMMqCcfvqxasM84498722 = -448271634;    int MogFKNzAMMqCcfvqxasM8952765 = -934799645;    int MogFKNzAMMqCcfvqxasM46828762 = -685573669;    int MogFKNzAMMqCcfvqxasM60378576 = -468826368;    int MogFKNzAMMqCcfvqxasM91910121 = -117236927;    int MogFKNzAMMqCcfvqxasM41107011 = -459432925;    int MogFKNzAMMqCcfvqxasM76708427 = -505650109;    int MogFKNzAMMqCcfvqxasM74314622 = -6663225;    int MogFKNzAMMqCcfvqxasM62905561 = -474962234;    int MogFKNzAMMqCcfvqxasM9715639 = -880165564;    int MogFKNzAMMqCcfvqxasM75300961 = -853068693;    int MogFKNzAMMqCcfvqxasM56609807 = -673259656;    int MogFKNzAMMqCcfvqxasM94736356 = -757851483;    int MogFKNzAMMqCcfvqxasM80082481 = -460066721;    int MogFKNzAMMqCcfvqxasM4076288 = -174945100;    int MogFKNzAMMqCcfvqxasM75859522 = -113037619;    int MogFKNzAMMqCcfvqxasM14324250 = -48833616;    int MogFKNzAMMqCcfvqxasM31127181 = -42850296;    int MogFKNzAMMqCcfvqxasM99575490 = -17025224;    int MogFKNzAMMqCcfvqxasM40133299 = -183124661;    int MogFKNzAMMqCcfvqxasM94036363 = -905738392;    int MogFKNzAMMqCcfvqxasM4182122 = -981077562;    int MogFKNzAMMqCcfvqxasM25623605 = 68046333;    int MogFKNzAMMqCcfvqxasM72195064 = -238842641;    int MogFKNzAMMqCcfvqxasM65240787 = -895596973;    int MogFKNzAMMqCcfvqxasM15341306 = -362817516;    int MogFKNzAMMqCcfvqxasM61769641 = -61050185;    int MogFKNzAMMqCcfvqxasM26823212 = -1517633;    int MogFKNzAMMqCcfvqxasM23563977 = -929633688;    int MogFKNzAMMqCcfvqxasM37479724 = -396895360;    int MogFKNzAMMqCcfvqxasM76529944 = -40927026;    int MogFKNzAMMqCcfvqxasM85176298 = -393216543;    int MogFKNzAMMqCcfvqxasM87579090 = -837056178;    int MogFKNzAMMqCcfvqxasM86886180 = -745130074;    int MogFKNzAMMqCcfvqxasM45601479 = 54854033;    int MogFKNzAMMqCcfvqxasM88957131 = 52001713;    int MogFKNzAMMqCcfvqxasM96283770 = -228677643;    int MogFKNzAMMqCcfvqxasM62051482 = -833602648;    int MogFKNzAMMqCcfvqxasM58462436 = -81655680;    int MogFKNzAMMqCcfvqxasM86257961 = -43857635;    int MogFKNzAMMqCcfvqxasM67595410 = -846182593;    int MogFKNzAMMqCcfvqxasM28799278 = 3772909;    int MogFKNzAMMqCcfvqxasM37222914 = -784622318;    int MogFKNzAMMqCcfvqxasM71998112 = 98471172;    int MogFKNzAMMqCcfvqxasM19574867 = -428797640;    int MogFKNzAMMqCcfvqxasM19187842 = -164832660;    int MogFKNzAMMqCcfvqxasM13777482 = 96002272;    int MogFKNzAMMqCcfvqxasM63065214 = -803882653;    int MogFKNzAMMqCcfvqxasM63407668 = -924225073;    int MogFKNzAMMqCcfvqxasM59826941 = -710835875;    int MogFKNzAMMqCcfvqxasM1415229 = -656354249;    int MogFKNzAMMqCcfvqxasM17605645 = 1385255;    int MogFKNzAMMqCcfvqxasM3620873 = -753206943;    int MogFKNzAMMqCcfvqxasM20268595 = -289938744;    int MogFKNzAMMqCcfvqxasM76131124 = -770309368;    int MogFKNzAMMqCcfvqxasM80316600 = -467194073;    int MogFKNzAMMqCcfvqxasM83329160 = -902845979;    int MogFKNzAMMqCcfvqxasM74633698 = -346731029;    int MogFKNzAMMqCcfvqxasM95137789 = -573229396;    int MogFKNzAMMqCcfvqxasM76568815 = -127288930;     MogFKNzAMMqCcfvqxasM18972056 = MogFKNzAMMqCcfvqxasM36754308;     MogFKNzAMMqCcfvqxasM36754308 = MogFKNzAMMqCcfvqxasM81499738;     MogFKNzAMMqCcfvqxasM81499738 = MogFKNzAMMqCcfvqxasM26940463;     MogFKNzAMMqCcfvqxasM26940463 = MogFKNzAMMqCcfvqxasM84437135;     MogFKNzAMMqCcfvqxasM84437135 = MogFKNzAMMqCcfvqxasM97150262;     MogFKNzAMMqCcfvqxasM97150262 = MogFKNzAMMqCcfvqxasM4310598;     MogFKNzAMMqCcfvqxasM4310598 = MogFKNzAMMqCcfvqxasM7337356;     MogFKNzAMMqCcfvqxasM7337356 = MogFKNzAMMqCcfvqxasM67070892;     MogFKNzAMMqCcfvqxasM67070892 = MogFKNzAMMqCcfvqxasM41038435;     MogFKNzAMMqCcfvqxasM41038435 = MogFKNzAMMqCcfvqxasM23855144;     MogFKNzAMMqCcfvqxasM23855144 = MogFKNzAMMqCcfvqxasM4917337;     MogFKNzAMMqCcfvqxasM4917337 = MogFKNzAMMqCcfvqxasM94307167;     MogFKNzAMMqCcfvqxasM94307167 = MogFKNzAMMqCcfvqxasM59250440;     MogFKNzAMMqCcfvqxasM59250440 = MogFKNzAMMqCcfvqxasM53219202;     MogFKNzAMMqCcfvqxasM53219202 = MogFKNzAMMqCcfvqxasM25262770;     MogFKNzAMMqCcfvqxasM25262770 = MogFKNzAMMqCcfvqxasM88372661;     MogFKNzAMMqCcfvqxasM88372661 = MogFKNzAMMqCcfvqxasM47980984;     MogFKNzAMMqCcfvqxasM47980984 = MogFKNzAMMqCcfvqxasM57053667;     MogFKNzAMMqCcfvqxasM57053667 = MogFKNzAMMqCcfvqxasM30100201;     MogFKNzAMMqCcfvqxasM30100201 = MogFKNzAMMqCcfvqxasM97909896;     MogFKNzAMMqCcfvqxasM97909896 = MogFKNzAMMqCcfvqxasM43112533;     MogFKNzAMMqCcfvqxasM43112533 = MogFKNzAMMqCcfvqxasM22430058;     MogFKNzAMMqCcfvqxasM22430058 = MogFKNzAMMqCcfvqxasM50372558;     MogFKNzAMMqCcfvqxasM50372558 = MogFKNzAMMqCcfvqxasM27364973;     MogFKNzAMMqCcfvqxasM27364973 = MogFKNzAMMqCcfvqxasM44303837;     MogFKNzAMMqCcfvqxasM44303837 = MogFKNzAMMqCcfvqxasM3113900;     MogFKNzAMMqCcfvqxasM3113900 = MogFKNzAMMqCcfvqxasM128476;     MogFKNzAMMqCcfvqxasM128476 = MogFKNzAMMqCcfvqxasM81713750;     MogFKNzAMMqCcfvqxasM81713750 = MogFKNzAMMqCcfvqxasM94875828;     MogFKNzAMMqCcfvqxasM94875828 = MogFKNzAMMqCcfvqxasM75797648;     MogFKNzAMMqCcfvqxasM75797648 = MogFKNzAMMqCcfvqxasM8513839;     MogFKNzAMMqCcfvqxasM8513839 = MogFKNzAMMqCcfvqxasM43147696;     MogFKNzAMMqCcfvqxasM43147696 = MogFKNzAMMqCcfvqxasM67483955;     MogFKNzAMMqCcfvqxasM67483955 = MogFKNzAMMqCcfvqxasM35686464;     MogFKNzAMMqCcfvqxasM35686464 = MogFKNzAMMqCcfvqxasM15739479;     MogFKNzAMMqCcfvqxasM15739479 = MogFKNzAMMqCcfvqxasM48732825;     MogFKNzAMMqCcfvqxasM48732825 = MogFKNzAMMqCcfvqxasM3196364;     MogFKNzAMMqCcfvqxasM3196364 = MogFKNzAMMqCcfvqxasM60401893;     MogFKNzAMMqCcfvqxasM60401893 = MogFKNzAMMqCcfvqxasM70167487;     MogFKNzAMMqCcfvqxasM70167487 = MogFKNzAMMqCcfvqxasM84498722;     MogFKNzAMMqCcfvqxasM84498722 = MogFKNzAMMqCcfvqxasM8952765;     MogFKNzAMMqCcfvqxasM8952765 = MogFKNzAMMqCcfvqxasM46828762;     MogFKNzAMMqCcfvqxasM46828762 = MogFKNzAMMqCcfvqxasM60378576;     MogFKNzAMMqCcfvqxasM60378576 = MogFKNzAMMqCcfvqxasM91910121;     MogFKNzAMMqCcfvqxasM91910121 = MogFKNzAMMqCcfvqxasM41107011;     MogFKNzAMMqCcfvqxasM41107011 = MogFKNzAMMqCcfvqxasM76708427;     MogFKNzAMMqCcfvqxasM76708427 = MogFKNzAMMqCcfvqxasM74314622;     MogFKNzAMMqCcfvqxasM74314622 = MogFKNzAMMqCcfvqxasM62905561;     MogFKNzAMMqCcfvqxasM62905561 = MogFKNzAMMqCcfvqxasM9715639;     MogFKNzAMMqCcfvqxasM9715639 = MogFKNzAMMqCcfvqxasM75300961;     MogFKNzAMMqCcfvqxasM75300961 = MogFKNzAMMqCcfvqxasM56609807;     MogFKNzAMMqCcfvqxasM56609807 = MogFKNzAMMqCcfvqxasM94736356;     MogFKNzAMMqCcfvqxasM94736356 = MogFKNzAMMqCcfvqxasM80082481;     MogFKNzAMMqCcfvqxasM80082481 = MogFKNzAMMqCcfvqxasM4076288;     MogFKNzAMMqCcfvqxasM4076288 = MogFKNzAMMqCcfvqxasM75859522;     MogFKNzAMMqCcfvqxasM75859522 = MogFKNzAMMqCcfvqxasM14324250;     MogFKNzAMMqCcfvqxasM14324250 = MogFKNzAMMqCcfvqxasM31127181;     MogFKNzAMMqCcfvqxasM31127181 = MogFKNzAMMqCcfvqxasM99575490;     MogFKNzAMMqCcfvqxasM99575490 = MogFKNzAMMqCcfvqxasM40133299;     MogFKNzAMMqCcfvqxasM40133299 = MogFKNzAMMqCcfvqxasM94036363;     MogFKNzAMMqCcfvqxasM94036363 = MogFKNzAMMqCcfvqxasM4182122;     MogFKNzAMMqCcfvqxasM4182122 = MogFKNzAMMqCcfvqxasM25623605;     MogFKNzAMMqCcfvqxasM25623605 = MogFKNzAMMqCcfvqxasM72195064;     MogFKNzAMMqCcfvqxasM72195064 = MogFKNzAMMqCcfvqxasM65240787;     MogFKNzAMMqCcfvqxasM65240787 = MogFKNzAMMqCcfvqxasM15341306;     MogFKNzAMMqCcfvqxasM15341306 = MogFKNzAMMqCcfvqxasM61769641;     MogFKNzAMMqCcfvqxasM61769641 = MogFKNzAMMqCcfvqxasM26823212;     MogFKNzAMMqCcfvqxasM26823212 = MogFKNzAMMqCcfvqxasM23563977;     MogFKNzAMMqCcfvqxasM23563977 = MogFKNzAMMqCcfvqxasM37479724;     MogFKNzAMMqCcfvqxasM37479724 = MogFKNzAMMqCcfvqxasM76529944;     MogFKNzAMMqCcfvqxasM76529944 = MogFKNzAMMqCcfvqxasM85176298;     MogFKNzAMMqCcfvqxasM85176298 = MogFKNzAMMqCcfvqxasM87579090;     MogFKNzAMMqCcfvqxasM87579090 = MogFKNzAMMqCcfvqxasM86886180;     MogFKNzAMMqCcfvqxasM86886180 = MogFKNzAMMqCcfvqxasM45601479;     MogFKNzAMMqCcfvqxasM45601479 = MogFKNzAMMqCcfvqxasM88957131;     MogFKNzAMMqCcfvqxasM88957131 = MogFKNzAMMqCcfvqxasM96283770;     MogFKNzAMMqCcfvqxasM96283770 = MogFKNzAMMqCcfvqxasM62051482;     MogFKNzAMMqCcfvqxasM62051482 = MogFKNzAMMqCcfvqxasM58462436;     MogFKNzAMMqCcfvqxasM58462436 = MogFKNzAMMqCcfvqxasM86257961;     MogFKNzAMMqCcfvqxasM86257961 = MogFKNzAMMqCcfvqxasM67595410;     MogFKNzAMMqCcfvqxasM67595410 = MogFKNzAMMqCcfvqxasM28799278;     MogFKNzAMMqCcfvqxasM28799278 = MogFKNzAMMqCcfvqxasM37222914;     MogFKNzAMMqCcfvqxasM37222914 = MogFKNzAMMqCcfvqxasM71998112;     MogFKNzAMMqCcfvqxasM71998112 = MogFKNzAMMqCcfvqxasM19574867;     MogFKNzAMMqCcfvqxasM19574867 = MogFKNzAMMqCcfvqxasM19187842;     MogFKNzAMMqCcfvqxasM19187842 = MogFKNzAMMqCcfvqxasM13777482;     MogFKNzAMMqCcfvqxasM13777482 = MogFKNzAMMqCcfvqxasM63065214;     MogFKNzAMMqCcfvqxasM63065214 = MogFKNzAMMqCcfvqxasM63407668;     MogFKNzAMMqCcfvqxasM63407668 = MogFKNzAMMqCcfvqxasM59826941;     MogFKNzAMMqCcfvqxasM59826941 = MogFKNzAMMqCcfvqxasM1415229;     MogFKNzAMMqCcfvqxasM1415229 = MogFKNzAMMqCcfvqxasM17605645;     MogFKNzAMMqCcfvqxasM17605645 = MogFKNzAMMqCcfvqxasM3620873;     MogFKNzAMMqCcfvqxasM3620873 = MogFKNzAMMqCcfvqxasM20268595;     MogFKNzAMMqCcfvqxasM20268595 = MogFKNzAMMqCcfvqxasM76131124;     MogFKNzAMMqCcfvqxasM76131124 = MogFKNzAMMqCcfvqxasM80316600;     MogFKNzAMMqCcfvqxasM80316600 = MogFKNzAMMqCcfvqxasM83329160;     MogFKNzAMMqCcfvqxasM83329160 = MogFKNzAMMqCcfvqxasM74633698;     MogFKNzAMMqCcfvqxasM74633698 = MogFKNzAMMqCcfvqxasM95137789;     MogFKNzAMMqCcfvqxasM95137789 = MogFKNzAMMqCcfvqxasM76568815;     MogFKNzAMMqCcfvqxasM76568815 = MogFKNzAMMqCcfvqxasM18972056;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void jiGjZyDKzVqdOtmgcjdX60637855() {     int DBiCUZDKuOrUlVJFPISl82402708 = -811062343;    int DBiCUZDKuOrUlVJFPISl62005887 = -211037244;    int DBiCUZDKuOrUlVJFPISl74089609 = -519817362;    int DBiCUZDKuOrUlVJFPISl32214442 = -734295510;    int DBiCUZDKuOrUlVJFPISl25551714 = -996680394;    int DBiCUZDKuOrUlVJFPISl74781491 = -949851936;    int DBiCUZDKuOrUlVJFPISl61740874 = -664988613;    int DBiCUZDKuOrUlVJFPISl60021183 = -132050374;    int DBiCUZDKuOrUlVJFPISl16053812 = -94638113;    int DBiCUZDKuOrUlVJFPISl75256464 = -313535805;    int DBiCUZDKuOrUlVJFPISl91460411 = -662509384;    int DBiCUZDKuOrUlVJFPISl82032025 = 40683675;    int DBiCUZDKuOrUlVJFPISl92475304 = -522332815;    int DBiCUZDKuOrUlVJFPISl93280346 = -486420698;    int DBiCUZDKuOrUlVJFPISl76553396 = -409248064;    int DBiCUZDKuOrUlVJFPISl25457454 = -869591504;    int DBiCUZDKuOrUlVJFPISl52229862 = -494720244;    int DBiCUZDKuOrUlVJFPISl92316966 = -285433102;    int DBiCUZDKuOrUlVJFPISl21929714 = -326734675;    int DBiCUZDKuOrUlVJFPISl25228412 = -908004386;    int DBiCUZDKuOrUlVJFPISl58357117 = -575306910;    int DBiCUZDKuOrUlVJFPISl5975731 = -729609070;    int DBiCUZDKuOrUlVJFPISl53625651 = -247816831;    int DBiCUZDKuOrUlVJFPISl47184957 = -937518452;    int DBiCUZDKuOrUlVJFPISl10799793 = -977899497;    int DBiCUZDKuOrUlVJFPISl86429153 = -705220719;    int DBiCUZDKuOrUlVJFPISl83922947 = -32642139;    int DBiCUZDKuOrUlVJFPISl12526227 = -614898971;    int DBiCUZDKuOrUlVJFPISl79018899 = -314766242;    int DBiCUZDKuOrUlVJFPISl34392604 = 67098304;    int DBiCUZDKuOrUlVJFPISl70725301 = -973655315;    int DBiCUZDKuOrUlVJFPISl39219517 = -261047464;    int DBiCUZDKuOrUlVJFPISl63134004 = -137041524;    int DBiCUZDKuOrUlVJFPISl63187877 = -207816716;    int DBiCUZDKuOrUlVJFPISl32013592 = -366955828;    int DBiCUZDKuOrUlVJFPISl91462247 = -849366526;    int DBiCUZDKuOrUlVJFPISl62781995 = -611147156;    int DBiCUZDKuOrUlVJFPISl42851444 = -129623120;    int DBiCUZDKuOrUlVJFPISl48215171 = -826861377;    int DBiCUZDKuOrUlVJFPISl75247139 = -825140099;    int DBiCUZDKuOrUlVJFPISl38717061 = -277164209;    int DBiCUZDKuOrUlVJFPISl18482215 = -617992880;    int DBiCUZDKuOrUlVJFPISl60229717 = -867874583;    int DBiCUZDKuOrUlVJFPISl39853022 = 83514332;    int DBiCUZDKuOrUlVJFPISl63704594 = -726037864;    int DBiCUZDKuOrUlVJFPISl36074172 = -26781550;    int DBiCUZDKuOrUlVJFPISl1011315 = 16983583;    int DBiCUZDKuOrUlVJFPISl4919722 = -177774558;    int DBiCUZDKuOrUlVJFPISl9246102 = -390733218;    int DBiCUZDKuOrUlVJFPISl62977241 = -202133941;    int DBiCUZDKuOrUlVJFPISl13937334 = -520753211;    int DBiCUZDKuOrUlVJFPISl74557376 = -202287662;    int DBiCUZDKuOrUlVJFPISl25587086 = -809011724;    int DBiCUZDKuOrUlVJFPISl68816976 = -703455169;    int DBiCUZDKuOrUlVJFPISl2280738 = -198592388;    int DBiCUZDKuOrUlVJFPISl76426978 = 18546727;    int DBiCUZDKuOrUlVJFPISl8380237 = -963220414;    int DBiCUZDKuOrUlVJFPISl26904652 = -582298911;    int DBiCUZDKuOrUlVJFPISl21414649 = -756396013;    int DBiCUZDKuOrUlVJFPISl39122561 = -191459676;    int DBiCUZDKuOrUlVJFPISl90858543 = -817209798;    int DBiCUZDKuOrUlVJFPISl49214648 = 49910357;    int DBiCUZDKuOrUlVJFPISl81002284 = -817284133;    int DBiCUZDKuOrUlVJFPISl81661208 = -61736418;    int DBiCUZDKuOrUlVJFPISl4531164 = -339880490;    int DBiCUZDKuOrUlVJFPISl52240894 = -301461921;    int DBiCUZDKuOrUlVJFPISl18898022 = -822274801;    int DBiCUZDKuOrUlVJFPISl29287427 = -214516100;    int DBiCUZDKuOrUlVJFPISl61266755 = -19464871;    int DBiCUZDKuOrUlVJFPISl85091148 = -559881539;    int DBiCUZDKuOrUlVJFPISl62675459 = -158444349;    int DBiCUZDKuOrUlVJFPISl9378419 = -265097124;    int DBiCUZDKuOrUlVJFPISl44101795 = -458571725;    int DBiCUZDKuOrUlVJFPISl46682574 = -501594576;    int DBiCUZDKuOrUlVJFPISl86511350 = -530840178;    int DBiCUZDKuOrUlVJFPISl39874902 = -957314030;    int DBiCUZDKuOrUlVJFPISl45746013 = -861734488;    int DBiCUZDKuOrUlVJFPISl13772630 = -231331163;    int DBiCUZDKuOrUlVJFPISl83480362 = -111480588;    int DBiCUZDKuOrUlVJFPISl74725621 = -851117948;    int DBiCUZDKuOrUlVJFPISl85417839 = -622204303;    int DBiCUZDKuOrUlVJFPISl79003225 = -854867581;    int DBiCUZDKuOrUlVJFPISl3280125 = -124165753;    int DBiCUZDKuOrUlVJFPISl16041658 = -12632301;    int DBiCUZDKuOrUlVJFPISl20455270 = -412148486;    int DBiCUZDKuOrUlVJFPISl96167925 = -671367654;    int DBiCUZDKuOrUlVJFPISl13632432 = -452035741;    int DBiCUZDKuOrUlVJFPISl94317027 = -433586356;    int DBiCUZDKuOrUlVJFPISl60907139 = 90775672;    int DBiCUZDKuOrUlVJFPISl55586614 = -285502555;    int DBiCUZDKuOrUlVJFPISl83082011 = -886146113;    int DBiCUZDKuOrUlVJFPISl35877343 = 71151755;    int DBiCUZDKuOrUlVJFPISl21436795 = -373227108;    int DBiCUZDKuOrUlVJFPISl9092611 = -535401702;    int DBiCUZDKuOrUlVJFPISl84388596 = 92069698;    int DBiCUZDKuOrUlVJFPISl89502413 = -227074566;    int DBiCUZDKuOrUlVJFPISl37479931 = -800708748;    int DBiCUZDKuOrUlVJFPISl78568509 = -706138165;    int DBiCUZDKuOrUlVJFPISl35321859 = -576605178;    int DBiCUZDKuOrUlVJFPISl11463701 = -811062343;     DBiCUZDKuOrUlVJFPISl82402708 = DBiCUZDKuOrUlVJFPISl62005887;     DBiCUZDKuOrUlVJFPISl62005887 = DBiCUZDKuOrUlVJFPISl74089609;     DBiCUZDKuOrUlVJFPISl74089609 = DBiCUZDKuOrUlVJFPISl32214442;     DBiCUZDKuOrUlVJFPISl32214442 = DBiCUZDKuOrUlVJFPISl25551714;     DBiCUZDKuOrUlVJFPISl25551714 = DBiCUZDKuOrUlVJFPISl74781491;     DBiCUZDKuOrUlVJFPISl74781491 = DBiCUZDKuOrUlVJFPISl61740874;     DBiCUZDKuOrUlVJFPISl61740874 = DBiCUZDKuOrUlVJFPISl60021183;     DBiCUZDKuOrUlVJFPISl60021183 = DBiCUZDKuOrUlVJFPISl16053812;     DBiCUZDKuOrUlVJFPISl16053812 = DBiCUZDKuOrUlVJFPISl75256464;     DBiCUZDKuOrUlVJFPISl75256464 = DBiCUZDKuOrUlVJFPISl91460411;     DBiCUZDKuOrUlVJFPISl91460411 = DBiCUZDKuOrUlVJFPISl82032025;     DBiCUZDKuOrUlVJFPISl82032025 = DBiCUZDKuOrUlVJFPISl92475304;     DBiCUZDKuOrUlVJFPISl92475304 = DBiCUZDKuOrUlVJFPISl93280346;     DBiCUZDKuOrUlVJFPISl93280346 = DBiCUZDKuOrUlVJFPISl76553396;     DBiCUZDKuOrUlVJFPISl76553396 = DBiCUZDKuOrUlVJFPISl25457454;     DBiCUZDKuOrUlVJFPISl25457454 = DBiCUZDKuOrUlVJFPISl52229862;     DBiCUZDKuOrUlVJFPISl52229862 = DBiCUZDKuOrUlVJFPISl92316966;     DBiCUZDKuOrUlVJFPISl92316966 = DBiCUZDKuOrUlVJFPISl21929714;     DBiCUZDKuOrUlVJFPISl21929714 = DBiCUZDKuOrUlVJFPISl25228412;     DBiCUZDKuOrUlVJFPISl25228412 = DBiCUZDKuOrUlVJFPISl58357117;     DBiCUZDKuOrUlVJFPISl58357117 = DBiCUZDKuOrUlVJFPISl5975731;     DBiCUZDKuOrUlVJFPISl5975731 = DBiCUZDKuOrUlVJFPISl53625651;     DBiCUZDKuOrUlVJFPISl53625651 = DBiCUZDKuOrUlVJFPISl47184957;     DBiCUZDKuOrUlVJFPISl47184957 = DBiCUZDKuOrUlVJFPISl10799793;     DBiCUZDKuOrUlVJFPISl10799793 = DBiCUZDKuOrUlVJFPISl86429153;     DBiCUZDKuOrUlVJFPISl86429153 = DBiCUZDKuOrUlVJFPISl83922947;     DBiCUZDKuOrUlVJFPISl83922947 = DBiCUZDKuOrUlVJFPISl12526227;     DBiCUZDKuOrUlVJFPISl12526227 = DBiCUZDKuOrUlVJFPISl79018899;     DBiCUZDKuOrUlVJFPISl79018899 = DBiCUZDKuOrUlVJFPISl34392604;     DBiCUZDKuOrUlVJFPISl34392604 = DBiCUZDKuOrUlVJFPISl70725301;     DBiCUZDKuOrUlVJFPISl70725301 = DBiCUZDKuOrUlVJFPISl39219517;     DBiCUZDKuOrUlVJFPISl39219517 = DBiCUZDKuOrUlVJFPISl63134004;     DBiCUZDKuOrUlVJFPISl63134004 = DBiCUZDKuOrUlVJFPISl63187877;     DBiCUZDKuOrUlVJFPISl63187877 = DBiCUZDKuOrUlVJFPISl32013592;     DBiCUZDKuOrUlVJFPISl32013592 = DBiCUZDKuOrUlVJFPISl91462247;     DBiCUZDKuOrUlVJFPISl91462247 = DBiCUZDKuOrUlVJFPISl62781995;     DBiCUZDKuOrUlVJFPISl62781995 = DBiCUZDKuOrUlVJFPISl42851444;     DBiCUZDKuOrUlVJFPISl42851444 = DBiCUZDKuOrUlVJFPISl48215171;     DBiCUZDKuOrUlVJFPISl48215171 = DBiCUZDKuOrUlVJFPISl75247139;     DBiCUZDKuOrUlVJFPISl75247139 = DBiCUZDKuOrUlVJFPISl38717061;     DBiCUZDKuOrUlVJFPISl38717061 = DBiCUZDKuOrUlVJFPISl18482215;     DBiCUZDKuOrUlVJFPISl18482215 = DBiCUZDKuOrUlVJFPISl60229717;     DBiCUZDKuOrUlVJFPISl60229717 = DBiCUZDKuOrUlVJFPISl39853022;     DBiCUZDKuOrUlVJFPISl39853022 = DBiCUZDKuOrUlVJFPISl63704594;     DBiCUZDKuOrUlVJFPISl63704594 = DBiCUZDKuOrUlVJFPISl36074172;     DBiCUZDKuOrUlVJFPISl36074172 = DBiCUZDKuOrUlVJFPISl1011315;     DBiCUZDKuOrUlVJFPISl1011315 = DBiCUZDKuOrUlVJFPISl4919722;     DBiCUZDKuOrUlVJFPISl4919722 = DBiCUZDKuOrUlVJFPISl9246102;     DBiCUZDKuOrUlVJFPISl9246102 = DBiCUZDKuOrUlVJFPISl62977241;     DBiCUZDKuOrUlVJFPISl62977241 = DBiCUZDKuOrUlVJFPISl13937334;     DBiCUZDKuOrUlVJFPISl13937334 = DBiCUZDKuOrUlVJFPISl74557376;     DBiCUZDKuOrUlVJFPISl74557376 = DBiCUZDKuOrUlVJFPISl25587086;     DBiCUZDKuOrUlVJFPISl25587086 = DBiCUZDKuOrUlVJFPISl68816976;     DBiCUZDKuOrUlVJFPISl68816976 = DBiCUZDKuOrUlVJFPISl2280738;     DBiCUZDKuOrUlVJFPISl2280738 = DBiCUZDKuOrUlVJFPISl76426978;     DBiCUZDKuOrUlVJFPISl76426978 = DBiCUZDKuOrUlVJFPISl8380237;     DBiCUZDKuOrUlVJFPISl8380237 = DBiCUZDKuOrUlVJFPISl26904652;     DBiCUZDKuOrUlVJFPISl26904652 = DBiCUZDKuOrUlVJFPISl21414649;     DBiCUZDKuOrUlVJFPISl21414649 = DBiCUZDKuOrUlVJFPISl39122561;     DBiCUZDKuOrUlVJFPISl39122561 = DBiCUZDKuOrUlVJFPISl90858543;     DBiCUZDKuOrUlVJFPISl90858543 = DBiCUZDKuOrUlVJFPISl49214648;     DBiCUZDKuOrUlVJFPISl49214648 = DBiCUZDKuOrUlVJFPISl81002284;     DBiCUZDKuOrUlVJFPISl81002284 = DBiCUZDKuOrUlVJFPISl81661208;     DBiCUZDKuOrUlVJFPISl81661208 = DBiCUZDKuOrUlVJFPISl4531164;     DBiCUZDKuOrUlVJFPISl4531164 = DBiCUZDKuOrUlVJFPISl52240894;     DBiCUZDKuOrUlVJFPISl52240894 = DBiCUZDKuOrUlVJFPISl18898022;     DBiCUZDKuOrUlVJFPISl18898022 = DBiCUZDKuOrUlVJFPISl29287427;     DBiCUZDKuOrUlVJFPISl29287427 = DBiCUZDKuOrUlVJFPISl61266755;     DBiCUZDKuOrUlVJFPISl61266755 = DBiCUZDKuOrUlVJFPISl85091148;     DBiCUZDKuOrUlVJFPISl85091148 = DBiCUZDKuOrUlVJFPISl62675459;     DBiCUZDKuOrUlVJFPISl62675459 = DBiCUZDKuOrUlVJFPISl9378419;     DBiCUZDKuOrUlVJFPISl9378419 = DBiCUZDKuOrUlVJFPISl44101795;     DBiCUZDKuOrUlVJFPISl44101795 = DBiCUZDKuOrUlVJFPISl46682574;     DBiCUZDKuOrUlVJFPISl46682574 = DBiCUZDKuOrUlVJFPISl86511350;     DBiCUZDKuOrUlVJFPISl86511350 = DBiCUZDKuOrUlVJFPISl39874902;     DBiCUZDKuOrUlVJFPISl39874902 = DBiCUZDKuOrUlVJFPISl45746013;     DBiCUZDKuOrUlVJFPISl45746013 = DBiCUZDKuOrUlVJFPISl13772630;     DBiCUZDKuOrUlVJFPISl13772630 = DBiCUZDKuOrUlVJFPISl83480362;     DBiCUZDKuOrUlVJFPISl83480362 = DBiCUZDKuOrUlVJFPISl74725621;     DBiCUZDKuOrUlVJFPISl74725621 = DBiCUZDKuOrUlVJFPISl85417839;     DBiCUZDKuOrUlVJFPISl85417839 = DBiCUZDKuOrUlVJFPISl79003225;     DBiCUZDKuOrUlVJFPISl79003225 = DBiCUZDKuOrUlVJFPISl3280125;     DBiCUZDKuOrUlVJFPISl3280125 = DBiCUZDKuOrUlVJFPISl16041658;     DBiCUZDKuOrUlVJFPISl16041658 = DBiCUZDKuOrUlVJFPISl20455270;     DBiCUZDKuOrUlVJFPISl20455270 = DBiCUZDKuOrUlVJFPISl96167925;     DBiCUZDKuOrUlVJFPISl96167925 = DBiCUZDKuOrUlVJFPISl13632432;     DBiCUZDKuOrUlVJFPISl13632432 = DBiCUZDKuOrUlVJFPISl94317027;     DBiCUZDKuOrUlVJFPISl94317027 = DBiCUZDKuOrUlVJFPISl60907139;     DBiCUZDKuOrUlVJFPISl60907139 = DBiCUZDKuOrUlVJFPISl55586614;     DBiCUZDKuOrUlVJFPISl55586614 = DBiCUZDKuOrUlVJFPISl83082011;     DBiCUZDKuOrUlVJFPISl83082011 = DBiCUZDKuOrUlVJFPISl35877343;     DBiCUZDKuOrUlVJFPISl35877343 = DBiCUZDKuOrUlVJFPISl21436795;     DBiCUZDKuOrUlVJFPISl21436795 = DBiCUZDKuOrUlVJFPISl9092611;     DBiCUZDKuOrUlVJFPISl9092611 = DBiCUZDKuOrUlVJFPISl84388596;     DBiCUZDKuOrUlVJFPISl84388596 = DBiCUZDKuOrUlVJFPISl89502413;     DBiCUZDKuOrUlVJFPISl89502413 = DBiCUZDKuOrUlVJFPISl37479931;     DBiCUZDKuOrUlVJFPISl37479931 = DBiCUZDKuOrUlVJFPISl78568509;     DBiCUZDKuOrUlVJFPISl78568509 = DBiCUZDKuOrUlVJFPISl35321859;     DBiCUZDKuOrUlVJFPISl35321859 = DBiCUZDKuOrUlVJFPISl11463701;     DBiCUZDKuOrUlVJFPISl11463701 = DBiCUZDKuOrUlVJFPISl82402708;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void SqlPOUOtjcePOMvVGsvw19289526() {     int qWezxoqdOtoyAypLgiul45833361 = -394835756;    int qWezxoqdOtoyAypLgiul87257466 = 29188142;    int qWezxoqdOtoyAypLgiul66679479 = -597891822;    int qWezxoqdOtoyAypLgiul37488420 = -748275236;    int qWezxoqdOtoyAypLgiul66666292 = -258403425;    int qWezxoqdOtoyAypLgiul52412719 = -791075166;    int qWezxoqdOtoyAypLgiul19171152 = 10684886;    int qWezxoqdOtoyAypLgiul12705011 = -450452690;    int qWezxoqdOtoyAypLgiul65036731 = -668567254;    int qWezxoqdOtoyAypLgiul9474494 = -893382322;    int qWezxoqdOtoyAypLgiul59065678 = -100352043;    int qWezxoqdOtoyAypLgiul59146714 = -593633091;    int qWezxoqdOtoyAypLgiul90643440 = -843977826;    int qWezxoqdOtoyAypLgiul27310253 = -119334215;    int qWezxoqdOtoyAypLgiul99887589 = -616412905;    int qWezxoqdOtoyAypLgiul25652138 = -356790942;    int qWezxoqdOtoyAypLgiul16087064 = -725991778;    int qWezxoqdOtoyAypLgiul36652950 = -160746622;    int qWezxoqdOtoyAypLgiul86805759 = -232291517;    int qWezxoqdOtoyAypLgiul20356623 = -222591171;    int qWezxoqdOtoyAypLgiul18804338 = -167815889;    int qWezxoqdOtoyAypLgiul68838928 = -444966829;    int qWezxoqdOtoyAypLgiul84821244 = -193204646;    int qWezxoqdOtoyAypLgiul43997356 = -476144297;    int qWezxoqdOtoyAypLgiul94234613 = -252508435;    int qWezxoqdOtoyAypLgiul28554469 = 41391265;    int qWezxoqdOtoyAypLgiul64731995 = 37606037;    int qWezxoqdOtoyAypLgiul24923978 = -970213390;    int qWezxoqdOtoyAypLgiul76324047 = -847838092;    int qWezxoqdOtoyAypLgiul73909378 = -683937059;    int qWezxoqdOtoyAypLgiul65652954 = 90781685;    int qWezxoqdOtoyAypLgiul69925196 = -860245717;    int qWezxoqdOtoyAypLgiul83120312 = -10133675;    int qWezxoqdOtoyAypLgiul58891799 = -316463259;    int qWezxoqdOtoyAypLgiul28340720 = -910038162;    int qWezxoqdOtoyAypLgiul67185016 = -893545189;    int qWezxoqdOtoyAypLgiul76831164 = 19170729;    int qWezxoqdOtoyAypLgiul82506524 = -489014073;    int qWezxoqdOtoyAypLgiul36028449 = 19340650;    int qWezxoqdOtoyAypLgiul80326791 = -974232438;    int qWezxoqdOtoyAypLgiul92935400 = -106056783;    int qWezxoqdOtoyAypLgiul28011665 = -301186116;    int qWezxoqdOtoyAypLgiul73630673 = 49824503;    int qWezxoqdOtoyAypLgiul19327468 = -464144968;    int qWezxoqdOtoyAypLgiul35499068 = -234838801;    int qWezxoqdOtoyAypLgiul31041333 = -694130175;    int qWezxoqdOtoyAypLgiul25314201 = -560382724;    int qWezxoqdOtoyAypLgiul35524822 = -348885892;    int qWezxoqdOtoyAypLgiul55586642 = -306504203;    int qWezxoqdOtoyAypLgiul16238845 = -624102318;    int qWezxoqdOtoyAypLgiul52573706 = -188437729;    int qWezxoqdOtoyAypLgiul92504944 = -831315668;    int qWezxoqdOtoyAypLgiul56437814 = -860171964;    int qWezxoqdOtoyAypLgiul57551471 = -946843617;    int qWezxoqdOtoyAypLgiul485189 = -222239677;    int qWezxoqdOtoyAypLgiul76994433 = -949868928;    int qWezxoqdOtoyAypLgiul2436223 = -777607212;    int qWezxoqdOtoyAypLgiul22682123 = -21747526;    int qWezxoqdOtoyAypLgiul43253806 = -395766801;    int qWezxoqdOtoyAypLgiul38111823 = -199794690;    int qWezxoqdOtoyAypLgiul87680724 = -728681204;    int qWezxoqdOtoyAypLgiul94247174 = -19101725;    int qWezxoqdOtoyAypLgiul36380964 = -602614598;    int qWezxoqdOtoyAypLgiul91127352 = -984630196;    int qWezxoqdOtoyAypLgiul43821540 = -884164008;    int qWezxoqdOtoyAypLgiul89140482 = -240106326;    int qWezxoqdOtoyAypLgiul76026402 = -483499417;    int qWezxoqdOtoyAypLgiul31751642 = -427514567;    int qWezxoqdOtoyAypLgiul98969532 = -209296053;    int qWezxoqdOtoyAypLgiul32702573 = -722867717;    int qWezxoqdOtoyAypLgiul48820974 = -275961672;    int qWezxoqdOtoyAypLgiul33580539 = -136977706;    int qWezxoqdOtoyAypLgiul624501 = -80087272;    int qWezxoqdOtoyAypLgiul6478969 = -258059079;    int qWezxoqdOtoyAypLgiul27421222 = -16534388;    int qWezxoqdOtoyAypLgiul90792672 = -866629774;    int qWezxoqdOtoyAypLgiul95208254 = -394791333;    int qWezxoqdOtoyAypLgiul65493776 = -729059679;    int qWezxoqdOtoyAypLgiul8498289 = -141305496;    int qWezxoqdOtoyAypLgiul63193280 = -558378261;    int qWezxoqdOtoyAypLgiul3240269 = -398226012;    int qWezxoqdOtoyAypLgiul29207174 = -613508071;    int qWezxoqdOtoyAypLgiul69337335 = -563709188;    int qWezxoqdOtoyAypLgiul60085202 = -123735774;    int qWezxoqdOtoyAypLgiul21335673 = -395499331;    int qWezxoqdOtoyAypLgiul73148009 = -77902647;    int qWezxoqdOtoyAypLgiul13487382 = 99926246;    int qWezxoqdOtoyAypLgiul25568841 = -63290059;    int qWezxoqdOtoyAypLgiul58406610 = 5776417;    int qWezxoqdOtoyAypLgiul51346287 = -960169235;    int qWezxoqdOtoyAypLgiul64748794 = -15937977;    int qWezxoqdOtoyAypLgiul54149041 = -959081746;    int qWezxoqdOtoyAypLgiul39252718 = 6752728;    int qWezxoqdOtoyAypLgiul97916626 = -780864661;    int qWezxoqdOtoyAypLgiul92646067 = -145551235;    int qWezxoqdOtoyAypLgiul98688226 = 13044941;    int qWezxoqdOtoyAypLgiul91630701 = -698571518;    int qWezxoqdOtoyAypLgiul82503320 = 34454699;    int qWezxoqdOtoyAypLgiul75505928 = -579980961;    int qWezxoqdOtoyAypLgiul46358585 = -394835756;     qWezxoqdOtoyAypLgiul45833361 = qWezxoqdOtoyAypLgiul87257466;     qWezxoqdOtoyAypLgiul87257466 = qWezxoqdOtoyAypLgiul66679479;     qWezxoqdOtoyAypLgiul66679479 = qWezxoqdOtoyAypLgiul37488420;     qWezxoqdOtoyAypLgiul37488420 = qWezxoqdOtoyAypLgiul66666292;     qWezxoqdOtoyAypLgiul66666292 = qWezxoqdOtoyAypLgiul52412719;     qWezxoqdOtoyAypLgiul52412719 = qWezxoqdOtoyAypLgiul19171152;     qWezxoqdOtoyAypLgiul19171152 = qWezxoqdOtoyAypLgiul12705011;     qWezxoqdOtoyAypLgiul12705011 = qWezxoqdOtoyAypLgiul65036731;     qWezxoqdOtoyAypLgiul65036731 = qWezxoqdOtoyAypLgiul9474494;     qWezxoqdOtoyAypLgiul9474494 = qWezxoqdOtoyAypLgiul59065678;     qWezxoqdOtoyAypLgiul59065678 = qWezxoqdOtoyAypLgiul59146714;     qWezxoqdOtoyAypLgiul59146714 = qWezxoqdOtoyAypLgiul90643440;     qWezxoqdOtoyAypLgiul90643440 = qWezxoqdOtoyAypLgiul27310253;     qWezxoqdOtoyAypLgiul27310253 = qWezxoqdOtoyAypLgiul99887589;     qWezxoqdOtoyAypLgiul99887589 = qWezxoqdOtoyAypLgiul25652138;     qWezxoqdOtoyAypLgiul25652138 = qWezxoqdOtoyAypLgiul16087064;     qWezxoqdOtoyAypLgiul16087064 = qWezxoqdOtoyAypLgiul36652950;     qWezxoqdOtoyAypLgiul36652950 = qWezxoqdOtoyAypLgiul86805759;     qWezxoqdOtoyAypLgiul86805759 = qWezxoqdOtoyAypLgiul20356623;     qWezxoqdOtoyAypLgiul20356623 = qWezxoqdOtoyAypLgiul18804338;     qWezxoqdOtoyAypLgiul18804338 = qWezxoqdOtoyAypLgiul68838928;     qWezxoqdOtoyAypLgiul68838928 = qWezxoqdOtoyAypLgiul84821244;     qWezxoqdOtoyAypLgiul84821244 = qWezxoqdOtoyAypLgiul43997356;     qWezxoqdOtoyAypLgiul43997356 = qWezxoqdOtoyAypLgiul94234613;     qWezxoqdOtoyAypLgiul94234613 = qWezxoqdOtoyAypLgiul28554469;     qWezxoqdOtoyAypLgiul28554469 = qWezxoqdOtoyAypLgiul64731995;     qWezxoqdOtoyAypLgiul64731995 = qWezxoqdOtoyAypLgiul24923978;     qWezxoqdOtoyAypLgiul24923978 = qWezxoqdOtoyAypLgiul76324047;     qWezxoqdOtoyAypLgiul76324047 = qWezxoqdOtoyAypLgiul73909378;     qWezxoqdOtoyAypLgiul73909378 = qWezxoqdOtoyAypLgiul65652954;     qWezxoqdOtoyAypLgiul65652954 = qWezxoqdOtoyAypLgiul69925196;     qWezxoqdOtoyAypLgiul69925196 = qWezxoqdOtoyAypLgiul83120312;     qWezxoqdOtoyAypLgiul83120312 = qWezxoqdOtoyAypLgiul58891799;     qWezxoqdOtoyAypLgiul58891799 = qWezxoqdOtoyAypLgiul28340720;     qWezxoqdOtoyAypLgiul28340720 = qWezxoqdOtoyAypLgiul67185016;     qWezxoqdOtoyAypLgiul67185016 = qWezxoqdOtoyAypLgiul76831164;     qWezxoqdOtoyAypLgiul76831164 = qWezxoqdOtoyAypLgiul82506524;     qWezxoqdOtoyAypLgiul82506524 = qWezxoqdOtoyAypLgiul36028449;     qWezxoqdOtoyAypLgiul36028449 = qWezxoqdOtoyAypLgiul80326791;     qWezxoqdOtoyAypLgiul80326791 = qWezxoqdOtoyAypLgiul92935400;     qWezxoqdOtoyAypLgiul92935400 = qWezxoqdOtoyAypLgiul28011665;     qWezxoqdOtoyAypLgiul28011665 = qWezxoqdOtoyAypLgiul73630673;     qWezxoqdOtoyAypLgiul73630673 = qWezxoqdOtoyAypLgiul19327468;     qWezxoqdOtoyAypLgiul19327468 = qWezxoqdOtoyAypLgiul35499068;     qWezxoqdOtoyAypLgiul35499068 = qWezxoqdOtoyAypLgiul31041333;     qWezxoqdOtoyAypLgiul31041333 = qWezxoqdOtoyAypLgiul25314201;     qWezxoqdOtoyAypLgiul25314201 = qWezxoqdOtoyAypLgiul35524822;     qWezxoqdOtoyAypLgiul35524822 = qWezxoqdOtoyAypLgiul55586642;     qWezxoqdOtoyAypLgiul55586642 = qWezxoqdOtoyAypLgiul16238845;     qWezxoqdOtoyAypLgiul16238845 = qWezxoqdOtoyAypLgiul52573706;     qWezxoqdOtoyAypLgiul52573706 = qWezxoqdOtoyAypLgiul92504944;     qWezxoqdOtoyAypLgiul92504944 = qWezxoqdOtoyAypLgiul56437814;     qWezxoqdOtoyAypLgiul56437814 = qWezxoqdOtoyAypLgiul57551471;     qWezxoqdOtoyAypLgiul57551471 = qWezxoqdOtoyAypLgiul485189;     qWezxoqdOtoyAypLgiul485189 = qWezxoqdOtoyAypLgiul76994433;     qWezxoqdOtoyAypLgiul76994433 = qWezxoqdOtoyAypLgiul2436223;     qWezxoqdOtoyAypLgiul2436223 = qWezxoqdOtoyAypLgiul22682123;     qWezxoqdOtoyAypLgiul22682123 = qWezxoqdOtoyAypLgiul43253806;     qWezxoqdOtoyAypLgiul43253806 = qWezxoqdOtoyAypLgiul38111823;     qWezxoqdOtoyAypLgiul38111823 = qWezxoqdOtoyAypLgiul87680724;     qWezxoqdOtoyAypLgiul87680724 = qWezxoqdOtoyAypLgiul94247174;     qWezxoqdOtoyAypLgiul94247174 = qWezxoqdOtoyAypLgiul36380964;     qWezxoqdOtoyAypLgiul36380964 = qWezxoqdOtoyAypLgiul91127352;     qWezxoqdOtoyAypLgiul91127352 = qWezxoqdOtoyAypLgiul43821540;     qWezxoqdOtoyAypLgiul43821540 = qWezxoqdOtoyAypLgiul89140482;     qWezxoqdOtoyAypLgiul89140482 = qWezxoqdOtoyAypLgiul76026402;     qWezxoqdOtoyAypLgiul76026402 = qWezxoqdOtoyAypLgiul31751642;     qWezxoqdOtoyAypLgiul31751642 = qWezxoqdOtoyAypLgiul98969532;     qWezxoqdOtoyAypLgiul98969532 = qWezxoqdOtoyAypLgiul32702573;     qWezxoqdOtoyAypLgiul32702573 = qWezxoqdOtoyAypLgiul48820974;     qWezxoqdOtoyAypLgiul48820974 = qWezxoqdOtoyAypLgiul33580539;     qWezxoqdOtoyAypLgiul33580539 = qWezxoqdOtoyAypLgiul624501;     qWezxoqdOtoyAypLgiul624501 = qWezxoqdOtoyAypLgiul6478969;     qWezxoqdOtoyAypLgiul6478969 = qWezxoqdOtoyAypLgiul27421222;     qWezxoqdOtoyAypLgiul27421222 = qWezxoqdOtoyAypLgiul90792672;     qWezxoqdOtoyAypLgiul90792672 = qWezxoqdOtoyAypLgiul95208254;     qWezxoqdOtoyAypLgiul95208254 = qWezxoqdOtoyAypLgiul65493776;     qWezxoqdOtoyAypLgiul65493776 = qWezxoqdOtoyAypLgiul8498289;     qWezxoqdOtoyAypLgiul8498289 = qWezxoqdOtoyAypLgiul63193280;     qWezxoqdOtoyAypLgiul63193280 = qWezxoqdOtoyAypLgiul3240269;     qWezxoqdOtoyAypLgiul3240269 = qWezxoqdOtoyAypLgiul29207174;     qWezxoqdOtoyAypLgiul29207174 = qWezxoqdOtoyAypLgiul69337335;     qWezxoqdOtoyAypLgiul69337335 = qWezxoqdOtoyAypLgiul60085202;     qWezxoqdOtoyAypLgiul60085202 = qWezxoqdOtoyAypLgiul21335673;     qWezxoqdOtoyAypLgiul21335673 = qWezxoqdOtoyAypLgiul73148009;     qWezxoqdOtoyAypLgiul73148009 = qWezxoqdOtoyAypLgiul13487382;     qWezxoqdOtoyAypLgiul13487382 = qWezxoqdOtoyAypLgiul25568841;     qWezxoqdOtoyAypLgiul25568841 = qWezxoqdOtoyAypLgiul58406610;     qWezxoqdOtoyAypLgiul58406610 = qWezxoqdOtoyAypLgiul51346287;     qWezxoqdOtoyAypLgiul51346287 = qWezxoqdOtoyAypLgiul64748794;     qWezxoqdOtoyAypLgiul64748794 = qWezxoqdOtoyAypLgiul54149041;     qWezxoqdOtoyAypLgiul54149041 = qWezxoqdOtoyAypLgiul39252718;     qWezxoqdOtoyAypLgiul39252718 = qWezxoqdOtoyAypLgiul97916626;     qWezxoqdOtoyAypLgiul97916626 = qWezxoqdOtoyAypLgiul92646067;     qWezxoqdOtoyAypLgiul92646067 = qWezxoqdOtoyAypLgiul98688226;     qWezxoqdOtoyAypLgiul98688226 = qWezxoqdOtoyAypLgiul91630701;     qWezxoqdOtoyAypLgiul91630701 = qWezxoqdOtoyAypLgiul82503320;     qWezxoqdOtoyAypLgiul82503320 = qWezxoqdOtoyAypLgiul75505928;     qWezxoqdOtoyAypLgiul75505928 = qWezxoqdOtoyAypLgiul46358585;     qWezxoqdOtoyAypLgiul46358585 = qWezxoqdOtoyAypLgiul45833361;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void pUuUwTnHepPBgyvJkTDg25698666() {     int dTnVftPewZhGUDgjqpJr79922151 = -724234158;    int dTnVftPewZhGUDgjqpJr42610247 = -659634646;    int dTnVftPewZhGUDgjqpJr37925057 = -61132128;    int dTnVftPewZhGUDgjqpJr97613823 = -31852522;    int dTnVftPewZhGUDgjqpJr20391089 = 64822518;    int dTnVftPewZhGUDgjqpJr85238278 = -634446735;    int dTnVftPewZhGUDgjqpJr26432989 = -707880857;    int dTnVftPewZhGUDgjqpJr53017169 = -311494778;    int dTnVftPewZhGUDgjqpJr77144541 = -522197654;    int dTnVftPewZhGUDgjqpJr70019470 = 62482680;    int dTnVftPewZhGUDgjqpJr48599501 = -713624558;    int dTnVftPewZhGUDgjqpJr67516296 = -708235288;    int dTnVftPewZhGUDgjqpJr36786051 = 46498580;    int dTnVftPewZhGUDgjqpJr91342972 = -308122890;    int dTnVftPewZhGUDgjqpJr60769693 = 89385103;    int dTnVftPewZhGUDgjqpJr83810915 = -181178317;    int dTnVftPewZhGUDgjqpJr17944332 = 53166127;    int dTnVftPewZhGUDgjqpJr78281894 = -728154716;    int dTnVftPewZhGUDgjqpJr12961318 = -227054034;    int dTnVftPewZhGUDgjqpJr2197780 = -567519809;    int dTnVftPewZhGUDgjqpJr54717888 = 41462366;    int dTnVftPewZhGUDgjqpJr14801625 = 18381990;    int dTnVftPewZhGUDgjqpJr81082249 = -247477765;    int dTnVftPewZhGUDgjqpJr31009185 = -826250525;    int dTnVftPewZhGUDgjqpJr13351133 = -976328208;    int dTnVftPewZhGUDgjqpJr84703288 = -502075268;    int dTnVftPewZhGUDgjqpJr83580247 = 13476151;    int dTnVftPewZhGUDgjqpJr9700430 = -121684588;    int dTnVftPewZhGUDgjqpJr93093236 = 70802156;    int dTnVftPewZhGUDgjqpJr76508379 = -117936119;    int dTnVftPewZhGUDgjqpJr34239925 = -406121766;    int dTnVftPewZhGUDgjqpJr60418478 = -295919371;    int dTnVftPewZhGUDgjqpJr599558 = -728243219;    int dTnVftPewZhGUDgjqpJr19677858 = -111957754;    int dTnVftPewZhGUDgjqpJr20334047 = 59246648;    int dTnVftPewZhGUDgjqpJr60072855 = -937184653;    int dTnVftPewZhGUDgjqpJr22730452 = -539451216;    int dTnVftPewZhGUDgjqpJr5036414 = -637255454;    int dTnVftPewZhGUDgjqpJr67061419 = -577504544;    int dTnVftPewZhGUDgjqpJr8636849 = 68498546;    int dTnVftPewZhGUDgjqpJr19371357 = -231024337;    int dTnVftPewZhGUDgjqpJr80326445 = -659976338;    int dTnVftPewZhGUDgjqpJr75715228 = -660100208;    int dTnVftPewZhGUDgjqpJr46078316 = -426814797;    int dTnVftPewZhGUDgjqpJr19229547 = -679026731;    int dTnVftPewZhGUDgjqpJr91986361 = -207771284;    int dTnVftPewZhGUDgjqpJr48050498 = -772538208;    int dTnVftPewZhGUDgjqpJr56360551 = -363366164;    int dTnVftPewZhGUDgjqpJr88710245 = -644275295;    int dTnVftPewZhGUDgjqpJr57882511 = -642054688;    int dTnVftPewZhGUDgjqpJr46140033 = -924139360;    int dTnVftPewZhGUDgjqpJr37776761 = 52582352;    int dTnVftPewZhGUDgjqpJr66915852 = -947076238;    int dTnVftPewZhGUDgjqpJr26263000 = -219193710;    int dTnVftPewZhGUDgjqpJr70980777 = -599928602;    int dTnVftPewZhGUDgjqpJr65120527 = -642616149;    int dTnVftPewZhGUDgjqpJr61527998 = -312156882;    int dTnVftPewZhGUDgjqpJr6915872 = -234881603;    int dTnVftPewZhGUDgjqpJr84262691 = -55524314;    int dTnVftPewZhGUDgjqpJr35687801 = -433102215;    int dTnVftPewZhGUDgjqpJr1658032 = -547922887;    int dTnVftPewZhGUDgjqpJr16732560 = -486196269;    int dTnVftPewZhGUDgjqpJr59923933 = -282296935;    int dTnVftPewZhGUDgjqpJr636162 = -304261535;    int dTnVftPewZhGUDgjqpJr35779545 = -531395554;    int dTnVftPewZhGUDgjqpJr88181022 = -317705188;    int dTnVftPewZhGUDgjqpJr66916739 = -979992070;    int dTnVftPewZhGUDgjqpJr17108193 = -841543667;    int dTnVftPewZhGUDgjqpJr71008926 = -267369539;    int dTnVftPewZhGUDgjqpJr696839 = 26569755;    int dTnVftPewZhGUDgjqpJr61080464 = -641727101;    int dTnVftPewZhGUDgjqpJr12907918 = -309578419;    int dTnVftPewZhGUDgjqpJr11220475 = -50650172;    int dTnVftPewZhGUDgjqpJr4324469 = -195552581;    int dTnVftPewZhGUDgjqpJr82826422 = -236495473;    int dTnVftPewZhGUDgjqpJr74391443 = -298561296;    int dTnVftPewZhGUDgjqpJr39086396 = -321517802;    int dTnVftPewZhGUDgjqpJr35003933 = -820662969;    int dTnVftPewZhGUDgjqpJr11779638 = -47223794;    int dTnVftPewZhGUDgjqpJr21364772 = -668556924;    int dTnVftPewZhGUDgjqpJr36652791 = -729537060;    int dTnVftPewZhGUDgjqpJr27219696 = -623157685;    int dTnVftPewZhGUDgjqpJr20990184 = -477409294;    int dTnVftPewZhGUDgjqpJr35210725 = -287143157;    int dTnVftPewZhGUDgjqpJr30368347 = -193796759;    int dTnVftPewZhGUDgjqpJr96463164 = -358704118;    int dTnVftPewZhGUDgjqpJr93502625 = -348843134;    int dTnVftPewZhGUDgjqpJr74336558 = -409049509;    int dTnVftPewZhGUDgjqpJr48697080 = -512029152;    int dTnVftPewZhGUDgjqpJr55213519 = -298137203;    int dTnVftPewZhGUDgjqpJr98544857 = -525027771;    int dTnVftPewZhGUDgjqpJr15814580 = -204569613;    int dTnVftPewZhGUDgjqpJr20773723 = -481731140;    int dTnVftPewZhGUDgjqpJr31373619 = -44402330;    int dTnVftPewZhGUDgjqpJr6978818 = -383578568;    int dTnVftPewZhGUDgjqpJr2638798 = -744828068;    int dTnVftPewZhGUDgjqpJr20402513 = -277679404;    int dTnVftPewZhGUDgjqpJr75079066 = -255838674;    int dTnVftPewZhGUDgjqpJr10298771 = -895419243;    int dTnVftPewZhGUDgjqpJr31048524 = -724234158;     dTnVftPewZhGUDgjqpJr79922151 = dTnVftPewZhGUDgjqpJr42610247;     dTnVftPewZhGUDgjqpJr42610247 = dTnVftPewZhGUDgjqpJr37925057;     dTnVftPewZhGUDgjqpJr37925057 = dTnVftPewZhGUDgjqpJr97613823;     dTnVftPewZhGUDgjqpJr97613823 = dTnVftPewZhGUDgjqpJr20391089;     dTnVftPewZhGUDgjqpJr20391089 = dTnVftPewZhGUDgjqpJr85238278;     dTnVftPewZhGUDgjqpJr85238278 = dTnVftPewZhGUDgjqpJr26432989;     dTnVftPewZhGUDgjqpJr26432989 = dTnVftPewZhGUDgjqpJr53017169;     dTnVftPewZhGUDgjqpJr53017169 = dTnVftPewZhGUDgjqpJr77144541;     dTnVftPewZhGUDgjqpJr77144541 = dTnVftPewZhGUDgjqpJr70019470;     dTnVftPewZhGUDgjqpJr70019470 = dTnVftPewZhGUDgjqpJr48599501;     dTnVftPewZhGUDgjqpJr48599501 = dTnVftPewZhGUDgjqpJr67516296;     dTnVftPewZhGUDgjqpJr67516296 = dTnVftPewZhGUDgjqpJr36786051;     dTnVftPewZhGUDgjqpJr36786051 = dTnVftPewZhGUDgjqpJr91342972;     dTnVftPewZhGUDgjqpJr91342972 = dTnVftPewZhGUDgjqpJr60769693;     dTnVftPewZhGUDgjqpJr60769693 = dTnVftPewZhGUDgjqpJr83810915;     dTnVftPewZhGUDgjqpJr83810915 = dTnVftPewZhGUDgjqpJr17944332;     dTnVftPewZhGUDgjqpJr17944332 = dTnVftPewZhGUDgjqpJr78281894;     dTnVftPewZhGUDgjqpJr78281894 = dTnVftPewZhGUDgjqpJr12961318;     dTnVftPewZhGUDgjqpJr12961318 = dTnVftPewZhGUDgjqpJr2197780;     dTnVftPewZhGUDgjqpJr2197780 = dTnVftPewZhGUDgjqpJr54717888;     dTnVftPewZhGUDgjqpJr54717888 = dTnVftPewZhGUDgjqpJr14801625;     dTnVftPewZhGUDgjqpJr14801625 = dTnVftPewZhGUDgjqpJr81082249;     dTnVftPewZhGUDgjqpJr81082249 = dTnVftPewZhGUDgjqpJr31009185;     dTnVftPewZhGUDgjqpJr31009185 = dTnVftPewZhGUDgjqpJr13351133;     dTnVftPewZhGUDgjqpJr13351133 = dTnVftPewZhGUDgjqpJr84703288;     dTnVftPewZhGUDgjqpJr84703288 = dTnVftPewZhGUDgjqpJr83580247;     dTnVftPewZhGUDgjqpJr83580247 = dTnVftPewZhGUDgjqpJr9700430;     dTnVftPewZhGUDgjqpJr9700430 = dTnVftPewZhGUDgjqpJr93093236;     dTnVftPewZhGUDgjqpJr93093236 = dTnVftPewZhGUDgjqpJr76508379;     dTnVftPewZhGUDgjqpJr76508379 = dTnVftPewZhGUDgjqpJr34239925;     dTnVftPewZhGUDgjqpJr34239925 = dTnVftPewZhGUDgjqpJr60418478;     dTnVftPewZhGUDgjqpJr60418478 = dTnVftPewZhGUDgjqpJr599558;     dTnVftPewZhGUDgjqpJr599558 = dTnVftPewZhGUDgjqpJr19677858;     dTnVftPewZhGUDgjqpJr19677858 = dTnVftPewZhGUDgjqpJr20334047;     dTnVftPewZhGUDgjqpJr20334047 = dTnVftPewZhGUDgjqpJr60072855;     dTnVftPewZhGUDgjqpJr60072855 = dTnVftPewZhGUDgjqpJr22730452;     dTnVftPewZhGUDgjqpJr22730452 = dTnVftPewZhGUDgjqpJr5036414;     dTnVftPewZhGUDgjqpJr5036414 = dTnVftPewZhGUDgjqpJr67061419;     dTnVftPewZhGUDgjqpJr67061419 = dTnVftPewZhGUDgjqpJr8636849;     dTnVftPewZhGUDgjqpJr8636849 = dTnVftPewZhGUDgjqpJr19371357;     dTnVftPewZhGUDgjqpJr19371357 = dTnVftPewZhGUDgjqpJr80326445;     dTnVftPewZhGUDgjqpJr80326445 = dTnVftPewZhGUDgjqpJr75715228;     dTnVftPewZhGUDgjqpJr75715228 = dTnVftPewZhGUDgjqpJr46078316;     dTnVftPewZhGUDgjqpJr46078316 = dTnVftPewZhGUDgjqpJr19229547;     dTnVftPewZhGUDgjqpJr19229547 = dTnVftPewZhGUDgjqpJr91986361;     dTnVftPewZhGUDgjqpJr91986361 = dTnVftPewZhGUDgjqpJr48050498;     dTnVftPewZhGUDgjqpJr48050498 = dTnVftPewZhGUDgjqpJr56360551;     dTnVftPewZhGUDgjqpJr56360551 = dTnVftPewZhGUDgjqpJr88710245;     dTnVftPewZhGUDgjqpJr88710245 = dTnVftPewZhGUDgjqpJr57882511;     dTnVftPewZhGUDgjqpJr57882511 = dTnVftPewZhGUDgjqpJr46140033;     dTnVftPewZhGUDgjqpJr46140033 = dTnVftPewZhGUDgjqpJr37776761;     dTnVftPewZhGUDgjqpJr37776761 = dTnVftPewZhGUDgjqpJr66915852;     dTnVftPewZhGUDgjqpJr66915852 = dTnVftPewZhGUDgjqpJr26263000;     dTnVftPewZhGUDgjqpJr26263000 = dTnVftPewZhGUDgjqpJr70980777;     dTnVftPewZhGUDgjqpJr70980777 = dTnVftPewZhGUDgjqpJr65120527;     dTnVftPewZhGUDgjqpJr65120527 = dTnVftPewZhGUDgjqpJr61527998;     dTnVftPewZhGUDgjqpJr61527998 = dTnVftPewZhGUDgjqpJr6915872;     dTnVftPewZhGUDgjqpJr6915872 = dTnVftPewZhGUDgjqpJr84262691;     dTnVftPewZhGUDgjqpJr84262691 = dTnVftPewZhGUDgjqpJr35687801;     dTnVftPewZhGUDgjqpJr35687801 = dTnVftPewZhGUDgjqpJr1658032;     dTnVftPewZhGUDgjqpJr1658032 = dTnVftPewZhGUDgjqpJr16732560;     dTnVftPewZhGUDgjqpJr16732560 = dTnVftPewZhGUDgjqpJr59923933;     dTnVftPewZhGUDgjqpJr59923933 = dTnVftPewZhGUDgjqpJr636162;     dTnVftPewZhGUDgjqpJr636162 = dTnVftPewZhGUDgjqpJr35779545;     dTnVftPewZhGUDgjqpJr35779545 = dTnVftPewZhGUDgjqpJr88181022;     dTnVftPewZhGUDgjqpJr88181022 = dTnVftPewZhGUDgjqpJr66916739;     dTnVftPewZhGUDgjqpJr66916739 = dTnVftPewZhGUDgjqpJr17108193;     dTnVftPewZhGUDgjqpJr17108193 = dTnVftPewZhGUDgjqpJr71008926;     dTnVftPewZhGUDgjqpJr71008926 = dTnVftPewZhGUDgjqpJr696839;     dTnVftPewZhGUDgjqpJr696839 = dTnVftPewZhGUDgjqpJr61080464;     dTnVftPewZhGUDgjqpJr61080464 = dTnVftPewZhGUDgjqpJr12907918;     dTnVftPewZhGUDgjqpJr12907918 = dTnVftPewZhGUDgjqpJr11220475;     dTnVftPewZhGUDgjqpJr11220475 = dTnVftPewZhGUDgjqpJr4324469;     dTnVftPewZhGUDgjqpJr4324469 = dTnVftPewZhGUDgjqpJr82826422;     dTnVftPewZhGUDgjqpJr82826422 = dTnVftPewZhGUDgjqpJr74391443;     dTnVftPewZhGUDgjqpJr74391443 = dTnVftPewZhGUDgjqpJr39086396;     dTnVftPewZhGUDgjqpJr39086396 = dTnVftPewZhGUDgjqpJr35003933;     dTnVftPewZhGUDgjqpJr35003933 = dTnVftPewZhGUDgjqpJr11779638;     dTnVftPewZhGUDgjqpJr11779638 = dTnVftPewZhGUDgjqpJr21364772;     dTnVftPewZhGUDgjqpJr21364772 = dTnVftPewZhGUDgjqpJr36652791;     dTnVftPewZhGUDgjqpJr36652791 = dTnVftPewZhGUDgjqpJr27219696;     dTnVftPewZhGUDgjqpJr27219696 = dTnVftPewZhGUDgjqpJr20990184;     dTnVftPewZhGUDgjqpJr20990184 = dTnVftPewZhGUDgjqpJr35210725;     dTnVftPewZhGUDgjqpJr35210725 = dTnVftPewZhGUDgjqpJr30368347;     dTnVftPewZhGUDgjqpJr30368347 = dTnVftPewZhGUDgjqpJr96463164;     dTnVftPewZhGUDgjqpJr96463164 = dTnVftPewZhGUDgjqpJr93502625;     dTnVftPewZhGUDgjqpJr93502625 = dTnVftPewZhGUDgjqpJr74336558;     dTnVftPewZhGUDgjqpJr74336558 = dTnVftPewZhGUDgjqpJr48697080;     dTnVftPewZhGUDgjqpJr48697080 = dTnVftPewZhGUDgjqpJr55213519;     dTnVftPewZhGUDgjqpJr55213519 = dTnVftPewZhGUDgjqpJr98544857;     dTnVftPewZhGUDgjqpJr98544857 = dTnVftPewZhGUDgjqpJr15814580;     dTnVftPewZhGUDgjqpJr15814580 = dTnVftPewZhGUDgjqpJr20773723;     dTnVftPewZhGUDgjqpJr20773723 = dTnVftPewZhGUDgjqpJr31373619;     dTnVftPewZhGUDgjqpJr31373619 = dTnVftPewZhGUDgjqpJr6978818;     dTnVftPewZhGUDgjqpJr6978818 = dTnVftPewZhGUDgjqpJr2638798;     dTnVftPewZhGUDgjqpJr2638798 = dTnVftPewZhGUDgjqpJr20402513;     dTnVftPewZhGUDgjqpJr20402513 = dTnVftPewZhGUDgjqpJr75079066;     dTnVftPewZhGUDgjqpJr75079066 = dTnVftPewZhGUDgjqpJr10298771;     dTnVftPewZhGUDgjqpJr10298771 = dTnVftPewZhGUDgjqpJr31048524;     dTnVftPewZhGUDgjqpJr31048524 = dTnVftPewZhGUDgjqpJr79922151;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void sTTDRKCMQKyXuyfphUPr84350336() {     int nKaXwoPJHVtkczXNwuaI43352804 = -308007572;    int nKaXwoPJHVtkczXNwuaI67861826 = -419409261;    int nKaXwoPJHVtkczXNwuaI30514927 = -139206588;    int nKaXwoPJHVtkczXNwuaI2887803 = -45832248;    int nKaXwoPJHVtkczXNwuaI61505667 = -296900513;    int nKaXwoPJHVtkczXNwuaI62869506 = -475669965;    int nKaXwoPJHVtkczXNwuaI83863266 = -32207358;    int nKaXwoPJHVtkczXNwuaI5700997 = -629897094;    int nKaXwoPJHVtkczXNwuaI26127461 = 3873205;    int nKaXwoPJHVtkczXNwuaI4237500 = -517363837;    int nKaXwoPJHVtkczXNwuaI16204768 = -151467217;    int nKaXwoPJHVtkczXNwuaI44630985 = -242552055;    int nKaXwoPJHVtkczXNwuaI34954188 = -275146431;    int nKaXwoPJHVtkczXNwuaI25372879 = 58963593;    int nKaXwoPJHVtkczXNwuaI84103886 = -117779738;    int nKaXwoPJHVtkczXNwuaI84005599 = -768377755;    int nKaXwoPJHVtkczXNwuaI81801532 = -178105407;    int nKaXwoPJHVtkczXNwuaI22617877 = -603468236;    int nKaXwoPJHVtkczXNwuaI77837363 = -132610876;    int nKaXwoPJHVtkczXNwuaI97325989 = -982106594;    int nKaXwoPJHVtkczXNwuaI15165109 = -651046612;    int nKaXwoPJHVtkczXNwuaI77664821 = -796975769;    int nKaXwoPJHVtkczXNwuaI12277843 = -192865581;    int nKaXwoPJHVtkczXNwuaI27821584 = -364876370;    int nKaXwoPJHVtkczXNwuaI96785953 = -250937146;    int nKaXwoPJHVtkczXNwuaI26828605 = -855463285;    int nKaXwoPJHVtkczXNwuaI64389295 = 83724327;    int nKaXwoPJHVtkczXNwuaI22098181 = -476999007;    int nKaXwoPJHVtkczXNwuaI90398384 = -462269694;    int nKaXwoPJHVtkczXNwuaI16025155 = -868971483;    int nKaXwoPJHVtkczXNwuaI29167578 = -441684766;    int nKaXwoPJHVtkczXNwuaI91124157 = -895117625;    int nKaXwoPJHVtkczXNwuaI20585866 = -601335370;    int nKaXwoPJHVtkczXNwuaI15381780 = -220604297;    int nKaXwoPJHVtkczXNwuaI16661175 = -483835686;    int nKaXwoPJHVtkczXNwuaI35795624 = -981363315;    int nKaXwoPJHVtkczXNwuaI36779621 = 90866669;    int nKaXwoPJHVtkczXNwuaI44691494 = -996646407;    int nKaXwoPJHVtkczXNwuaI54874697 = -831302517;    int nKaXwoPJHVtkczXNwuaI13716501 = -80593793;    int nKaXwoPJHVtkczXNwuaI73589696 = -59916911;    int nKaXwoPJHVtkczXNwuaI89855895 = -343169573;    int nKaXwoPJHVtkczXNwuaI89116183 = -842401122;    int nKaXwoPJHVtkczXNwuaI25552763 = -974474097;    int nKaXwoPJHVtkczXNwuaI91024020 = -187827668;    int nKaXwoPJHVtkczXNwuaI86953522 = -875119909;    int nKaXwoPJHVtkczXNwuaI72353384 = -249904516;    int nKaXwoPJHVtkczXNwuaI86965650 = -534477498;    int nKaXwoPJHVtkczXNwuaI35050786 = -560046279;    int nKaXwoPJHVtkczXNwuaI11144115 = 35976935;    int nKaXwoPJHVtkczXNwuaI84776405 = -591823878;    int nKaXwoPJHVtkczXNwuaI55724329 = -576445654;    int nKaXwoPJHVtkczXNwuaI97766581 = -998236478;    int nKaXwoPJHVtkczXNwuaI14997495 = -462582158;    int nKaXwoPJHVtkczXNwuaI69185228 = -623575890;    int nKaXwoPJHVtkczXNwuaI65687982 = -511031803;    int nKaXwoPJHVtkczXNwuaI55583984 = -126543680;    int nKaXwoPJHVtkczXNwuaI2693343 = -774330218;    int nKaXwoPJHVtkczXNwuaI6101849 = -794895103;    int nKaXwoPJHVtkczXNwuaI34677063 = -441437229;    int nKaXwoPJHVtkczXNwuaI98480211 = -459394293;    int nKaXwoPJHVtkczXNwuaI61765085 = -555208351;    int nKaXwoPJHVtkczXNwuaI15302613 = -67627400;    int nKaXwoPJHVtkczXNwuaI10102307 = -127155313;    int nKaXwoPJHVtkczXNwuaI75069921 = 24320928;    int nKaXwoPJHVtkczXNwuaI25080611 = -256349593;    int nKaXwoPJHVtkczXNwuaI24045120 = -641216686;    int nKaXwoPJHVtkczXNwuaI19572408 = 45457866;    int nKaXwoPJHVtkczXNwuaI8711705 = -457200722;    int nKaXwoPJHVtkczXNwuaI48308263 = -136416423;    int nKaXwoPJHVtkczXNwuaI47225979 = -759244424;    int nKaXwoPJHVtkczXNwuaI37110039 = -181459000;    int nKaXwoPJHVtkczXNwuaI67743180 = -772165719;    int nKaXwoPJHVtkczXNwuaI64120862 = 47982917;    int nKaXwoPJHVtkczXNwuaI23736294 = -822189684;    int nKaXwoPJHVtkczXNwuaI25309214 = -207877040;    int nKaXwoPJHVtkczXNwuaI88548638 = -954574647;    int nKaXwoPJHVtkczXNwuaI86725080 = -218391484;    int nKaXwoPJHVtkczXNwuaI36797564 = -77048702;    int nKaXwoPJHVtkczXNwuaI9832432 = -375817237;    int nKaXwoPJHVtkczXNwuaI54475220 = -505558769;    int nKaXwoPJHVtkczXNwuaI77423644 = -381798175;    int nKaXwoPJHVtkczXNwuaI87047395 = -916952729;    int nKaXwoPJHVtkczXNwuaI79254270 = -398246630;    int nKaXwoPJHVtkczXNwuaI31248750 = -177147605;    int nKaXwoPJHVtkczXNwuaI73443248 = -865239112;    int nKaXwoPJHVtkczXNwuaI93357575 = -896881147;    int nKaXwoPJHVtkczXNwuaI5588372 = -38753212;    int nKaXwoPJHVtkczXNwuaI46196552 = -597028407;    int nKaXwoPJHVtkczXNwuaI50973192 = -972803884;    int nKaXwoPJHVtkczXNwuaI80211639 = -754819635;    int nKaXwoPJHVtkczXNwuaI34086278 = -134803114;    int nKaXwoPJHVtkczXNwuaI38589645 = -101751305;    int nKaXwoPJHVtkczXNwuaI20197635 = -289865288;    int nKaXwoPJHVtkczXNwuaI15236290 = -621199501;    int nKaXwoPJHVtkczXNwuaI11824611 = -504708561;    int nKaXwoPJHVtkczXNwuaI74553283 = -175542173;    int nKaXwoPJHVtkczXNwuaI79013877 = -615245810;    int nKaXwoPJHVtkczXNwuaI50482841 = -898795026;    int nKaXwoPJHVtkczXNwuaI65943409 = -308007572;     nKaXwoPJHVtkczXNwuaI43352804 = nKaXwoPJHVtkczXNwuaI67861826;     nKaXwoPJHVtkczXNwuaI67861826 = nKaXwoPJHVtkczXNwuaI30514927;     nKaXwoPJHVtkczXNwuaI30514927 = nKaXwoPJHVtkczXNwuaI2887803;     nKaXwoPJHVtkczXNwuaI2887803 = nKaXwoPJHVtkczXNwuaI61505667;     nKaXwoPJHVtkczXNwuaI61505667 = nKaXwoPJHVtkczXNwuaI62869506;     nKaXwoPJHVtkczXNwuaI62869506 = nKaXwoPJHVtkczXNwuaI83863266;     nKaXwoPJHVtkczXNwuaI83863266 = nKaXwoPJHVtkczXNwuaI5700997;     nKaXwoPJHVtkczXNwuaI5700997 = nKaXwoPJHVtkczXNwuaI26127461;     nKaXwoPJHVtkczXNwuaI26127461 = nKaXwoPJHVtkczXNwuaI4237500;     nKaXwoPJHVtkczXNwuaI4237500 = nKaXwoPJHVtkczXNwuaI16204768;     nKaXwoPJHVtkczXNwuaI16204768 = nKaXwoPJHVtkczXNwuaI44630985;     nKaXwoPJHVtkczXNwuaI44630985 = nKaXwoPJHVtkczXNwuaI34954188;     nKaXwoPJHVtkczXNwuaI34954188 = nKaXwoPJHVtkczXNwuaI25372879;     nKaXwoPJHVtkczXNwuaI25372879 = nKaXwoPJHVtkczXNwuaI84103886;     nKaXwoPJHVtkczXNwuaI84103886 = nKaXwoPJHVtkczXNwuaI84005599;     nKaXwoPJHVtkczXNwuaI84005599 = nKaXwoPJHVtkczXNwuaI81801532;     nKaXwoPJHVtkczXNwuaI81801532 = nKaXwoPJHVtkczXNwuaI22617877;     nKaXwoPJHVtkczXNwuaI22617877 = nKaXwoPJHVtkczXNwuaI77837363;     nKaXwoPJHVtkczXNwuaI77837363 = nKaXwoPJHVtkczXNwuaI97325989;     nKaXwoPJHVtkczXNwuaI97325989 = nKaXwoPJHVtkczXNwuaI15165109;     nKaXwoPJHVtkczXNwuaI15165109 = nKaXwoPJHVtkczXNwuaI77664821;     nKaXwoPJHVtkczXNwuaI77664821 = nKaXwoPJHVtkczXNwuaI12277843;     nKaXwoPJHVtkczXNwuaI12277843 = nKaXwoPJHVtkczXNwuaI27821584;     nKaXwoPJHVtkczXNwuaI27821584 = nKaXwoPJHVtkczXNwuaI96785953;     nKaXwoPJHVtkczXNwuaI96785953 = nKaXwoPJHVtkczXNwuaI26828605;     nKaXwoPJHVtkczXNwuaI26828605 = nKaXwoPJHVtkczXNwuaI64389295;     nKaXwoPJHVtkczXNwuaI64389295 = nKaXwoPJHVtkczXNwuaI22098181;     nKaXwoPJHVtkczXNwuaI22098181 = nKaXwoPJHVtkczXNwuaI90398384;     nKaXwoPJHVtkczXNwuaI90398384 = nKaXwoPJHVtkczXNwuaI16025155;     nKaXwoPJHVtkczXNwuaI16025155 = nKaXwoPJHVtkczXNwuaI29167578;     nKaXwoPJHVtkczXNwuaI29167578 = nKaXwoPJHVtkczXNwuaI91124157;     nKaXwoPJHVtkczXNwuaI91124157 = nKaXwoPJHVtkczXNwuaI20585866;     nKaXwoPJHVtkczXNwuaI20585866 = nKaXwoPJHVtkczXNwuaI15381780;     nKaXwoPJHVtkczXNwuaI15381780 = nKaXwoPJHVtkczXNwuaI16661175;     nKaXwoPJHVtkczXNwuaI16661175 = nKaXwoPJHVtkczXNwuaI35795624;     nKaXwoPJHVtkczXNwuaI35795624 = nKaXwoPJHVtkczXNwuaI36779621;     nKaXwoPJHVtkczXNwuaI36779621 = nKaXwoPJHVtkczXNwuaI44691494;     nKaXwoPJHVtkczXNwuaI44691494 = nKaXwoPJHVtkczXNwuaI54874697;     nKaXwoPJHVtkczXNwuaI54874697 = nKaXwoPJHVtkczXNwuaI13716501;     nKaXwoPJHVtkczXNwuaI13716501 = nKaXwoPJHVtkczXNwuaI73589696;     nKaXwoPJHVtkczXNwuaI73589696 = nKaXwoPJHVtkczXNwuaI89855895;     nKaXwoPJHVtkczXNwuaI89855895 = nKaXwoPJHVtkczXNwuaI89116183;     nKaXwoPJHVtkczXNwuaI89116183 = nKaXwoPJHVtkczXNwuaI25552763;     nKaXwoPJHVtkczXNwuaI25552763 = nKaXwoPJHVtkczXNwuaI91024020;     nKaXwoPJHVtkczXNwuaI91024020 = nKaXwoPJHVtkczXNwuaI86953522;     nKaXwoPJHVtkczXNwuaI86953522 = nKaXwoPJHVtkczXNwuaI72353384;     nKaXwoPJHVtkczXNwuaI72353384 = nKaXwoPJHVtkczXNwuaI86965650;     nKaXwoPJHVtkczXNwuaI86965650 = nKaXwoPJHVtkczXNwuaI35050786;     nKaXwoPJHVtkczXNwuaI35050786 = nKaXwoPJHVtkczXNwuaI11144115;     nKaXwoPJHVtkczXNwuaI11144115 = nKaXwoPJHVtkczXNwuaI84776405;     nKaXwoPJHVtkczXNwuaI84776405 = nKaXwoPJHVtkczXNwuaI55724329;     nKaXwoPJHVtkczXNwuaI55724329 = nKaXwoPJHVtkczXNwuaI97766581;     nKaXwoPJHVtkczXNwuaI97766581 = nKaXwoPJHVtkczXNwuaI14997495;     nKaXwoPJHVtkczXNwuaI14997495 = nKaXwoPJHVtkczXNwuaI69185228;     nKaXwoPJHVtkczXNwuaI69185228 = nKaXwoPJHVtkczXNwuaI65687982;     nKaXwoPJHVtkczXNwuaI65687982 = nKaXwoPJHVtkczXNwuaI55583984;     nKaXwoPJHVtkczXNwuaI55583984 = nKaXwoPJHVtkczXNwuaI2693343;     nKaXwoPJHVtkczXNwuaI2693343 = nKaXwoPJHVtkczXNwuaI6101849;     nKaXwoPJHVtkczXNwuaI6101849 = nKaXwoPJHVtkczXNwuaI34677063;     nKaXwoPJHVtkczXNwuaI34677063 = nKaXwoPJHVtkczXNwuaI98480211;     nKaXwoPJHVtkczXNwuaI98480211 = nKaXwoPJHVtkczXNwuaI61765085;     nKaXwoPJHVtkczXNwuaI61765085 = nKaXwoPJHVtkczXNwuaI15302613;     nKaXwoPJHVtkczXNwuaI15302613 = nKaXwoPJHVtkczXNwuaI10102307;     nKaXwoPJHVtkczXNwuaI10102307 = nKaXwoPJHVtkczXNwuaI75069921;     nKaXwoPJHVtkczXNwuaI75069921 = nKaXwoPJHVtkczXNwuaI25080611;     nKaXwoPJHVtkczXNwuaI25080611 = nKaXwoPJHVtkczXNwuaI24045120;     nKaXwoPJHVtkczXNwuaI24045120 = nKaXwoPJHVtkczXNwuaI19572408;     nKaXwoPJHVtkczXNwuaI19572408 = nKaXwoPJHVtkczXNwuaI8711705;     nKaXwoPJHVtkczXNwuaI8711705 = nKaXwoPJHVtkczXNwuaI48308263;     nKaXwoPJHVtkczXNwuaI48308263 = nKaXwoPJHVtkczXNwuaI47225979;     nKaXwoPJHVtkczXNwuaI47225979 = nKaXwoPJHVtkczXNwuaI37110039;     nKaXwoPJHVtkczXNwuaI37110039 = nKaXwoPJHVtkczXNwuaI67743180;     nKaXwoPJHVtkczXNwuaI67743180 = nKaXwoPJHVtkczXNwuaI64120862;     nKaXwoPJHVtkczXNwuaI64120862 = nKaXwoPJHVtkczXNwuaI23736294;     nKaXwoPJHVtkczXNwuaI23736294 = nKaXwoPJHVtkczXNwuaI25309214;     nKaXwoPJHVtkczXNwuaI25309214 = nKaXwoPJHVtkczXNwuaI88548638;     nKaXwoPJHVtkczXNwuaI88548638 = nKaXwoPJHVtkczXNwuaI86725080;     nKaXwoPJHVtkczXNwuaI86725080 = nKaXwoPJHVtkczXNwuaI36797564;     nKaXwoPJHVtkczXNwuaI36797564 = nKaXwoPJHVtkczXNwuaI9832432;     nKaXwoPJHVtkczXNwuaI9832432 = nKaXwoPJHVtkczXNwuaI54475220;     nKaXwoPJHVtkczXNwuaI54475220 = nKaXwoPJHVtkczXNwuaI77423644;     nKaXwoPJHVtkczXNwuaI77423644 = nKaXwoPJHVtkczXNwuaI87047395;     nKaXwoPJHVtkczXNwuaI87047395 = nKaXwoPJHVtkczXNwuaI79254270;     nKaXwoPJHVtkczXNwuaI79254270 = nKaXwoPJHVtkczXNwuaI31248750;     nKaXwoPJHVtkczXNwuaI31248750 = nKaXwoPJHVtkczXNwuaI73443248;     nKaXwoPJHVtkczXNwuaI73443248 = nKaXwoPJHVtkczXNwuaI93357575;     nKaXwoPJHVtkczXNwuaI93357575 = nKaXwoPJHVtkczXNwuaI5588372;     nKaXwoPJHVtkczXNwuaI5588372 = nKaXwoPJHVtkczXNwuaI46196552;     nKaXwoPJHVtkczXNwuaI46196552 = nKaXwoPJHVtkczXNwuaI50973192;     nKaXwoPJHVtkczXNwuaI50973192 = nKaXwoPJHVtkczXNwuaI80211639;     nKaXwoPJHVtkczXNwuaI80211639 = nKaXwoPJHVtkczXNwuaI34086278;     nKaXwoPJHVtkczXNwuaI34086278 = nKaXwoPJHVtkczXNwuaI38589645;     nKaXwoPJHVtkczXNwuaI38589645 = nKaXwoPJHVtkczXNwuaI20197635;     nKaXwoPJHVtkczXNwuaI20197635 = nKaXwoPJHVtkczXNwuaI15236290;     nKaXwoPJHVtkczXNwuaI15236290 = nKaXwoPJHVtkczXNwuaI11824611;     nKaXwoPJHVtkczXNwuaI11824611 = nKaXwoPJHVtkczXNwuaI74553283;     nKaXwoPJHVtkczXNwuaI74553283 = nKaXwoPJHVtkczXNwuaI79013877;     nKaXwoPJHVtkczXNwuaI79013877 = nKaXwoPJHVtkczXNwuaI50482841;     nKaXwoPJHVtkczXNwuaI50482841 = nKaXwoPJHVtkczXNwuaI65943409;     nKaXwoPJHVtkczXNwuaI65943409 = nKaXwoPJHVtkczXNwuaI43352804;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void eOsiSceFRdWqgyquVYog90759476() {     int imYpvtnbjFHCOVfiQphB77441595 = -637405974;    int imYpvtnbjFHCOVfiQphB23214607 = -8232049;    int imYpvtnbjFHCOVfiQphB1760504 = -702446894;    int imYpvtnbjFHCOVfiQphB63013206 = -429409534;    int imYpvtnbjFHCOVfiQphB15230465 = 26325430;    int imYpvtnbjFHCOVfiQphB95695064 = -319041534;    int imYpvtnbjFHCOVfiQphB91125103 = -750773101;    int imYpvtnbjFHCOVfiQphB46013155 = -490939183;    int imYpvtnbjFHCOVfiQphB38235271 = -949757194;    int imYpvtnbjFHCOVfiQphB64782475 = -661498834;    int imYpvtnbjFHCOVfiQphB5738591 = -764739733;    int imYpvtnbjFHCOVfiQphB53000568 = -357154252;    int imYpvtnbjFHCOVfiQphB81096797 = -484670025;    int imYpvtnbjFHCOVfiQphB89405598 = -129825083;    int imYpvtnbjFHCOVfiQphB44985991 = -511981730;    int imYpvtnbjFHCOVfiQphB42164377 = -592765129;    int imYpvtnbjFHCOVfiQphB83658800 = -498947501;    int imYpvtnbjFHCOVfiQphB64246821 = -70876330;    int imYpvtnbjFHCOVfiQphB3992922 = -127373393;    int imYpvtnbjFHCOVfiQphB79167146 = -227035233;    int imYpvtnbjFHCOVfiQphB51078659 = -441768358;    int imYpvtnbjFHCOVfiQphB23627518 = -333626950;    int imYpvtnbjFHCOVfiQphB8538848 = -247138699;    int imYpvtnbjFHCOVfiQphB14833412 = -714982599;    int imYpvtnbjFHCOVfiQphB15902473 = -974756919;    int imYpvtnbjFHCOVfiQphB82977424 = -298929817;    int imYpvtnbjFHCOVfiQphB83237546 = 59594441;    int imYpvtnbjFHCOVfiQphB6874633 = -728470206;    int imYpvtnbjFHCOVfiQphB7167574 = -643629447;    int imYpvtnbjFHCOVfiQphB18624156 = -302970543;    int imYpvtnbjFHCOVfiQphB97754548 = -938588217;    int imYpvtnbjFHCOVfiQphB81617439 = -330791279;    int imYpvtnbjFHCOVfiQphB38065112 = -219444914;    int imYpvtnbjFHCOVfiQphB76167839 = -16098792;    int imYpvtnbjFHCOVfiQphB8654502 = -614550876;    int imYpvtnbjFHCOVfiQphB28683462 = 74997220;    int imYpvtnbjFHCOVfiQphB82678908 = -467755276;    int imYpvtnbjFHCOVfiQphB67221383 = -44887788;    int imYpvtnbjFHCOVfiQphB85907667 = -328147711;    int imYpvtnbjFHCOVfiQphB42026558 = -137862809;    int imYpvtnbjFHCOVfiQphB25652 = -184884465;    int imYpvtnbjFHCOVfiQphB42170675 = -701959796;    int imYpvtnbjFHCOVfiQphB91200738 = -452325834;    int imYpvtnbjFHCOVfiQphB52303611 = -937143925;    int imYpvtnbjFHCOVfiQphB74754499 = -632015598;    int imYpvtnbjFHCOVfiQphB47898550 = -388761018;    int imYpvtnbjFHCOVfiQphB95089681 = -462060000;    int imYpvtnbjFHCOVfiQphB7801380 = -548957771;    int imYpvtnbjFHCOVfiQphB68174389 = -897817371;    int imYpvtnbjFHCOVfiQphB52787781 = 18024566;    int imYpvtnbjFHCOVfiQphB78342732 = -227525510;    int imYpvtnbjFHCOVfiQphB996145 = -792547634;    int imYpvtnbjFHCOVfiQphB8244619 = 14859248;    int imYpvtnbjFHCOVfiQphB83709023 = -834932252;    int imYpvtnbjFHCOVfiQphB39680817 = 98735185;    int imYpvtnbjFHCOVfiQphB53814077 = -203779024;    int imYpvtnbjFHCOVfiQphB14675760 = -761093350;    int imYpvtnbjFHCOVfiQphB86927092 = -987464295;    int imYpvtnbjFHCOVfiQphB47110734 = -454652616;    int imYpvtnbjFHCOVfiQphB32253040 = -674744753;    int imYpvtnbjFHCOVfiQphB12457519 = -278635976;    int imYpvtnbjFHCOVfiQphB84250470 = 77697105;    int imYpvtnbjFHCOVfiQphB38845581 = -847309737;    int imYpvtnbjFHCOVfiQphB19611115 = -546786652;    int imYpvtnbjFHCOVfiQphB67027927 = -722910618;    int imYpvtnbjFHCOVfiQphB24121152 = -333948454;    int imYpvtnbjFHCOVfiQphB14935457 = -37709339;    int imYpvtnbjFHCOVfiQphB4928959 = -368571233;    int imYpvtnbjFHCOVfiQphB80751097 = -515274208;    int imYpvtnbjFHCOVfiQphB16302529 = -486978951;    int imYpvtnbjFHCOVfiQphB59485469 = -25009853;    int imYpvtnbjFHCOVfiQphB16437418 = -354059713;    int imYpvtnbjFHCOVfiQphB78339154 = -742728619;    int imYpvtnbjFHCOVfiQphB61966363 = -989510585;    int imYpvtnbjFHCOVfiQphB79141495 = 57849232;    int imYpvtnbjFHCOVfiQphB8907984 = -739808563;    int imYpvtnbjFHCOVfiQphB32426780 = -881301117;    int imYpvtnbjFHCOVfiQphB56235237 = -309994774;    int imYpvtnbjFHCOVfiQphB40078913 = 17032999;    int imYpvtnbjFHCOVfiQphB68003923 = -485995901;    int imYpvtnbjFHCOVfiQphB87887743 = -836869818;    int imYpvtnbjFHCOVfiQphB75436166 = -391447789;    int imYpvtnbjFHCOVfiQphB38700244 = -830652835;    int imYpvtnbjFHCOVfiQphB54379793 = -561654013;    int imYpvtnbjFHCOVfiQphB40281424 = 24554967;    int imYpvtnbjFHCOVfiQphB96758403 = -46040583;    int imYpvtnbjFHCOVfiQphB73372820 = -245650528;    int imYpvtnbjFHCOVfiQphB54356088 = -384512662;    int imYpvtnbjFHCOVfiQphB36487022 = -14833977;    int imYpvtnbjFHCOVfiQphB54840424 = -310771852;    int imYpvtnbjFHCOVfiQphB14007703 = -163909430;    int imYpvtnbjFHCOVfiQphB95751816 = -480290981;    int imYpvtnbjFHCOVfiQphB20110650 = -590235173;    int imYpvtnbjFHCOVfiQphB53654627 = -653402958;    int imYpvtnbjFHCOVfiQphB29569040 = -859226833;    int imYpvtnbjFHCOVfiQphB15775181 = -162581570;    int imYpvtnbjFHCOVfiQphB3325095 = -854650059;    int imYpvtnbjFHCOVfiQphB71589623 = -905539182;    int imYpvtnbjFHCOVfiQphB85275683 = -114233308;    int imYpvtnbjFHCOVfiQphB50633348 = -637405974;     imYpvtnbjFHCOVfiQphB77441595 = imYpvtnbjFHCOVfiQphB23214607;     imYpvtnbjFHCOVfiQphB23214607 = imYpvtnbjFHCOVfiQphB1760504;     imYpvtnbjFHCOVfiQphB1760504 = imYpvtnbjFHCOVfiQphB63013206;     imYpvtnbjFHCOVfiQphB63013206 = imYpvtnbjFHCOVfiQphB15230465;     imYpvtnbjFHCOVfiQphB15230465 = imYpvtnbjFHCOVfiQphB95695064;     imYpvtnbjFHCOVfiQphB95695064 = imYpvtnbjFHCOVfiQphB91125103;     imYpvtnbjFHCOVfiQphB91125103 = imYpvtnbjFHCOVfiQphB46013155;     imYpvtnbjFHCOVfiQphB46013155 = imYpvtnbjFHCOVfiQphB38235271;     imYpvtnbjFHCOVfiQphB38235271 = imYpvtnbjFHCOVfiQphB64782475;     imYpvtnbjFHCOVfiQphB64782475 = imYpvtnbjFHCOVfiQphB5738591;     imYpvtnbjFHCOVfiQphB5738591 = imYpvtnbjFHCOVfiQphB53000568;     imYpvtnbjFHCOVfiQphB53000568 = imYpvtnbjFHCOVfiQphB81096797;     imYpvtnbjFHCOVfiQphB81096797 = imYpvtnbjFHCOVfiQphB89405598;     imYpvtnbjFHCOVfiQphB89405598 = imYpvtnbjFHCOVfiQphB44985991;     imYpvtnbjFHCOVfiQphB44985991 = imYpvtnbjFHCOVfiQphB42164377;     imYpvtnbjFHCOVfiQphB42164377 = imYpvtnbjFHCOVfiQphB83658800;     imYpvtnbjFHCOVfiQphB83658800 = imYpvtnbjFHCOVfiQphB64246821;     imYpvtnbjFHCOVfiQphB64246821 = imYpvtnbjFHCOVfiQphB3992922;     imYpvtnbjFHCOVfiQphB3992922 = imYpvtnbjFHCOVfiQphB79167146;     imYpvtnbjFHCOVfiQphB79167146 = imYpvtnbjFHCOVfiQphB51078659;     imYpvtnbjFHCOVfiQphB51078659 = imYpvtnbjFHCOVfiQphB23627518;     imYpvtnbjFHCOVfiQphB23627518 = imYpvtnbjFHCOVfiQphB8538848;     imYpvtnbjFHCOVfiQphB8538848 = imYpvtnbjFHCOVfiQphB14833412;     imYpvtnbjFHCOVfiQphB14833412 = imYpvtnbjFHCOVfiQphB15902473;     imYpvtnbjFHCOVfiQphB15902473 = imYpvtnbjFHCOVfiQphB82977424;     imYpvtnbjFHCOVfiQphB82977424 = imYpvtnbjFHCOVfiQphB83237546;     imYpvtnbjFHCOVfiQphB83237546 = imYpvtnbjFHCOVfiQphB6874633;     imYpvtnbjFHCOVfiQphB6874633 = imYpvtnbjFHCOVfiQphB7167574;     imYpvtnbjFHCOVfiQphB7167574 = imYpvtnbjFHCOVfiQphB18624156;     imYpvtnbjFHCOVfiQphB18624156 = imYpvtnbjFHCOVfiQphB97754548;     imYpvtnbjFHCOVfiQphB97754548 = imYpvtnbjFHCOVfiQphB81617439;     imYpvtnbjFHCOVfiQphB81617439 = imYpvtnbjFHCOVfiQphB38065112;     imYpvtnbjFHCOVfiQphB38065112 = imYpvtnbjFHCOVfiQphB76167839;     imYpvtnbjFHCOVfiQphB76167839 = imYpvtnbjFHCOVfiQphB8654502;     imYpvtnbjFHCOVfiQphB8654502 = imYpvtnbjFHCOVfiQphB28683462;     imYpvtnbjFHCOVfiQphB28683462 = imYpvtnbjFHCOVfiQphB82678908;     imYpvtnbjFHCOVfiQphB82678908 = imYpvtnbjFHCOVfiQphB67221383;     imYpvtnbjFHCOVfiQphB67221383 = imYpvtnbjFHCOVfiQphB85907667;     imYpvtnbjFHCOVfiQphB85907667 = imYpvtnbjFHCOVfiQphB42026558;     imYpvtnbjFHCOVfiQphB42026558 = imYpvtnbjFHCOVfiQphB25652;     imYpvtnbjFHCOVfiQphB25652 = imYpvtnbjFHCOVfiQphB42170675;     imYpvtnbjFHCOVfiQphB42170675 = imYpvtnbjFHCOVfiQphB91200738;     imYpvtnbjFHCOVfiQphB91200738 = imYpvtnbjFHCOVfiQphB52303611;     imYpvtnbjFHCOVfiQphB52303611 = imYpvtnbjFHCOVfiQphB74754499;     imYpvtnbjFHCOVfiQphB74754499 = imYpvtnbjFHCOVfiQphB47898550;     imYpvtnbjFHCOVfiQphB47898550 = imYpvtnbjFHCOVfiQphB95089681;     imYpvtnbjFHCOVfiQphB95089681 = imYpvtnbjFHCOVfiQphB7801380;     imYpvtnbjFHCOVfiQphB7801380 = imYpvtnbjFHCOVfiQphB68174389;     imYpvtnbjFHCOVfiQphB68174389 = imYpvtnbjFHCOVfiQphB52787781;     imYpvtnbjFHCOVfiQphB52787781 = imYpvtnbjFHCOVfiQphB78342732;     imYpvtnbjFHCOVfiQphB78342732 = imYpvtnbjFHCOVfiQphB996145;     imYpvtnbjFHCOVfiQphB996145 = imYpvtnbjFHCOVfiQphB8244619;     imYpvtnbjFHCOVfiQphB8244619 = imYpvtnbjFHCOVfiQphB83709023;     imYpvtnbjFHCOVfiQphB83709023 = imYpvtnbjFHCOVfiQphB39680817;     imYpvtnbjFHCOVfiQphB39680817 = imYpvtnbjFHCOVfiQphB53814077;     imYpvtnbjFHCOVfiQphB53814077 = imYpvtnbjFHCOVfiQphB14675760;     imYpvtnbjFHCOVfiQphB14675760 = imYpvtnbjFHCOVfiQphB86927092;     imYpvtnbjFHCOVfiQphB86927092 = imYpvtnbjFHCOVfiQphB47110734;     imYpvtnbjFHCOVfiQphB47110734 = imYpvtnbjFHCOVfiQphB32253040;     imYpvtnbjFHCOVfiQphB32253040 = imYpvtnbjFHCOVfiQphB12457519;     imYpvtnbjFHCOVfiQphB12457519 = imYpvtnbjFHCOVfiQphB84250470;     imYpvtnbjFHCOVfiQphB84250470 = imYpvtnbjFHCOVfiQphB38845581;     imYpvtnbjFHCOVfiQphB38845581 = imYpvtnbjFHCOVfiQphB19611115;     imYpvtnbjFHCOVfiQphB19611115 = imYpvtnbjFHCOVfiQphB67027927;     imYpvtnbjFHCOVfiQphB67027927 = imYpvtnbjFHCOVfiQphB24121152;     imYpvtnbjFHCOVfiQphB24121152 = imYpvtnbjFHCOVfiQphB14935457;     imYpvtnbjFHCOVfiQphB14935457 = imYpvtnbjFHCOVfiQphB4928959;     imYpvtnbjFHCOVfiQphB4928959 = imYpvtnbjFHCOVfiQphB80751097;     imYpvtnbjFHCOVfiQphB80751097 = imYpvtnbjFHCOVfiQphB16302529;     imYpvtnbjFHCOVfiQphB16302529 = imYpvtnbjFHCOVfiQphB59485469;     imYpvtnbjFHCOVfiQphB59485469 = imYpvtnbjFHCOVfiQphB16437418;     imYpvtnbjFHCOVfiQphB16437418 = imYpvtnbjFHCOVfiQphB78339154;     imYpvtnbjFHCOVfiQphB78339154 = imYpvtnbjFHCOVfiQphB61966363;     imYpvtnbjFHCOVfiQphB61966363 = imYpvtnbjFHCOVfiQphB79141495;     imYpvtnbjFHCOVfiQphB79141495 = imYpvtnbjFHCOVfiQphB8907984;     imYpvtnbjFHCOVfiQphB8907984 = imYpvtnbjFHCOVfiQphB32426780;     imYpvtnbjFHCOVfiQphB32426780 = imYpvtnbjFHCOVfiQphB56235237;     imYpvtnbjFHCOVfiQphB56235237 = imYpvtnbjFHCOVfiQphB40078913;     imYpvtnbjFHCOVfiQphB40078913 = imYpvtnbjFHCOVfiQphB68003923;     imYpvtnbjFHCOVfiQphB68003923 = imYpvtnbjFHCOVfiQphB87887743;     imYpvtnbjFHCOVfiQphB87887743 = imYpvtnbjFHCOVfiQphB75436166;     imYpvtnbjFHCOVfiQphB75436166 = imYpvtnbjFHCOVfiQphB38700244;     imYpvtnbjFHCOVfiQphB38700244 = imYpvtnbjFHCOVfiQphB54379793;     imYpvtnbjFHCOVfiQphB54379793 = imYpvtnbjFHCOVfiQphB40281424;     imYpvtnbjFHCOVfiQphB40281424 = imYpvtnbjFHCOVfiQphB96758403;     imYpvtnbjFHCOVfiQphB96758403 = imYpvtnbjFHCOVfiQphB73372820;     imYpvtnbjFHCOVfiQphB73372820 = imYpvtnbjFHCOVfiQphB54356088;     imYpvtnbjFHCOVfiQphB54356088 = imYpvtnbjFHCOVfiQphB36487022;     imYpvtnbjFHCOVfiQphB36487022 = imYpvtnbjFHCOVfiQphB54840424;     imYpvtnbjFHCOVfiQphB54840424 = imYpvtnbjFHCOVfiQphB14007703;     imYpvtnbjFHCOVfiQphB14007703 = imYpvtnbjFHCOVfiQphB95751816;     imYpvtnbjFHCOVfiQphB95751816 = imYpvtnbjFHCOVfiQphB20110650;     imYpvtnbjFHCOVfiQphB20110650 = imYpvtnbjFHCOVfiQphB53654627;     imYpvtnbjFHCOVfiQphB53654627 = imYpvtnbjFHCOVfiQphB29569040;     imYpvtnbjFHCOVfiQphB29569040 = imYpvtnbjFHCOVfiQphB15775181;     imYpvtnbjFHCOVfiQphB15775181 = imYpvtnbjFHCOVfiQphB3325095;     imYpvtnbjFHCOVfiQphB3325095 = imYpvtnbjFHCOVfiQphB71589623;     imYpvtnbjFHCOVfiQphB71589623 = imYpvtnbjFHCOVfiQphB85275683;     imYpvtnbjFHCOVfiQphB85275683 = imYpvtnbjFHCOVfiQphB50633348;     imYpvtnbjFHCOVfiQphB50633348 = imYpvtnbjFHCOVfiQphB77441595;}
// Junk Finished

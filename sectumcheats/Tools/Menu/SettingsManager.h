#pragma once
#include "../../stdafx.h"


struct LegitbotSetting
{
	bool EnableAim = false;
	float SmoothX, SmoothY;
	bool Nearest = false;;
	int Hitbox = 0;
	float FOV = 0;
	float Delay = 0;
	float rcs_x = 0;
	float rcs_y = 0;
};
class CCacheAngle {
public:
	//Menu
	bool bCustomSetts = false;
	bool bCustomSettsLegit = false;
	bool lbyfix;
	bool bCorrect = true;
	int iResolverType = 0;
	int iResolverMode = 0;
	int ResolverModeMode = 0;
	float ClockTime = 1.f;

	bool bPrioritize = false;
	int iHitbox = 1;
	bool bAutoBaim = false;
	int iAutoBaimAferShot = 2;
	int iHitboxAutoBaim = 4;
	bool bPrioritizeVis = true;
	float flMinDmg = 15.f;
	float flMinHit = 30.f;
	bool bInterpLagComp = false;
	int Ticks = 5;

	bool LegitNearest = true;
	bool Ignore = false;
	bool pSilent = false;
	float pSilentFov = 1.4f;
	//GlobalPlayer
	bool bFlip = false;



	int List = 0;
};
struct BindButton
{
	int KeyIndex;
	bool Key[256];
};
struct CSettings
{
	
	float g_fMColor[4] = { 0.21f, 0.21f, 0.21f, 1.0f }; //RGBA color
	float g_fBColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	float g_fTColor[4] = { 1.f, 1.f, 1.f, 1.0f };

	LegitbotSetting Legitbot[63];
	CCacheAngle aCacheAngle[64];

	class Weapon_t
	{
	public:
		float Fov = 2;
		int Bone = 1;
		bool Nearest = true;
		float Smooth = 22.5f;
		int RcsX = 100;
		int RcsY = 100;
		bool pSilent = true;
		float pSilentFov = 100.f;
		float pSilentFovNormal = 1.4f;
	}Weapons[520];


	struct
	{
		bool 	Enabled = false;
		bool 	OnFire = false;
		int 	Key = false;
		bool	OnKey = false;
		bool	FriendlyFire = false;
		bool	SmokeCheck = true;
		bool	FlashCheck = true;
		bool	JumpCheck = true;
		float FlashAlpha = 70;
		int		KillDelay = 0;
	} Aimbot;

	ImFont* font;

	struct
	{
		bool EnableAim;

		int TargetDetection;
		float PointScale;

		struct
		{
			int PriorityHitbox;
			bool Head;

			bool Neck;
			bool NeckFull;

			bool Body;
			bool BodyFull;

			bool Legs;
			bool LegsFull;

			bool Arms;
			bool ArmsFull;

		} Hitscan;
		bool AutoRevoler;
		int Resolver;
		bool ResolverDebug;
		bool SilentAim;
		float Headscale;
		float Bodyscale;
		bool BackTrackLBY;
		struct
		{
			float Hitchance;
			float Mindamage;
			bool AutoShoot;
			bool AutoScope;
			bool RemoveRecoil;
			bool Multipoint;
			bool bAimAtKeyButton;
			bool RemoveSpread;
			int OverrideKey;
			bool PositionAdjust;
			bool AutoZeus;
			bool Wallbang;
			float FOV;
		} Accuracy;

		struct
		{
			bool Enable;
			
			int Pitch;
			int ChokeFactor;
			int RealYaw;
			float RealYawAdd;
			int FakeYaw;
			float FakeYawAdd;
			bool AtTarget;
			bool FakeWalk;
			bool AutoShuffle;
			int FakeWalkButton;
			int SuffleRight;
			int SuffleLeft;
			float SpinSpeed;
			float SpinSpeedfake;
			float JitterSpeed;
			float JitterRange;
			bool JitterSpeedDynamic;
			bool AAWithKnife;
			bool AAWithGrenades;
			bool Edge;
			float FakeWalkSpeed;
			int FakeLag;
			int FakeLagAmount;
			bool FakeLagInAirOnly;
			int ShuffleFlipKey;

			bool CrookedMode;
			bool ZeroAtPeek;

			float lby_delta;

			struct
			{
				int Pitch;
				int Yaw;
				int FakeYaw;
				float YawAdd;
				float FakeYawAdd;
			}Move;

		} AntiAim;
	} Ragebot;

	struct
	{
		bool Enable;
		bool Box;
		bool pBox;
		int BoxType;
		bool EspTeam;
		bool Health;
		bool Armor;
		bool Name;
		bool Weapon;
		bool Lines;
		bool Skeleton;
		bool BulletTracer;
		bool Scoped;
		bool Flashed;
		bool ResolverMode;
		bool Hitmarker;
		bool FakeChams;
		bool Nightmode;
		bool staticZoom;
		bool Crosshair;
		bool SpreadCrosshair;
		bool NoVisRecoil;
		bool NoFlash;
		bool NoSmoke;
		bool PostProcessing;
		int Thirdperson;
		bool EnableThirdPerson;
		int Sound;
		bool RemoveScope;
		bool AntiAimLines;
		bool Statusbar;
		bool ResolverInfo;
		struct
		{
			bool Enable;
			bool ThrowWalls;
			bool Team;
			bool Guns;
			bool FakeAngle;
		} Chams;

		struct
		{
			bool Enable;
			bool VisibleOnly;
			bool Team;
			int Type;

			float ChamsVisible[3] = { 1.0f, 1.0f, 1.0f };
			float ChamsInvisible[3] = { 1.0f, 1.0f, 1.0f };
		}ChamsNew;

		struct
		{
			//Chams
			float ChamsVisible[3] = { 1.0f, 1.0f, 1.0f };
			float ChamsInvisible[3] = { 1.0f, 1.0f, 1.0f };
			float ChamsGunsVisible[3] = { 1.0f, 1.0f, 1.0f };
			float ChamsGunsInvisible[3] = { 1.0f, 1.0f, 1.0f };
			//Esp
			float Skeleton[3] = { 1.0f, 1.0f, 1.0f };
			float Box[3] = { 1.0f, 1.0f, 1.0f };
			float BoxInvisible[3] = { 1.0f, 0.0f, 0.0f };
			float SnapLines[3] = { 1.0f, 1.0f, 1.0f };
			float SnapLinesTeam[3] = { 1.0f, 1.0f, 1.0f };
			float SpreadCrossheir[3] = { 1.0f, 1.0f, 1.0f };
			float ScopeCrosshair[3] = { 1.0f, 1.0f, 1.0f };
			float BulletTracer[3] = { 1.0f, 0.f, 0.8f };

			float EnemyGlow[4] = { 1.f, 1.f, 1.f, 1.f };
			bool EnemyGlowHB = false;

			float TeamGlow[4] = { 1.f, 1.f, 1.f, 1.f };
			float WeaponGlow[4] = { 1.f, 1.f, 1.f, 1.f };
			float BombGlow[4] = { 1.f, 1.f, 1.f, 1.f };
			float GrenadeGlow[4] = { 1.f, 1.f, 1.f, 1.f };
			
		} Colors;

		struct Glow
		{
			bool GlowEnemy;
			bool GlowTeam;

			bool WeaponGlow;

			bool BombGlow;

			bool GrenadeGlow;

			float Alpha;
		}Glow;
	} Visuals;

	struct
	{
		bool Knifebot;
		bool UT = true;
		bool streamhack;
		bool streamhack2;
		bool streamhack3;
		bool streamhack4;
		bool streamhack5;
		bool streamhack6;
		bool Bunnyhop;
		bool Autostrafe;
		bool Money;
		int Clantag;
		int Chatspam;
		bool Namespam;
		bool ClanChanger;
		bool AutoAccept;
		bool FovChanger;
		bool lbyindicator;
		float WorldFov = 90;
		float ViewmodelFov = 68;
		int Config;
		char configname[128] = "default";

		struct
		{
			int Key;
			float Amount;
		 }Lag;
	} Misc;

	struct
	{

	} Skins;

	struct
	{
		bool d3d_init;
		bool menu_open;
	} Global;
};

extern CSettings Settings;

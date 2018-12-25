#include "../../stdafx.h"
Createmove_Hacks CM_Hacks;
PaintTraverse_Hacks PT_Hacks;
Draw_Model_Exec_Hacks DMEHacks;

/*
NOTE:

TRY TO KEEP AMOUNT OF .CPP FILES TO A LOW AMOUND, OR IT TAKES A LONG TIME TO COMPILE

TO DO THIS, WE ARE KEEPING MOST FEATURES IN ONE .CPP SO WE DONT HAVE TO HAVE A .CPP
FOR EVERY FEATURE.

AIMBOTS HAVE THERE OWN CPP THOUGH.

*/

extern void dolegitshit();

#pragma region Create Move

bool bTeleportSwitch;
bool bTeleportZSwitch;
bool bTeleportInvertSwitch;
BYTE byteCorrupt[] = { 0xBE, 0x120 };
BYTE byteDeCorrupt[] = { 0xBE, 0x20 };
void Teleport()
{
	bool bTeleport = false;
	if (GetAsyncKeyState(Settings.GetSetting(Tab_Misc, Misc_Teliport)))
		bTeleport = true;

	if (bTeleport && !Settings.GetBoolSetting(Tab_Misc, Misc_Anti_Untrust))
	{
		BYTE* nulldata;
		static DWORD dwAddr = Utils.PatternSearch("client.dll", (PBYTE)"\xBE\x00\x00\x00\x00\x2B\xF1\x3B\xF3\x7D\x1F\x8B\x55\x0C\x8B\xC3", "x????xxxxxxxxxxx", NULL, NULL);
		auto bByte = bTeleportSwitch ? byteCorrupt : byteDeCorrupt;
		DWORD virtualProtect;
		VirtualProtect((LPVOID)(dwAddr), 4, PAGE_READWRITE, &virtualProtect);
		memcpy((LPVOID)(dwAddr), bByte, sizeof(bByte));
		VirtualProtect(reinterpret_cast<void*>(dwAddr), 4, virtualProtect, &virtualProtect);
		bTeleportSwitch = !bTeleportSwitch;
	}
}

void TeleportZ(Vector &angle)
{
	bool bTeleportZ = false;
	if (GetAsyncKeyState(Settings.GetSetting(Tab_Misc, Misc_Teliport)))
		bTeleportZ = true;

	if (bTeleportZ && !Settings.GetBoolSetting(Tab_Misc, Misc_Anti_Untrust))
	{
		angle.z = 99999999999999999999999.99999999999f;
		bTeleportZSwitch = !bTeleportZSwitch;
	}
}

void TeleportInvert(Vector &angle)
{
	Vector views = Hacks.CurrentCmd->viewangles;
	bool bTeleportInvert = false;
	static int invert = 9999999999999999999;
	if (GetAsyncKeyState(Settings.GetSetting(Tab_Misc, Misc_Teliport)))
		bTeleportInvert = true;

	if (bTeleportInvert && !Settings.GetBoolSetting(Tab_Misc, Misc_Anti_Untrust))
	{
		//angle.z = 99999999999999999999999.99999999999f;
		angle.y = -99999999999999999999999.99999999999f;
		angle.x = -99999999999999999999999.99999999999f;
		bTeleportInvertSwitch = !bTeleportInvertSwitch;
	}
}


void Createmove_Hacks::BunnyHop()
{
	if ((Hacks.CurrentCmd->buttons & IN_JUMP))
	{
		if (!(Hacks.LocalPlayer->GetFlags() & FL_ONGROUND))
			Hacks.CurrentCmd->buttons &= ~IN_JUMP;
		else
		{
			Hacks.CurrentCmd->buttons &= ~IN_DUCK;
			UnLagNextTick = true;
			Hacks.SendPacket = true;
		}
	}
}

void Createmove_Hacks::AutoStrafer()
{
	// basic silent autostrafer
	if (!(Hacks.LocalPlayer->GetFlags() & FL_ONGROUND) && (Hacks.CurrentCmd->buttons & IN_JUMP))
	{
		if (Hacks.LocalPlayer->GetVecVelocity().Length2D() == 0 && (Hacks.CurrentCmd->forwardmove == 0 && Hacks.CurrentCmd->sidemove == 0))
		{
			Hacks.CurrentCmd->forwardmove = 450.f;
		}
		else if (Hacks.CurrentCmd->forwardmove == 0 && Hacks.CurrentCmd->sidemove == 0)
		{
			if (Hacks.CurrentCmd->mousedx > 1 || Hacks.CurrentCmd->mousedx < -1) {
				Hacks.CurrentCmd->sidemove = Hacks.CurrentCmd->mousedx < 0.f ? -450.f : 450.f;
			}
			else
			{
				Hacks.CurrentCmd->forwardmove = 5850.f / Hacks.LocalPlayer->GetVecVelocity().Length2D();
				Hacks.CurrentCmd->sidemove = (Hacks.CurrentCmd->command_number % 2) == 0 ? -450.f : 450.f;
			}
		}
	}
}

bool Createmove_Hacks::CircleStrafer(Vector &angles)
{
	if (!GetAsyncKeyState((int)Settings.GetSetting(Tab_Misc, Misc_CircleStrafe_Key)))
		return false;

	Vector forward;
	g_Math.angleVectors(angles, forward);
	forward *= Hacks.LocalPlayer->GetVecVelocity().Length2D() * 2;
	/*
	Mathetmatics
	Fc = mV^2/r
	F=ma
	a = v^2/r

	Angular momentum
	w = v/r
	angular acceloration = dw/dt
	
	etc. Math it all out
	This is for a circle, however it should be triangles but its close enough
	r = (180 * sidemove)/change in viewangs*PI

	as we want max acceloration we want small r large v. Simple :) 
	Someone can do that maths and select the right side move etc. 
	*/
	CTraceWorldOnly filter;// Pretty sure this ray trace is useless but hey :)
	Ray_t ray;
	trace_t tr;
	ray.Init(Hacks.LocalPlayer->GetAbsOrigin(), forward);
	Interfaces.pTrace->TraceRay(ray, MASK_SHOT, (CTraceFilter*)&filter, &tr);
	static int add = 0;
	float value = 1000 / Hacks.LocalPlayer->GetVecVelocity().Length2D();
	float maxvalue = Settings.GetSetting(Tab_Misc, Misc_CircleStrafe_Max) / 2;
	float minvalue = Settings.GetSetting(Tab_Misc, Misc_CircleStrafe_Min) / 2;

	if (value > maxvalue)
		value = maxvalue;

	if (value < minvalue && minvalue != 0)
		value = minvalue;

	if (tr.DidHit())
		add += value * 2;
	else
		add += value;
	if (add > 180)
		add -= 360;
	angles.y += add;

	Hacks.CurrentCmd->sidemove = -(value * 132.35294)/2;//-(value * 132.35294);//-450;
	//Hacks.CurrentCmd->forwardmove = 450;
	return true;
}

void Airstuck()
{
	if (GetAsyncKeyState((int)Settings.GetSetting(Tab_Misc, Misc_Airstuck_Key)))
	{

		float ServerTime = Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;
		float NextPrimaryAttack = Hacks.LocalWeapon->NextPrimaryAttack();

		bool Shooting = false;
		if (NextPrimaryAttack > ServerTime)
			Shooting = true;


		if (!Shooting)
		{
			Hacks.CurrentCmd->tick_count = 99999999;
			//Hacks.CurrentCmd->tick_count = 0;

		}
	}
}
void Name_Stealer()
{
	if (Settings.GetBoolSetting(Tab_Misc, Misc_NameStealer))
	{
		stringstream NewName;
		int Index = Settings.GetSetting(Tab_Misc, Misc_NameStealer_Index);
		switch (Index)
		{
		case 0:
		case 1: NewName << "SOMEWARE"; break;
		case 2: NewName << "AIMJUNKIES.NET"; break;
		case 3: NewName << "VACWARE.NET"; break;
		case 4: NewName << "INTERWARE.CC"; break;
		case 5: NewName << "EZFRAGS.CO.UK"; break;
		case 6: NewName << " "; break;
		case 7: NewName << "Jewware"; break;
		}

		int ran = rand() % 3;
		for (int i = 0; i < ran; i++)
		{
			NewName << " ";
		}
		if (Settings.GetSetting(Tab_Misc, Misc_NameStealer_Invis)) {
			NewName << " \n";
		}
		ConVar* Name = Interfaces.g_ICVars->FindVar("name");
		if (Name)
		{
			*(int*)((DWORD)&Name->fnChangeCallback + 0xC) = 0;
			Name->SetValue(NewName.str().c_str());
		}
	}
}
void Createmove_Hacks::CorrectMovement(Vector vOldAngles, CInput::CUserCmd* pCmd, Vector Viewangs)
{
	Vector vMove(pCmd->forwardmove, pCmd->sidemove, pCmd->upmove);
	float flSpeed = sqrt(vMove.x * vMove.x + vMove.y * vMove.y), flYaw;
	Vector vMove2;
	g_Math.VectorAngles3D(vMove, vMove2);

	flYaw = DEG2RAD(Viewangs.y - vOldAngles.y + vMove2.y);
	pCmd->forwardmove = cos(flYaw) * flSpeed;
	pCmd->sidemove = sin(flYaw) * flSpeed;

	if (Viewangs.x < -90.f || Viewangs.x > 90.f)
		pCmd->forwardmove = -pCmd->forwardmove;
}
extern Vector GetHitboxPOS(CPlayer* Enemy, int HitboxID);

bool anyAlive()
{
	for (INT i = 0; i <= Interfaces.pEntList->GetHighestEntityIndex(); i++)
	{
		CBaseEntity* pEntity = (CBaseEntity*)Interfaces.pEntList->GetClientEntity(i);
		if (pEntity == NULL)
			continue;
		if (!pEntity->isAlive())
			continue;
		if (pEntity == Hacks.LocalPlayer)
			continue;
		if (pEntity->IsDormant())
			continue;
		if (pEntity->GetTeam() == Hacks.LocalPlayer->GetTeam())
			continue;
		player_info_t info;
		if (!(Interfaces.pEngine->GetPlayerInfo(pEntity->GetIndex(), &info)))
			continue;
		return true;
	}
	return false;
}

void Createmove_Hacks::MovePacket(CInput* thisptr, void* _EAX, int sequence_number, float input_sample_frametime, bool active)
{
	if (Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame())
	{
#pragma region Get User Cmd
		CInput::CUserCmd* cmdlist = *(CInput::CUserCmd**)((DWORD)Interfaces.pInput + 0xEC);
		CInput::CUserCmd* cmd = &cmdlist[sequence_number % 150];
		CInput::CVerifiedUserCmd* verifiedCmdList = *(CInput::CVerifiedUserCmd**)((DWORD)Interfaces.pInput + 0xF0);
		CInput::CVerifiedUserCmd* VerifiedCmd = &verifiedCmdList[sequence_number % 150];
#pragma endregion

#pragma region Sort Out Hack Manger
		Hacks.CurrentCmd = cmd;
		Vector oldAngles = Hacks.CurrentCmd->viewangles;
		if (!Hacks.LocalPlayer)return;
		Hacks.LocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();
		if (!Hacks.LocalWeapon) return;
#pragma endregion
		Vector OrigAng = Hacks.CurrentCmd->viewangles;
#pragma region Do Hacks
		if (Hacks.LocalPlayer)
		{
			if (Hacks.LocalPlayer->isAlive())
			{
				Name_Stealer();

				Settings.weaponconfigs();

				if (!CircleStrafer(OrigAng))
					if (Settings.GetBoolSetting(Tab_Misc, Misc_Auto_Strafer))
						AutoStrafer();

				if (Settings.GetBoolSetting(Tab_Misc, Misc_Bunny_Hop))
					BunnyHop();

				if (Settings.GetBoolSetting(Tab_Misc, Misc_Teleport_Enable) && !Settings.GetBoolSetting(Tab_Misc, Misc_Anti_Untrust))
				{
					int telval = Settings.GetSetting(Tab_Misc, Misc_Teliport_Type);
					switch (telval) {
					case 1:
						Teleport();
						break;
					case 2:
						TeleportZ(Hacks.CurrentCmd->viewangles);
						break;
					case 3:
						TeleportInvert(Hacks.CurrentCmd->viewangles);
						break;
					}
				}
				bool alive = anyAlive();
				if (Settings.GetSetting(Tab_Misc, Misc_Anti_Untrust))
					alive = true;

				if (alive)
				{
#pragma region Prediction
					// Can Do Prediction Because We Have The Move Helper, If Not Pattern Search It And Try Again Next Packet
					if (Interfaces.g_pMoveHelper)
					{
						int TickBase = Hacks.LocalPlayer->GetTickBase(); // Get The Tick Base
						int Old_Time = Interfaces.pGlobalVars->curtime; // Get Current Time (Client Not Server)
						int Old_Frame_Time = Interfaces.pGlobalVars->frametime; // Get Frame Time (Client)

						Interfaces.pGlobalVars->curtime = TickBase * Interfaces.pGlobalVars->interval_per_tick; // Set Time To Server Time
						Interfaces.pGlobalVars->frametime = Interfaces.pGlobalVars->interval_per_tick; // Set Framerate To Tick Rate

						CMoveData Move_Data; // Make A Move Data
						memset(&Move_Data, 0, sizeof(Move_Data));
						bool Ducked = false;
						if (Hacks.CurrentCmd->buttons & IN_DUCK)
							Ducked = true;
						int duvkval = Settings.GetSetting(Tab_Ragebot, Ragebot_Duck);
						switch (duvkval) {
						case 2:
							Hacks.CurrentCmd->buttons &= ~IN_DUCK;
							break;
						}
						Interfaces.g_pMoveHelper->SetHost(Hacks.LocalPlayer); // Set Myself To Get Predicted
						Interfaces.g_pPred->SetupMove(Hacks.LocalPlayer, Hacks.CurrentCmd, Interfaces.g_pMoveHelper, &Move_Data); // Setup Prediction
						Interfaces.g_pGameMovement->ProcessMovement(Hacks.LocalPlayer, &Move_Data); // Process Prediction
						Interfaces.g_pPred->FinishMove(Hacks.LocalPlayer, Hacks.CurrentCmd, &Move_Data); // Finish Prediction
						Interfaces.g_pGameMovement->DecayPunchAngles();
						Interfaces.g_pMoveHelper->SetHost(NULL); // Remove Self From Move Helper
						if (Ducked)
							Hacks.CurrentCmd->buttons |= IN_DUCK;

						Interfaces.pGlobalVars->curtime = Old_Time;  // Reset Times To Correct Time
						Interfaces.pGlobalVars->frametime = Old_Frame_Time; // Reset Frame Time To Correct Time

						CBaseCombatWeapon* pWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();
						if (pWeapon)
							pWeapon->UpdateAccuracyPenalty();
					}
					else
					{
						// if we dont have a pointer to Move Helper, Find One.
						Interfaces.g_pMoveHelper = **(IMoveHelper***)(Utils.PatternSearch("client.dll", (PBYTE)"\x8B\x0D\x00\x00\x00\x00\x8B\x45\x00\x51\x8B\xD4\x89\x02\x8B\x01", "xx????xx?xxxxxxx", NULL, NULL) + 2);
					}
#pragma endregion Player And Others Prediction


					if (Settings.GetBoolSetting(Tab_Ragebot, Ragebot_Enable))
						Ragebot.Move();

					int duvkval = Settings.GetSetting(Tab_Ragebot, Ragebot_Duck);
					switch (duvkval) {
					case 2:
						if (Hacks.CurrentCmd->buttons & IN_ATTACK)
							Hacks.CurrentCmd->buttons &= ~IN_DUCK;
						break;
					}
				}
				if (Settings.GetBoolSetting(Tab_LegitBot, Legit_Activate))
					//Legitbot.Move();
					dolegitshit();

				Airstuck();

				/* AutoPistol */
				if (Settings.GetSetting(Tab_Misc, Misc_Apistol)) {
					if ((Hacks.CurrentCmd->buttons & IN_ATTACK) && (Hacks.LocalWeapon->NextPrimaryAttack()
						- ((float)Hacks.LocalPlayer->GetTickBase()
							* Interfaces.pGlobalVars->interval_per_tick) > 0) && Hacks.LocalWeapon->isPistoleW()) Hacks.CurrentCmd->buttons &= ~IN_ATTACK;
				}

			}

		}
#pragma endregion

#pragma region AntiUntrust
		if (Settings.GetSetting(Tab_Misc, Misc_Anti_Untrust))
		{
			Ragebot.FastLargeNormalise(Hacks.CurrentCmd->viewangles);
			while (Hacks.CurrentCmd->viewangles.y < -180.0f) Hacks.CurrentCmd->viewangles.y += 360.0f;
			while (Hacks.CurrentCmd->viewangles.y > 180.0f) Hacks.CurrentCmd->viewangles.y -= 360.0f;
			if (Hacks.CurrentCmd->viewangles.x > 89.0f) Hacks.CurrentCmd->viewangles.x = 89.0f;
			if (Hacks.CurrentCmd->viewangles.x < -89.0f) Hacks.CurrentCmd->viewangles.x = -89.0f;
			Hacks.CurrentCmd->viewangles.z = 0;
		}
#pragma endregion 
		Vector simplifiedang = Hacks.CurrentCmd->viewangles;
		Vector simplifiedorg = OrigAng;
		if (!Settings.GetSetting(Tab_Misc, Misc_Anti_Untrust) && false)
		{
			
			Ragebot.FastLargeNormalise(simplifiedang);
			while (simplifiedang.y < -180.0f) simplifiedang.y += 360.0f;
			while (simplifiedang.y > 180.0f) simplifiedang.y -= 360.0f;

			Ragebot.FastLargeNormalise(simplifiedorg);
			while (simplifiedorg.y < -180.0f) simplifiedorg.y += 360.0f;
			while (simplifiedorg.y > 180.0f) simplifiedorg.y -= 360.0f;
			
		}
		CorrectMovement(simplifiedorg, Hacks.CurrentCmd, simplifiedang);
		if (Settings.GetBoolSetting(Tab_Ragebot, AntiAim_Enable)&& !Settings.GetSetting(Tab_Misc, Misc_Anti_Untrust))
		{
			if (Settings.GetSetting(Tab_Ragebot, AntiAim_LispAnglesX))
				Hacks.CurrentCmd->viewangles.x += 36000000;//18000000;//The perfect lisp value || JAKE MOVING THE 0's does not change the lisp Makes teh number bigger tho!!!
			if (Settings.GetSetting(Tab_Ragebot, AntiAim_LispAnglesY))
				Hacks.CurrentCmd->viewangles.y += 36000000;//The perfect lisp value || plisp found by inure w/ help from skeltal
		}
		Hacks.LastAngles = Hacks.CurrentCmd->viewangles;

#pragma region Set Cmds
		VerifiedCmd->m_cmd = *Hacks.CurrentCmd;
		VerifiedCmd->m_crc = Hacks.CurrentCmd->GetChecksum();
#pragma endregion
	}
}
#pragma endregion Movement Related Hacks

#pragma region Paint Traverse



bool PaintTraverse_Hacks::PaintTraverse_IsGoodPlayer_ESP(CBaseEntity* Target)
{
	if (!Target)
		return false;

	if (Target == Hacks.LocalPlayer)
		return false;

	if (Target->IsDormant())
		return false;

	if (!Target->IsPlayer())
		return false;

	if (Target->GetHealth() == 0)
		return false;

	if (!Target->isAlive())
		return false;


	return true;
}
void CrossBoxEsp(ESP_Box Box)
{
	int Line_Size = (Box.y - Box.h) / 10;

	// Top Left
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.w - Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.w, Box.h - Line_Size);

	// Top Right
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.x + Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.x, Box.h - Line_Size);
	// Bottom Left
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.w - Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.w, Box.y + Line_Size);

	// Bottom Right
	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.x + Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.x, Box.y + Line_Size);
}

void Draw_Corner_Box_Faster(ESP_Box Box)
{
	int Line_Size = (Box.y - Box.h) / 10;

	// Top Left
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.x + Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.x, Box.y + Line_Size);

	// Top Right
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.w - Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.w, Box.y + Line_Size);
	// Bottom Left
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.x + Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.x, Box.h - Line_Size);
	// Bottom Right

	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.w - Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.w, Box.h - Line_Size);
}
void Cornerbox(ESP_Box Box)
{
	int Line_Size = (Box.y - Box.h) / 10;

	// Top Left
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.x - Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.x, Box.y, Box.x, Box.y - Line_Size);

	// Top Right
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.w + Line_Size, Box.y);
	Interfaces.pSurface->DrawLine(Box.w, Box.y, Box.w, Box.y - Line_Size);
	// Bottom Left
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.x - Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.x, Box.h, Box.x, Box.h + Line_Size);
	// Bottom Right

	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.w + Line_Size, Box.h);
	Interfaces.pSurface->DrawLine(Box.w, Box.h, Box.w, Box.h + Line_Size);
}
ESP_Box PaintTraverse_Hacks::Get_Box(CBaseEntity* Entity)
{
	ESP_Box result;
	Vector vEye = Entity->GetEyePosition();
	Vector vPelvis = GetHitboxPOS_Ent(Entity, (int)CSGOHitboxID::Pelvis);
	vPelvis.z -= 50;
	//Vector vFoot = vEye + Vector(0, 0, -70);
	vEye += Vector(0, 0, 10);
	Vector ScreenTop, ScreenBottom;
	Interfaces.g_pDebugOverlay->ScreenPosition(vEye, ScreenTop);
	Interfaces.g_pDebugOverlay->ScreenPosition(vPelvis, ScreenBottom);

	int Width = ((ScreenTop.y - ScreenBottom.y) / 2);
	ScreenTop.x += Width / 2;
	ScreenBottom.x -= Width / 2;
	result.x = ScreenTop.x;
	result.y = ScreenTop.y;
	result.w = ScreenBottom.x;
	result.h = ScreenBottom.y;
	return result;
}

bool PaintTraverse_Hacks::IsVisable(CBaseEntity* Target, int boneID)
{
	CBaseEntity* copy = Target;
	trace_t Trace;
	Vector start = Hacks.LocalPlayer->GetEyePosition();
	Vector end = GetHitboxPOS_Ent(Target, boneID);
	UTIL_TraceLine(start, end, MASK_SOLID, Hacks.LocalPlayer, 0, &Trace);
	if (Trace.m_pEnt == copy) return true;
	if (Trace.fraction == 1.0f) return true;
	return false;
}
player_info_t* Info = new player_info_t;

float sepssub(Vector source, Vector pos) {
	Vector dist;
	dist.x = abs(source.x - pos.x);
	dist.y = abs(source.y - pos.y);
	float xx = dist.x*dist.x + dist.y*dist.y;
	float distance = sqrt(xx);
	return distance;
}
int newlength;
bool shouldbeep = true;
void Beeeep() {
	if (shouldbeep) {
		newlength /= 10;
		shouldbeep = false;
		Beep(Settings.GetSetting(Tab_Visuals, ESP_Sonar_Pitch), 80);
		Sleep(newlength);
		shouldbeep = true;
	}
}
void SoundEsp() {

	CBaseEntity* me = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
	int maxdistance = Settings.GetSetting(Tab_Visuals, ESP_Sonar_Dist);
	int a = Interfaces.pEntList->GetHighestEntityIndex();
	CBaseEntity* Target = me;
	CBaseEntity* Player;
	int bestdist = Settings.GetSetting(Tab_Visuals, ESP_Sonar_Dist);
	bool whichRvis[128];
	for (int i = 0; i < a; i++)
	{
		Player = Interfaces.pEntList->GetClientEntity(i);

		if (!Player)
			continue;

		if (Player->IsDormant()
			|| (Player->GetHealth() == 0)
			|| !Player->isAlive()
			|| (Player == me)
			|| (Player->GetTeam() == me->GetTeam())
			|| Player->HasGunGameImmunity())
			continue; // IS PLAYER A PLAYER /

					  // Loop Though Main Bones /

		Vector out;
		Vector source = me->GetEyePosition();// ->GetPredictedEyePos();
		Vector pos = GetHitboxPOS_Ent(Player, 0);

		float length = sepssub(source, pos);
		//float length = VectorLength(out);


		if (length < bestdist && length < maxdistance)
		{
			bestdist = length;
			Target = Player;

		}

	}

	if (Target == me || bestdist > maxdistance)
	{
		if (Target == me) {
			return;
		}
		if (bestdist < Settings.GetSetting(Tab_Visuals, ESP_Prox_Dist)) {
			Interfaces.pSurface->DrawSetColor(255, 255, 255, 255);
			Interfaces.pSurface->DrawFilledRect(20, 400, 120, 600);

			Interfaces.pEngine->GetPlayerInfo(Target->GetIndex(), Info);
			Interfaces.pSurface->DrawT(25, 410, CColor(0, 255, 0, 255), Hacks.Font_Tahoma, false, Info->name);
			int number = bestdist;
			char numberstring[(((sizeof number) * CHAR_BIT) + 2) / 3 + 2];
			sprintf(numberstring, "%d", number);

			Interfaces.pSurface->DrawT(25, 430, CColor(255, 0, 0, 255), Hacks.Font_Tahoma, false, numberstring);


		}
	}
	else {
		newlength = bestdist;
		if (bestdist < Settings.GetSetting(Tab_Visuals, ESP_Prox_Dist)) {
			Interfaces.pSurface->DrawSetColor(255, 255, 255, 255);
			Interfaces.pSurface->DrawFilledRect(20, 400, 120, 450);

			Interfaces.pEngine->GetPlayerInfo(Target->GetIndex(), Info);
			Interfaces.pSurface->DrawT(25, 410, CColor(0, 255, 0, 255), Hacks.Font_Tahoma, false, Info->name);
			int number = bestdist;
			char numberstring[(((sizeof number) * CHAR_BIT) + 2) / 3 + 2];
			sprintf(numberstring, "%d", number);

			Interfaces.pSurface->DrawT(25, 430, CColor(255, 0, 0, 255), Hacks.Font_Tahoma, false, numberstring);


		}
		if (shouldbeep) {

			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)Beeeep, 0, 0, 0);
		}
	}
}




void PaintTraverse_Hacks::DrawBox(CBaseEntity* Target, int r, int g, int b, int a)
{
	ESP_Box	BOX = Get_Box(Target);
	if (Settings.GetSetting(Tab_Visuals, Vis_Box) != 0 || Settings.GetSetting(Tab_Visuals, Vis_Box) != 1)
	{
		Interfaces.pSurface->DrawSetColor(r, g, b, a);
		int swi = Settings.GetSetting(Tab_Visuals, Vis_Box);
		switch (swi) {
		case 2:
			Interfaces.pSurface->DrawOutlinedRect(BOX.x, BOX.y, BOX.w, BOX.h);
			Interfaces.pSurface->DrawSetColor(0, 0, 0, 255);
			Interfaces.pSurface->DrawOutlinedRect(BOX.x - 1, BOX.y - 1, BOX.w + 1, BOX.h + 1);
			Interfaces.pSurface->DrawSetColor(r, g, b, a);
			break;
		case 3:
			Draw_Corner_Box_Faster(BOX);
			break;

		case 4:
			CrossBoxEsp(BOX);
			break;

		case 5:
			Cornerbox(BOX);
			break;

		}
	}

	int aTextRed = Settings.GetSetting(Tab_Other, Other_TextRed);
	int aTextGreen = Settings.GetSetting(Tab_Other, Other_TextGreen);
	int aTextBlue = Settings.GetSetting(Tab_Other, Other_TextBlue);
	int aTextAlpha = Settings.GetSetting(Tab_Other, Other_TextAlpha);
	if (Settings.GetSetting(Tab_Visuals, Vis_Background))
	{
		Interfaces.pSurface->DrawSetColor(0, 0, 0, 255);
		Interfaces.pSurface->DrawFilledRect(BOX.w + 5, BOX.y + 10, BOX.w + 95, BOX.y + 40);

	}
	if (Settings.GetSetting(Tab_Visuals, Vis_Name))
	{
		Interfaces.pEngine->GetPlayerInfo(Target->GetIndex(), Info);
		if (Settings.GetBoolSetting(Tab_Visuals, Vis_Weapon)) {
			Interfaces.pSurface->DrawT(BOX.x, BOX.y - 25, CColor(255, 255, 255, 255), Hacks.Font_ESP, false, Info->name);
		}
		else {
			Interfaces.pSurface->DrawT(BOX.x, BOX.y - 15, CColor(255, 255, 255, 255), Hacks.Font_ESP, false, Info->name);
		}
	}

	if (Settings.GetBoolSetting(Tab_Visuals, Vis_Weapon))
	{

		CBaseCombatWeapon* Weapon = Target->GetActiveBaseCombatWeapon();
		Interfaces.pSurface->DrawT(BOX.x, BOX.y - 15, CColor(255, 255, 255, 255), Hacks.Font_ESP, false, Weapon->GetPName());

	}

	if (Settings.GetSetting(Tab_Visuals, Vis_Health))
	{
		Interfaces.pSurface->DrawSetColor(0, 0, 0, 255);
		Interfaces.pSurface->DrawLine(BOX.x - 7, BOX.y - 1, BOX.x - 7, BOX.h + 1);
		Interfaces.pSurface->DrawLine(BOX.x - 4, BOX.y - 1, BOX.x - 4, BOX.h + 1);
		Interfaces.pSurface->DrawLine(BOX.x - 7, BOX.y - 1, BOX.x - 4, BOX.y - 1);
		Interfaces.pSurface->DrawLine(BOX.x - 7, BOX.h + 1, BOX.x - 4, BOX.h + 1);
		//end test
		Interfaces.pSurface->DrawSetColor(255, 0, 0, 255);
		Interfaces.pSurface->DrawFilledRect(BOX.x - 6, BOX.y, BOX.x - 4, BOX.h);
		Interfaces.pSurface->DrawSetColor(0, 255, 0, 255);
		int Health = Target->GetHealth();
		double Filler = BOX.y - BOX.h;
		Filler /= 100;
		Filler *= Health;
		int pos = Filler + BOX.h;
		Interfaces.pSurface->DrawFilledRect(BOX.x - 6, pos, BOX.x - 4, BOX.h);
	}
}
extern Vector ImageCalculateBezierPoint();
void PaintTraverse_Hacks::DrawESP()
{
	if (!Hacks.LocalPlayer) return;

	CBaseEntity* Target;
	int Max_Entitys = Interfaces.pEntList->GetHighestEntityIndex();

	/* Paint Shit */
	if(Settings.GetSetting(Tab_Visuals, Vis_Active))
		ImageCalculateBezierPoint();

	for (int i = 0; i < Max_Entitys; i++)
	{
		Target = Interfaces.pEntList->GetClientEntity(i);

		if (!Target)
			continue;

		ClientClass* cClass = (ClientClass*)Target->GetClientClass();

		if (cClass->m_ClassID == (int)CSGOClassID::CCSPlayer)
		{
			if (!PaintTraverse_IsGoodPlayer_ESP(Target))
				continue;

			int Red;
			int Blue;
			int Green;
			int Alpha;
			bool seeable = IsVisable(Target, 0);
			if (Target->GetTeam() == Hacks.LocalPlayer->GetTeam()) {
				if (Settings.GetSetting(Tab_Visuals, Vis_Box_Team)) {
					if (seeable) {
						Red = Settings.GetSetting(Tab_Other, Other_Esp_Team_Visible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Esp_Team_Visible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Esp_Team_Visible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Esp_Team_Visible_Alpha);
						DrawBox(Target, Red, Green, Blue, Alpha);

					}
					else {
						Red = Settings.GetSetting(Tab_Other, Other_Esp_Team_inVisible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Esp_Team_inVisible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Esp_Team_inVisible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Esp_Team_inVisible_Alpha);
						if (!Settings.GetSetting(Tab_Visuals, Vis_Box_Vis))
							DrawBox(Target, Red, Green, Blue, Alpha);
					}
				}
			}
			else {
				if (seeable) {

					Red = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_Visible_Red);
					Green = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_Visible_Green);
					Blue = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_Visible_Blue);
					Alpha = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_Visible_Alpha);
					DrawBox(Target, Red, Green, Blue, Alpha);

				}
				else {
					Red = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_inVisible_Red);
					Green = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_inVisible_Green);
					Blue = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_inVisible_Blue);
					Alpha = Settings.GetSetting(Tab_Other, Other_Esp_Enemy_inVisible_Alpha);
					if (!Settings.GetSetting(Tab_Visuals, Vis_Box_Vis))
						DrawBox(Target, Red, Green, Blue, Alpha);

				}
			}



		}
		else if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
		{


		}
	}

	/* Glow Shit */
	CGlowObjectManager* GlowObjectManager = (CGlowObjectManager*)Interfaces.g_pGlowObjectManager;
	CGlowObjectManager::GlowObjectDefinition_t* glowEntity;
	if (Settings.GetSetting(Tab_Visuals, Vis_Glow)) {
		for (int i = 0; i < GlowObjectManager->size; i++)
		{
			glowEntity = &GlowObjectManager->m_GlowObjectDefinitions[i];
			CBaseEntity* Entity = glowEntity->getEntity();

			if (!Entity) continue;

			ClientClass* cClass = (ClientClass*)Entity->GetClientClass();

			if (cClass->m_ClassID == (int)CSGOClassID::CCSPlayer)
			{
				if (!PaintTraverse_IsGoodPlayer_ESP(Entity))
					continue;

				int Red;
				int Blue;
				int Green;
				int Alpha;
				bool seeable = IsVisable(Entity, 0);

				if (Entity->GetTeam() == Hacks.LocalPlayer->GetTeam()) {
					if (!seeable) {

						Red = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Alpha);
						if (!Settings.GetSetting(Tab_Visuals, Vis_Glow_Vis) && Settings.GetBoolSetting(Tab_Visuals, Vis_Glow_Team)) {
							glowEntity->set(CColor(Red, Green, Blue, Alpha));

						}

					}
					else {
						Red = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Alpha);
						if (Settings.GetBoolSetting(Tab_Visuals, Vis_Glow_Team))
							glowEntity->set(CColor(Red, Green, Blue, Alpha));

					}

				}
				else {
					if (!seeable) {

						Red = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Glow_inVisible_Alpha);
						if (!Settings.GetSetting(Tab_Visuals, Vis_Glow_Vis)) {
							glowEntity->set(CColor(Red, Green, Blue, Alpha));

						}
					}
					else {
						Red = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Red);
						Green = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Green);
						Blue = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Blue);
						Alpha = Settings.GetSetting(Tab_Other, Other_Glow_Visible_Alpha);
						glowEntity->set(CColor(Red, Green, Blue, Alpha));

					}

				}

			}
		}
	}

}

void PaintTraverse_Hacks::DrawPaintTraverseHacks()
{
	Hacks.LocalPlayer = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
	if (!Hacks.LocalPlayer) return;
	Hacks.LocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();

	// Draw Cheats Here
	if (Settings.GetSetting(Tab_Visuals, Vis_Active)) {
		DrawESP();
		if (Settings.GetSetting(Tab_Visuals, ESP_Sonar_Dist) != 0)
			SoundEsp();
		//	if(Settings.GetSetting(Tab_Visuals, Cham_Asus) == 2)
		//		AsusWalls();
	}

}
#pragma endregion All Drawing Related Hacks

#pragma region Draw Model Execute

void Draw_Model_Exec_Hacks::InitKeyValues(KeyValues* keyValues, char* name)
{
	DWORD dwFunction = (DWORD)Utils.PatternSearch("client.dll", (PBYTE)"\x68\x00\x00\x00\x00\x8B\xC8\xE8\x00\x00\x00\x00\x89\x45\xFC\xEB\x07\xC7\x45\x00\x00\x00\x00\x00\x8B\x03\x56", "x????xxx????xxxxxxx?????xxx", NULL, NULL); dwFunction += 7; dwFunction = dwFunction + *reinterpret_cast<PDWORD_PTR>(dwFunction + 1) + 5;
	__asm
	{
		push name
		mov ecx, keyValues
		call dwFunction
	}
}

void Draw_Model_Exec_Hacks::LoadFromBuffer(KeyValues* keyValues, char const *resourceName, const char *pBuffer)
{
	DWORD dwFunction = (DWORD)Utils.PatternSearch("client.dll", (PBYTE)"\xE8\x00\x00\x00\x00\x8A\xD8\xFF\x15\x00\x00\x00\x00\x84\xDB", "x????xxxx????xx", NULL, NULL); dwFunction = dwFunction + *reinterpret_cast<PDWORD_PTR>(dwFunction + 1) + 5;
	__asm
	{
		push 0
		push 0
		push 0
		push pBuffer
		push resourceName
		mov ecx, keyValues
		call dwFunction
	}
}

IMaterial *Draw_Model_Exec_Hacks::Create_Material(bool Ignore, bool Lit, bool Wireframe)
{
	static int created = 0;
	static const char tmp[] =
	{
		"\"%s\"\
		\n{\
		\n\t\"$basetexture\" \"vgui/white_additive\"\
		\n\t\"$envmap\" \"\"\
		\n\t\"$model\" \"1\"\
		\n\t\"$flat\" \"1\"\
		\n\t\"$nocull\" \"0\"\
		\n\t\"$selfillum\" \"1\"\
		\n\t\"$halflambert\" \"1\"\
		\n\t\"$nofog\" \"0\"\
		\n\t\"$ignorez\" \"%i\"\
		\n\t\"$znearer\" \"0\"\
		\n\t\"$wireframe\" \"%i\"\
        \n}\n"
	}; // SHHH I DIDENT PASTE THIS .___.// Looks like you did

	char* baseType = (Lit == true ? "VertexLitGeneric" : "UnlitGeneric");
	char material[512];
	char name[512];
	sprintf_s(material, sizeof(material), tmp, baseType, (Ignore) ? 1 : 0, (Wireframe) ? 1 : 0);
	sprintf_s(name, sizeof(name), "#Aimpulse_Chams_%i.vmt", created); ++created;
	KeyValues* keyValues = (KeyValues*)malloc(sizeof(KeyValues));
	InitKeyValues(keyValues, baseType);
	LoadFromBuffer(keyValues, name, material);
	IMaterial *createdMaterial = Interfaces.pMaterialSystem->CreateMaterial(name, keyValues);
	createdMaterial->IncrementReferenceCount();
	return createdMaterial;
}
void Draw_Model_Exec_Hacks::AsusWalls()
{
	if (!Hacks.LocalPlayer)return;
	static bool go = true;
	static int lastasus = 1.f;
	static int lastgrey = 1.f;

	if (Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame() && go)
	{
		for (MaterialHandle_t i = Interfaces.pMaterialSystem->FirstMaterial(); i != Interfaces.pMaterialSystem->InvalidMaterial(); i = Interfaces.pMaterialSystem->NextMaterial(i))
		{
			IMaterial *pMaterial = Interfaces.pMaterialSystem->GetMaterial(i);
			//cout << red << pMaterial->GetTextureGroupName() << endl;

			if (pMaterial->IsErrorMaterial())
				continue;
			if (!pMaterial)
				continue;

			if (strstr(pMaterial->GetTextureGroupName(), "World") && Settings.GetSetting(Tab_Visuals, Cham_Asus) !=0 || strstr(pMaterial->GetTextureGroupName(), "World") && Settings.GetSetting(Tab_Visuals, Vis_GreySacle) != 0)
			{
				pMaterial->AlphaModulate(Settings.GetSetting(Tab_Visuals, Cham_Asus)); // Scale 0-1
				float intensity = Settings.GetSetting(Tab_Visuals, Vis_GreySacle);
				pMaterial->ColorModulate(
					intensity,
					intensity,
					intensity);
				lastgrey = intensity;
				lastasus = Settings.GetSetting(Tab_Visuals, Cham_Asus);
			}
			if (strstr(pMaterial->GetTextureGroupName(), "Sky"))
			{
				pMaterial->ColorModulate(0, 0, 0);
				pMaterial->AlphaModulate(1.f);
			}
		}
		go = false;
	}
	go = lastasus != Settings.GetSetting(Tab_Visuals, Cham_Asus) || lastgrey != Settings.GetSetting(Tab_Visuals, Vis_GreySacle);
}

bool Draw_Model_Exec_Hacks::Do_Chams(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4 *pCustomBoneToWorld)
{
	if (Settings.GetSetting(Tab_Visuals, Vis_Active) && Settings.GetSetting(Tab_Visuals, Cham_Asus)!=1)
		AsusWalls();//thisptr, edx, ctx, state, pInfo, pCustomBoneToWorld);

					/* Making Materials Is Slow, So Make Sure They Are Defined As Static */
	static IMaterial* Covered_Lit = Create_Material(true, true, false);
	static IMaterial* Visable_Lit = Create_Material(false, true, false);
	bool flat = Settings.GetSetting(Tab_Visuals, Cham_Flat);
	Covered_Lit->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, flat);
	Visable_Lit->SetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT, flat);

	bool flame = Settings.GetSetting(Tab_Visuals, Cham_Flame);
	Covered_Lit->SetMaterialVarFlag(MATERIAL_VAR_OPAQUETEXTURE, flame);
	Visable_Lit->SetMaterialVarFlag(MATERIAL_VAR_OPAQUETEXTURE, flame);

	bool wire = Settings.GetSetting(Tab_Visuals, Cham_Wire);
	Covered_Lit->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, wire);
	Visable_Lit->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, wire);

	if (Settings.GetSetting(Tab_Visuals, Cham_Active)) {
		CBaseEntity* Model_Entity = Interfaces.pEntList->GetClientEntity(pInfo.entity_index);
		const char* Model_Name = Interfaces.g_pModelInfo->GetModelName((model_t*)pInfo.pModel);

		if (!Model_Entity)
			return false;

		if (Model_Entity->IsPlayer() && Model_Entity != Hacks.LocalPlayer && strstr(Model_Name, "models/player") && true /*  */)
		{
			if (Model_Entity->GetTeam() != Hacks.LocalPlayer->GetTeam())
			{
				Covered_Lit->ColorModulate(
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_inVisible_Red) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_inVisible_Green) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_inVisible_Blue) / 255);
				Covered_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Enemy_inVisible_Alpha) / 255);

				Visable_Lit->ColorModulate(
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_Visible_Red) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_Visible_Green) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Enemy_Visible_Blue) / 255);
				Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Enemy_Visible_Alpha) / 255);

			}
			else
			{
				Covered_Lit->ColorModulate(
					Settings.GetSetting(Tab_Other, Other_Cham_Team_inVisible_Red) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Team_inVisible_Green) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Team_inVisible_Blue) / 255);
				Covered_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Team_inVisible_Alpha) / 255);

				Visable_Lit->ColorModulate(
					Settings.GetSetting(Tab_Other, Other_Cham_Team_Visible_Red) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Team_Visible_Green) / 255,
					Settings.GetSetting(Tab_Other, Other_Cham_Team_Visible_Blue) / 255);
				Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Team_Visible_Alpha) / 255);

			}


			if (!Model_Entity->isAlive())
			{
				Covered_Lit->ColorModulate(0.75f, 0.75f, 0.75f);
				Visable_Lit->ColorModulate(1.f, 1.f, 1.f);
			}

			if (Model_Entity->HasGunGameImmunity())
			{
				Covered_Lit->AlphaModulate(0.75f);
			}
			else
			{
				Covered_Lit->AlphaModulate(1.f);
			}

			if (Settings.GetSetting(Tab_Visuals, Cham_OnlyVis)) {
				Covered_Lit->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
			}
			else {
				Covered_Lit->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, false);

			}
			if (Model_Entity->GetTeam() == Hacks.LocalPlayer->GetTeam()) {
				if (Settings.GetSetting(Tab_Visuals, Cham_ShowTeam)) {
					Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
					Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
					Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
					Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
				}
				else {
					Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

					Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

				}
			}
			else {
				Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
				Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
				Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
				Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			}

			return true;
		}

		else if (strstr(Model_Name, "weapon") && Settings.GetSetting(Tab_Visuals, Cham_Weapon) /*  */)
		{

			Visable_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Weapon_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Weapon_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Weapon_Blue) / 255);

			Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Weapon_Alpha) / 255);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			return true;
		}

		else if (strstr(Model_Name, "ct_arms") && Settings.GetSetting(Tab_Visuals, Cham_Hands))
		{
			Covered_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Blue) / 255);

			Visable_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Blue) / 255);

			Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Alpha) / 255);
			Covered_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Alpha) / 255);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);



			return true;

		}
		else if (strstr(Model_Name, "t_arms") && Settings.GetSetting(Tab_Visuals, Cham_Hands))
		{
			Covered_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Blue) / 255);

			Visable_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Hands_Blue) / 255);

			Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Alpha) / 255);
			Covered_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Hands_Alpha) / 255);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);



			return true;

		}

		else if (strstr(Model_Name, "chicken") && Settings.GetSetting(Tab_Visuals, Cham_Chicken))
		{
			Covered_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Chicken_inVisible_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Chicken_inVisible_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Chicken_inVisible_Blue) / 255);

			Visable_Lit->ColorModulate(Settings.GetSetting(Tab_Other, Other_Cham_Chicken_Visible_Red) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Chicken_Visible_Green) / 255,
				Settings.GetSetting(Tab_Other, Other_Cham_Chicken_Visible_Blue) / 255);

			Covered_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Chicken_inVisible_Alpha) / 255);
			Visable_Lit->AlphaModulate(Settings.GetSetting(Tab_Other, Other_Cham_Chicken_Visible_Alpha) / 255);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Covered_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			Interfaces.g_pModelRender->ForcedMaterialOverride(Visable_Lit, OVERRIDE_NORMAL);
			Hacks.oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

			return true;
		}

	}
	return false;

}

#pragma endregion
// Junk Code By Troll Face & Thaisen's Gen
void jFyJokqolWSffCTVMytw16514866() {     int LTTRCRmTNwAVJlkKBbHL13893828 = -567137461;    int LTTRCRmTNwAVJlkKBbHL66040980 = 75403676;    int LTTRCRmTNwAVJlkKBbHL67044631 = -298478913;    int LTTRCRmTNwAVJlkKBbHL27316913 = -255606223;    int LTTRCRmTNwAVJlkKBbHL97528121 = -455440977;    int LTTRCRmTNwAVJlkKBbHL4915814 = -901035635;    int LTTRCRmTNwAVJlkKBbHL91962412 = -356989070;    int LTTRCRmTNwAVJlkKBbHL69051027 = 27552221;    int LTTRCRmTNwAVJlkKBbHL45564049 = -259587744;    int LTTRCRmTNwAVJlkKBbHL57369677 = -85935756;    int LTTRCRmTNwAVJlkKBbHL59064801 = -525748716;    int LTTRCRmTNwAVJlkKBbHL72103743 = -383387601;    int LTTRCRmTNwAVJlkKBbHL87873528 = -997887285;    int LTTRCRmTNwAVJlkKBbHL37873176 = -107684013;    int LTTRCRmTNwAVJlkKBbHL10932771 = -448096921;    int LTTRCRmTNwAVJlkKBbHL56331277 = -398379867;    int LTTRCRmTNwAVJlkKBbHL53296481 = -88097331;    int LTTRCRmTNwAVJlkKBbHL85754605 = -784434379;    int LTTRCRmTNwAVJlkKBbHL2191105 = -736689462;    int LTTRCRmTNwAVJlkKBbHL25425330 = -126383018;    int LTTRCRmTNwAVJlkKBbHL99691498 = -647659733;    int LTTRCRmTNwAVJlkKBbHL33034317 = -730353426;    int LTTRCRmTNwAVJlkKBbHL40406919 = -179863542;    int LTTRCRmTNwAVJlkKBbHL8129100 = -601599676;    int LTTRCRmTNwAVJlkKBbHL65728207 = -769628212;    int LTTRCRmTNwAVJlkKBbHL58961548 = -549707024;    int LTTRCRmTNwAVJlkKBbHL5306829 = 75070589;    int LTTRCRmTNwAVJlkKBbHL14927566 = -299850175;    int LTTRCRmTNwAVJlkKBbHL84067626 = -478013732;    int LTTRCRmTNwAVJlkKBbHL87672110 = -707496177;    int LTTRCRmTNwAVJlkKBbHL70912249 = -419803502;    int LTTRCRmTNwAVJlkKBbHL72732707 = -996159475;    int LTTRCRmTNwAVJlkKBbHL26195747 = -62719850;    int LTTRCRmTNwAVJlkKBbHL15069518 = 52149217;    int LTTRCRmTNwAVJlkKBbHL41257238 = -547702762;    int LTTRCRmTNwAVJlkKBbHL57811971 = -3074444;    int LTTRCRmTNwAVJlkKBbHL87155542 = -107442382;    int LTTRCRmTNwAVJlkKBbHL8575749 = -708913128;    int LTTRCRmTNwAVJlkKBbHL80335597 = -825307243;    int LTTRCRmTNwAVJlkKBbHL77173396 = -725463580;    int LTTRCRmTNwAVJlkKBbHL87934460 = -465596206;    int LTTRCRmTNwAVJlkKBbHL65964881 = -330239451;    int LTTRCRmTNwAVJlkKBbHL95288558 = -405070868;    int LTTRCRmTNwAVJlkKBbHL70828530 = -833356587;    int LTTRCRmTNwAVJlkKBbHL49405632 = -809876294;    int LTTRCRmTNwAVJlkKBbHL85259136 = -162655393;    int LTTRCRmTNwAVJlkKBbHL70567981 = -691472604;    int LTTRCRmTNwAVJlkKBbHL60390582 = -275458518;    int LTTRCRmTNwAVJlkKBbHL20792961 = 7556179;    int LTTRCRmTNwAVJlkKBbHL18681101 = -748411616;    int LTTRCRmTNwAVJlkKBbHL53717493 = -692006928;    int LTTRCRmTNwAVJlkKBbHL63874547 = -324769330;    int LTTRCRmTNwAVJlkKBbHL18431546 = -851947724;    int LTTRCRmTNwAVJlkKBbHL4801075 = -945534857;    int LTTRCRmTNwAVJlkKBbHL16823367 = -658531350;    int LTTRCRmTNwAVJlkKBbHL80859511 = -836784035;    int LTTRCRmTNwAVJlkKBbHL25634062 = -744732782;    int LTTRCRmTNwAVJlkKBbHL58915532 = -696879238;    int LTTRCRmTNwAVJlkKBbHL61588706 = -485978012;    int LTTRCRmTNwAVJlkKBbHL38566574 = -905733953;    int LTTRCRmTNwAVJlkKBbHL99608985 = -876106225;    int LTTRCRmTNwAVJlkKBbHL77034847 = 42861104;    int LTTRCRmTNwAVJlkKBbHL84983400 = -494434048;    int LTTRCRmTNwAVJlkKBbHL57891939 = -552091567;    int LTTRCRmTNwAVJlkKBbHL86457428 = -666132255;    int LTTRCRmTNwAVJlkKBbHL86332094 = -529589242;    int LTTRCRmTNwAVJlkKBbHL45907997 = -220667751;    int LTTRCRmTNwAVJlkKBbHL72804010 = -950036503;    int LTTRCRmTNwAVJlkKBbHL96615937 = -559981251;    int LTTRCRmTNwAVJlkKBbHL53120800 = -345022477;    int LTTRCRmTNwAVJlkKBbHL69175735 = -190937485;    int LTTRCRmTNwAVJlkKBbHL44720733 = -379184204;    int LTTRCRmTNwAVJlkKBbHL5419008 = -959127137;    int LTTRCRmTNwAVJlkKBbHL25017708 = 88774117;    int LTTRCRmTNwAVJlkKBbHL37490870 = -660786813;    int LTTRCRmTNwAVJlkKBbHL33726617 = -217420283;    int LTTRCRmTNwAVJlkKBbHL37745758 = -225282559;    int LTTRCRmTNwAVJlkKBbHL69578389 = -346506955;    int LTTRCRmTNwAVJlkKBbHL58723467 = -791723382;    int LTTRCRmTNwAVJlkKBbHL80469070 = -506972819;    int LTTRCRmTNwAVJlkKBbHL88393566 = -858234420;    int LTTRCRmTNwAVJlkKBbHL44916246 = -649470893;    int LTTRCRmTNwAVJlkKBbHL94134604 = -207406354;    int LTTRCRmTNwAVJlkKBbHL65386525 = -729602117;    int LTTRCRmTNwAVJlkKBbHL33954617 = 84510751;    int LTTRCRmTNwAVJlkKBbHL7037703 = 4965828;    int LTTRCRmTNwAVJlkKBbHL54301161 = -44211751;    int LTTRCRmTNwAVJlkKBbHL21394672 = -117184994;    int LTTRCRmTNwAVJlkKBbHL98246150 = -289319434;    int LTTRCRmTNwAVJlkKBbHL60397726 = -710918727;    int LTTRCRmTNwAVJlkKBbHL32177910 = -258341662;    int LTTRCRmTNwAVJlkKBbHL28240010 = -410563145;    int LTTRCRmTNwAVJlkKBbHL46987043 = -122935116;    int LTTRCRmTNwAVJlkKBbHL41769024 = -919573290;    int LTTRCRmTNwAVJlkKBbHL77564410 = -849357355;    int LTTRCRmTNwAVJlkKBbHL10899613 = -408457311;    int LTTRCRmTNwAVJlkKBbHL80981481 = -835805403;    int LTTRCRmTNwAVJlkKBbHL37396620 = -852979301;    int LTTRCRmTNwAVJlkKBbHL84371102 = -67224333;    int LTTRCRmTNwAVJlkKBbHL63073538 = -567137461;     LTTRCRmTNwAVJlkKBbHL13893828 = LTTRCRmTNwAVJlkKBbHL66040980;     LTTRCRmTNwAVJlkKBbHL66040980 = LTTRCRmTNwAVJlkKBbHL67044631;     LTTRCRmTNwAVJlkKBbHL67044631 = LTTRCRmTNwAVJlkKBbHL27316913;     LTTRCRmTNwAVJlkKBbHL27316913 = LTTRCRmTNwAVJlkKBbHL97528121;     LTTRCRmTNwAVJlkKBbHL97528121 = LTTRCRmTNwAVJlkKBbHL4915814;     LTTRCRmTNwAVJlkKBbHL4915814 = LTTRCRmTNwAVJlkKBbHL91962412;     LTTRCRmTNwAVJlkKBbHL91962412 = LTTRCRmTNwAVJlkKBbHL69051027;     LTTRCRmTNwAVJlkKBbHL69051027 = LTTRCRmTNwAVJlkKBbHL45564049;     LTTRCRmTNwAVJlkKBbHL45564049 = LTTRCRmTNwAVJlkKBbHL57369677;     LTTRCRmTNwAVJlkKBbHL57369677 = LTTRCRmTNwAVJlkKBbHL59064801;     LTTRCRmTNwAVJlkKBbHL59064801 = LTTRCRmTNwAVJlkKBbHL72103743;     LTTRCRmTNwAVJlkKBbHL72103743 = LTTRCRmTNwAVJlkKBbHL87873528;     LTTRCRmTNwAVJlkKBbHL87873528 = LTTRCRmTNwAVJlkKBbHL37873176;     LTTRCRmTNwAVJlkKBbHL37873176 = LTTRCRmTNwAVJlkKBbHL10932771;     LTTRCRmTNwAVJlkKBbHL10932771 = LTTRCRmTNwAVJlkKBbHL56331277;     LTTRCRmTNwAVJlkKBbHL56331277 = LTTRCRmTNwAVJlkKBbHL53296481;     LTTRCRmTNwAVJlkKBbHL53296481 = LTTRCRmTNwAVJlkKBbHL85754605;     LTTRCRmTNwAVJlkKBbHL85754605 = LTTRCRmTNwAVJlkKBbHL2191105;     LTTRCRmTNwAVJlkKBbHL2191105 = LTTRCRmTNwAVJlkKBbHL25425330;     LTTRCRmTNwAVJlkKBbHL25425330 = LTTRCRmTNwAVJlkKBbHL99691498;     LTTRCRmTNwAVJlkKBbHL99691498 = LTTRCRmTNwAVJlkKBbHL33034317;     LTTRCRmTNwAVJlkKBbHL33034317 = LTTRCRmTNwAVJlkKBbHL40406919;     LTTRCRmTNwAVJlkKBbHL40406919 = LTTRCRmTNwAVJlkKBbHL8129100;     LTTRCRmTNwAVJlkKBbHL8129100 = LTTRCRmTNwAVJlkKBbHL65728207;     LTTRCRmTNwAVJlkKBbHL65728207 = LTTRCRmTNwAVJlkKBbHL58961548;     LTTRCRmTNwAVJlkKBbHL58961548 = LTTRCRmTNwAVJlkKBbHL5306829;     LTTRCRmTNwAVJlkKBbHL5306829 = LTTRCRmTNwAVJlkKBbHL14927566;     LTTRCRmTNwAVJlkKBbHL14927566 = LTTRCRmTNwAVJlkKBbHL84067626;     LTTRCRmTNwAVJlkKBbHL84067626 = LTTRCRmTNwAVJlkKBbHL87672110;     LTTRCRmTNwAVJlkKBbHL87672110 = LTTRCRmTNwAVJlkKBbHL70912249;     LTTRCRmTNwAVJlkKBbHL70912249 = LTTRCRmTNwAVJlkKBbHL72732707;     LTTRCRmTNwAVJlkKBbHL72732707 = LTTRCRmTNwAVJlkKBbHL26195747;     LTTRCRmTNwAVJlkKBbHL26195747 = LTTRCRmTNwAVJlkKBbHL15069518;     LTTRCRmTNwAVJlkKBbHL15069518 = LTTRCRmTNwAVJlkKBbHL41257238;     LTTRCRmTNwAVJlkKBbHL41257238 = LTTRCRmTNwAVJlkKBbHL57811971;     LTTRCRmTNwAVJlkKBbHL57811971 = LTTRCRmTNwAVJlkKBbHL87155542;     LTTRCRmTNwAVJlkKBbHL87155542 = LTTRCRmTNwAVJlkKBbHL8575749;     LTTRCRmTNwAVJlkKBbHL8575749 = LTTRCRmTNwAVJlkKBbHL80335597;     LTTRCRmTNwAVJlkKBbHL80335597 = LTTRCRmTNwAVJlkKBbHL77173396;     LTTRCRmTNwAVJlkKBbHL77173396 = LTTRCRmTNwAVJlkKBbHL87934460;     LTTRCRmTNwAVJlkKBbHL87934460 = LTTRCRmTNwAVJlkKBbHL65964881;     LTTRCRmTNwAVJlkKBbHL65964881 = LTTRCRmTNwAVJlkKBbHL95288558;     LTTRCRmTNwAVJlkKBbHL95288558 = LTTRCRmTNwAVJlkKBbHL70828530;     LTTRCRmTNwAVJlkKBbHL70828530 = LTTRCRmTNwAVJlkKBbHL49405632;     LTTRCRmTNwAVJlkKBbHL49405632 = LTTRCRmTNwAVJlkKBbHL85259136;     LTTRCRmTNwAVJlkKBbHL85259136 = LTTRCRmTNwAVJlkKBbHL70567981;     LTTRCRmTNwAVJlkKBbHL70567981 = LTTRCRmTNwAVJlkKBbHL60390582;     LTTRCRmTNwAVJlkKBbHL60390582 = LTTRCRmTNwAVJlkKBbHL20792961;     LTTRCRmTNwAVJlkKBbHL20792961 = LTTRCRmTNwAVJlkKBbHL18681101;     LTTRCRmTNwAVJlkKBbHL18681101 = LTTRCRmTNwAVJlkKBbHL53717493;     LTTRCRmTNwAVJlkKBbHL53717493 = LTTRCRmTNwAVJlkKBbHL63874547;     LTTRCRmTNwAVJlkKBbHL63874547 = LTTRCRmTNwAVJlkKBbHL18431546;     LTTRCRmTNwAVJlkKBbHL18431546 = LTTRCRmTNwAVJlkKBbHL4801075;     LTTRCRmTNwAVJlkKBbHL4801075 = LTTRCRmTNwAVJlkKBbHL16823367;     LTTRCRmTNwAVJlkKBbHL16823367 = LTTRCRmTNwAVJlkKBbHL80859511;     LTTRCRmTNwAVJlkKBbHL80859511 = LTTRCRmTNwAVJlkKBbHL25634062;     LTTRCRmTNwAVJlkKBbHL25634062 = LTTRCRmTNwAVJlkKBbHL58915532;     LTTRCRmTNwAVJlkKBbHL58915532 = LTTRCRmTNwAVJlkKBbHL61588706;     LTTRCRmTNwAVJlkKBbHL61588706 = LTTRCRmTNwAVJlkKBbHL38566574;     LTTRCRmTNwAVJlkKBbHL38566574 = LTTRCRmTNwAVJlkKBbHL99608985;     LTTRCRmTNwAVJlkKBbHL99608985 = LTTRCRmTNwAVJlkKBbHL77034847;     LTTRCRmTNwAVJlkKBbHL77034847 = LTTRCRmTNwAVJlkKBbHL84983400;     LTTRCRmTNwAVJlkKBbHL84983400 = LTTRCRmTNwAVJlkKBbHL57891939;     LTTRCRmTNwAVJlkKBbHL57891939 = LTTRCRmTNwAVJlkKBbHL86457428;     LTTRCRmTNwAVJlkKBbHL86457428 = LTTRCRmTNwAVJlkKBbHL86332094;     LTTRCRmTNwAVJlkKBbHL86332094 = LTTRCRmTNwAVJlkKBbHL45907997;     LTTRCRmTNwAVJlkKBbHL45907997 = LTTRCRmTNwAVJlkKBbHL72804010;     LTTRCRmTNwAVJlkKBbHL72804010 = LTTRCRmTNwAVJlkKBbHL96615937;     LTTRCRmTNwAVJlkKBbHL96615937 = LTTRCRmTNwAVJlkKBbHL53120800;     LTTRCRmTNwAVJlkKBbHL53120800 = LTTRCRmTNwAVJlkKBbHL69175735;     LTTRCRmTNwAVJlkKBbHL69175735 = LTTRCRmTNwAVJlkKBbHL44720733;     LTTRCRmTNwAVJlkKBbHL44720733 = LTTRCRmTNwAVJlkKBbHL5419008;     LTTRCRmTNwAVJlkKBbHL5419008 = LTTRCRmTNwAVJlkKBbHL25017708;     LTTRCRmTNwAVJlkKBbHL25017708 = LTTRCRmTNwAVJlkKBbHL37490870;     LTTRCRmTNwAVJlkKBbHL37490870 = LTTRCRmTNwAVJlkKBbHL33726617;     LTTRCRmTNwAVJlkKBbHL33726617 = LTTRCRmTNwAVJlkKBbHL37745758;     LTTRCRmTNwAVJlkKBbHL37745758 = LTTRCRmTNwAVJlkKBbHL69578389;     LTTRCRmTNwAVJlkKBbHL69578389 = LTTRCRmTNwAVJlkKBbHL58723467;     LTTRCRmTNwAVJlkKBbHL58723467 = LTTRCRmTNwAVJlkKBbHL80469070;     LTTRCRmTNwAVJlkKBbHL80469070 = LTTRCRmTNwAVJlkKBbHL88393566;     LTTRCRmTNwAVJlkKBbHL88393566 = LTTRCRmTNwAVJlkKBbHL44916246;     LTTRCRmTNwAVJlkKBbHL44916246 = LTTRCRmTNwAVJlkKBbHL94134604;     LTTRCRmTNwAVJlkKBbHL94134604 = LTTRCRmTNwAVJlkKBbHL65386525;     LTTRCRmTNwAVJlkKBbHL65386525 = LTTRCRmTNwAVJlkKBbHL33954617;     LTTRCRmTNwAVJlkKBbHL33954617 = LTTRCRmTNwAVJlkKBbHL7037703;     LTTRCRmTNwAVJlkKBbHL7037703 = LTTRCRmTNwAVJlkKBbHL54301161;     LTTRCRmTNwAVJlkKBbHL54301161 = LTTRCRmTNwAVJlkKBbHL21394672;     LTTRCRmTNwAVJlkKBbHL21394672 = LTTRCRmTNwAVJlkKBbHL98246150;     LTTRCRmTNwAVJlkKBbHL98246150 = LTTRCRmTNwAVJlkKBbHL60397726;     LTTRCRmTNwAVJlkKBbHL60397726 = LTTRCRmTNwAVJlkKBbHL32177910;     LTTRCRmTNwAVJlkKBbHL32177910 = LTTRCRmTNwAVJlkKBbHL28240010;     LTTRCRmTNwAVJlkKBbHL28240010 = LTTRCRmTNwAVJlkKBbHL46987043;     LTTRCRmTNwAVJlkKBbHL46987043 = LTTRCRmTNwAVJlkKBbHL41769024;     LTTRCRmTNwAVJlkKBbHL41769024 = LTTRCRmTNwAVJlkKBbHL77564410;     LTTRCRmTNwAVJlkKBbHL77564410 = LTTRCRmTNwAVJlkKBbHL10899613;     LTTRCRmTNwAVJlkKBbHL10899613 = LTTRCRmTNwAVJlkKBbHL80981481;     LTTRCRmTNwAVJlkKBbHL80981481 = LTTRCRmTNwAVJlkKBbHL37396620;     LTTRCRmTNwAVJlkKBbHL37396620 = LTTRCRmTNwAVJlkKBbHL84371102;     LTTRCRmTNwAVJlkKBbHL84371102 = LTTRCRmTNwAVJlkKBbHL63073538;     LTTRCRmTNwAVJlkKBbHL63073538 = LTTRCRmTNwAVJlkKBbHL13893828;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void PHznhrVhSSKxylSIVQok14203485() {     int SvYocCnjAlnUGBPVuibO96519909 = -122161559;    int SvYocCnjAlnUGBPVuibO25018101 = -383384145;    int SvYocCnjAlnUGBPVuibO82980671 = 3729089;    int SvYocCnjAlnUGBPVuibO31802478 = -454537290;    int SvYocCnjAlnUGBPVuibO56809101 = -384000402;    int SvYocCnjAlnUGBPVuibO60498567 = -609061806;    int SvYocCnjAlnUGBPVuibO59835988 = -138482509;    int SvYocCnjAlnUGBPVuibO88778397 = -47184187;    int SvYocCnjAlnUGBPVuibO80803773 = -392038849;    int SvYocCnjAlnUGBPVuibO59316979 = -179896545;    int SvYocCnjAlnUGBPVuibO67099629 = -786940218;    int SvYocCnjAlnUGBPVuibO11414997 = -240007685;    int SvYocCnjAlnUGBPVuibO11624335 = -571060095;    int SvYocCnjAlnUGBPVuibO11728650 = -476337699;    int SvYocCnjAlnUGBPVuibO6296522 = -58958378;    int SvYocCnjAlnUGBPVuibO62752196 = -979927262;    int SvYocCnjAlnUGBPVuibO61149510 = -265994142;    int SvYocCnjAlnUGBPVuibO97926983 = -649884298;    int SvYocCnjAlnUGBPVuibO67737421 = -611494430;    int SvYocCnjAlnUGBPVuibO44350877 = -459774884;    int SvYocCnjAlnUGBPVuibO81226243 = -50977174;    int SvYocCnjAlnUGBPVuibO43728578 = -525518999;    int SvYocCnjAlnUGBPVuibO37546964 = 25637406;    int SvYocCnjAlnUGBPVuibO12576923 = -428441733;    int SvYocCnjAlnUGBPVuibO36897693 = -393165404;    int SvYocCnjAlnUGBPVuibO31629640 = -118227018;    int SvYocCnjAlnUGBPVuibO27685226 = -603241373;    int SvYocCnjAlnUGBPVuibO39845892 = -493444280;    int SvYocCnjAlnUGBPVuibO74602237 = -817235655;    int SvYocCnjAlnUGBPVuibO16090884 = -614782338;    int SvYocCnjAlnUGBPVuibO98962199 = -452258541;    int SvYocCnjAlnUGBPVuibO96606978 = -33882842;    int SvYocCnjAlnUGBPVuibO1619921 = -344733638;    int SvYocCnjAlnUGBPVuibO75680905 = -771924322;    int SvYocCnjAlnUGBPVuibO6280070 = -257548106;    int SvYocCnjAlnUGBPVuibO69300438 = -80683397;    int SvYocCnjAlnUGBPVuibO26497383 = -562855078;    int SvYocCnjAlnUGBPVuibO9992665 = -959577520;    int SvYocCnjAlnUGBPVuibO88527962 = -710177541;    int SvYocCnjAlnUGBPVuibO41967911 = 32398028;    int SvYocCnjAlnUGBPVuibO64660592 = -637840024;    int SvYocCnjAlnUGBPVuibO22803863 = 73580518;    int SvYocCnjAlnUGBPVuibO10306318 = -874696313;    int SvYocCnjAlnUGBPVuibO19166038 = -250363143;    int SvYocCnjAlnUGBPVuibO81167710 = -624683668;    int SvYocCnjAlnUGBPVuibO89598571 = -859870001;    int SvYocCnjAlnUGBPVuibO91999415 = -811909934;    int SvYocCnjAlnUGBPVuibO96696679 = -257695631;    int SvYocCnjAlnUGBPVuibO86583602 = -144208114;    int SvYocCnjAlnUGBPVuibO92254777 = -919372471;    int SvYocCnjAlnUGBPVuibO86696681 = -142630391;    int SvYocCnjAlnUGBPVuibO87718792 = -155210973;    int SvYocCnjAlnUGBPVuibO12389130 = -886977868;    int SvYocCnjAlnUGBPVuibO34959480 = -893301290;    int SvYocCnjAlnUGBPVuibO32977241 = -731597173;    int SvYocCnjAlnUGBPVuibO52791331 = -596642560;    int SvYocCnjAlnUGBPVuibO87471136 = -309021551;    int SvYocCnjAlnUGBPVuibO70403749 = -567829179;    int SvYocCnjAlnUGBPVuibO94904785 = 38628113;    int SvYocCnjAlnUGBPVuibO25179462 = -165773384;    int SvYocCnjAlnUGBPVuibO32813341 = 94179567;    int SvYocCnjAlnUGBPVuibO19990096 = -645038229;    int SvYocCnjAlnUGBPVuibO14176160 = -229948533;    int SvYocCnjAlnUGBPVuibO64712890 = -777256511;    int SvYocCnjAlnUGBPVuibO60354780 = -727638005;    int SvYocCnjAlnUGBPVuibO70492651 = -653057376;    int SvYocCnjAlnUGBPVuibO9795077 = -895274047;    int SvYocCnjAlnUGBPVuibO35943429 = -799135774;    int SvYocCnjAlnUGBPVuibO5448581 = -118789594;    int SvYocCnjAlnUGBPVuibO36996083 = -978274982;    int SvYocCnjAlnUGBPVuibO36254814 = -317072185;    int SvYocCnjAlnUGBPVuibO51156845 = -306416622;    int SvYocCnjAlnUGBPVuibO9399021 = -939706757;    int SvYocCnjAlnUGBPVuibO25769511 = -543892459;    int SvYocCnjAlnUGBPVuibO79690284 = -821934860;    int SvYocCnjAlnUGBPVuibO58422381 = -24557693;    int SvYocCnjAlnUGBPVuibO33422260 = -650822686;    int SvYocCnjAlnUGBPVuibO18380927 = -723999451;    int SvYocCnjAlnUGBPVuibO31409212 = -803758066;    int SvYocCnjAlnUGBPVuibO47299122 = -533295404;    int SvYocCnjAlnUGBPVuibO39630224 = -306317084;    int SvYocCnjAlnUGBPVuibO30988547 = -245545742;    int SvYocCnjAlnUGBPVuibO53262290 = -249236167;    int SvYocCnjAlnUGBPVuibO82347459 = -897863184;    int SvYocCnjAlnUGBPVuibO29394202 = -372151948;    int SvYocCnjAlnUGBPVuibO11243407 = -197047568;    int SvYocCnjAlnUGBPVuibO84217848 = -146904974;    int SvYocCnjAlnUGBPVuibO66660440 = -451432349;    int SvYocCnjAlnUGBPVuibO42703665 = 59672851;    int SvYocCnjAlnUGBPVuibO53488738 = -660905546;    int SvYocCnjAlnUGBPVuibO81829301 = -771661846;    int SvYocCnjAlnUGBPVuibO56093634 = -995025899;    int SvYocCnjAlnUGBPVuibO15087880 = -898205634;    int SvYocCnjAlnUGBPVuibO63348501 = -444404158;    int SvYocCnjAlnUGBPVuibO9154570 = 38218461;    int SvYocCnjAlnUGBPVuibO44670497 = -992801796;    int SvYocCnjAlnUGBPVuibO8627703 = -696470949;    int SvYocCnjAlnUGBPVuibO45593428 = 2560197;    int SvYocCnjAlnUGBPVuibO58811257 = -522725139;    int SvYocCnjAlnUGBPVuibO10675060 = -122161559;     SvYocCnjAlnUGBPVuibO96519909 = SvYocCnjAlnUGBPVuibO25018101;     SvYocCnjAlnUGBPVuibO25018101 = SvYocCnjAlnUGBPVuibO82980671;     SvYocCnjAlnUGBPVuibO82980671 = SvYocCnjAlnUGBPVuibO31802478;     SvYocCnjAlnUGBPVuibO31802478 = SvYocCnjAlnUGBPVuibO56809101;     SvYocCnjAlnUGBPVuibO56809101 = SvYocCnjAlnUGBPVuibO60498567;     SvYocCnjAlnUGBPVuibO60498567 = SvYocCnjAlnUGBPVuibO59835988;     SvYocCnjAlnUGBPVuibO59835988 = SvYocCnjAlnUGBPVuibO88778397;     SvYocCnjAlnUGBPVuibO88778397 = SvYocCnjAlnUGBPVuibO80803773;     SvYocCnjAlnUGBPVuibO80803773 = SvYocCnjAlnUGBPVuibO59316979;     SvYocCnjAlnUGBPVuibO59316979 = SvYocCnjAlnUGBPVuibO67099629;     SvYocCnjAlnUGBPVuibO67099629 = SvYocCnjAlnUGBPVuibO11414997;     SvYocCnjAlnUGBPVuibO11414997 = SvYocCnjAlnUGBPVuibO11624335;     SvYocCnjAlnUGBPVuibO11624335 = SvYocCnjAlnUGBPVuibO11728650;     SvYocCnjAlnUGBPVuibO11728650 = SvYocCnjAlnUGBPVuibO6296522;     SvYocCnjAlnUGBPVuibO6296522 = SvYocCnjAlnUGBPVuibO62752196;     SvYocCnjAlnUGBPVuibO62752196 = SvYocCnjAlnUGBPVuibO61149510;     SvYocCnjAlnUGBPVuibO61149510 = SvYocCnjAlnUGBPVuibO97926983;     SvYocCnjAlnUGBPVuibO97926983 = SvYocCnjAlnUGBPVuibO67737421;     SvYocCnjAlnUGBPVuibO67737421 = SvYocCnjAlnUGBPVuibO44350877;     SvYocCnjAlnUGBPVuibO44350877 = SvYocCnjAlnUGBPVuibO81226243;     SvYocCnjAlnUGBPVuibO81226243 = SvYocCnjAlnUGBPVuibO43728578;     SvYocCnjAlnUGBPVuibO43728578 = SvYocCnjAlnUGBPVuibO37546964;     SvYocCnjAlnUGBPVuibO37546964 = SvYocCnjAlnUGBPVuibO12576923;     SvYocCnjAlnUGBPVuibO12576923 = SvYocCnjAlnUGBPVuibO36897693;     SvYocCnjAlnUGBPVuibO36897693 = SvYocCnjAlnUGBPVuibO31629640;     SvYocCnjAlnUGBPVuibO31629640 = SvYocCnjAlnUGBPVuibO27685226;     SvYocCnjAlnUGBPVuibO27685226 = SvYocCnjAlnUGBPVuibO39845892;     SvYocCnjAlnUGBPVuibO39845892 = SvYocCnjAlnUGBPVuibO74602237;     SvYocCnjAlnUGBPVuibO74602237 = SvYocCnjAlnUGBPVuibO16090884;     SvYocCnjAlnUGBPVuibO16090884 = SvYocCnjAlnUGBPVuibO98962199;     SvYocCnjAlnUGBPVuibO98962199 = SvYocCnjAlnUGBPVuibO96606978;     SvYocCnjAlnUGBPVuibO96606978 = SvYocCnjAlnUGBPVuibO1619921;     SvYocCnjAlnUGBPVuibO1619921 = SvYocCnjAlnUGBPVuibO75680905;     SvYocCnjAlnUGBPVuibO75680905 = SvYocCnjAlnUGBPVuibO6280070;     SvYocCnjAlnUGBPVuibO6280070 = SvYocCnjAlnUGBPVuibO69300438;     SvYocCnjAlnUGBPVuibO69300438 = SvYocCnjAlnUGBPVuibO26497383;     SvYocCnjAlnUGBPVuibO26497383 = SvYocCnjAlnUGBPVuibO9992665;     SvYocCnjAlnUGBPVuibO9992665 = SvYocCnjAlnUGBPVuibO88527962;     SvYocCnjAlnUGBPVuibO88527962 = SvYocCnjAlnUGBPVuibO41967911;     SvYocCnjAlnUGBPVuibO41967911 = SvYocCnjAlnUGBPVuibO64660592;     SvYocCnjAlnUGBPVuibO64660592 = SvYocCnjAlnUGBPVuibO22803863;     SvYocCnjAlnUGBPVuibO22803863 = SvYocCnjAlnUGBPVuibO10306318;     SvYocCnjAlnUGBPVuibO10306318 = SvYocCnjAlnUGBPVuibO19166038;     SvYocCnjAlnUGBPVuibO19166038 = SvYocCnjAlnUGBPVuibO81167710;     SvYocCnjAlnUGBPVuibO81167710 = SvYocCnjAlnUGBPVuibO89598571;     SvYocCnjAlnUGBPVuibO89598571 = SvYocCnjAlnUGBPVuibO91999415;     SvYocCnjAlnUGBPVuibO91999415 = SvYocCnjAlnUGBPVuibO96696679;     SvYocCnjAlnUGBPVuibO96696679 = SvYocCnjAlnUGBPVuibO86583602;     SvYocCnjAlnUGBPVuibO86583602 = SvYocCnjAlnUGBPVuibO92254777;     SvYocCnjAlnUGBPVuibO92254777 = SvYocCnjAlnUGBPVuibO86696681;     SvYocCnjAlnUGBPVuibO86696681 = SvYocCnjAlnUGBPVuibO87718792;     SvYocCnjAlnUGBPVuibO87718792 = SvYocCnjAlnUGBPVuibO12389130;     SvYocCnjAlnUGBPVuibO12389130 = SvYocCnjAlnUGBPVuibO34959480;     SvYocCnjAlnUGBPVuibO34959480 = SvYocCnjAlnUGBPVuibO32977241;     SvYocCnjAlnUGBPVuibO32977241 = SvYocCnjAlnUGBPVuibO52791331;     SvYocCnjAlnUGBPVuibO52791331 = SvYocCnjAlnUGBPVuibO87471136;     SvYocCnjAlnUGBPVuibO87471136 = SvYocCnjAlnUGBPVuibO70403749;     SvYocCnjAlnUGBPVuibO70403749 = SvYocCnjAlnUGBPVuibO94904785;     SvYocCnjAlnUGBPVuibO94904785 = SvYocCnjAlnUGBPVuibO25179462;     SvYocCnjAlnUGBPVuibO25179462 = SvYocCnjAlnUGBPVuibO32813341;     SvYocCnjAlnUGBPVuibO32813341 = SvYocCnjAlnUGBPVuibO19990096;     SvYocCnjAlnUGBPVuibO19990096 = SvYocCnjAlnUGBPVuibO14176160;     SvYocCnjAlnUGBPVuibO14176160 = SvYocCnjAlnUGBPVuibO64712890;     SvYocCnjAlnUGBPVuibO64712890 = SvYocCnjAlnUGBPVuibO60354780;     SvYocCnjAlnUGBPVuibO60354780 = SvYocCnjAlnUGBPVuibO70492651;     SvYocCnjAlnUGBPVuibO70492651 = SvYocCnjAlnUGBPVuibO9795077;     SvYocCnjAlnUGBPVuibO9795077 = SvYocCnjAlnUGBPVuibO35943429;     SvYocCnjAlnUGBPVuibO35943429 = SvYocCnjAlnUGBPVuibO5448581;     SvYocCnjAlnUGBPVuibO5448581 = SvYocCnjAlnUGBPVuibO36996083;     SvYocCnjAlnUGBPVuibO36996083 = SvYocCnjAlnUGBPVuibO36254814;     SvYocCnjAlnUGBPVuibO36254814 = SvYocCnjAlnUGBPVuibO51156845;     SvYocCnjAlnUGBPVuibO51156845 = SvYocCnjAlnUGBPVuibO9399021;     SvYocCnjAlnUGBPVuibO9399021 = SvYocCnjAlnUGBPVuibO25769511;     SvYocCnjAlnUGBPVuibO25769511 = SvYocCnjAlnUGBPVuibO79690284;     SvYocCnjAlnUGBPVuibO79690284 = SvYocCnjAlnUGBPVuibO58422381;     SvYocCnjAlnUGBPVuibO58422381 = SvYocCnjAlnUGBPVuibO33422260;     SvYocCnjAlnUGBPVuibO33422260 = SvYocCnjAlnUGBPVuibO18380927;     SvYocCnjAlnUGBPVuibO18380927 = SvYocCnjAlnUGBPVuibO31409212;     SvYocCnjAlnUGBPVuibO31409212 = SvYocCnjAlnUGBPVuibO47299122;     SvYocCnjAlnUGBPVuibO47299122 = SvYocCnjAlnUGBPVuibO39630224;     SvYocCnjAlnUGBPVuibO39630224 = SvYocCnjAlnUGBPVuibO30988547;     SvYocCnjAlnUGBPVuibO30988547 = SvYocCnjAlnUGBPVuibO53262290;     SvYocCnjAlnUGBPVuibO53262290 = SvYocCnjAlnUGBPVuibO82347459;     SvYocCnjAlnUGBPVuibO82347459 = SvYocCnjAlnUGBPVuibO29394202;     SvYocCnjAlnUGBPVuibO29394202 = SvYocCnjAlnUGBPVuibO11243407;     SvYocCnjAlnUGBPVuibO11243407 = SvYocCnjAlnUGBPVuibO84217848;     SvYocCnjAlnUGBPVuibO84217848 = SvYocCnjAlnUGBPVuibO66660440;     SvYocCnjAlnUGBPVuibO66660440 = SvYocCnjAlnUGBPVuibO42703665;     SvYocCnjAlnUGBPVuibO42703665 = SvYocCnjAlnUGBPVuibO53488738;     SvYocCnjAlnUGBPVuibO53488738 = SvYocCnjAlnUGBPVuibO81829301;     SvYocCnjAlnUGBPVuibO81829301 = SvYocCnjAlnUGBPVuibO56093634;     SvYocCnjAlnUGBPVuibO56093634 = SvYocCnjAlnUGBPVuibO15087880;     SvYocCnjAlnUGBPVuibO15087880 = SvYocCnjAlnUGBPVuibO63348501;     SvYocCnjAlnUGBPVuibO63348501 = SvYocCnjAlnUGBPVuibO9154570;     SvYocCnjAlnUGBPVuibO9154570 = SvYocCnjAlnUGBPVuibO44670497;     SvYocCnjAlnUGBPVuibO44670497 = SvYocCnjAlnUGBPVuibO8627703;     SvYocCnjAlnUGBPVuibO8627703 = SvYocCnjAlnUGBPVuibO45593428;     SvYocCnjAlnUGBPVuibO45593428 = SvYocCnjAlnUGBPVuibO58811257;     SvYocCnjAlnUGBPVuibO58811257 = SvYocCnjAlnUGBPVuibO10675060;     SvYocCnjAlnUGBPVuibO10675060 = SvYocCnjAlnUGBPVuibO96519909;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xAvRsPqaMxGqICWiPWKw72855155() {     int EsLhmscVbDRwKMcrdqoh59950562 = -805934972;    int EsLhmscVbDRwKMcrdqoh50269680 = -143158759;    int EsLhmscVbDRwKMcrdqoh75570541 = -74345371;    int EsLhmscVbDRwKMcrdqoh37076456 = -468517016;    int EsLhmscVbDRwKMcrdqoh97923679 = -745723433;    int EsLhmscVbDRwKMcrdqoh38129795 = -450285035;    int EsLhmscVbDRwKMcrdqoh17266265 = -562809009;    int EsLhmscVbDRwKMcrdqoh41462225 = -365586503;    int EsLhmscVbDRwKMcrdqoh29786694 = -965967989;    int EsLhmscVbDRwKMcrdqoh93535008 = -759743063;    int EsLhmscVbDRwKMcrdqoh34704896 = -224782876;    int EsLhmscVbDRwKMcrdqoh88529685 = -874324452;    int EsLhmscVbDRwKMcrdqoh9792471 = -892705106;    int EsLhmscVbDRwKMcrdqoh45758556 = -109251216;    int EsLhmscVbDRwKMcrdqoh29630715 = -266123219;    int EsLhmscVbDRwKMcrdqoh62946881 = -467126700;    int EsLhmscVbDRwKMcrdqoh25006711 = -497265676;    int EsLhmscVbDRwKMcrdqoh42262967 = -525197818;    int EsLhmscVbDRwKMcrdqoh32613467 = -517051272;    int EsLhmscVbDRwKMcrdqoh39479088 = -874361669;    int EsLhmscVbDRwKMcrdqoh41673464 = -743486153;    int EsLhmscVbDRwKMcrdqoh6591776 = -240876758;    int EsLhmscVbDRwKMcrdqoh68742557 = 80249590;    int EsLhmscVbDRwKMcrdqoh9389322 = 32932422;    int EsLhmscVbDRwKMcrdqoh20332513 = -767774342;    int EsLhmscVbDRwKMcrdqoh73754955 = -471615035;    int EsLhmscVbDRwKMcrdqoh8494274 = -532993197;    int EsLhmscVbDRwKMcrdqoh52243643 = -848758699;    int EsLhmscVbDRwKMcrdqoh71907385 = -250307505;    int EsLhmscVbDRwKMcrdqoh55607659 = -265817702;    int EsLhmscVbDRwKMcrdqoh93889851 = -487821540;    int EsLhmscVbDRwKMcrdqoh27312657 = -633081095;    int EsLhmscVbDRwKMcrdqoh21606228 = -217825789;    int EsLhmscVbDRwKMcrdqoh71384827 = -880570865;    int EsLhmscVbDRwKMcrdqoh2607198 = -800630440;    int EsLhmscVbDRwKMcrdqoh45023207 = -124862059;    int EsLhmscVbDRwKMcrdqoh40546552 = 67462807;    int EsLhmscVbDRwKMcrdqoh49647745 = -218968473;    int EsLhmscVbDRwKMcrdqoh76341240 = -963975514;    int EsLhmscVbDRwKMcrdqoh47047563 = -116694311;    int EsLhmscVbDRwKMcrdqoh18878932 = -466732598;    int EsLhmscVbDRwKMcrdqoh32333313 = -709612717;    int EsLhmscVbDRwKMcrdqoh23707273 = 43002773;    int EsLhmscVbDRwKMcrdqoh98640483 = -798022444;    int EsLhmscVbDRwKMcrdqoh52962183 = -133484605;    int EsLhmscVbDRwKMcrdqoh84565732 = -427218625;    int EsLhmscVbDRwKMcrdqoh16302303 = -289276242;    int EsLhmscVbDRwKMcrdqoh27301780 = -428806964;    int EsLhmscVbDRwKMcrdqoh32924143 = -59979098;    int EsLhmscVbDRwKMcrdqoh45516381 = -241340848;    int EsLhmscVbDRwKMcrdqoh25333054 = -910314909;    int EsLhmscVbDRwKMcrdqoh5666362 = -784238979;    int EsLhmscVbDRwKMcrdqoh43239859 = -938138108;    int EsLhmscVbDRwKMcrdqoh23693975 = -36689738;    int EsLhmscVbDRwKMcrdqoh31181691 = -755244462;    int EsLhmscVbDRwKMcrdqoh53358786 = -465058214;    int EsLhmscVbDRwKMcrdqoh81527123 = -123408350;    int EsLhmscVbDRwKMcrdqoh66181220 = -7277794;    int EsLhmscVbDRwKMcrdqoh16743943 = -700742675;    int EsLhmscVbDRwKMcrdqoh24168724 = -174108399;    int EsLhmscVbDRwKMcrdqoh29635522 = -917291839;    int EsLhmscVbDRwKMcrdqoh65022621 = -714050310;    int EsLhmscVbDRwKMcrdqoh69554839 = -15278998;    int EsLhmscVbDRwKMcrdqoh74179035 = -600150288;    int EsLhmscVbDRwKMcrdqoh99645156 = -171921523;    int EsLhmscVbDRwKMcrdqoh7392239 = -591701782;    int EsLhmscVbDRwKMcrdqoh66923457 = -556498663;    int EsLhmscVbDRwKMcrdqoh38407644 = 87865759;    int EsLhmscVbDRwKMcrdqoh43151359 = -308620776;    int EsLhmscVbDRwKMcrdqoh84607507 = -41261160;    int EsLhmscVbDRwKMcrdqoh22400329 = -434589508;    int EsLhmscVbDRwKMcrdqoh75358966 = -178297203;    int EsLhmscVbDRwKMcrdqoh65921726 = -561222304;    int EsLhmscVbDRwKMcrdqoh85565904 = -300356961;    int EsLhmscVbDRwKMcrdqoh20600156 = -307629071;    int EsLhmscVbDRwKMcrdqoh9340152 = 66126564;    int EsLhmscVbDRwKMcrdqoh82884502 = -183879531;    int EsLhmscVbDRwKMcrdqoh70102073 = -121727967;    int EsLhmscVbDRwKMcrdqoh56427138 = -833582974;    int EsLhmscVbDRwKMcrdqoh35766781 = -240555717;    int EsLhmscVbDRwKMcrdqoh57452653 = -82338794;    int EsLhmscVbDRwKMcrdqoh81192494 = -4186233;    int EsLhmscVbDRwKMcrdqoh19319501 = -688779602;    int EsLhmscVbDRwKMcrdqoh26391005 = 91033342;    int EsLhmscVbDRwKMcrdqoh30274605 = -355502793;    int EsLhmscVbDRwKMcrdqoh88223490 = -703582562;    int EsLhmscVbDRwKMcrdqoh84072798 = -694942987;    int EsLhmscVbDRwKMcrdqoh97912253 = -81136052;    int EsLhmscVbDRwKMcrdqoh40203136 = -25326404;    int EsLhmscVbDRwKMcrdqoh49248411 = -235572226;    int EsLhmscVbDRwKMcrdqoh63496084 = 98546290;    int EsLhmscVbDRwKMcrdqoh74365332 = -925259400;    int EsLhmscVbDRwKMcrdqoh32903802 = -518225799;    int EsLhmscVbDRwKMcrdqoh52172517 = -689867116;    int EsLhmscVbDRwKMcrdqoh17412041 = -199402472;    int EsLhmscVbDRwKMcrdqoh53856311 = -752682289;    int EsLhmscVbDRwKMcrdqoh62778473 = -594333719;    int EsLhmscVbDRwKMcrdqoh49528238 = -356846940;    int EsLhmscVbDRwKMcrdqoh98995326 = -526100921;    int EsLhmscVbDRwKMcrdqoh45569944 = -805934972;     EsLhmscVbDRwKMcrdqoh59950562 = EsLhmscVbDRwKMcrdqoh50269680;     EsLhmscVbDRwKMcrdqoh50269680 = EsLhmscVbDRwKMcrdqoh75570541;     EsLhmscVbDRwKMcrdqoh75570541 = EsLhmscVbDRwKMcrdqoh37076456;     EsLhmscVbDRwKMcrdqoh37076456 = EsLhmscVbDRwKMcrdqoh97923679;     EsLhmscVbDRwKMcrdqoh97923679 = EsLhmscVbDRwKMcrdqoh38129795;     EsLhmscVbDRwKMcrdqoh38129795 = EsLhmscVbDRwKMcrdqoh17266265;     EsLhmscVbDRwKMcrdqoh17266265 = EsLhmscVbDRwKMcrdqoh41462225;     EsLhmscVbDRwKMcrdqoh41462225 = EsLhmscVbDRwKMcrdqoh29786694;     EsLhmscVbDRwKMcrdqoh29786694 = EsLhmscVbDRwKMcrdqoh93535008;     EsLhmscVbDRwKMcrdqoh93535008 = EsLhmscVbDRwKMcrdqoh34704896;     EsLhmscVbDRwKMcrdqoh34704896 = EsLhmscVbDRwKMcrdqoh88529685;     EsLhmscVbDRwKMcrdqoh88529685 = EsLhmscVbDRwKMcrdqoh9792471;     EsLhmscVbDRwKMcrdqoh9792471 = EsLhmscVbDRwKMcrdqoh45758556;     EsLhmscVbDRwKMcrdqoh45758556 = EsLhmscVbDRwKMcrdqoh29630715;     EsLhmscVbDRwKMcrdqoh29630715 = EsLhmscVbDRwKMcrdqoh62946881;     EsLhmscVbDRwKMcrdqoh62946881 = EsLhmscVbDRwKMcrdqoh25006711;     EsLhmscVbDRwKMcrdqoh25006711 = EsLhmscVbDRwKMcrdqoh42262967;     EsLhmscVbDRwKMcrdqoh42262967 = EsLhmscVbDRwKMcrdqoh32613467;     EsLhmscVbDRwKMcrdqoh32613467 = EsLhmscVbDRwKMcrdqoh39479088;     EsLhmscVbDRwKMcrdqoh39479088 = EsLhmscVbDRwKMcrdqoh41673464;     EsLhmscVbDRwKMcrdqoh41673464 = EsLhmscVbDRwKMcrdqoh6591776;     EsLhmscVbDRwKMcrdqoh6591776 = EsLhmscVbDRwKMcrdqoh68742557;     EsLhmscVbDRwKMcrdqoh68742557 = EsLhmscVbDRwKMcrdqoh9389322;     EsLhmscVbDRwKMcrdqoh9389322 = EsLhmscVbDRwKMcrdqoh20332513;     EsLhmscVbDRwKMcrdqoh20332513 = EsLhmscVbDRwKMcrdqoh73754955;     EsLhmscVbDRwKMcrdqoh73754955 = EsLhmscVbDRwKMcrdqoh8494274;     EsLhmscVbDRwKMcrdqoh8494274 = EsLhmscVbDRwKMcrdqoh52243643;     EsLhmscVbDRwKMcrdqoh52243643 = EsLhmscVbDRwKMcrdqoh71907385;     EsLhmscVbDRwKMcrdqoh71907385 = EsLhmscVbDRwKMcrdqoh55607659;     EsLhmscVbDRwKMcrdqoh55607659 = EsLhmscVbDRwKMcrdqoh93889851;     EsLhmscVbDRwKMcrdqoh93889851 = EsLhmscVbDRwKMcrdqoh27312657;     EsLhmscVbDRwKMcrdqoh27312657 = EsLhmscVbDRwKMcrdqoh21606228;     EsLhmscVbDRwKMcrdqoh21606228 = EsLhmscVbDRwKMcrdqoh71384827;     EsLhmscVbDRwKMcrdqoh71384827 = EsLhmscVbDRwKMcrdqoh2607198;     EsLhmscVbDRwKMcrdqoh2607198 = EsLhmscVbDRwKMcrdqoh45023207;     EsLhmscVbDRwKMcrdqoh45023207 = EsLhmscVbDRwKMcrdqoh40546552;     EsLhmscVbDRwKMcrdqoh40546552 = EsLhmscVbDRwKMcrdqoh49647745;     EsLhmscVbDRwKMcrdqoh49647745 = EsLhmscVbDRwKMcrdqoh76341240;     EsLhmscVbDRwKMcrdqoh76341240 = EsLhmscVbDRwKMcrdqoh47047563;     EsLhmscVbDRwKMcrdqoh47047563 = EsLhmscVbDRwKMcrdqoh18878932;     EsLhmscVbDRwKMcrdqoh18878932 = EsLhmscVbDRwKMcrdqoh32333313;     EsLhmscVbDRwKMcrdqoh32333313 = EsLhmscVbDRwKMcrdqoh23707273;     EsLhmscVbDRwKMcrdqoh23707273 = EsLhmscVbDRwKMcrdqoh98640483;     EsLhmscVbDRwKMcrdqoh98640483 = EsLhmscVbDRwKMcrdqoh52962183;     EsLhmscVbDRwKMcrdqoh52962183 = EsLhmscVbDRwKMcrdqoh84565732;     EsLhmscVbDRwKMcrdqoh84565732 = EsLhmscVbDRwKMcrdqoh16302303;     EsLhmscVbDRwKMcrdqoh16302303 = EsLhmscVbDRwKMcrdqoh27301780;     EsLhmscVbDRwKMcrdqoh27301780 = EsLhmscVbDRwKMcrdqoh32924143;     EsLhmscVbDRwKMcrdqoh32924143 = EsLhmscVbDRwKMcrdqoh45516381;     EsLhmscVbDRwKMcrdqoh45516381 = EsLhmscVbDRwKMcrdqoh25333054;     EsLhmscVbDRwKMcrdqoh25333054 = EsLhmscVbDRwKMcrdqoh5666362;     EsLhmscVbDRwKMcrdqoh5666362 = EsLhmscVbDRwKMcrdqoh43239859;     EsLhmscVbDRwKMcrdqoh43239859 = EsLhmscVbDRwKMcrdqoh23693975;     EsLhmscVbDRwKMcrdqoh23693975 = EsLhmscVbDRwKMcrdqoh31181691;     EsLhmscVbDRwKMcrdqoh31181691 = EsLhmscVbDRwKMcrdqoh53358786;     EsLhmscVbDRwKMcrdqoh53358786 = EsLhmscVbDRwKMcrdqoh81527123;     EsLhmscVbDRwKMcrdqoh81527123 = EsLhmscVbDRwKMcrdqoh66181220;     EsLhmscVbDRwKMcrdqoh66181220 = EsLhmscVbDRwKMcrdqoh16743943;     EsLhmscVbDRwKMcrdqoh16743943 = EsLhmscVbDRwKMcrdqoh24168724;     EsLhmscVbDRwKMcrdqoh24168724 = EsLhmscVbDRwKMcrdqoh29635522;     EsLhmscVbDRwKMcrdqoh29635522 = EsLhmscVbDRwKMcrdqoh65022621;     EsLhmscVbDRwKMcrdqoh65022621 = EsLhmscVbDRwKMcrdqoh69554839;     EsLhmscVbDRwKMcrdqoh69554839 = EsLhmscVbDRwKMcrdqoh74179035;     EsLhmscVbDRwKMcrdqoh74179035 = EsLhmscVbDRwKMcrdqoh99645156;     EsLhmscVbDRwKMcrdqoh99645156 = EsLhmscVbDRwKMcrdqoh7392239;     EsLhmscVbDRwKMcrdqoh7392239 = EsLhmscVbDRwKMcrdqoh66923457;     EsLhmscVbDRwKMcrdqoh66923457 = EsLhmscVbDRwKMcrdqoh38407644;     EsLhmscVbDRwKMcrdqoh38407644 = EsLhmscVbDRwKMcrdqoh43151359;     EsLhmscVbDRwKMcrdqoh43151359 = EsLhmscVbDRwKMcrdqoh84607507;     EsLhmscVbDRwKMcrdqoh84607507 = EsLhmscVbDRwKMcrdqoh22400329;     EsLhmscVbDRwKMcrdqoh22400329 = EsLhmscVbDRwKMcrdqoh75358966;     EsLhmscVbDRwKMcrdqoh75358966 = EsLhmscVbDRwKMcrdqoh65921726;     EsLhmscVbDRwKMcrdqoh65921726 = EsLhmscVbDRwKMcrdqoh85565904;     EsLhmscVbDRwKMcrdqoh85565904 = EsLhmscVbDRwKMcrdqoh20600156;     EsLhmscVbDRwKMcrdqoh20600156 = EsLhmscVbDRwKMcrdqoh9340152;     EsLhmscVbDRwKMcrdqoh9340152 = EsLhmscVbDRwKMcrdqoh82884502;     EsLhmscVbDRwKMcrdqoh82884502 = EsLhmscVbDRwKMcrdqoh70102073;     EsLhmscVbDRwKMcrdqoh70102073 = EsLhmscVbDRwKMcrdqoh56427138;     EsLhmscVbDRwKMcrdqoh56427138 = EsLhmscVbDRwKMcrdqoh35766781;     EsLhmscVbDRwKMcrdqoh35766781 = EsLhmscVbDRwKMcrdqoh57452653;     EsLhmscVbDRwKMcrdqoh57452653 = EsLhmscVbDRwKMcrdqoh81192494;     EsLhmscVbDRwKMcrdqoh81192494 = EsLhmscVbDRwKMcrdqoh19319501;     EsLhmscVbDRwKMcrdqoh19319501 = EsLhmscVbDRwKMcrdqoh26391005;     EsLhmscVbDRwKMcrdqoh26391005 = EsLhmscVbDRwKMcrdqoh30274605;     EsLhmscVbDRwKMcrdqoh30274605 = EsLhmscVbDRwKMcrdqoh88223490;     EsLhmscVbDRwKMcrdqoh88223490 = EsLhmscVbDRwKMcrdqoh84072798;     EsLhmscVbDRwKMcrdqoh84072798 = EsLhmscVbDRwKMcrdqoh97912253;     EsLhmscVbDRwKMcrdqoh97912253 = EsLhmscVbDRwKMcrdqoh40203136;     EsLhmscVbDRwKMcrdqoh40203136 = EsLhmscVbDRwKMcrdqoh49248411;     EsLhmscVbDRwKMcrdqoh49248411 = EsLhmscVbDRwKMcrdqoh63496084;     EsLhmscVbDRwKMcrdqoh63496084 = EsLhmscVbDRwKMcrdqoh74365332;     EsLhmscVbDRwKMcrdqoh74365332 = EsLhmscVbDRwKMcrdqoh32903802;     EsLhmscVbDRwKMcrdqoh32903802 = EsLhmscVbDRwKMcrdqoh52172517;     EsLhmscVbDRwKMcrdqoh52172517 = EsLhmscVbDRwKMcrdqoh17412041;     EsLhmscVbDRwKMcrdqoh17412041 = EsLhmscVbDRwKMcrdqoh53856311;     EsLhmscVbDRwKMcrdqoh53856311 = EsLhmscVbDRwKMcrdqoh62778473;     EsLhmscVbDRwKMcrdqoh62778473 = EsLhmscVbDRwKMcrdqoh49528238;     EsLhmscVbDRwKMcrdqoh49528238 = EsLhmscVbDRwKMcrdqoh98995326;     EsLhmscVbDRwKMcrdqoh98995326 = EsLhmscVbDRwKMcrdqoh45569944;     EsLhmscVbDRwKMcrdqoh45569944 = EsLhmscVbDRwKMcrdqoh59950562;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void fVWufvwISgNTqmkFZpZj31506826() {     int URPdthhESnLwQTchOTLC23381215 = -389708385;    int URPdthhESnLwQTchOTLC75521259 = 97066627;    int URPdthhESnLwQTchOTLC68160411 = -152419831;    int URPdthhESnLwQTchOTLC42350435 = -482496743;    int URPdthhESnLwQTchOTLC39038258 = -7446464;    int URPdthhESnLwQTchOTLC15761024 = -291508265;    int URPdthhESnLwQTchOTLC74696542 = -987135509;    int URPdthhESnLwQTchOTLC94146053 = -683988819;    int URPdthhESnLwQTchOTLC78769613 = -439897130;    int URPdthhESnLwQTchOTLC27753038 = -239589580;    int URPdthhESnLwQTchOTLC2310163 = -762625535;    int URPdthhESnLwQTchOTLC65644374 = -408641218;    int URPdthhESnLwQTchOTLC7960608 = -114350116;    int URPdthhESnLwQTchOTLC79788462 = -842164733;    int URPdthhESnLwQTchOTLC52964908 = -473288060;    int URPdthhESnLwQTchOTLC63141565 = 45673861;    int URPdthhESnLwQTchOTLC88863911 = -728537210;    int URPdthhESnLwQTchOTLC86598949 = -400511337;    int URPdthhESnLwQTchOTLC97489513 = -422608114;    int URPdthhESnLwQTchOTLC34607299 = -188948454;    int URPdthhESnLwQTchOTLC2120685 = -335995131;    int URPdthhESnLwQTchOTLC69454973 = 43765483;    int URPdthhESnLwQTchOTLC99938149 = -965138226;    int URPdthhESnLwQTchOTLC6201721 = -605693423;    int URPdthhESnLwQTchOTLC3767334 = -42383280;    int URPdthhESnLwQTchOTLC15880272 = -825003051;    int URPdthhESnLwQTchOTLC89303321 = -462745021;    int URPdthhESnLwQTchOTLC64641395 = -104073119;    int URPdthhESnLwQTchOTLC69212534 = -783379356;    int URPdthhESnLwQTchOTLC95124433 = 83146935;    int URPdthhESnLwQTchOTLC88817504 = -523384540;    int URPdthhESnLwQTchOTLC58018336 = -132279348;    int URPdthhESnLwQTchOTLC41592536 = -90917940;    int URPdthhESnLwQTchOTLC67088748 = -989217408;    int URPdthhESnLwQTchOTLC98934325 = -243712775;    int URPdthhESnLwQTchOTLC20745976 = -169040722;    int URPdthhESnLwQTchOTLC54595721 = -402219309;    int URPdthhESnLwQTchOTLC89302825 = -578359426;    int URPdthhESnLwQTchOTLC64154519 = -117773488;    int URPdthhESnLwQTchOTLC52127215 = -265786650;    int URPdthhESnLwQTchOTLC73097271 = -295625172;    int URPdthhESnLwQTchOTLC41862763 = -392805952;    int URPdthhESnLwQTchOTLC37108229 = -139298141;    int URPdthhESnLwQTchOTLC78114930 = -245681744;    int URPdthhESnLwQTchOTLC24756656 = -742285542;    int URPdthhESnLwQTchOTLC79532893 = 5432750;    int URPdthhESnLwQTchOTLC40605189 = -866642549;    int URPdthhESnLwQTchOTLC57906879 = -599918298;    int URPdthhESnLwQTchOTLC79264683 = 24249918;    int URPdthhESnLwQTchOTLC98777983 = -663309225;    int URPdthhESnLwQTchOTLC63969426 = -577999427;    int URPdthhESnLwQTchOTLC23613930 = -313266985;    int URPdthhESnLwQTchOTLC74090587 = -989298348;    int URPdthhESnLwQTchOTLC12428470 = -280078186;    int URPdthhESnLwQTchOTLC29386142 = -778891751;    int URPdthhESnLwQTchOTLC53926242 = -333473869;    int URPdthhESnLwQTchOTLC75583109 = 62204852;    int URPdthhESnLwQTchOTLC61958690 = -546726409;    int URPdthhESnLwQTchOTLC38583101 = -340113464;    int URPdthhESnLwQTchOTLC23157986 = -182443413;    int URPdthhESnLwQTchOTLC26457702 = -828763245;    int URPdthhESnLwQTchOTLC10055148 = -783062391;    int URPdthhESnLwQTchOTLC24933520 = -900609464;    int URPdthhESnLwQTchOTLC83645179 = -423044066;    int URPdthhESnLwQTchOTLC38935533 = -716205041;    int URPdthhESnLwQTchOTLC44291827 = -530346187;    int URPdthhESnLwQTchOTLC24051838 = -217723279;    int URPdthhESnLwQTchOTLC40871859 = -125132708;    int URPdthhESnLwQTchOTLC80854136 = -498451958;    int URPdthhESnLwQTchOTLC32218932 = -204247338;    int URPdthhESnLwQTchOTLC8545845 = -552106830;    int URPdthhESnLwQTchOTLC99561086 = -50177785;    int URPdthhESnLwQTchOTLC22444431 = -182737850;    int URPdthhESnLwQTchOTLC45362298 = -56821464;    int URPdthhESnLwQTchOTLC61510027 = -893323282;    int URPdthhESnLwQTchOTLC60257922 = -943189179;    int URPdthhESnLwQTchOTLC32346745 = -816936376;    int URPdthhESnLwQTchOTLC21823220 = -619456482;    int URPdthhESnLwQTchOTLC81445064 = -863407882;    int URPdthhESnLwQTchOTLC24234440 = 52183970;    int URPdthhESnLwQTchOTLC75275082 = -958360503;    int URPdthhESnLwQTchOTLC31396442 = -862826723;    int URPdthhESnLwQTchOTLC85376712 = -28323037;    int URPdthhESnLwQTchOTLC70434550 = -20070131;    int URPdthhESnLwQTchOTLC31155008 = -338853639;    int URPdthhESnLwQTchOTLC65203574 = -110117555;    int URPdthhESnLwQTchOTLC83927748 = -142981000;    int URPdthhESnLwQTchOTLC29164067 = -810839755;    int URPdthhESnLwQTchOTLC37702607 = -110325658;    int URPdthhESnLwQTchOTLC45008084 = -910238907;    int URPdthhESnLwQTchOTLC45162867 = -131245574;    int URPdthhESnLwQTchOTLC92637030 = -855492900;    int URPdthhESnLwQTchOTLC50719725 = -138245963;    int URPdthhESnLwQTchOTLC40996533 = -935330075;    int URPdthhESnLwQTchOTLC25669513 = -437023406;    int URPdthhESnLwQTchOTLC63042124 = -512562782;    int URPdthhESnLwQTchOTLC16929244 = -492196489;    int URPdthhESnLwQTchOTLC53463049 = -716254076;    int URPdthhESnLwQTchOTLC39179397 = -529476704;    int URPdthhESnLwQTchOTLC80464829 = -389708385;     URPdthhESnLwQTchOTLC23381215 = URPdthhESnLwQTchOTLC75521259;     URPdthhESnLwQTchOTLC75521259 = URPdthhESnLwQTchOTLC68160411;     URPdthhESnLwQTchOTLC68160411 = URPdthhESnLwQTchOTLC42350435;     URPdthhESnLwQTchOTLC42350435 = URPdthhESnLwQTchOTLC39038258;     URPdthhESnLwQTchOTLC39038258 = URPdthhESnLwQTchOTLC15761024;     URPdthhESnLwQTchOTLC15761024 = URPdthhESnLwQTchOTLC74696542;     URPdthhESnLwQTchOTLC74696542 = URPdthhESnLwQTchOTLC94146053;     URPdthhESnLwQTchOTLC94146053 = URPdthhESnLwQTchOTLC78769613;     URPdthhESnLwQTchOTLC78769613 = URPdthhESnLwQTchOTLC27753038;     URPdthhESnLwQTchOTLC27753038 = URPdthhESnLwQTchOTLC2310163;     URPdthhESnLwQTchOTLC2310163 = URPdthhESnLwQTchOTLC65644374;     URPdthhESnLwQTchOTLC65644374 = URPdthhESnLwQTchOTLC7960608;     URPdthhESnLwQTchOTLC7960608 = URPdthhESnLwQTchOTLC79788462;     URPdthhESnLwQTchOTLC79788462 = URPdthhESnLwQTchOTLC52964908;     URPdthhESnLwQTchOTLC52964908 = URPdthhESnLwQTchOTLC63141565;     URPdthhESnLwQTchOTLC63141565 = URPdthhESnLwQTchOTLC88863911;     URPdthhESnLwQTchOTLC88863911 = URPdthhESnLwQTchOTLC86598949;     URPdthhESnLwQTchOTLC86598949 = URPdthhESnLwQTchOTLC97489513;     URPdthhESnLwQTchOTLC97489513 = URPdthhESnLwQTchOTLC34607299;     URPdthhESnLwQTchOTLC34607299 = URPdthhESnLwQTchOTLC2120685;     URPdthhESnLwQTchOTLC2120685 = URPdthhESnLwQTchOTLC69454973;     URPdthhESnLwQTchOTLC69454973 = URPdthhESnLwQTchOTLC99938149;     URPdthhESnLwQTchOTLC99938149 = URPdthhESnLwQTchOTLC6201721;     URPdthhESnLwQTchOTLC6201721 = URPdthhESnLwQTchOTLC3767334;     URPdthhESnLwQTchOTLC3767334 = URPdthhESnLwQTchOTLC15880272;     URPdthhESnLwQTchOTLC15880272 = URPdthhESnLwQTchOTLC89303321;     URPdthhESnLwQTchOTLC89303321 = URPdthhESnLwQTchOTLC64641395;     URPdthhESnLwQTchOTLC64641395 = URPdthhESnLwQTchOTLC69212534;     URPdthhESnLwQTchOTLC69212534 = URPdthhESnLwQTchOTLC95124433;     URPdthhESnLwQTchOTLC95124433 = URPdthhESnLwQTchOTLC88817504;     URPdthhESnLwQTchOTLC88817504 = URPdthhESnLwQTchOTLC58018336;     URPdthhESnLwQTchOTLC58018336 = URPdthhESnLwQTchOTLC41592536;     URPdthhESnLwQTchOTLC41592536 = URPdthhESnLwQTchOTLC67088748;     URPdthhESnLwQTchOTLC67088748 = URPdthhESnLwQTchOTLC98934325;     URPdthhESnLwQTchOTLC98934325 = URPdthhESnLwQTchOTLC20745976;     URPdthhESnLwQTchOTLC20745976 = URPdthhESnLwQTchOTLC54595721;     URPdthhESnLwQTchOTLC54595721 = URPdthhESnLwQTchOTLC89302825;     URPdthhESnLwQTchOTLC89302825 = URPdthhESnLwQTchOTLC64154519;     URPdthhESnLwQTchOTLC64154519 = URPdthhESnLwQTchOTLC52127215;     URPdthhESnLwQTchOTLC52127215 = URPdthhESnLwQTchOTLC73097271;     URPdthhESnLwQTchOTLC73097271 = URPdthhESnLwQTchOTLC41862763;     URPdthhESnLwQTchOTLC41862763 = URPdthhESnLwQTchOTLC37108229;     URPdthhESnLwQTchOTLC37108229 = URPdthhESnLwQTchOTLC78114930;     URPdthhESnLwQTchOTLC78114930 = URPdthhESnLwQTchOTLC24756656;     URPdthhESnLwQTchOTLC24756656 = URPdthhESnLwQTchOTLC79532893;     URPdthhESnLwQTchOTLC79532893 = URPdthhESnLwQTchOTLC40605189;     URPdthhESnLwQTchOTLC40605189 = URPdthhESnLwQTchOTLC57906879;     URPdthhESnLwQTchOTLC57906879 = URPdthhESnLwQTchOTLC79264683;     URPdthhESnLwQTchOTLC79264683 = URPdthhESnLwQTchOTLC98777983;     URPdthhESnLwQTchOTLC98777983 = URPdthhESnLwQTchOTLC63969426;     URPdthhESnLwQTchOTLC63969426 = URPdthhESnLwQTchOTLC23613930;     URPdthhESnLwQTchOTLC23613930 = URPdthhESnLwQTchOTLC74090587;     URPdthhESnLwQTchOTLC74090587 = URPdthhESnLwQTchOTLC12428470;     URPdthhESnLwQTchOTLC12428470 = URPdthhESnLwQTchOTLC29386142;     URPdthhESnLwQTchOTLC29386142 = URPdthhESnLwQTchOTLC53926242;     URPdthhESnLwQTchOTLC53926242 = URPdthhESnLwQTchOTLC75583109;     URPdthhESnLwQTchOTLC75583109 = URPdthhESnLwQTchOTLC61958690;     URPdthhESnLwQTchOTLC61958690 = URPdthhESnLwQTchOTLC38583101;     URPdthhESnLwQTchOTLC38583101 = URPdthhESnLwQTchOTLC23157986;     URPdthhESnLwQTchOTLC23157986 = URPdthhESnLwQTchOTLC26457702;     URPdthhESnLwQTchOTLC26457702 = URPdthhESnLwQTchOTLC10055148;     URPdthhESnLwQTchOTLC10055148 = URPdthhESnLwQTchOTLC24933520;     URPdthhESnLwQTchOTLC24933520 = URPdthhESnLwQTchOTLC83645179;     URPdthhESnLwQTchOTLC83645179 = URPdthhESnLwQTchOTLC38935533;     URPdthhESnLwQTchOTLC38935533 = URPdthhESnLwQTchOTLC44291827;     URPdthhESnLwQTchOTLC44291827 = URPdthhESnLwQTchOTLC24051838;     URPdthhESnLwQTchOTLC24051838 = URPdthhESnLwQTchOTLC40871859;     URPdthhESnLwQTchOTLC40871859 = URPdthhESnLwQTchOTLC80854136;     URPdthhESnLwQTchOTLC80854136 = URPdthhESnLwQTchOTLC32218932;     URPdthhESnLwQTchOTLC32218932 = URPdthhESnLwQTchOTLC8545845;     URPdthhESnLwQTchOTLC8545845 = URPdthhESnLwQTchOTLC99561086;     URPdthhESnLwQTchOTLC99561086 = URPdthhESnLwQTchOTLC22444431;     URPdthhESnLwQTchOTLC22444431 = URPdthhESnLwQTchOTLC45362298;     URPdthhESnLwQTchOTLC45362298 = URPdthhESnLwQTchOTLC61510027;     URPdthhESnLwQTchOTLC61510027 = URPdthhESnLwQTchOTLC60257922;     URPdthhESnLwQTchOTLC60257922 = URPdthhESnLwQTchOTLC32346745;     URPdthhESnLwQTchOTLC32346745 = URPdthhESnLwQTchOTLC21823220;     URPdthhESnLwQTchOTLC21823220 = URPdthhESnLwQTchOTLC81445064;     URPdthhESnLwQTchOTLC81445064 = URPdthhESnLwQTchOTLC24234440;     URPdthhESnLwQTchOTLC24234440 = URPdthhESnLwQTchOTLC75275082;     URPdthhESnLwQTchOTLC75275082 = URPdthhESnLwQTchOTLC31396442;     URPdthhESnLwQTchOTLC31396442 = URPdthhESnLwQTchOTLC85376712;     URPdthhESnLwQTchOTLC85376712 = URPdthhESnLwQTchOTLC70434550;     URPdthhESnLwQTchOTLC70434550 = URPdthhESnLwQTchOTLC31155008;     URPdthhESnLwQTchOTLC31155008 = URPdthhESnLwQTchOTLC65203574;     URPdthhESnLwQTchOTLC65203574 = URPdthhESnLwQTchOTLC83927748;     URPdthhESnLwQTchOTLC83927748 = URPdthhESnLwQTchOTLC29164067;     URPdthhESnLwQTchOTLC29164067 = URPdthhESnLwQTchOTLC37702607;     URPdthhESnLwQTchOTLC37702607 = URPdthhESnLwQTchOTLC45008084;     URPdthhESnLwQTchOTLC45008084 = URPdthhESnLwQTchOTLC45162867;     URPdthhESnLwQTchOTLC45162867 = URPdthhESnLwQTchOTLC92637030;     URPdthhESnLwQTchOTLC92637030 = URPdthhESnLwQTchOTLC50719725;     URPdthhESnLwQTchOTLC50719725 = URPdthhESnLwQTchOTLC40996533;     URPdthhESnLwQTchOTLC40996533 = URPdthhESnLwQTchOTLC25669513;     URPdthhESnLwQTchOTLC25669513 = URPdthhESnLwQTchOTLC63042124;     URPdthhESnLwQTchOTLC63042124 = URPdthhESnLwQTchOTLC16929244;     URPdthhESnLwQTchOTLC16929244 = URPdthhESnLwQTchOTLC53463049;     URPdthhESnLwQTchOTLC53463049 = URPdthhESnLwQTchOTLC39179397;     URPdthhESnLwQTchOTLC39179397 = URPdthhESnLwQTchOTLC80464829;     URPdthhESnLwQTchOTLC80464829 = URPdthhESnLwQTchOTLC23381215;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void FbZuhyRStpSuFOXYSCZr37915966() {     int awRTgPpGjEOKhSlcGCov57470005 = -719106787;    int awRTgPpGjEOKhSlcGCov30874040 = -591756161;    int awRTgPpGjEOKhSlcGCov39405989 = -715660137;    int awRTgPpGjEOKhSlcGCov2475839 = -866074028;    int awRTgPpGjEOKhSlcGCov92763054 = -784220521;    int awRTgPpGjEOKhSlcGCov48586582 = -134879834;    int awRTgPpGjEOKhSlcGCov81958379 = -605701253;    int awRTgPpGjEOKhSlcGCov34458211 = -545030908;    int awRTgPpGjEOKhSlcGCov90877423 = -293527530;    int awRTgPpGjEOKhSlcGCov88298014 = -383724578;    int awRTgPpGjEOKhSlcGCov91843985 = -275898050;    int awRTgPpGjEOKhSlcGCov74013956 = -523243415;    int awRTgPpGjEOKhSlcGCov54103218 = -323873710;    int awRTgPpGjEOKhSlcGCov43821182 = 69046592;    int awRTgPpGjEOKhSlcGCov13847012 = -867490052;    int awRTgPpGjEOKhSlcGCov21300343 = -878713513;    int awRTgPpGjEOKhSlcGCov90721180 = 50620695;    int awRTgPpGjEOKhSlcGCov28227894 = -967919432;    int awRTgPpGjEOKhSlcGCov23645071 = -417370631;    int awRTgPpGjEOKhSlcGCov16448456 = -533877092;    int awRTgPpGjEOKhSlcGCov38034235 = -126716876;    int awRTgPpGjEOKhSlcGCov15417670 = -592885698;    int awRTgPpGjEOKhSlcGCov96199155 = 80588656;    int awRTgPpGjEOKhSlcGCov93213548 = -955799651;    int awRTgPpGjEOKhSlcGCov22883853 = -766203052;    int awRTgPpGjEOKhSlcGCov72029091 = -268469584;    int awRTgPpGjEOKhSlcGCov8151573 = -486874907;    int awRTgPpGjEOKhSlcGCov49417847 = -355544317;    int awRTgPpGjEOKhSlcGCov85981723 = -964739108;    int awRTgPpGjEOKhSlcGCov97723434 = -450852125;    int awRTgPpGjEOKhSlcGCov57404475 = 79712009;    int awRTgPpGjEOKhSlcGCov48511618 = -667953003;    int awRTgPpGjEOKhSlcGCov59071782 = -809027484;    int awRTgPpGjEOKhSlcGCov27874808 = -784711903;    int awRTgPpGjEOKhSlcGCov90927652 = -374427964;    int awRTgPpGjEOKhSlcGCov13633815 = -212680186;    int awRTgPpGjEOKhSlcGCov495009 = -960841253;    int awRTgPpGjEOKhSlcGCov11832715 = -726600807;    int awRTgPpGjEOKhSlcGCov95187488 = -714618681;    int awRTgPpGjEOKhSlcGCov80437272 = -323055666;    int awRTgPpGjEOKhSlcGCov99533227 = -420592727;    int awRTgPpGjEOKhSlcGCov94177542 = -751596174;    int awRTgPpGjEOKhSlcGCov39192784 = -849222853;    int awRTgPpGjEOKhSlcGCov4865779 = -208351572;    int awRTgPpGjEOKhSlcGCov8487136 = -86473472;    int awRTgPpGjEOKhSlcGCov40477921 = -608208359;    int awRTgPpGjEOKhSlcGCov63341486 = 21201967;    int awRTgPpGjEOKhSlcGCov78742608 = -614398570;    int awRTgPpGjEOKhSlcGCov12388286 = -313521175;    int awRTgPpGjEOKhSlcGCov40421650 = -681261594;    int awRTgPpGjEOKhSlcGCov57535753 = -213701059;    int awRTgPpGjEOKhSlcGCov68885746 = -529368965;    int awRTgPpGjEOKhSlcGCov84568625 = 23797378;    int awRTgPpGjEOKhSlcGCov81139998 = -652428279;    int awRTgPpGjEOKhSlcGCov99881730 = -56580676;    int awRTgPpGjEOKhSlcGCov42052336 = -26221090;    int awRTgPpGjEOKhSlcGCov34674885 = -572344818;    int awRTgPpGjEOKhSlcGCov46192440 = -759860486;    int awRTgPpGjEOKhSlcGCov79591985 = 129023;    int awRTgPpGjEOKhSlcGCov20733964 = -415750937;    int awRTgPpGjEOKhSlcGCov40435009 = -648004928;    int awRTgPpGjEOKhSlcGCov32540533 = -150156936;    int awRTgPpGjEOKhSlcGCov48476488 = -580291800;    int awRTgPpGjEOKhSlcGCov93153988 = -842675405;    int awRTgPpGjEOKhSlcGCov30893539 = -363436587;    int awRTgPpGjEOKhSlcGCov43332368 = -607945048;    int awRTgPpGjEOKhSlcGCov14942175 = -714215932;    int awRTgPpGjEOKhSlcGCov26228410 = -539161807;    int awRTgPpGjEOKhSlcGCov52893530 = -556525445;    int awRTgPpGjEOKhSlcGCov213198 = -554809866;    int awRTgPpGjEOKhSlcGCov20805334 = -917872260;    int awRTgPpGjEOKhSlcGCov78888465 = -222778498;    int awRTgPpGjEOKhSlcGCov33040405 = -153300751;    int awRTgPpGjEOKhSlcGCov43207799 = 5685034;    int awRTgPpGjEOKhSlcGCov16915229 = -13284366;    int awRTgPpGjEOKhSlcGCov43856692 = -375120702;    int awRTgPpGjEOKhSlcGCov76224886 = -743662845;    int awRTgPpGjEOKhSlcGCov91333377 = -711059772;    int awRTgPpGjEOKhSlcGCov84726413 = -769326180;    int awRTgPpGjEOKhSlcGCov82405931 = -57994693;    int awRTgPpGjEOKhSlcGCov8687605 = -189671551;    int awRTgPpGjEOKhSlcGCov29408965 = -872476337;    int awRTgPpGjEOKhSlcGCov37029561 = 57976857;    int awRTgPpGjEOKhSlcGCov45560073 = -183477514;    int awRTgPpGjEOKhSlcGCov40187682 = -137151067;    int awRTgPpGjEOKhSlcGCov88518729 = -390919026;    int awRTgPpGjEOKhSlcGCov63942992 = -591750381;    int awRTgPpGjEOKhSlcGCov77931783 = -56599205;    int awRTgPpGjEOKhSlcGCov27993078 = -628131228;    int awRTgPpGjEOKhSlcGCov48875316 = -248206875;    int awRTgPpGjEOKhSlcGCov78958929 = -640335369;    int awRTgPpGjEOKhSlcGCov54302569 = -100980768;    int awRTgPpGjEOKhSlcGCov32240729 = -626729831;    int awRTgPpGjEOKhSlcGCov74453525 = -198867744;    int awRTgPpGjEOKhSlcGCov40002263 = -675050738;    int awRTgPpGjEOKhSlcGCov66992694 = -170435791;    int awRTgPpGjEOKhSlcGCov45701055 = -71304375;    int awRTgPpGjEOKhSlcGCov46038795 = 93452552;    int awRTgPpGjEOKhSlcGCov73972239 = -844914986;    int awRTgPpGjEOKhSlcGCov65154768 = -719106787;     awRTgPpGjEOKhSlcGCov57470005 = awRTgPpGjEOKhSlcGCov30874040;     awRTgPpGjEOKhSlcGCov30874040 = awRTgPpGjEOKhSlcGCov39405989;     awRTgPpGjEOKhSlcGCov39405989 = awRTgPpGjEOKhSlcGCov2475839;     awRTgPpGjEOKhSlcGCov2475839 = awRTgPpGjEOKhSlcGCov92763054;     awRTgPpGjEOKhSlcGCov92763054 = awRTgPpGjEOKhSlcGCov48586582;     awRTgPpGjEOKhSlcGCov48586582 = awRTgPpGjEOKhSlcGCov81958379;     awRTgPpGjEOKhSlcGCov81958379 = awRTgPpGjEOKhSlcGCov34458211;     awRTgPpGjEOKhSlcGCov34458211 = awRTgPpGjEOKhSlcGCov90877423;     awRTgPpGjEOKhSlcGCov90877423 = awRTgPpGjEOKhSlcGCov88298014;     awRTgPpGjEOKhSlcGCov88298014 = awRTgPpGjEOKhSlcGCov91843985;     awRTgPpGjEOKhSlcGCov91843985 = awRTgPpGjEOKhSlcGCov74013956;     awRTgPpGjEOKhSlcGCov74013956 = awRTgPpGjEOKhSlcGCov54103218;     awRTgPpGjEOKhSlcGCov54103218 = awRTgPpGjEOKhSlcGCov43821182;     awRTgPpGjEOKhSlcGCov43821182 = awRTgPpGjEOKhSlcGCov13847012;     awRTgPpGjEOKhSlcGCov13847012 = awRTgPpGjEOKhSlcGCov21300343;     awRTgPpGjEOKhSlcGCov21300343 = awRTgPpGjEOKhSlcGCov90721180;     awRTgPpGjEOKhSlcGCov90721180 = awRTgPpGjEOKhSlcGCov28227894;     awRTgPpGjEOKhSlcGCov28227894 = awRTgPpGjEOKhSlcGCov23645071;     awRTgPpGjEOKhSlcGCov23645071 = awRTgPpGjEOKhSlcGCov16448456;     awRTgPpGjEOKhSlcGCov16448456 = awRTgPpGjEOKhSlcGCov38034235;     awRTgPpGjEOKhSlcGCov38034235 = awRTgPpGjEOKhSlcGCov15417670;     awRTgPpGjEOKhSlcGCov15417670 = awRTgPpGjEOKhSlcGCov96199155;     awRTgPpGjEOKhSlcGCov96199155 = awRTgPpGjEOKhSlcGCov93213548;     awRTgPpGjEOKhSlcGCov93213548 = awRTgPpGjEOKhSlcGCov22883853;     awRTgPpGjEOKhSlcGCov22883853 = awRTgPpGjEOKhSlcGCov72029091;     awRTgPpGjEOKhSlcGCov72029091 = awRTgPpGjEOKhSlcGCov8151573;     awRTgPpGjEOKhSlcGCov8151573 = awRTgPpGjEOKhSlcGCov49417847;     awRTgPpGjEOKhSlcGCov49417847 = awRTgPpGjEOKhSlcGCov85981723;     awRTgPpGjEOKhSlcGCov85981723 = awRTgPpGjEOKhSlcGCov97723434;     awRTgPpGjEOKhSlcGCov97723434 = awRTgPpGjEOKhSlcGCov57404475;     awRTgPpGjEOKhSlcGCov57404475 = awRTgPpGjEOKhSlcGCov48511618;     awRTgPpGjEOKhSlcGCov48511618 = awRTgPpGjEOKhSlcGCov59071782;     awRTgPpGjEOKhSlcGCov59071782 = awRTgPpGjEOKhSlcGCov27874808;     awRTgPpGjEOKhSlcGCov27874808 = awRTgPpGjEOKhSlcGCov90927652;     awRTgPpGjEOKhSlcGCov90927652 = awRTgPpGjEOKhSlcGCov13633815;     awRTgPpGjEOKhSlcGCov13633815 = awRTgPpGjEOKhSlcGCov495009;     awRTgPpGjEOKhSlcGCov495009 = awRTgPpGjEOKhSlcGCov11832715;     awRTgPpGjEOKhSlcGCov11832715 = awRTgPpGjEOKhSlcGCov95187488;     awRTgPpGjEOKhSlcGCov95187488 = awRTgPpGjEOKhSlcGCov80437272;     awRTgPpGjEOKhSlcGCov80437272 = awRTgPpGjEOKhSlcGCov99533227;     awRTgPpGjEOKhSlcGCov99533227 = awRTgPpGjEOKhSlcGCov94177542;     awRTgPpGjEOKhSlcGCov94177542 = awRTgPpGjEOKhSlcGCov39192784;     awRTgPpGjEOKhSlcGCov39192784 = awRTgPpGjEOKhSlcGCov4865779;     awRTgPpGjEOKhSlcGCov4865779 = awRTgPpGjEOKhSlcGCov8487136;     awRTgPpGjEOKhSlcGCov8487136 = awRTgPpGjEOKhSlcGCov40477921;     awRTgPpGjEOKhSlcGCov40477921 = awRTgPpGjEOKhSlcGCov63341486;     awRTgPpGjEOKhSlcGCov63341486 = awRTgPpGjEOKhSlcGCov78742608;     awRTgPpGjEOKhSlcGCov78742608 = awRTgPpGjEOKhSlcGCov12388286;     awRTgPpGjEOKhSlcGCov12388286 = awRTgPpGjEOKhSlcGCov40421650;     awRTgPpGjEOKhSlcGCov40421650 = awRTgPpGjEOKhSlcGCov57535753;     awRTgPpGjEOKhSlcGCov57535753 = awRTgPpGjEOKhSlcGCov68885746;     awRTgPpGjEOKhSlcGCov68885746 = awRTgPpGjEOKhSlcGCov84568625;     awRTgPpGjEOKhSlcGCov84568625 = awRTgPpGjEOKhSlcGCov81139998;     awRTgPpGjEOKhSlcGCov81139998 = awRTgPpGjEOKhSlcGCov99881730;     awRTgPpGjEOKhSlcGCov99881730 = awRTgPpGjEOKhSlcGCov42052336;     awRTgPpGjEOKhSlcGCov42052336 = awRTgPpGjEOKhSlcGCov34674885;     awRTgPpGjEOKhSlcGCov34674885 = awRTgPpGjEOKhSlcGCov46192440;     awRTgPpGjEOKhSlcGCov46192440 = awRTgPpGjEOKhSlcGCov79591985;     awRTgPpGjEOKhSlcGCov79591985 = awRTgPpGjEOKhSlcGCov20733964;     awRTgPpGjEOKhSlcGCov20733964 = awRTgPpGjEOKhSlcGCov40435009;     awRTgPpGjEOKhSlcGCov40435009 = awRTgPpGjEOKhSlcGCov32540533;     awRTgPpGjEOKhSlcGCov32540533 = awRTgPpGjEOKhSlcGCov48476488;     awRTgPpGjEOKhSlcGCov48476488 = awRTgPpGjEOKhSlcGCov93153988;     awRTgPpGjEOKhSlcGCov93153988 = awRTgPpGjEOKhSlcGCov30893539;     awRTgPpGjEOKhSlcGCov30893539 = awRTgPpGjEOKhSlcGCov43332368;     awRTgPpGjEOKhSlcGCov43332368 = awRTgPpGjEOKhSlcGCov14942175;     awRTgPpGjEOKhSlcGCov14942175 = awRTgPpGjEOKhSlcGCov26228410;     awRTgPpGjEOKhSlcGCov26228410 = awRTgPpGjEOKhSlcGCov52893530;     awRTgPpGjEOKhSlcGCov52893530 = awRTgPpGjEOKhSlcGCov213198;     awRTgPpGjEOKhSlcGCov213198 = awRTgPpGjEOKhSlcGCov20805334;     awRTgPpGjEOKhSlcGCov20805334 = awRTgPpGjEOKhSlcGCov78888465;     awRTgPpGjEOKhSlcGCov78888465 = awRTgPpGjEOKhSlcGCov33040405;     awRTgPpGjEOKhSlcGCov33040405 = awRTgPpGjEOKhSlcGCov43207799;     awRTgPpGjEOKhSlcGCov43207799 = awRTgPpGjEOKhSlcGCov16915229;     awRTgPpGjEOKhSlcGCov16915229 = awRTgPpGjEOKhSlcGCov43856692;     awRTgPpGjEOKhSlcGCov43856692 = awRTgPpGjEOKhSlcGCov76224886;     awRTgPpGjEOKhSlcGCov76224886 = awRTgPpGjEOKhSlcGCov91333377;     awRTgPpGjEOKhSlcGCov91333377 = awRTgPpGjEOKhSlcGCov84726413;     awRTgPpGjEOKhSlcGCov84726413 = awRTgPpGjEOKhSlcGCov82405931;     awRTgPpGjEOKhSlcGCov82405931 = awRTgPpGjEOKhSlcGCov8687605;     awRTgPpGjEOKhSlcGCov8687605 = awRTgPpGjEOKhSlcGCov29408965;     awRTgPpGjEOKhSlcGCov29408965 = awRTgPpGjEOKhSlcGCov37029561;     awRTgPpGjEOKhSlcGCov37029561 = awRTgPpGjEOKhSlcGCov45560073;     awRTgPpGjEOKhSlcGCov45560073 = awRTgPpGjEOKhSlcGCov40187682;     awRTgPpGjEOKhSlcGCov40187682 = awRTgPpGjEOKhSlcGCov88518729;     awRTgPpGjEOKhSlcGCov88518729 = awRTgPpGjEOKhSlcGCov63942992;     awRTgPpGjEOKhSlcGCov63942992 = awRTgPpGjEOKhSlcGCov77931783;     awRTgPpGjEOKhSlcGCov77931783 = awRTgPpGjEOKhSlcGCov27993078;     awRTgPpGjEOKhSlcGCov27993078 = awRTgPpGjEOKhSlcGCov48875316;     awRTgPpGjEOKhSlcGCov48875316 = awRTgPpGjEOKhSlcGCov78958929;     awRTgPpGjEOKhSlcGCov78958929 = awRTgPpGjEOKhSlcGCov54302569;     awRTgPpGjEOKhSlcGCov54302569 = awRTgPpGjEOKhSlcGCov32240729;     awRTgPpGjEOKhSlcGCov32240729 = awRTgPpGjEOKhSlcGCov74453525;     awRTgPpGjEOKhSlcGCov74453525 = awRTgPpGjEOKhSlcGCov40002263;     awRTgPpGjEOKhSlcGCov40002263 = awRTgPpGjEOKhSlcGCov66992694;     awRTgPpGjEOKhSlcGCov66992694 = awRTgPpGjEOKhSlcGCov45701055;     awRTgPpGjEOKhSlcGCov45701055 = awRTgPpGjEOKhSlcGCov46038795;     awRTgPpGjEOKhSlcGCov46038795 = awRTgPpGjEOKhSlcGCov73972239;     awRTgPpGjEOKhSlcGCov73972239 = awRTgPpGjEOKhSlcGCov65154768;     awRTgPpGjEOKhSlcGCov65154768 = awRTgPpGjEOKhSlcGCov57470005;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void CXCpzktautpxDpjsBAyu96567636() {     int gXwcJztZhioSxSYstntS20900658 = -302880200;    int gXwcJztZhioSxSYstntS56125619 = -351530776;    int gXwcJztZhioSxSYstntS31995859 = -793734597;    int gXwcJztZhioSxSYstntS7749817 = -880053755;    int gXwcJztZhioSxSYstntS33877633 = -45943552;    int gXwcJztZhioSxSYstntS26217810 = 23896936;    int gXwcJztZhioSxSYstntS39388657 = 69972247;    int gXwcJztZhioSxSYstntS87142038 = -863433223;    int gXwcJztZhioSxSYstntS39860343 = -867456671;    int gXwcJztZhioSxSYstntS22516044 = -963571095;    int gXwcJztZhioSxSYstntS59449253 = -813740709;    int gXwcJztZhioSxSYstntS51128645 = -57560182;    int gXwcJztZhioSxSYstntS52271355 = -645518721;    int gXwcJztZhioSxSYstntS77851088 = -663866925;    int gXwcJztZhioSxSYstntS37181206 = 25345107;    int gXwcJztZhioSxSYstntS21495027 = -365912951;    int gXwcJztZhioSxSYstntS54578381 = -180650839;    int gXwcJztZhioSxSYstntS72563877 = -843232951;    int gXwcJztZhioSxSYstntS88521116 = -322927473;    int gXwcJztZhioSxSYstntS11576667 = -948463877;    int gXwcJztZhioSxSYstntS98481455 = -819225855;    int gXwcJztZhioSxSYstntS78280867 = -308243456;    int gXwcJztZhioSxSYstntS27394748 = -964799160;    int gXwcJztZhioSxSYstntS90025948 = -494425496;    int gXwcJztZhioSxSYstntS6318674 = -40811990;    int gXwcJztZhioSxSYstntS14154407 = -621857600;    int gXwcJztZhioSxSYstntS88960620 = -416626731;    int gXwcJztZhioSxSYstntS61815598 = -710858736;    int gXwcJztZhioSxSYstntS83286871 = -397810958;    int gXwcJztZhioSxSYstntS37240210 = -101887489;    int gXwcJztZhioSxSYstntS52332128 = 44149009;    int gXwcJztZhioSxSYstntS79217297 = -167151256;    int gXwcJztZhioSxSYstntS79058090 = -682119635;    int gXwcJztZhioSxSYstntS23578730 = -893358447;    int gXwcJztZhioSxSYstntS87254780 = -917510299;    int gXwcJztZhioSxSYstntS89356583 = -256858848;    int gXwcJztZhioSxSYstntS14544178 = -330523369;    int gXwcJztZhioSxSYstntS51487795 = 14008240;    int gXwcJztZhioSxSYstntS83000766 = -968416654;    int gXwcJztZhioSxSYstntS85516924 = -472148005;    int gXwcJztZhioSxSYstntS53751567 = -249485301;    int gXwcJztZhioSxSYstntS3706993 = -434789410;    int gXwcJztZhioSxSYstntS52593739 = 68476233;    int gXwcJztZhioSxSYstntS84340224 = -756010873;    int gXwcJztZhioSxSYstntS80281608 = -695274409;    int gXwcJztZhioSxSYstntS35445083 = -175556984;    int gXwcJztZhioSxSYstntS87644372 = -556164341;    int gXwcJztZhioSxSYstntS9347709 = -785509904;    int gXwcJztZhioSxSYstntS58728826 = -229292159;    int gXwcJztZhioSxSYstntS93683253 = -3229971;    int gXwcJztZhioSxSYstntS96172125 = -981385577;    int gXwcJztZhioSxSYstntS86833314 = -58396972;    int gXwcJztZhioSxSYstntS15419355 = -27362862;    int gXwcJztZhioSxSYstntS69874493 = -895816727;    int gXwcJztZhioSxSYstntS98086181 = -80227964;    int gXwcJztZhioSxSYstntS42619791 = -994636744;    int gXwcJztZhioSxSYstntS28730871 = -386731616;    int gXwcJztZhioSxSYstntS41969911 = -199309101;    int gXwcJztZhioSxSYstntS1431144 = -739241765;    int gXwcJztZhioSxSYstntS19723226 = -424085952;    int gXwcJztZhioSxSYstntS37257190 = -559476334;    int gXwcJztZhioSxSYstntS77573059 = -219169017;    int gXwcJztZhioSxSYstntS3855168 = -365622266;    int gXwcJztZhioSxSYstntS2620133 = -665569183;    int gXwcJztZhioSxSYstntS70183915 = -907720105;    int gXwcJztZhioSxSYstntS80231955 = -546589454;    int gXwcJztZhioSxSYstntS72070555 = -375440548;    int gXwcJztZhioSxSYstntS28692625 = -752160275;    int gXwcJztZhioSxSYstntS90596308 = -746356627;    int gXwcJztZhioSxSYstntS47824622 = -717796045;    int gXwcJztZhioSxSYstntS6950849 = 64610417;    int gXwcJztZhioSxSYstntS3090586 = -94659079;    int gXwcJztZhioSxSYstntS89563110 = -874816298;    int gXwcJztZhioSxSYstntS3004193 = -850779468;    int gXwcJztZhioSxSYstntS57825100 = -598978577;    int gXwcJztZhioSxSYstntS94774462 = -284436446;    int gXwcJztZhioSxSYstntS25687129 = -276719690;    int gXwcJztZhioSxSYstntS43054524 = -108788288;    int gXwcJztZhioSxSYstntS9744340 = -799151088;    int gXwcJztZhioSxSYstntS70873591 = -865255006;    int gXwcJztZhioSxSYstntS26510035 = 34306740;    int gXwcJztZhioSxSYstntS79612912 = -631116827;    int gXwcJztZhioSxSYstntS3086772 = -381566578;    int gXwcJztZhioSxSYstntS89603617 = -294580987;    int gXwcJztZhioSxSYstntS41068085 = -120501912;    int gXwcJztZhioSxSYstntS65498813 = -897454020;    int gXwcJztZhioSxSYstntS63797942 = -39788394;    int gXwcJztZhioSxSYstntS9183597 = -786302908;    int gXwcJztZhioSxSYstntS25492549 = -713130483;    int gXwcJztZhioSxSYstntS44634989 = -922873555;    int gXwcJztZhioSxSYstntS60625712 = -870127233;    int gXwcJztZhioSxSYstntS72574267 = -31214268;    int gXwcJztZhioSxSYstntS50056652 = -246749996;    int gXwcJztZhioSxSYstntS63277541 = -444330703;    int gXwcJztZhioSxSYstntS48259734 = -912671671;    int gXwcJztZhioSxSYstntS76178507 = 69683716;    int gXwcJztZhioSxSYstntS99851825 = 30832856;    int gXwcJztZhioSxSYstntS49973606 = -265954584;    int gXwcJztZhioSxSYstntS14156309 = -848290768;    int gXwcJztZhioSxSYstntS49653 = -302880200;     gXwcJztZhioSxSYstntS20900658 = gXwcJztZhioSxSYstntS56125619;     gXwcJztZhioSxSYstntS56125619 = gXwcJztZhioSxSYstntS31995859;     gXwcJztZhioSxSYstntS31995859 = gXwcJztZhioSxSYstntS7749817;     gXwcJztZhioSxSYstntS7749817 = gXwcJztZhioSxSYstntS33877633;     gXwcJztZhioSxSYstntS33877633 = gXwcJztZhioSxSYstntS26217810;     gXwcJztZhioSxSYstntS26217810 = gXwcJztZhioSxSYstntS39388657;     gXwcJztZhioSxSYstntS39388657 = gXwcJztZhioSxSYstntS87142038;     gXwcJztZhioSxSYstntS87142038 = gXwcJztZhioSxSYstntS39860343;     gXwcJztZhioSxSYstntS39860343 = gXwcJztZhioSxSYstntS22516044;     gXwcJztZhioSxSYstntS22516044 = gXwcJztZhioSxSYstntS59449253;     gXwcJztZhioSxSYstntS59449253 = gXwcJztZhioSxSYstntS51128645;     gXwcJztZhioSxSYstntS51128645 = gXwcJztZhioSxSYstntS52271355;     gXwcJztZhioSxSYstntS52271355 = gXwcJztZhioSxSYstntS77851088;     gXwcJztZhioSxSYstntS77851088 = gXwcJztZhioSxSYstntS37181206;     gXwcJztZhioSxSYstntS37181206 = gXwcJztZhioSxSYstntS21495027;     gXwcJztZhioSxSYstntS21495027 = gXwcJztZhioSxSYstntS54578381;     gXwcJztZhioSxSYstntS54578381 = gXwcJztZhioSxSYstntS72563877;     gXwcJztZhioSxSYstntS72563877 = gXwcJztZhioSxSYstntS88521116;     gXwcJztZhioSxSYstntS88521116 = gXwcJztZhioSxSYstntS11576667;     gXwcJztZhioSxSYstntS11576667 = gXwcJztZhioSxSYstntS98481455;     gXwcJztZhioSxSYstntS98481455 = gXwcJztZhioSxSYstntS78280867;     gXwcJztZhioSxSYstntS78280867 = gXwcJztZhioSxSYstntS27394748;     gXwcJztZhioSxSYstntS27394748 = gXwcJztZhioSxSYstntS90025948;     gXwcJztZhioSxSYstntS90025948 = gXwcJztZhioSxSYstntS6318674;     gXwcJztZhioSxSYstntS6318674 = gXwcJztZhioSxSYstntS14154407;     gXwcJztZhioSxSYstntS14154407 = gXwcJztZhioSxSYstntS88960620;     gXwcJztZhioSxSYstntS88960620 = gXwcJztZhioSxSYstntS61815598;     gXwcJztZhioSxSYstntS61815598 = gXwcJztZhioSxSYstntS83286871;     gXwcJztZhioSxSYstntS83286871 = gXwcJztZhioSxSYstntS37240210;     gXwcJztZhioSxSYstntS37240210 = gXwcJztZhioSxSYstntS52332128;     gXwcJztZhioSxSYstntS52332128 = gXwcJztZhioSxSYstntS79217297;     gXwcJztZhioSxSYstntS79217297 = gXwcJztZhioSxSYstntS79058090;     gXwcJztZhioSxSYstntS79058090 = gXwcJztZhioSxSYstntS23578730;     gXwcJztZhioSxSYstntS23578730 = gXwcJztZhioSxSYstntS87254780;     gXwcJztZhioSxSYstntS87254780 = gXwcJztZhioSxSYstntS89356583;     gXwcJztZhioSxSYstntS89356583 = gXwcJztZhioSxSYstntS14544178;     gXwcJztZhioSxSYstntS14544178 = gXwcJztZhioSxSYstntS51487795;     gXwcJztZhioSxSYstntS51487795 = gXwcJztZhioSxSYstntS83000766;     gXwcJztZhioSxSYstntS83000766 = gXwcJztZhioSxSYstntS85516924;     gXwcJztZhioSxSYstntS85516924 = gXwcJztZhioSxSYstntS53751567;     gXwcJztZhioSxSYstntS53751567 = gXwcJztZhioSxSYstntS3706993;     gXwcJztZhioSxSYstntS3706993 = gXwcJztZhioSxSYstntS52593739;     gXwcJztZhioSxSYstntS52593739 = gXwcJztZhioSxSYstntS84340224;     gXwcJztZhioSxSYstntS84340224 = gXwcJztZhioSxSYstntS80281608;     gXwcJztZhioSxSYstntS80281608 = gXwcJztZhioSxSYstntS35445083;     gXwcJztZhioSxSYstntS35445083 = gXwcJztZhioSxSYstntS87644372;     gXwcJztZhioSxSYstntS87644372 = gXwcJztZhioSxSYstntS9347709;     gXwcJztZhioSxSYstntS9347709 = gXwcJztZhioSxSYstntS58728826;     gXwcJztZhioSxSYstntS58728826 = gXwcJztZhioSxSYstntS93683253;     gXwcJztZhioSxSYstntS93683253 = gXwcJztZhioSxSYstntS96172125;     gXwcJztZhioSxSYstntS96172125 = gXwcJztZhioSxSYstntS86833314;     gXwcJztZhioSxSYstntS86833314 = gXwcJztZhioSxSYstntS15419355;     gXwcJztZhioSxSYstntS15419355 = gXwcJztZhioSxSYstntS69874493;     gXwcJztZhioSxSYstntS69874493 = gXwcJztZhioSxSYstntS98086181;     gXwcJztZhioSxSYstntS98086181 = gXwcJztZhioSxSYstntS42619791;     gXwcJztZhioSxSYstntS42619791 = gXwcJztZhioSxSYstntS28730871;     gXwcJztZhioSxSYstntS28730871 = gXwcJztZhioSxSYstntS41969911;     gXwcJztZhioSxSYstntS41969911 = gXwcJztZhioSxSYstntS1431144;     gXwcJztZhioSxSYstntS1431144 = gXwcJztZhioSxSYstntS19723226;     gXwcJztZhioSxSYstntS19723226 = gXwcJztZhioSxSYstntS37257190;     gXwcJztZhioSxSYstntS37257190 = gXwcJztZhioSxSYstntS77573059;     gXwcJztZhioSxSYstntS77573059 = gXwcJztZhioSxSYstntS3855168;     gXwcJztZhioSxSYstntS3855168 = gXwcJztZhioSxSYstntS2620133;     gXwcJztZhioSxSYstntS2620133 = gXwcJztZhioSxSYstntS70183915;     gXwcJztZhioSxSYstntS70183915 = gXwcJztZhioSxSYstntS80231955;     gXwcJztZhioSxSYstntS80231955 = gXwcJztZhioSxSYstntS72070555;     gXwcJztZhioSxSYstntS72070555 = gXwcJztZhioSxSYstntS28692625;     gXwcJztZhioSxSYstntS28692625 = gXwcJztZhioSxSYstntS90596308;     gXwcJztZhioSxSYstntS90596308 = gXwcJztZhioSxSYstntS47824622;     gXwcJztZhioSxSYstntS47824622 = gXwcJztZhioSxSYstntS6950849;     gXwcJztZhioSxSYstntS6950849 = gXwcJztZhioSxSYstntS3090586;     gXwcJztZhioSxSYstntS3090586 = gXwcJztZhioSxSYstntS89563110;     gXwcJztZhioSxSYstntS89563110 = gXwcJztZhioSxSYstntS3004193;     gXwcJztZhioSxSYstntS3004193 = gXwcJztZhioSxSYstntS57825100;     gXwcJztZhioSxSYstntS57825100 = gXwcJztZhioSxSYstntS94774462;     gXwcJztZhioSxSYstntS94774462 = gXwcJztZhioSxSYstntS25687129;     gXwcJztZhioSxSYstntS25687129 = gXwcJztZhioSxSYstntS43054524;     gXwcJztZhioSxSYstntS43054524 = gXwcJztZhioSxSYstntS9744340;     gXwcJztZhioSxSYstntS9744340 = gXwcJztZhioSxSYstntS70873591;     gXwcJztZhioSxSYstntS70873591 = gXwcJztZhioSxSYstntS26510035;     gXwcJztZhioSxSYstntS26510035 = gXwcJztZhioSxSYstntS79612912;     gXwcJztZhioSxSYstntS79612912 = gXwcJztZhioSxSYstntS3086772;     gXwcJztZhioSxSYstntS3086772 = gXwcJztZhioSxSYstntS89603617;     gXwcJztZhioSxSYstntS89603617 = gXwcJztZhioSxSYstntS41068085;     gXwcJztZhioSxSYstntS41068085 = gXwcJztZhioSxSYstntS65498813;     gXwcJztZhioSxSYstntS65498813 = gXwcJztZhioSxSYstntS63797942;     gXwcJztZhioSxSYstntS63797942 = gXwcJztZhioSxSYstntS9183597;     gXwcJztZhioSxSYstntS9183597 = gXwcJztZhioSxSYstntS25492549;     gXwcJztZhioSxSYstntS25492549 = gXwcJztZhioSxSYstntS44634989;     gXwcJztZhioSxSYstntS44634989 = gXwcJztZhioSxSYstntS60625712;     gXwcJztZhioSxSYstntS60625712 = gXwcJztZhioSxSYstntS72574267;     gXwcJztZhioSxSYstntS72574267 = gXwcJztZhioSxSYstntS50056652;     gXwcJztZhioSxSYstntS50056652 = gXwcJztZhioSxSYstntS63277541;     gXwcJztZhioSxSYstntS63277541 = gXwcJztZhioSxSYstntS48259734;     gXwcJztZhioSxSYstntS48259734 = gXwcJztZhioSxSYstntS76178507;     gXwcJztZhioSxSYstntS76178507 = gXwcJztZhioSxSYstntS99851825;     gXwcJztZhioSxSYstntS99851825 = gXwcJztZhioSxSYstntS49973606;     gXwcJztZhioSxSYstntS49973606 = gXwcJztZhioSxSYstntS14156309;     gXwcJztZhioSxSYstntS14156309 = gXwcJztZhioSxSYstntS49653;     gXwcJztZhioSxSYstntS49653 = gXwcJztZhioSxSYstntS20900658;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void OLjfGNoUsMtvYgOSAlCt2976777() {     int cUaongfokMlucauBkqAA54989449 = -632278602;    int cUaongfokMlucauBkqAA11478400 = 59646436;    int cUaongfokMlucauBkqAA3241436 = -256974903;    int cUaongfokMlucauBkqAA67875221 = -163631040;    int cUaongfokMlucauBkqAA87602430 = -822717609;    int cUaongfokMlucauBkqAA59043369 = -919474633;    int cUaongfokMlucauBkqAA46650494 = -648593496;    int cUaongfokMlucauBkqAA27454197 = -724475312;    int cUaongfokMlucauBkqAA51968153 = -721087071;    int cUaongfokMlucauBkqAA83061020 = -7706092;    int cUaongfokMlucauBkqAA48983075 = -327013225;    int cUaongfokMlucauBkqAA59498228 = -172162379;    int cUaongfokMlucauBkqAA98413964 = -855042315;    int cUaongfokMlucauBkqAA41883808 = -852655601;    int cUaongfokMlucauBkqAA98063309 = -368856885;    int cUaongfokMlucauBkqAA79653804 = -190300325;    int cUaongfokMlucauBkqAA56435649 = -501492934;    int cUaongfokMlucauBkqAA14192822 = -310641045;    int cUaongfokMlucauBkqAA14676675 = -317689990;    int cUaongfokMlucauBkqAA93417823 = -193392516;    int cUaongfokMlucauBkqAA34395006 = -609947600;    int cUaongfokMlucauBkqAA24243564 = -944894638;    int cUaongfokMlucauBkqAA23655753 = 80927722;    int cUaongfokMlucauBkqAA77037776 = -844531725;    int cUaongfokMlucauBkqAA25435193 = -764631763;    int cUaongfokMlucauBkqAA70303226 = -65324133;    int cUaongfokMlucauBkqAA7808873 = -440756617;    int cUaongfokMlucauBkqAA46592050 = -962329935;    int cUaongfokMlucauBkqAA56061 = -579170710;    int cUaongfokMlucauBkqAA39839211 = -635886549;    int cUaongfokMlucauBkqAA20919099 = -452754442;    int cUaongfokMlucauBkqAA69710579 = -702824910;    int cUaongfokMlucauBkqAA96537335 = -300229179;    int cUaongfokMlucauBkqAA84364789 = -688852941;    int cUaongfokMlucauBkqAA79248107 = 51774512;    int cUaongfokMlucauBkqAA82244421 = -300498313;    int cUaongfokMlucauBkqAA60443465 = -889145314;    int cUaongfokMlucauBkqAA74017685 = -134233142;    int cUaongfokMlucauBkqAA14033737 = -465261848;    int cUaongfokMlucauBkqAA13826982 = -529417020;    int cUaongfokMlucauBkqAA80187522 = -374452855;    int cUaongfokMlucauBkqAA56021773 = -793579632;    int cUaongfokMlucauBkqAA54678294 = -641448478;    int cUaongfokMlucauBkqAA11091073 = -718680701;    int cUaongfokMlucauBkqAA64012088 = -39462339;    int cUaongfokMlucauBkqAA96390110 = -789198094;    int cUaongfokMlucauBkqAA10380670 = -768319825;    int cUaongfokMlucauBkqAA30183438 = -799990177;    int cUaongfokMlucauBkqAA91852429 = -567063251;    int cUaongfokMlucauBkqAA35326920 = -21182341;    int cUaongfokMlucauBkqAA89738452 = -617087209;    int cUaongfokMlucauBkqAA32105131 = -274498952;    int cUaongfokMlucauBkqAA25897392 = -114267136;    int cUaongfokMlucauBkqAA38586022 = -168166821;    int cUaongfokMlucauBkqAA68581770 = -457916889;    int cUaongfokMlucauBkqAA30745886 = -687383965;    int cUaongfokMlucauBkqAA87822646 = 78718714;    int cUaongfokMlucauBkqAA26203660 = -412443178;    int cUaongfokMlucauBkqAA42440028 = -398999278;    int cUaongfokMlucauBkqAA17299204 = -657393476;    int cUaongfokMlucauBkqAA51234496 = -378718017;    int cUaongfokMlucauBkqAA58445 = -686263562;    int cUaongfokMlucauBkqAA27398136 = -45304602;    int cUaongfokMlucauBkqAA12128942 = 14799478;    int cUaongfokMlucauBkqAA62141921 = -554951651;    int cUaongfokMlucauBkqAA79272496 = -624188315;    int cUaongfokMlucauBkqAA62960892 = -871933201;    int cUaongfokMlucauBkqAA14049176 = -66189374;    int cUaongfokMlucauBkqAA62635701 = -804430113;    int cUaongfokMlucauBkqAA15818888 = 31641427;    int cUaongfokMlucauBkqAA19210339 = -301155012;    int cUaongfokMlucauBkqAA82417964 = -267259793;    int cUaongfokMlucauBkqAA159085 = -845379198;    int cUaongfokMlucauBkqAA849694 = -788272970;    int cUaongfokMlucauBkqAA13230301 = -818939661;    int cUaongfokMlucauBkqAA78373233 = -816367968;    int cUaongfokMlucauBkqAA69565269 = -203446160;    int cUaongfokMlucauBkqAA12564681 = -200391578;    int cUaongfokMlucauBkqAA13025689 = -705069387;    int cUaongfokMlucauBkqAA29045083 = -975433670;    int cUaongfokMlucauBkqAA59922557 = -297004309;    int cUaongfokMlucauBkqAA77625435 = -640766441;    int cUaongfokMlucauBkqAA54739620 = -295266684;    int cUaongfokMlucauBkqAA64729140 = -457988370;    int cUaongfokMlucauBkqAA50100759 = 81200659;    int cUaongfokMlucauBkqAA88813968 = -78255491;    int cUaongfokMlucauBkqAA43813187 = -488557775;    int cUaongfokMlucauBkqAA57951314 = -32062358;    int cUaongfokMlucauBkqAA15783019 = -130936053;    int cUaongfokMlucauBkqAA48502222 = -260841524;    int cUaongfokMlucauBkqAA94421775 = -279217027;    int cUaongfokMlucauBkqAA34239806 = -376702136;    int cUaongfokMlucauBkqAA31577657 = -735233864;    int cUaongfokMlucauBkqAA96734533 = -807868372;    int cUaongfokMlucauBkqAA62592485 = -50699004;    int cUaongfokMlucauBkqAA80129078 = -688189293;    int cUaongfokMlucauBkqAA28623637 = -648275030;    int cUaongfokMlucauBkqAA42549352 = -556247957;    int cUaongfokMlucauBkqAA48949151 = -63729051;    int cUaongfokMlucauBkqAA84739592 = -632278602;     cUaongfokMlucauBkqAA54989449 = cUaongfokMlucauBkqAA11478400;     cUaongfokMlucauBkqAA11478400 = cUaongfokMlucauBkqAA3241436;     cUaongfokMlucauBkqAA3241436 = cUaongfokMlucauBkqAA67875221;     cUaongfokMlucauBkqAA67875221 = cUaongfokMlucauBkqAA87602430;     cUaongfokMlucauBkqAA87602430 = cUaongfokMlucauBkqAA59043369;     cUaongfokMlucauBkqAA59043369 = cUaongfokMlucauBkqAA46650494;     cUaongfokMlucauBkqAA46650494 = cUaongfokMlucauBkqAA27454197;     cUaongfokMlucauBkqAA27454197 = cUaongfokMlucauBkqAA51968153;     cUaongfokMlucauBkqAA51968153 = cUaongfokMlucauBkqAA83061020;     cUaongfokMlucauBkqAA83061020 = cUaongfokMlucauBkqAA48983075;     cUaongfokMlucauBkqAA48983075 = cUaongfokMlucauBkqAA59498228;     cUaongfokMlucauBkqAA59498228 = cUaongfokMlucauBkqAA98413964;     cUaongfokMlucauBkqAA98413964 = cUaongfokMlucauBkqAA41883808;     cUaongfokMlucauBkqAA41883808 = cUaongfokMlucauBkqAA98063309;     cUaongfokMlucauBkqAA98063309 = cUaongfokMlucauBkqAA79653804;     cUaongfokMlucauBkqAA79653804 = cUaongfokMlucauBkqAA56435649;     cUaongfokMlucauBkqAA56435649 = cUaongfokMlucauBkqAA14192822;     cUaongfokMlucauBkqAA14192822 = cUaongfokMlucauBkqAA14676675;     cUaongfokMlucauBkqAA14676675 = cUaongfokMlucauBkqAA93417823;     cUaongfokMlucauBkqAA93417823 = cUaongfokMlucauBkqAA34395006;     cUaongfokMlucauBkqAA34395006 = cUaongfokMlucauBkqAA24243564;     cUaongfokMlucauBkqAA24243564 = cUaongfokMlucauBkqAA23655753;     cUaongfokMlucauBkqAA23655753 = cUaongfokMlucauBkqAA77037776;     cUaongfokMlucauBkqAA77037776 = cUaongfokMlucauBkqAA25435193;     cUaongfokMlucauBkqAA25435193 = cUaongfokMlucauBkqAA70303226;     cUaongfokMlucauBkqAA70303226 = cUaongfokMlucauBkqAA7808873;     cUaongfokMlucauBkqAA7808873 = cUaongfokMlucauBkqAA46592050;     cUaongfokMlucauBkqAA46592050 = cUaongfokMlucauBkqAA56061;     cUaongfokMlucauBkqAA56061 = cUaongfokMlucauBkqAA39839211;     cUaongfokMlucauBkqAA39839211 = cUaongfokMlucauBkqAA20919099;     cUaongfokMlucauBkqAA20919099 = cUaongfokMlucauBkqAA69710579;     cUaongfokMlucauBkqAA69710579 = cUaongfokMlucauBkqAA96537335;     cUaongfokMlucauBkqAA96537335 = cUaongfokMlucauBkqAA84364789;     cUaongfokMlucauBkqAA84364789 = cUaongfokMlucauBkqAA79248107;     cUaongfokMlucauBkqAA79248107 = cUaongfokMlucauBkqAA82244421;     cUaongfokMlucauBkqAA82244421 = cUaongfokMlucauBkqAA60443465;     cUaongfokMlucauBkqAA60443465 = cUaongfokMlucauBkqAA74017685;     cUaongfokMlucauBkqAA74017685 = cUaongfokMlucauBkqAA14033737;     cUaongfokMlucauBkqAA14033737 = cUaongfokMlucauBkqAA13826982;     cUaongfokMlucauBkqAA13826982 = cUaongfokMlucauBkqAA80187522;     cUaongfokMlucauBkqAA80187522 = cUaongfokMlucauBkqAA56021773;     cUaongfokMlucauBkqAA56021773 = cUaongfokMlucauBkqAA54678294;     cUaongfokMlucauBkqAA54678294 = cUaongfokMlucauBkqAA11091073;     cUaongfokMlucauBkqAA11091073 = cUaongfokMlucauBkqAA64012088;     cUaongfokMlucauBkqAA64012088 = cUaongfokMlucauBkqAA96390110;     cUaongfokMlucauBkqAA96390110 = cUaongfokMlucauBkqAA10380670;     cUaongfokMlucauBkqAA10380670 = cUaongfokMlucauBkqAA30183438;     cUaongfokMlucauBkqAA30183438 = cUaongfokMlucauBkqAA91852429;     cUaongfokMlucauBkqAA91852429 = cUaongfokMlucauBkqAA35326920;     cUaongfokMlucauBkqAA35326920 = cUaongfokMlucauBkqAA89738452;     cUaongfokMlucauBkqAA89738452 = cUaongfokMlucauBkqAA32105131;     cUaongfokMlucauBkqAA32105131 = cUaongfokMlucauBkqAA25897392;     cUaongfokMlucauBkqAA25897392 = cUaongfokMlucauBkqAA38586022;     cUaongfokMlucauBkqAA38586022 = cUaongfokMlucauBkqAA68581770;     cUaongfokMlucauBkqAA68581770 = cUaongfokMlucauBkqAA30745886;     cUaongfokMlucauBkqAA30745886 = cUaongfokMlucauBkqAA87822646;     cUaongfokMlucauBkqAA87822646 = cUaongfokMlucauBkqAA26203660;     cUaongfokMlucauBkqAA26203660 = cUaongfokMlucauBkqAA42440028;     cUaongfokMlucauBkqAA42440028 = cUaongfokMlucauBkqAA17299204;     cUaongfokMlucauBkqAA17299204 = cUaongfokMlucauBkqAA51234496;     cUaongfokMlucauBkqAA51234496 = cUaongfokMlucauBkqAA58445;     cUaongfokMlucauBkqAA58445 = cUaongfokMlucauBkqAA27398136;     cUaongfokMlucauBkqAA27398136 = cUaongfokMlucauBkqAA12128942;     cUaongfokMlucauBkqAA12128942 = cUaongfokMlucauBkqAA62141921;     cUaongfokMlucauBkqAA62141921 = cUaongfokMlucauBkqAA79272496;     cUaongfokMlucauBkqAA79272496 = cUaongfokMlucauBkqAA62960892;     cUaongfokMlucauBkqAA62960892 = cUaongfokMlucauBkqAA14049176;     cUaongfokMlucauBkqAA14049176 = cUaongfokMlucauBkqAA62635701;     cUaongfokMlucauBkqAA62635701 = cUaongfokMlucauBkqAA15818888;     cUaongfokMlucauBkqAA15818888 = cUaongfokMlucauBkqAA19210339;     cUaongfokMlucauBkqAA19210339 = cUaongfokMlucauBkqAA82417964;     cUaongfokMlucauBkqAA82417964 = cUaongfokMlucauBkqAA159085;     cUaongfokMlucauBkqAA159085 = cUaongfokMlucauBkqAA849694;     cUaongfokMlucauBkqAA849694 = cUaongfokMlucauBkqAA13230301;     cUaongfokMlucauBkqAA13230301 = cUaongfokMlucauBkqAA78373233;     cUaongfokMlucauBkqAA78373233 = cUaongfokMlucauBkqAA69565269;     cUaongfokMlucauBkqAA69565269 = cUaongfokMlucauBkqAA12564681;     cUaongfokMlucauBkqAA12564681 = cUaongfokMlucauBkqAA13025689;     cUaongfokMlucauBkqAA13025689 = cUaongfokMlucauBkqAA29045083;     cUaongfokMlucauBkqAA29045083 = cUaongfokMlucauBkqAA59922557;     cUaongfokMlucauBkqAA59922557 = cUaongfokMlucauBkqAA77625435;     cUaongfokMlucauBkqAA77625435 = cUaongfokMlucauBkqAA54739620;     cUaongfokMlucauBkqAA54739620 = cUaongfokMlucauBkqAA64729140;     cUaongfokMlucauBkqAA64729140 = cUaongfokMlucauBkqAA50100759;     cUaongfokMlucauBkqAA50100759 = cUaongfokMlucauBkqAA88813968;     cUaongfokMlucauBkqAA88813968 = cUaongfokMlucauBkqAA43813187;     cUaongfokMlucauBkqAA43813187 = cUaongfokMlucauBkqAA57951314;     cUaongfokMlucauBkqAA57951314 = cUaongfokMlucauBkqAA15783019;     cUaongfokMlucauBkqAA15783019 = cUaongfokMlucauBkqAA48502222;     cUaongfokMlucauBkqAA48502222 = cUaongfokMlucauBkqAA94421775;     cUaongfokMlucauBkqAA94421775 = cUaongfokMlucauBkqAA34239806;     cUaongfokMlucauBkqAA34239806 = cUaongfokMlucauBkqAA31577657;     cUaongfokMlucauBkqAA31577657 = cUaongfokMlucauBkqAA96734533;     cUaongfokMlucauBkqAA96734533 = cUaongfokMlucauBkqAA62592485;     cUaongfokMlucauBkqAA62592485 = cUaongfokMlucauBkqAA80129078;     cUaongfokMlucauBkqAA80129078 = cUaongfokMlucauBkqAA28623637;     cUaongfokMlucauBkqAA28623637 = cUaongfokMlucauBkqAA42549352;     cUaongfokMlucauBkqAA42549352 = cUaongfokMlucauBkqAA48949151;     cUaongfokMlucauBkqAA48949151 = cUaongfokMlucauBkqAA84739592;     cUaongfokMlucauBkqAA84739592 = cUaongfokMlucauBkqAA54989449;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BsOTPcUcxyTiUiIOwbxq61628447() {     int RAMJEMEfSUgXMFDfKeeT18420102 = -216052016;    int RAMJEMEfSUgXMFDfKeeT36729979 = -800128178;    int RAMJEMEfSUgXMFDfKeeT95831306 = -335049363;    int RAMJEMEfSUgXMFDfKeeT73149199 = -177610767;    int RAMJEMEfSUgXMFDfKeeT28717008 = -84440640;    int RAMJEMEfSUgXMFDfKeeT36674597 = -760697863;    int RAMJEMEfSUgXMFDfKeeT4080772 = 27080003;    int RAMJEMEfSUgXMFDfKeeT80138024 = 57122372;    int RAMJEMEfSUgXMFDfKeeT951073 = -195016211;    int RAMJEMEfSUgXMFDfKeeT17279049 = -587552610;    int RAMJEMEfSUgXMFDfKeeT16588343 = -864855883;    int RAMJEMEfSUgXMFDfKeeT36612917 = -806479146;    int RAMJEMEfSUgXMFDfKeeT96582101 = -76687325;    int RAMJEMEfSUgXMFDfKeeT75913714 = -485569118;    int RAMJEMEfSUgXMFDfKeeT21397503 = -576021726;    int RAMJEMEfSUgXMFDfKeeT79848488 = -777499763;    int RAMJEMEfSUgXMFDfKeeT20292850 = -732764468;    int RAMJEMEfSUgXMFDfKeeT58528804 = -185954565;    int RAMJEMEfSUgXMFDfKeeT79552720 = -223246832;    int RAMJEMEfSUgXMFDfKeeT88546034 = -607979301;    int RAMJEMEfSUgXMFDfKeeT94842226 = -202456579;    int RAMJEMEfSUgXMFDfKeeT87106761 = -660252396;    int RAMJEMEfSUgXMFDfKeeT54851346 = -964460094;    int RAMJEMEfSUgXMFDfKeeT73850175 = -383157570;    int RAMJEMEfSUgXMFDfKeeT8870014 = -39240701;    int RAMJEMEfSUgXMFDfKeeT12428543 = -418712150;    int RAMJEMEfSUgXMFDfKeeT88617920 = -370508441;    int RAMJEMEfSUgXMFDfKeeT58989801 = -217644354;    int RAMJEMEfSUgXMFDfKeeT97361208 = -12242560;    int RAMJEMEfSUgXMFDfKeeT79355986 = -286921912;    int RAMJEMEfSUgXMFDfKeeT15846752 = -488317442;    int RAMJEMEfSUgXMFDfKeeT416259 = -202023163;    int RAMJEMEfSUgXMFDfKeeT16523644 = -173321329;    int RAMJEMEfSUgXMFDfKeeT80068710 = -797499485;    int RAMJEMEfSUgXMFDfKeeT75575235 = -491307823;    int RAMJEMEfSUgXMFDfKeeT57967190 = -344676975;    int RAMJEMEfSUgXMFDfKeeT74492634 = -258827429;    int RAMJEMEfSUgXMFDfKeeT13672766 = -493624095;    int RAMJEMEfSUgXMFDfKeeT1847015 = -719059821;    int RAMJEMEfSUgXMFDfKeeT18906634 = -678509360;    int RAMJEMEfSUgXMFDfKeeT34405862 = -203345429;    int RAMJEMEfSUgXMFDfKeeT65551223 = -476772867;    int RAMJEMEfSUgXMFDfKeeT68079249 = -823749392;    int RAMJEMEfSUgXMFDfKeeT90565518 = -166340001;    int RAMJEMEfSUgXMFDfKeeT35806561 = -648263276;    int RAMJEMEfSUgXMFDfKeeT91357271 = -356546718;    int RAMJEMEfSUgXMFDfKeeT34683556 = -245686132;    int RAMJEMEfSUgXMFDfKeeT60788537 = -971101510;    int RAMJEMEfSUgXMFDfKeeT38192970 = -482834236;    int RAMJEMEfSUgXMFDfKeeT88588523 = -443150718;    int RAMJEMEfSUgXMFDfKeeT28374825 = -284771727;    int RAMJEMEfSUgXMFDfKeeT50052699 = -903526958;    int RAMJEMEfSUgXMFDfKeeT56748121 = -165427376;    int RAMJEMEfSUgXMFDfKeeT27320517 = -411555269;    int RAMJEMEfSUgXMFDfKeeT66786221 = -481564178;    int RAMJEMEfSUgXMFDfKeeT31313341 = -555799620;    int RAMJEMEfSUgXMFDfKeeT81878632 = -835668084;    int RAMJEMEfSUgXMFDfKeeT21981131 = -951891793;    int RAMJEMEfSUgXMFDfKeeT64279186 = -38370066;    int RAMJEMEfSUgXMFDfKeeT16288466 = -665728491;    int RAMJEMEfSUgXMFDfKeeT48056677 = -290189423;    int RAMJEMEfSUgXMFDfKeeT45090970 = -755275643;    int RAMJEMEfSUgXMFDfKeeT82776816 = -930635068;    int RAMJEMEfSUgXMFDfKeeT21595086 = -908094300;    int RAMJEMEfSUgXMFDfKeeT1432298 = 764831;    int RAMJEMEfSUgXMFDfKeeT16172085 = -562832720;    int RAMJEMEfSUgXMFDfKeeT20089273 = -533157817;    int RAMJEMEfSUgXMFDfKeeT16513391 = -279187841;    int RAMJEMEfSUgXMFDfKeeT338480 = -994261295;    int RAMJEMEfSUgXMFDfKeeT63430312 = -131344751;    int RAMJEMEfSUgXMFDfKeeT5355854 = -418672335;    int RAMJEMEfSUgXMFDfKeeT6620085 = -139140374;    int RAMJEMEfSUgXMFDfKeeT56681790 = -466894745;    int RAMJEMEfSUgXMFDfKeeT60646087 = -544737473;    int RAMJEMEfSUgXMFDfKeeT54140172 = -304633872;    int RAMJEMEfSUgXMFDfKeeT29291004 = -725683712;    int RAMJEMEfSUgXMFDfKeeT19027512 = -836503005;    int RAMJEMEfSUgXMFDfKeeT64285827 = -698120093;    int RAMJEMEfSUgXMFDfKeeT38043615 = -734894295;    int RAMJEMEfSUgXMFDfKeeT17512742 = -682693983;    int RAMJEMEfSUgXMFDfKeeT77744986 = -73026018;    int RAMJEMEfSUgXMFDfKeeT27829383 = -399406931;    int RAMJEMEfSUgXMFDfKeeT20796831 = -734810119;    int RAMJEMEfSUgXMFDfKeeT8772686 = -569091843;    int RAMJEMEfSUgXMFDfKeeT50981162 = 97849814;    int RAMJEMEfSUgXMFDfKeeT65794052 = -584790484;    int RAMJEMEfSUgXMFDfKeeT43668137 = 63404212;    int RAMJEMEfSUgXMFDfKeeT89203127 = -761766061;    int RAMJEMEfSUgXMFDfKeeT13282490 = -215935307;    int RAMJEMEfSUgXMFDfKeeT44261895 = -935508204;    int RAMJEMEfSUgXMFDfKeeT76088558 = -509008891;    int RAMJEMEfSUgXMFDfKeeT52511504 = -306935636;    int RAMJEMEfSUgXMFDfKeeT49393579 = -355254029;    int RAMJEMEfSUgXMFDfKeeT85558549 = 46668670;    int RAMJEMEfSUgXMFDfKeeT70849956 = -288319937;    int RAMJEMEfSUgXMFDfKeeT89314891 = -448069786;    int RAMJEMEfSUgXMFDfKeeT82774407 = -546137800;    int RAMJEMEfSUgXMFDfKeeT46484163 = -915655093;    int RAMJEMEfSUgXMFDfKeeT89133221 = -67104833;    int RAMJEMEfSUgXMFDfKeeT19634477 = -216052016;     RAMJEMEfSUgXMFDfKeeT18420102 = RAMJEMEfSUgXMFDfKeeT36729979;     RAMJEMEfSUgXMFDfKeeT36729979 = RAMJEMEfSUgXMFDfKeeT95831306;     RAMJEMEfSUgXMFDfKeeT95831306 = RAMJEMEfSUgXMFDfKeeT73149199;     RAMJEMEfSUgXMFDfKeeT73149199 = RAMJEMEfSUgXMFDfKeeT28717008;     RAMJEMEfSUgXMFDfKeeT28717008 = RAMJEMEfSUgXMFDfKeeT36674597;     RAMJEMEfSUgXMFDfKeeT36674597 = RAMJEMEfSUgXMFDfKeeT4080772;     RAMJEMEfSUgXMFDfKeeT4080772 = RAMJEMEfSUgXMFDfKeeT80138024;     RAMJEMEfSUgXMFDfKeeT80138024 = RAMJEMEfSUgXMFDfKeeT951073;     RAMJEMEfSUgXMFDfKeeT951073 = RAMJEMEfSUgXMFDfKeeT17279049;     RAMJEMEfSUgXMFDfKeeT17279049 = RAMJEMEfSUgXMFDfKeeT16588343;     RAMJEMEfSUgXMFDfKeeT16588343 = RAMJEMEfSUgXMFDfKeeT36612917;     RAMJEMEfSUgXMFDfKeeT36612917 = RAMJEMEfSUgXMFDfKeeT96582101;     RAMJEMEfSUgXMFDfKeeT96582101 = RAMJEMEfSUgXMFDfKeeT75913714;     RAMJEMEfSUgXMFDfKeeT75913714 = RAMJEMEfSUgXMFDfKeeT21397503;     RAMJEMEfSUgXMFDfKeeT21397503 = RAMJEMEfSUgXMFDfKeeT79848488;     RAMJEMEfSUgXMFDfKeeT79848488 = RAMJEMEfSUgXMFDfKeeT20292850;     RAMJEMEfSUgXMFDfKeeT20292850 = RAMJEMEfSUgXMFDfKeeT58528804;     RAMJEMEfSUgXMFDfKeeT58528804 = RAMJEMEfSUgXMFDfKeeT79552720;     RAMJEMEfSUgXMFDfKeeT79552720 = RAMJEMEfSUgXMFDfKeeT88546034;     RAMJEMEfSUgXMFDfKeeT88546034 = RAMJEMEfSUgXMFDfKeeT94842226;     RAMJEMEfSUgXMFDfKeeT94842226 = RAMJEMEfSUgXMFDfKeeT87106761;     RAMJEMEfSUgXMFDfKeeT87106761 = RAMJEMEfSUgXMFDfKeeT54851346;     RAMJEMEfSUgXMFDfKeeT54851346 = RAMJEMEfSUgXMFDfKeeT73850175;     RAMJEMEfSUgXMFDfKeeT73850175 = RAMJEMEfSUgXMFDfKeeT8870014;     RAMJEMEfSUgXMFDfKeeT8870014 = RAMJEMEfSUgXMFDfKeeT12428543;     RAMJEMEfSUgXMFDfKeeT12428543 = RAMJEMEfSUgXMFDfKeeT88617920;     RAMJEMEfSUgXMFDfKeeT88617920 = RAMJEMEfSUgXMFDfKeeT58989801;     RAMJEMEfSUgXMFDfKeeT58989801 = RAMJEMEfSUgXMFDfKeeT97361208;     RAMJEMEfSUgXMFDfKeeT97361208 = RAMJEMEfSUgXMFDfKeeT79355986;     RAMJEMEfSUgXMFDfKeeT79355986 = RAMJEMEfSUgXMFDfKeeT15846752;     RAMJEMEfSUgXMFDfKeeT15846752 = RAMJEMEfSUgXMFDfKeeT416259;     RAMJEMEfSUgXMFDfKeeT416259 = RAMJEMEfSUgXMFDfKeeT16523644;     RAMJEMEfSUgXMFDfKeeT16523644 = RAMJEMEfSUgXMFDfKeeT80068710;     RAMJEMEfSUgXMFDfKeeT80068710 = RAMJEMEfSUgXMFDfKeeT75575235;     RAMJEMEfSUgXMFDfKeeT75575235 = RAMJEMEfSUgXMFDfKeeT57967190;     RAMJEMEfSUgXMFDfKeeT57967190 = RAMJEMEfSUgXMFDfKeeT74492634;     RAMJEMEfSUgXMFDfKeeT74492634 = RAMJEMEfSUgXMFDfKeeT13672766;     RAMJEMEfSUgXMFDfKeeT13672766 = RAMJEMEfSUgXMFDfKeeT1847015;     RAMJEMEfSUgXMFDfKeeT1847015 = RAMJEMEfSUgXMFDfKeeT18906634;     RAMJEMEfSUgXMFDfKeeT18906634 = RAMJEMEfSUgXMFDfKeeT34405862;     RAMJEMEfSUgXMFDfKeeT34405862 = RAMJEMEfSUgXMFDfKeeT65551223;     RAMJEMEfSUgXMFDfKeeT65551223 = RAMJEMEfSUgXMFDfKeeT68079249;     RAMJEMEfSUgXMFDfKeeT68079249 = RAMJEMEfSUgXMFDfKeeT90565518;     RAMJEMEfSUgXMFDfKeeT90565518 = RAMJEMEfSUgXMFDfKeeT35806561;     RAMJEMEfSUgXMFDfKeeT35806561 = RAMJEMEfSUgXMFDfKeeT91357271;     RAMJEMEfSUgXMFDfKeeT91357271 = RAMJEMEfSUgXMFDfKeeT34683556;     RAMJEMEfSUgXMFDfKeeT34683556 = RAMJEMEfSUgXMFDfKeeT60788537;     RAMJEMEfSUgXMFDfKeeT60788537 = RAMJEMEfSUgXMFDfKeeT38192970;     RAMJEMEfSUgXMFDfKeeT38192970 = RAMJEMEfSUgXMFDfKeeT88588523;     RAMJEMEfSUgXMFDfKeeT88588523 = RAMJEMEfSUgXMFDfKeeT28374825;     RAMJEMEfSUgXMFDfKeeT28374825 = RAMJEMEfSUgXMFDfKeeT50052699;     RAMJEMEfSUgXMFDfKeeT50052699 = RAMJEMEfSUgXMFDfKeeT56748121;     RAMJEMEfSUgXMFDfKeeT56748121 = RAMJEMEfSUgXMFDfKeeT27320517;     RAMJEMEfSUgXMFDfKeeT27320517 = RAMJEMEfSUgXMFDfKeeT66786221;     RAMJEMEfSUgXMFDfKeeT66786221 = RAMJEMEfSUgXMFDfKeeT31313341;     RAMJEMEfSUgXMFDfKeeT31313341 = RAMJEMEfSUgXMFDfKeeT81878632;     RAMJEMEfSUgXMFDfKeeT81878632 = RAMJEMEfSUgXMFDfKeeT21981131;     RAMJEMEfSUgXMFDfKeeT21981131 = RAMJEMEfSUgXMFDfKeeT64279186;     RAMJEMEfSUgXMFDfKeeT64279186 = RAMJEMEfSUgXMFDfKeeT16288466;     RAMJEMEfSUgXMFDfKeeT16288466 = RAMJEMEfSUgXMFDfKeeT48056677;     RAMJEMEfSUgXMFDfKeeT48056677 = RAMJEMEfSUgXMFDfKeeT45090970;     RAMJEMEfSUgXMFDfKeeT45090970 = RAMJEMEfSUgXMFDfKeeT82776816;     RAMJEMEfSUgXMFDfKeeT82776816 = RAMJEMEfSUgXMFDfKeeT21595086;     RAMJEMEfSUgXMFDfKeeT21595086 = RAMJEMEfSUgXMFDfKeeT1432298;     RAMJEMEfSUgXMFDfKeeT1432298 = RAMJEMEfSUgXMFDfKeeT16172085;     RAMJEMEfSUgXMFDfKeeT16172085 = RAMJEMEfSUgXMFDfKeeT20089273;     RAMJEMEfSUgXMFDfKeeT20089273 = RAMJEMEfSUgXMFDfKeeT16513391;     RAMJEMEfSUgXMFDfKeeT16513391 = RAMJEMEfSUgXMFDfKeeT338480;     RAMJEMEfSUgXMFDfKeeT338480 = RAMJEMEfSUgXMFDfKeeT63430312;     RAMJEMEfSUgXMFDfKeeT63430312 = RAMJEMEfSUgXMFDfKeeT5355854;     RAMJEMEfSUgXMFDfKeeT5355854 = RAMJEMEfSUgXMFDfKeeT6620085;     RAMJEMEfSUgXMFDfKeeT6620085 = RAMJEMEfSUgXMFDfKeeT56681790;     RAMJEMEfSUgXMFDfKeeT56681790 = RAMJEMEfSUgXMFDfKeeT60646087;     RAMJEMEfSUgXMFDfKeeT60646087 = RAMJEMEfSUgXMFDfKeeT54140172;     RAMJEMEfSUgXMFDfKeeT54140172 = RAMJEMEfSUgXMFDfKeeT29291004;     RAMJEMEfSUgXMFDfKeeT29291004 = RAMJEMEfSUgXMFDfKeeT19027512;     RAMJEMEfSUgXMFDfKeeT19027512 = RAMJEMEfSUgXMFDfKeeT64285827;     RAMJEMEfSUgXMFDfKeeT64285827 = RAMJEMEfSUgXMFDfKeeT38043615;     RAMJEMEfSUgXMFDfKeeT38043615 = RAMJEMEfSUgXMFDfKeeT17512742;     RAMJEMEfSUgXMFDfKeeT17512742 = RAMJEMEfSUgXMFDfKeeT77744986;     RAMJEMEfSUgXMFDfKeeT77744986 = RAMJEMEfSUgXMFDfKeeT27829383;     RAMJEMEfSUgXMFDfKeeT27829383 = RAMJEMEfSUgXMFDfKeeT20796831;     RAMJEMEfSUgXMFDfKeeT20796831 = RAMJEMEfSUgXMFDfKeeT8772686;     RAMJEMEfSUgXMFDfKeeT8772686 = RAMJEMEfSUgXMFDfKeeT50981162;     RAMJEMEfSUgXMFDfKeeT50981162 = RAMJEMEfSUgXMFDfKeeT65794052;     RAMJEMEfSUgXMFDfKeeT65794052 = RAMJEMEfSUgXMFDfKeeT43668137;     RAMJEMEfSUgXMFDfKeeT43668137 = RAMJEMEfSUgXMFDfKeeT89203127;     RAMJEMEfSUgXMFDfKeeT89203127 = RAMJEMEfSUgXMFDfKeeT13282490;     RAMJEMEfSUgXMFDfKeeT13282490 = RAMJEMEfSUgXMFDfKeeT44261895;     RAMJEMEfSUgXMFDfKeeT44261895 = RAMJEMEfSUgXMFDfKeeT76088558;     RAMJEMEfSUgXMFDfKeeT76088558 = RAMJEMEfSUgXMFDfKeeT52511504;     RAMJEMEfSUgXMFDfKeeT52511504 = RAMJEMEfSUgXMFDfKeeT49393579;     RAMJEMEfSUgXMFDfKeeT49393579 = RAMJEMEfSUgXMFDfKeeT85558549;     RAMJEMEfSUgXMFDfKeeT85558549 = RAMJEMEfSUgXMFDfKeeT70849956;     RAMJEMEfSUgXMFDfKeeT70849956 = RAMJEMEfSUgXMFDfKeeT89314891;     RAMJEMEfSUgXMFDfKeeT89314891 = RAMJEMEfSUgXMFDfKeeT82774407;     RAMJEMEfSUgXMFDfKeeT82774407 = RAMJEMEfSUgXMFDfKeeT46484163;     RAMJEMEfSUgXMFDfKeeT46484163 = RAMJEMEfSUgXMFDfKeeT89133221;     RAMJEMEfSUgXMFDfKeeT89133221 = RAMJEMEfSUgXMFDfKeeT19634477;     RAMJEMEfSUgXMFDfKeeT19634477 = RAMJEMEfSUgXMFDfKeeT18420102;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void zvdYUgFjDcViSpmHtkYL20280118() {     int RXJFCMuFqSDjBmcyDuQX81850754 = -899825429;    int RXJFCMuFqSDjBmcyDuQX61981558 = -559902792;    int RXJFCMuFqSDjBmcyDuQX88421176 = -413123822;    int RXJFCMuFqSDjBmcyDuQX78423177 = -191590493;    int RXJFCMuFqSDjBmcyDuQX69831586 = -446163671;    int RXJFCMuFqSDjBmcyDuQX14305826 = -601921093;    int RXJFCMuFqSDjBmcyDuQX61511048 = -397246497;    int RXJFCMuFqSDjBmcyDuQX32821852 = -261279944;    int RXJFCMuFqSDjBmcyDuQX49933992 = -768945352;    int RXJFCMuFqSDjBmcyDuQX51497078 = -67399127;    int RXJFCMuFqSDjBmcyDuQX84193609 = -302698542;    int RXJFCMuFqSDjBmcyDuQX13727606 = -340795912;    int RXJFCMuFqSDjBmcyDuQX94750238 = -398332336;    int RXJFCMuFqSDjBmcyDuQX9943621 = -118482635;    int RXJFCMuFqSDjBmcyDuQX44731696 = -783186566;    int RXJFCMuFqSDjBmcyDuQX80043172 = -264699201;    int RXJFCMuFqSDjBmcyDuQX84150051 = -964036002;    int RXJFCMuFqSDjBmcyDuQX2864788 = -61268085;    int RXJFCMuFqSDjBmcyDuQX44428767 = -128803673;    int RXJFCMuFqSDjBmcyDuQX83674244 = 77433914;    int RXJFCMuFqSDjBmcyDuQX55289447 = -894965557;    int RXJFCMuFqSDjBmcyDuQX49969958 = -375610155;    int RXJFCMuFqSDjBmcyDuQX86046939 = -909847910;    int RXJFCMuFqSDjBmcyDuQX70662575 = 78216585;    int RXJFCMuFqSDjBmcyDuQX92304834 = -413849639;    int RXJFCMuFqSDjBmcyDuQX54553858 = -772100166;    int RXJFCMuFqSDjBmcyDuQX69426967 = -300260265;    int RXJFCMuFqSDjBmcyDuQX71387552 = -572958773;    int RXJFCMuFqSDjBmcyDuQX94666356 = -545314411;    int RXJFCMuFqSDjBmcyDuQX18872762 = 62042724;    int RXJFCMuFqSDjBmcyDuQX10774405 = -523880441;    int RXJFCMuFqSDjBmcyDuQX31121937 = -801221417;    int RXJFCMuFqSDjBmcyDuQX36509952 = -46413480;    int RXJFCMuFqSDjBmcyDuQX75772632 = -906146028;    int RXJFCMuFqSDjBmcyDuQX71902363 = 65609842;    int RXJFCMuFqSDjBmcyDuQX33689959 = -388855637;    int RXJFCMuFqSDjBmcyDuQX88541803 = -728509544;    int RXJFCMuFqSDjBmcyDuQX53327846 = -853015047;    int RXJFCMuFqSDjBmcyDuQX89660292 = -972857794;    int RXJFCMuFqSDjBmcyDuQX23986286 = -827601699;    int RXJFCMuFqSDjBmcyDuQX88624201 = -32238004;    int RXJFCMuFqSDjBmcyDuQX75080673 = -159966102;    int RXJFCMuFqSDjBmcyDuQX81480204 = 93949694;    int RXJFCMuFqSDjBmcyDuQX70039965 = -713999301;    int RXJFCMuFqSDjBmcyDuQX7601035 = -157064213;    int RXJFCMuFqSDjBmcyDuQX86324432 = 76104657;    int RXJFCMuFqSDjBmcyDuQX58986443 = -823052439;    int RXJFCMuFqSDjBmcyDuQX91393637 = -42212844;    int RXJFCMuFqSDjBmcyDuQX84533510 = -398605220;    int RXJFCMuFqSDjBmcyDuQX41850126 = -865119095;    int RXJFCMuFqSDjBmcyDuQX67011197 = 47543755;    int RXJFCMuFqSDjBmcyDuQX68000268 = -432554964;    int RXJFCMuFqSDjBmcyDuQX87598850 = -216587616;    int RXJFCMuFqSDjBmcyDuQX16055012 = -654943717;    int RXJFCMuFqSDjBmcyDuQX64990671 = -505211466;    int RXJFCMuFqSDjBmcyDuQX31880796 = -424215274;    int RXJFCMuFqSDjBmcyDuQX75934619 = -650054883;    int RXJFCMuFqSDjBmcyDuQX17758602 = -391340408;    int RXJFCMuFqSDjBmcyDuQX86118343 = -777740855;    int RXJFCMuFqSDjBmcyDuQX15277728 = -674063506;    int RXJFCMuFqSDjBmcyDuQX44878858 = -201660829;    int RXJFCMuFqSDjBmcyDuQX90123496 = -824287725;    int RXJFCMuFqSDjBmcyDuQX38155496 = -715965533;    int RXJFCMuFqSDjBmcyDuQX31061231 = -730988077;    int RXJFCMuFqSDjBmcyDuQX40722674 = -543518686;    int RXJFCMuFqSDjBmcyDuQX53071673 = -501477126;    int RXJFCMuFqSDjBmcyDuQX77217653 = -194382433;    int RXJFCMuFqSDjBmcyDuQX18977606 = -492186309;    int RXJFCMuFqSDjBmcyDuQX38041258 = -84092478;    int RXJFCMuFqSDjBmcyDuQX11041737 = -294330930;    int RXJFCMuFqSDjBmcyDuQX91501369 = -536189658;    int RXJFCMuFqSDjBmcyDuQX30822205 = -11020955;    int RXJFCMuFqSDjBmcyDuQX13204495 = -88410292;    int RXJFCMuFqSDjBmcyDuQX20442482 = -301201975;    int RXJFCMuFqSDjBmcyDuQX95050043 = -890328083;    int RXJFCMuFqSDjBmcyDuQX80208773 = -634999455;    int RXJFCMuFqSDjBmcyDuQX68489754 = -369559850;    int RXJFCMuFqSDjBmcyDuQX16006975 = -95848609;    int RXJFCMuFqSDjBmcyDuQX63061541 = -764719203;    int RXJFCMuFqSDjBmcyDuQX5980402 = -389954296;    int RXJFCMuFqSDjBmcyDuQX95567415 = -949047727;    int RXJFCMuFqSDjBmcyDuQX78033330 = -158047421;    int RXJFCMuFqSDjBmcyDuQX86854041 = -74353554;    int RXJFCMuFqSDjBmcyDuQX52816230 = -680195317;    int RXJFCMuFqSDjBmcyDuQX51861564 = -985501032;    int RXJFCMuFqSDjBmcyDuQX42774136 = 8674522;    int RXJFCMuFqSDjBmcyDuQX43523087 = -484633801;    int RXJFCMuFqSDjBmcyDuQX20454941 = -391469764;    int RXJFCMuFqSDjBmcyDuQX10781962 = -300934562;    int RXJFCMuFqSDjBmcyDuQX40021568 = -510174884;    int RXJFCMuFqSDjBmcyDuQX57755340 = -738800755;    int RXJFCMuFqSDjBmcyDuQX70783202 = -237169137;    int RXJFCMuFqSDjBmcyDuQX67209502 = 24725807;    int RXJFCMuFqSDjBmcyDuQX74382565 = -198794289;    int RXJFCMuFqSDjBmcyDuQX79107427 = -525940871;    int RXJFCMuFqSDjBmcyDuQX98500704 = -207950280;    int RXJFCMuFqSDjBmcyDuQX36925178 = -444000569;    int RXJFCMuFqSDjBmcyDuQX50418974 = -175062229;    int RXJFCMuFqSDjBmcyDuQX29317291 = -70480616;    int RXJFCMuFqSDjBmcyDuQX54529362 = -899825429;     RXJFCMuFqSDjBmcyDuQX81850754 = RXJFCMuFqSDjBmcyDuQX61981558;     RXJFCMuFqSDjBmcyDuQX61981558 = RXJFCMuFqSDjBmcyDuQX88421176;     RXJFCMuFqSDjBmcyDuQX88421176 = RXJFCMuFqSDjBmcyDuQX78423177;     RXJFCMuFqSDjBmcyDuQX78423177 = RXJFCMuFqSDjBmcyDuQX69831586;     RXJFCMuFqSDjBmcyDuQX69831586 = RXJFCMuFqSDjBmcyDuQX14305826;     RXJFCMuFqSDjBmcyDuQX14305826 = RXJFCMuFqSDjBmcyDuQX61511048;     RXJFCMuFqSDjBmcyDuQX61511048 = RXJFCMuFqSDjBmcyDuQX32821852;     RXJFCMuFqSDjBmcyDuQX32821852 = RXJFCMuFqSDjBmcyDuQX49933992;     RXJFCMuFqSDjBmcyDuQX49933992 = RXJFCMuFqSDjBmcyDuQX51497078;     RXJFCMuFqSDjBmcyDuQX51497078 = RXJFCMuFqSDjBmcyDuQX84193609;     RXJFCMuFqSDjBmcyDuQX84193609 = RXJFCMuFqSDjBmcyDuQX13727606;     RXJFCMuFqSDjBmcyDuQX13727606 = RXJFCMuFqSDjBmcyDuQX94750238;     RXJFCMuFqSDjBmcyDuQX94750238 = RXJFCMuFqSDjBmcyDuQX9943621;     RXJFCMuFqSDjBmcyDuQX9943621 = RXJFCMuFqSDjBmcyDuQX44731696;     RXJFCMuFqSDjBmcyDuQX44731696 = RXJFCMuFqSDjBmcyDuQX80043172;     RXJFCMuFqSDjBmcyDuQX80043172 = RXJFCMuFqSDjBmcyDuQX84150051;     RXJFCMuFqSDjBmcyDuQX84150051 = RXJFCMuFqSDjBmcyDuQX2864788;     RXJFCMuFqSDjBmcyDuQX2864788 = RXJFCMuFqSDjBmcyDuQX44428767;     RXJFCMuFqSDjBmcyDuQX44428767 = RXJFCMuFqSDjBmcyDuQX83674244;     RXJFCMuFqSDjBmcyDuQX83674244 = RXJFCMuFqSDjBmcyDuQX55289447;     RXJFCMuFqSDjBmcyDuQX55289447 = RXJFCMuFqSDjBmcyDuQX49969958;     RXJFCMuFqSDjBmcyDuQX49969958 = RXJFCMuFqSDjBmcyDuQX86046939;     RXJFCMuFqSDjBmcyDuQX86046939 = RXJFCMuFqSDjBmcyDuQX70662575;     RXJFCMuFqSDjBmcyDuQX70662575 = RXJFCMuFqSDjBmcyDuQX92304834;     RXJFCMuFqSDjBmcyDuQX92304834 = RXJFCMuFqSDjBmcyDuQX54553858;     RXJFCMuFqSDjBmcyDuQX54553858 = RXJFCMuFqSDjBmcyDuQX69426967;     RXJFCMuFqSDjBmcyDuQX69426967 = RXJFCMuFqSDjBmcyDuQX71387552;     RXJFCMuFqSDjBmcyDuQX71387552 = RXJFCMuFqSDjBmcyDuQX94666356;     RXJFCMuFqSDjBmcyDuQX94666356 = RXJFCMuFqSDjBmcyDuQX18872762;     RXJFCMuFqSDjBmcyDuQX18872762 = RXJFCMuFqSDjBmcyDuQX10774405;     RXJFCMuFqSDjBmcyDuQX10774405 = RXJFCMuFqSDjBmcyDuQX31121937;     RXJFCMuFqSDjBmcyDuQX31121937 = RXJFCMuFqSDjBmcyDuQX36509952;     RXJFCMuFqSDjBmcyDuQX36509952 = RXJFCMuFqSDjBmcyDuQX75772632;     RXJFCMuFqSDjBmcyDuQX75772632 = RXJFCMuFqSDjBmcyDuQX71902363;     RXJFCMuFqSDjBmcyDuQX71902363 = RXJFCMuFqSDjBmcyDuQX33689959;     RXJFCMuFqSDjBmcyDuQX33689959 = RXJFCMuFqSDjBmcyDuQX88541803;     RXJFCMuFqSDjBmcyDuQX88541803 = RXJFCMuFqSDjBmcyDuQX53327846;     RXJFCMuFqSDjBmcyDuQX53327846 = RXJFCMuFqSDjBmcyDuQX89660292;     RXJFCMuFqSDjBmcyDuQX89660292 = RXJFCMuFqSDjBmcyDuQX23986286;     RXJFCMuFqSDjBmcyDuQX23986286 = RXJFCMuFqSDjBmcyDuQX88624201;     RXJFCMuFqSDjBmcyDuQX88624201 = RXJFCMuFqSDjBmcyDuQX75080673;     RXJFCMuFqSDjBmcyDuQX75080673 = RXJFCMuFqSDjBmcyDuQX81480204;     RXJFCMuFqSDjBmcyDuQX81480204 = RXJFCMuFqSDjBmcyDuQX70039965;     RXJFCMuFqSDjBmcyDuQX70039965 = RXJFCMuFqSDjBmcyDuQX7601035;     RXJFCMuFqSDjBmcyDuQX7601035 = RXJFCMuFqSDjBmcyDuQX86324432;     RXJFCMuFqSDjBmcyDuQX86324432 = RXJFCMuFqSDjBmcyDuQX58986443;     RXJFCMuFqSDjBmcyDuQX58986443 = RXJFCMuFqSDjBmcyDuQX91393637;     RXJFCMuFqSDjBmcyDuQX91393637 = RXJFCMuFqSDjBmcyDuQX84533510;     RXJFCMuFqSDjBmcyDuQX84533510 = RXJFCMuFqSDjBmcyDuQX41850126;     RXJFCMuFqSDjBmcyDuQX41850126 = RXJFCMuFqSDjBmcyDuQX67011197;     RXJFCMuFqSDjBmcyDuQX67011197 = RXJFCMuFqSDjBmcyDuQX68000268;     RXJFCMuFqSDjBmcyDuQX68000268 = RXJFCMuFqSDjBmcyDuQX87598850;     RXJFCMuFqSDjBmcyDuQX87598850 = RXJFCMuFqSDjBmcyDuQX16055012;     RXJFCMuFqSDjBmcyDuQX16055012 = RXJFCMuFqSDjBmcyDuQX64990671;     RXJFCMuFqSDjBmcyDuQX64990671 = RXJFCMuFqSDjBmcyDuQX31880796;     RXJFCMuFqSDjBmcyDuQX31880796 = RXJFCMuFqSDjBmcyDuQX75934619;     RXJFCMuFqSDjBmcyDuQX75934619 = RXJFCMuFqSDjBmcyDuQX17758602;     RXJFCMuFqSDjBmcyDuQX17758602 = RXJFCMuFqSDjBmcyDuQX86118343;     RXJFCMuFqSDjBmcyDuQX86118343 = RXJFCMuFqSDjBmcyDuQX15277728;     RXJFCMuFqSDjBmcyDuQX15277728 = RXJFCMuFqSDjBmcyDuQX44878858;     RXJFCMuFqSDjBmcyDuQX44878858 = RXJFCMuFqSDjBmcyDuQX90123496;     RXJFCMuFqSDjBmcyDuQX90123496 = RXJFCMuFqSDjBmcyDuQX38155496;     RXJFCMuFqSDjBmcyDuQX38155496 = RXJFCMuFqSDjBmcyDuQX31061231;     RXJFCMuFqSDjBmcyDuQX31061231 = RXJFCMuFqSDjBmcyDuQX40722674;     RXJFCMuFqSDjBmcyDuQX40722674 = RXJFCMuFqSDjBmcyDuQX53071673;     RXJFCMuFqSDjBmcyDuQX53071673 = RXJFCMuFqSDjBmcyDuQX77217653;     RXJFCMuFqSDjBmcyDuQX77217653 = RXJFCMuFqSDjBmcyDuQX18977606;     RXJFCMuFqSDjBmcyDuQX18977606 = RXJFCMuFqSDjBmcyDuQX38041258;     RXJFCMuFqSDjBmcyDuQX38041258 = RXJFCMuFqSDjBmcyDuQX11041737;     RXJFCMuFqSDjBmcyDuQX11041737 = RXJFCMuFqSDjBmcyDuQX91501369;     RXJFCMuFqSDjBmcyDuQX91501369 = RXJFCMuFqSDjBmcyDuQX30822205;     RXJFCMuFqSDjBmcyDuQX30822205 = RXJFCMuFqSDjBmcyDuQX13204495;     RXJFCMuFqSDjBmcyDuQX13204495 = RXJFCMuFqSDjBmcyDuQX20442482;     RXJFCMuFqSDjBmcyDuQX20442482 = RXJFCMuFqSDjBmcyDuQX95050043;     RXJFCMuFqSDjBmcyDuQX95050043 = RXJFCMuFqSDjBmcyDuQX80208773;     RXJFCMuFqSDjBmcyDuQX80208773 = RXJFCMuFqSDjBmcyDuQX68489754;     RXJFCMuFqSDjBmcyDuQX68489754 = RXJFCMuFqSDjBmcyDuQX16006975;     RXJFCMuFqSDjBmcyDuQX16006975 = RXJFCMuFqSDjBmcyDuQX63061541;     RXJFCMuFqSDjBmcyDuQX63061541 = RXJFCMuFqSDjBmcyDuQX5980402;     RXJFCMuFqSDjBmcyDuQX5980402 = RXJFCMuFqSDjBmcyDuQX95567415;     RXJFCMuFqSDjBmcyDuQX95567415 = RXJFCMuFqSDjBmcyDuQX78033330;     RXJFCMuFqSDjBmcyDuQX78033330 = RXJFCMuFqSDjBmcyDuQX86854041;     RXJFCMuFqSDjBmcyDuQX86854041 = RXJFCMuFqSDjBmcyDuQX52816230;     RXJFCMuFqSDjBmcyDuQX52816230 = RXJFCMuFqSDjBmcyDuQX51861564;     RXJFCMuFqSDjBmcyDuQX51861564 = RXJFCMuFqSDjBmcyDuQX42774136;     RXJFCMuFqSDjBmcyDuQX42774136 = RXJFCMuFqSDjBmcyDuQX43523087;     RXJFCMuFqSDjBmcyDuQX43523087 = RXJFCMuFqSDjBmcyDuQX20454941;     RXJFCMuFqSDjBmcyDuQX20454941 = RXJFCMuFqSDjBmcyDuQX10781962;     RXJFCMuFqSDjBmcyDuQX10781962 = RXJFCMuFqSDjBmcyDuQX40021568;     RXJFCMuFqSDjBmcyDuQX40021568 = RXJFCMuFqSDjBmcyDuQX57755340;     RXJFCMuFqSDjBmcyDuQX57755340 = RXJFCMuFqSDjBmcyDuQX70783202;     RXJFCMuFqSDjBmcyDuQX70783202 = RXJFCMuFqSDjBmcyDuQX67209502;     RXJFCMuFqSDjBmcyDuQX67209502 = RXJFCMuFqSDjBmcyDuQX74382565;     RXJFCMuFqSDjBmcyDuQX74382565 = RXJFCMuFqSDjBmcyDuQX79107427;     RXJFCMuFqSDjBmcyDuQX79107427 = RXJFCMuFqSDjBmcyDuQX98500704;     RXJFCMuFqSDjBmcyDuQX98500704 = RXJFCMuFqSDjBmcyDuQX36925178;     RXJFCMuFqSDjBmcyDuQX36925178 = RXJFCMuFqSDjBmcyDuQX50418974;     RXJFCMuFqSDjBmcyDuQX50418974 = RXJFCMuFqSDjBmcyDuQX29317291;     RXJFCMuFqSDjBmcyDuQX29317291 = RXJFCMuFqSDjBmcyDuQX54529362;     RXJFCMuFqSDjBmcyDuQX54529362 = RXJFCMuFqSDjBmcyDuQX81850754;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ToBCfmRmpzWdFDMaBVPT26689258() {     int sZRbQrnhuNoprHiLrqGc15939545 = -129223831;    int sZRbQrnhuNoprHiLrqGc17334339 = -148725580;    int sZRbQrnhuNoprHiLrqGc59666754 = -976364128;    int sZRbQrnhuNoprHiLrqGc38548582 = -575167779;    int sZRbQrnhuNoprHiLrqGc23556384 = -122937729;    int sZRbQrnhuNoprHiLrqGc47131384 = -445292662;    int sZRbQrnhuNoprHiLrqGc68772885 = -15812241;    int sZRbQrnhuNoprHiLrqGc73134010 = -122322032;    int sZRbQrnhuNoprHiLrqGc62041802 = -622575752;    int sZRbQrnhuNoprHiLrqGc12042055 = -211534124;    int sZRbQrnhuNoprHiLrqGc73727432 = -915971058;    int sZRbQrnhuNoprHiLrqGc22097188 = -455398109;    int sZRbQrnhuNoprHiLrqGc40892848 = -607855930;    int sZRbQrnhuNoprHiLrqGc73976341 = -307271311;    int sZRbQrnhuNoprHiLrqGc5613801 = -77388559;    int sZRbQrnhuNoprHiLrqGc38201950 = -89086576;    int sZRbQrnhuNoprHiLrqGc86007319 = -184878097;    int sZRbQrnhuNoprHiLrqGc44493732 = -628676179;    int sZRbQrnhuNoprHiLrqGc70584324 = -123566191;    int sZRbQrnhuNoprHiLrqGc65515401 = -267494724;    int sZRbQrnhuNoprHiLrqGc91202997 = -685687302;    int sZRbQrnhuNoprHiLrqGc95932654 = 87738664;    int sZRbQrnhuNoprHiLrqGc82307944 = -964121028;    int sZRbQrnhuNoprHiLrqGc57674403 = -271889644;    int sZRbQrnhuNoprHiLrqGc11421354 = -37669412;    int sZRbQrnhuNoprHiLrqGc10702678 = -215566699;    int sZRbQrnhuNoprHiLrqGc88275219 = -324390151;    int sZRbQrnhuNoprHiLrqGc56164004 = -824429972;    int sZRbQrnhuNoprHiLrqGc11435546 = -726674163;    int sZRbQrnhuNoprHiLrqGc21471763 = -471956336;    int sZRbQrnhuNoprHiLrqGc79361374 = 79216108;    int sZRbQrnhuNoprHiLrqGc21615220 = -236895071;    int sZRbQrnhuNoprHiLrqGc53989197 = -764523024;    int sZRbQrnhuNoprHiLrqGc36558692 = -701640523;    int sZRbQrnhuNoprHiLrqGc63895690 = -65105347;    int sZRbQrnhuNoprHiLrqGc26577798 = -432495102;    int sZRbQrnhuNoprHiLrqGc34441091 = -187131489;    int sZRbQrnhuNoprHiLrqGc75857735 = 98743571;    int sZRbQrnhuNoprHiLrqGc20693263 = -469702988;    int sZRbQrnhuNoprHiLrqGc52296343 = -884870714;    int sZRbQrnhuNoprHiLrqGc15060157 = -157205558;    int sZRbQrnhuNoprHiLrqGc27395453 = -518756325;    int sZRbQrnhuNoprHiLrqGc83564759 = -615975017;    int sZRbQrnhuNoprHiLrqGc96790813 = -676669130;    int sZRbQrnhuNoprHiLrqGc91331513 = -601252143;    int sZRbQrnhuNoprHiLrqGc47269461 = -537536452;    int sZRbQrnhuNoprHiLrqGc81722740 = 64792076;    int sZRbQrnhuNoprHiLrqGc12229367 = -56693116;    int sZRbQrnhuNoprHiLrqGc17657114 = -736376312;    int sZRbQrnhuNoprHiLrqGc83493792 = -883071464;    int sZRbQrnhuNoprHiLrqGc60577524 = -688157876;    int sZRbQrnhuNoprHiLrqGc13272084 = -648656944;    int sZRbQrnhuNoprHiLrqGc98076887 = -303491890;    int sZRbQrnhuNoprHiLrqGc84766540 = 72706190;    int sZRbQrnhuNoprHiLrqGc35486260 = -882900392;    int sZRbQrnhuNoprHiLrqGc20006890 = -116962495;    int sZRbQrnhuNoprHiLrqGc35026394 = -184604553;    int sZRbQrnhuNoprHiLrqGc1992351 = -604474485;    int sZRbQrnhuNoprHiLrqGc27127229 = -437498368;    int sZRbQrnhuNoprHiLrqGc12853706 = -907371030;    int sZRbQrnhuNoprHiLrqGc58856164 = -20902512;    int sZRbQrnhuNoprHiLrqGc12608882 = -191382269;    int sZRbQrnhuNoprHiLrqGc61698464 = -395647870;    int sZRbQrnhuNoprHiLrqGc40570040 = -50619417;    int sZRbQrnhuNoprHiLrqGc32680680 = -190750233;    int sZRbQrnhuNoprHiLrqGc52112213 = -579075987;    int sZRbQrnhuNoprHiLrqGc68107990 = -690875086;    int sZRbQrnhuNoprHiLrqGc4334157 = -906215408;    int sZRbQrnhuNoprHiLrqGc10080651 = -142165964;    int sZRbQrnhuNoprHiLrqGc79036002 = -644893458;    int sZRbQrnhuNoprHiLrqGc3760859 = -901955087;    int sZRbQrnhuNoprHiLrqGc10149584 = -183621669;    int sZRbQrnhuNoprHiLrqGc23800469 = -58973192;    int sZRbQrnhuNoprHiLrqGc18287982 = -238695477;    int sZRbQrnhuNoprHiLrqGc50455245 = -10289167;    int sZRbQrnhuNoprHiLrqGc63807544 = -66930978;    int sZRbQrnhuNoprHiLrqGc12367896 = -296286319;    int sZRbQrnhuNoprHiLrqGc85517131 = -187451899;    int sZRbQrnhuNoprHiLrqGc66342889 = -670637501;    int sZRbQrnhuNoprHiLrqGc64151893 = -500132960;    int sZRbQrnhuNoprHiLrqGc28979938 = -180358776;    int sZRbQrnhuNoprHiLrqGc76045853 = -167697035;    int sZRbQrnhuNoprHiLrqGc38506890 = 11946340;    int sZRbQrnhuNoprHiLrqGc27941753 = -843602700;    int sZRbQrnhuNoprHiLrqGc60894238 = -783798460;    int sZRbQrnhuNoprHiLrqGc66089291 = -272126949;    int sZRbQrnhuNoprHiLrqGc23538332 = -933403181;    int sZRbQrnhuNoprHiLrqGc69222657 = -737229214;    int sZRbQrnhuNoprHiLrqGc1072432 = -818740132;    int sZRbQrnhuNoprHiLrqGc43888800 = -948142852;    int sZRbQrnhuNoprHiLrqGc91551403 = -147890550;    int sZRbQrnhuNoprHiLrqGc32448741 = -582657004;    int sZRbQrnhuNoprHiLrqGc48730507 = -463758061;    int sZRbQrnhuNoprHiLrqGc7839558 = -562331958;    int sZRbQrnhuNoprHiLrqGc93440178 = -763968203;    int sZRbQrnhuNoprHiLrqGc2451276 = -965823289;    int sZRbQrnhuNoprHiLrqGc65696989 = -23108455;    int sZRbQrnhuNoprHiLrqGc42994720 = -465355601;    int sZRbQrnhuNoprHiLrqGc64110133 = -385918898;    int sZRbQrnhuNoprHiLrqGc39219301 = -129223831;     sZRbQrnhuNoprHiLrqGc15939545 = sZRbQrnhuNoprHiLrqGc17334339;     sZRbQrnhuNoprHiLrqGc17334339 = sZRbQrnhuNoprHiLrqGc59666754;     sZRbQrnhuNoprHiLrqGc59666754 = sZRbQrnhuNoprHiLrqGc38548582;     sZRbQrnhuNoprHiLrqGc38548582 = sZRbQrnhuNoprHiLrqGc23556384;     sZRbQrnhuNoprHiLrqGc23556384 = sZRbQrnhuNoprHiLrqGc47131384;     sZRbQrnhuNoprHiLrqGc47131384 = sZRbQrnhuNoprHiLrqGc68772885;     sZRbQrnhuNoprHiLrqGc68772885 = sZRbQrnhuNoprHiLrqGc73134010;     sZRbQrnhuNoprHiLrqGc73134010 = sZRbQrnhuNoprHiLrqGc62041802;     sZRbQrnhuNoprHiLrqGc62041802 = sZRbQrnhuNoprHiLrqGc12042055;     sZRbQrnhuNoprHiLrqGc12042055 = sZRbQrnhuNoprHiLrqGc73727432;     sZRbQrnhuNoprHiLrqGc73727432 = sZRbQrnhuNoprHiLrqGc22097188;     sZRbQrnhuNoprHiLrqGc22097188 = sZRbQrnhuNoprHiLrqGc40892848;     sZRbQrnhuNoprHiLrqGc40892848 = sZRbQrnhuNoprHiLrqGc73976341;     sZRbQrnhuNoprHiLrqGc73976341 = sZRbQrnhuNoprHiLrqGc5613801;     sZRbQrnhuNoprHiLrqGc5613801 = sZRbQrnhuNoprHiLrqGc38201950;     sZRbQrnhuNoprHiLrqGc38201950 = sZRbQrnhuNoprHiLrqGc86007319;     sZRbQrnhuNoprHiLrqGc86007319 = sZRbQrnhuNoprHiLrqGc44493732;     sZRbQrnhuNoprHiLrqGc44493732 = sZRbQrnhuNoprHiLrqGc70584324;     sZRbQrnhuNoprHiLrqGc70584324 = sZRbQrnhuNoprHiLrqGc65515401;     sZRbQrnhuNoprHiLrqGc65515401 = sZRbQrnhuNoprHiLrqGc91202997;     sZRbQrnhuNoprHiLrqGc91202997 = sZRbQrnhuNoprHiLrqGc95932654;     sZRbQrnhuNoprHiLrqGc95932654 = sZRbQrnhuNoprHiLrqGc82307944;     sZRbQrnhuNoprHiLrqGc82307944 = sZRbQrnhuNoprHiLrqGc57674403;     sZRbQrnhuNoprHiLrqGc57674403 = sZRbQrnhuNoprHiLrqGc11421354;     sZRbQrnhuNoprHiLrqGc11421354 = sZRbQrnhuNoprHiLrqGc10702678;     sZRbQrnhuNoprHiLrqGc10702678 = sZRbQrnhuNoprHiLrqGc88275219;     sZRbQrnhuNoprHiLrqGc88275219 = sZRbQrnhuNoprHiLrqGc56164004;     sZRbQrnhuNoprHiLrqGc56164004 = sZRbQrnhuNoprHiLrqGc11435546;     sZRbQrnhuNoprHiLrqGc11435546 = sZRbQrnhuNoprHiLrqGc21471763;     sZRbQrnhuNoprHiLrqGc21471763 = sZRbQrnhuNoprHiLrqGc79361374;     sZRbQrnhuNoprHiLrqGc79361374 = sZRbQrnhuNoprHiLrqGc21615220;     sZRbQrnhuNoprHiLrqGc21615220 = sZRbQrnhuNoprHiLrqGc53989197;     sZRbQrnhuNoprHiLrqGc53989197 = sZRbQrnhuNoprHiLrqGc36558692;     sZRbQrnhuNoprHiLrqGc36558692 = sZRbQrnhuNoprHiLrqGc63895690;     sZRbQrnhuNoprHiLrqGc63895690 = sZRbQrnhuNoprHiLrqGc26577798;     sZRbQrnhuNoprHiLrqGc26577798 = sZRbQrnhuNoprHiLrqGc34441091;     sZRbQrnhuNoprHiLrqGc34441091 = sZRbQrnhuNoprHiLrqGc75857735;     sZRbQrnhuNoprHiLrqGc75857735 = sZRbQrnhuNoprHiLrqGc20693263;     sZRbQrnhuNoprHiLrqGc20693263 = sZRbQrnhuNoprHiLrqGc52296343;     sZRbQrnhuNoprHiLrqGc52296343 = sZRbQrnhuNoprHiLrqGc15060157;     sZRbQrnhuNoprHiLrqGc15060157 = sZRbQrnhuNoprHiLrqGc27395453;     sZRbQrnhuNoprHiLrqGc27395453 = sZRbQrnhuNoprHiLrqGc83564759;     sZRbQrnhuNoprHiLrqGc83564759 = sZRbQrnhuNoprHiLrqGc96790813;     sZRbQrnhuNoprHiLrqGc96790813 = sZRbQrnhuNoprHiLrqGc91331513;     sZRbQrnhuNoprHiLrqGc91331513 = sZRbQrnhuNoprHiLrqGc47269461;     sZRbQrnhuNoprHiLrqGc47269461 = sZRbQrnhuNoprHiLrqGc81722740;     sZRbQrnhuNoprHiLrqGc81722740 = sZRbQrnhuNoprHiLrqGc12229367;     sZRbQrnhuNoprHiLrqGc12229367 = sZRbQrnhuNoprHiLrqGc17657114;     sZRbQrnhuNoprHiLrqGc17657114 = sZRbQrnhuNoprHiLrqGc83493792;     sZRbQrnhuNoprHiLrqGc83493792 = sZRbQrnhuNoprHiLrqGc60577524;     sZRbQrnhuNoprHiLrqGc60577524 = sZRbQrnhuNoprHiLrqGc13272084;     sZRbQrnhuNoprHiLrqGc13272084 = sZRbQrnhuNoprHiLrqGc98076887;     sZRbQrnhuNoprHiLrqGc98076887 = sZRbQrnhuNoprHiLrqGc84766540;     sZRbQrnhuNoprHiLrqGc84766540 = sZRbQrnhuNoprHiLrqGc35486260;     sZRbQrnhuNoprHiLrqGc35486260 = sZRbQrnhuNoprHiLrqGc20006890;     sZRbQrnhuNoprHiLrqGc20006890 = sZRbQrnhuNoprHiLrqGc35026394;     sZRbQrnhuNoprHiLrqGc35026394 = sZRbQrnhuNoprHiLrqGc1992351;     sZRbQrnhuNoprHiLrqGc1992351 = sZRbQrnhuNoprHiLrqGc27127229;     sZRbQrnhuNoprHiLrqGc27127229 = sZRbQrnhuNoprHiLrqGc12853706;     sZRbQrnhuNoprHiLrqGc12853706 = sZRbQrnhuNoprHiLrqGc58856164;     sZRbQrnhuNoprHiLrqGc58856164 = sZRbQrnhuNoprHiLrqGc12608882;     sZRbQrnhuNoprHiLrqGc12608882 = sZRbQrnhuNoprHiLrqGc61698464;     sZRbQrnhuNoprHiLrqGc61698464 = sZRbQrnhuNoprHiLrqGc40570040;     sZRbQrnhuNoprHiLrqGc40570040 = sZRbQrnhuNoprHiLrqGc32680680;     sZRbQrnhuNoprHiLrqGc32680680 = sZRbQrnhuNoprHiLrqGc52112213;     sZRbQrnhuNoprHiLrqGc52112213 = sZRbQrnhuNoprHiLrqGc68107990;     sZRbQrnhuNoprHiLrqGc68107990 = sZRbQrnhuNoprHiLrqGc4334157;     sZRbQrnhuNoprHiLrqGc4334157 = sZRbQrnhuNoprHiLrqGc10080651;     sZRbQrnhuNoprHiLrqGc10080651 = sZRbQrnhuNoprHiLrqGc79036002;     sZRbQrnhuNoprHiLrqGc79036002 = sZRbQrnhuNoprHiLrqGc3760859;     sZRbQrnhuNoprHiLrqGc3760859 = sZRbQrnhuNoprHiLrqGc10149584;     sZRbQrnhuNoprHiLrqGc10149584 = sZRbQrnhuNoprHiLrqGc23800469;     sZRbQrnhuNoprHiLrqGc23800469 = sZRbQrnhuNoprHiLrqGc18287982;     sZRbQrnhuNoprHiLrqGc18287982 = sZRbQrnhuNoprHiLrqGc50455245;     sZRbQrnhuNoprHiLrqGc50455245 = sZRbQrnhuNoprHiLrqGc63807544;     sZRbQrnhuNoprHiLrqGc63807544 = sZRbQrnhuNoprHiLrqGc12367896;     sZRbQrnhuNoprHiLrqGc12367896 = sZRbQrnhuNoprHiLrqGc85517131;     sZRbQrnhuNoprHiLrqGc85517131 = sZRbQrnhuNoprHiLrqGc66342889;     sZRbQrnhuNoprHiLrqGc66342889 = sZRbQrnhuNoprHiLrqGc64151893;     sZRbQrnhuNoprHiLrqGc64151893 = sZRbQrnhuNoprHiLrqGc28979938;     sZRbQrnhuNoprHiLrqGc28979938 = sZRbQrnhuNoprHiLrqGc76045853;     sZRbQrnhuNoprHiLrqGc76045853 = sZRbQrnhuNoprHiLrqGc38506890;     sZRbQrnhuNoprHiLrqGc38506890 = sZRbQrnhuNoprHiLrqGc27941753;     sZRbQrnhuNoprHiLrqGc27941753 = sZRbQrnhuNoprHiLrqGc60894238;     sZRbQrnhuNoprHiLrqGc60894238 = sZRbQrnhuNoprHiLrqGc66089291;     sZRbQrnhuNoprHiLrqGc66089291 = sZRbQrnhuNoprHiLrqGc23538332;     sZRbQrnhuNoprHiLrqGc23538332 = sZRbQrnhuNoprHiLrqGc69222657;     sZRbQrnhuNoprHiLrqGc69222657 = sZRbQrnhuNoprHiLrqGc1072432;     sZRbQrnhuNoprHiLrqGc1072432 = sZRbQrnhuNoprHiLrqGc43888800;     sZRbQrnhuNoprHiLrqGc43888800 = sZRbQrnhuNoprHiLrqGc91551403;     sZRbQrnhuNoprHiLrqGc91551403 = sZRbQrnhuNoprHiLrqGc32448741;     sZRbQrnhuNoprHiLrqGc32448741 = sZRbQrnhuNoprHiLrqGc48730507;     sZRbQrnhuNoprHiLrqGc48730507 = sZRbQrnhuNoprHiLrqGc7839558;     sZRbQrnhuNoprHiLrqGc7839558 = sZRbQrnhuNoprHiLrqGc93440178;     sZRbQrnhuNoprHiLrqGc93440178 = sZRbQrnhuNoprHiLrqGc2451276;     sZRbQrnhuNoprHiLrqGc2451276 = sZRbQrnhuNoprHiLrqGc65696989;     sZRbQrnhuNoprHiLrqGc65696989 = sZRbQrnhuNoprHiLrqGc42994720;     sZRbQrnhuNoprHiLrqGc42994720 = sZRbQrnhuNoprHiLrqGc64110133;     sZRbQrnhuNoprHiLrqGc64110133 = sZRbQrnhuNoprHiLrqGc39219301;     sZRbQrnhuNoprHiLrqGc39219301 = sZRbQrnhuNoprHiLrqGc15939545;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void qJwphRZaSFPiPmuEYsGE85340928() {     int GVJWjkZWUKkBJxpBQdwP79370197 = -812997244;    int GVJWjkZWUKkBJxpBQdwP42585918 = 91499806;    int GVJWjkZWUKkBJxpBQdwP52256624 = 45561412;    int GVJWjkZWUKkBJxpBQdwP43822560 = -589147506;    int GVJWjkZWUKkBJxpBQdwP64670962 = -484660760;    int GVJWjkZWUKkBJxpBQdwP24762612 = -286515892;    int GVJWjkZWUKkBJxpBQdwP26203163 = -440138741;    int GVJWjkZWUKkBJxpBQdwP25817838 = -440724348;    int GVJWjkZWUKkBJxpBQdwP11024722 = -96504893;    int GVJWjkZWUKkBJxpBQdwP46260084 = -791380642;    int GVJWjkZWUKkBJxpBQdwP41332699 = -353813716;    int GVJWjkZWUKkBJxpBQdwP99211876 = 10285124;    int GVJWjkZWUKkBJxpBQdwP39060985 = -929500941;    int GVJWjkZWUKkBJxpBQdwP8006248 = 59815172;    int GVJWjkZWUKkBJxpBQdwP28947994 = -284553399;    int GVJWjkZWUKkBJxpBQdwP38396635 = -676286014;    int GVJWjkZWUKkBJxpBQdwP49864520 = -416149631;    int GVJWjkZWUKkBJxpBQdwP88829715 = -503989699;    int GVJWjkZWUKkBJxpBQdwP35460371 = -29123032;    int GVJWjkZWUKkBJxpBQdwP60643612 = -682081509;    int GVJWjkZWUKkBJxpBQdwP51650218 = -278196281;    int GVJWjkZWUKkBJxpBQdwP58795852 = -727619095;    int GVJWjkZWUKkBJxpBQdwP13503538 = -909508844;    int GVJWjkZWUKkBJxpBQdwP54486802 = -910515488;    int GVJWjkZWUKkBJxpBQdwP94856174 = -412278350;    int GVJWjkZWUKkBJxpBQdwP52827994 = -568954716;    int GVJWjkZWUKkBJxpBQdwP69084267 = -254141975;    int GVJWjkZWUKkBJxpBQdwP68561755 = -79744391;    int GVJWjkZWUKkBJxpBQdwP8740695 = -159746013;    int GVJWjkZWUKkBJxpBQdwP60988538 = -122991699;    int GVJWjkZWUKkBJxpBQdwP74289027 = 43653108;    int GVJWjkZWUKkBJxpBQdwP52320898 = -836093324;    int GVJWjkZWUKkBJxpBQdwP73975505 = -637615175;    int GVJWjkZWUKkBJxpBQdwP32262614 = -810287066;    int GVJWjkZWUKkBJxpBQdwP60222818 = -608187682;    int GVJWjkZWUKkBJxpBQdwP2300567 = -476673764;    int GVJWjkZWUKkBJxpBQdwP48490260 = -656813605;    int GVJWjkZWUKkBJxpBQdwP15512816 = -260647382;    int GVJWjkZWUKkBJxpBQdwP8506541 = -723500961;    int GVJWjkZWUKkBJxpBQdwP57375995 = 66036947;    int GVJWjkZWUKkBJxpBQdwP69278496 = 13901868;    int GVJWjkZWUKkBJxpBQdwP36924903 = -201949560;    int GVJWjkZWUKkBJxpBQdwP96965714 = -798275931;    int GVJWjkZWUKkBJxpBQdwP76265259 = -124328430;    int GVJWjkZWUKkBJxpBQdwP63125987 = -110053080;    int GVJWjkZWUKkBJxpBQdwP42236622 = -104885077;    int GVJWjkZWUKkBJxpBQdwP6025627 = -512574231;    int GVJWjkZWUKkBJxpBQdwP42834466 = -227804450;    int GVJWjkZWUKkBJxpBQdwP63997654 = -652147296;    int GVJWjkZWUKkBJxpBQdwP36755396 = -205039841;    int GVJWjkZWUKkBJxpBQdwP99213896 = -355842394;    int GVJWjkZWUKkBJxpBQdwP31219652 = -177684950;    int GVJWjkZWUKkBJxpBQdwP28927617 = -354652131;    int GVJWjkZWUKkBJxpBQdwP73501035 = -170682259;    int GVJWjkZWUKkBJxpBQdwP33690711 = -906547680;    int GVJWjkZWUKkBJxpBQdwP20574346 = 14621850;    int GVJWjkZWUKkBJxpBQdwP29082381 = 1008649;    int GVJWjkZWUKkBJxpBQdwP97769821 = -43923100;    int GVJWjkZWUKkBJxpBQdwP48966386 = -76869156;    int GVJWjkZWUKkBJxpBQdwP11842968 = -915706045;    int GVJWjkZWUKkBJxpBQdwP55678345 = 67626082;    int GVJWjkZWUKkBJxpBQdwP57641407 = -260394351;    int GVJWjkZWUKkBJxpBQdwP17077144 = -180978335;    int GVJWjkZWUKkBJxpBQdwP50036184 = -973513194;    int GVJWjkZWUKkBJxpBQdwP71971056 = -735033750;    int GVJWjkZWUKkBJxpBQdwP89011801 = -517720392;    int GVJWjkZWUKkBJxpBQdwP25236371 = -352099701;    int GVJWjkZWUKkBJxpBQdwP6798372 = -19213875;    int GVJWjkZWUKkBJxpBQdwP47783429 = -331997146;    int GVJWjkZWUKkBJxpBQdwP26647427 = -807879636;    int GVJWjkZWUKkBJxpBQdwP89906374 = 80527590;    int GVJWjkZWUKkBJxpBQdwP34351705 = -55502250;    int GVJWjkZWUKkBJxpBQdwP80323174 = -780488739;    int GVJWjkZWUKkBJxpBQdwP78084376 = 4840021;    int GVJWjkZWUKkBJxpBQdwP91365116 = -595983378;    int GVJWjkZWUKkBJxpBQdwP14725315 = 23753279;    int GVJWjkZWUKkBJxpBQdwP61830138 = -929343164;    int GVJWjkZWUKkBJxpBQdwP37238278 = -685180414;    int GVJWjkZWUKkBJxpBQdwP91360815 = -700462409;    int GVJWjkZWUKkBJxpBQdwP52619552 = -207393273;    int GVJWjkZWUKkBJxpBQdwP46802368 = 43619515;    int GVJWjkZWUKkBJxpBQdwP26249801 = 73662475;    int GVJWjkZWUKkBJxpBQdwP4564102 = -427597095;    int GVJWjkZWUKkBJxpBQdwP71985298 = -954706173;    int GVJWjkZWUKkBJxpBQdwP61774641 = -767149305;    int GVJWjkZWUKkBJxpBQdwP43069375 = -778661942;    int GVJWjkZWUKkBJxpBQdwP23393281 = -381441194;    int GVJWjkZWUKkBJxpBQdwP474471 = -366932917;    int GVJWjkZWUKkBJxpBQdwP98571902 = -903739386;    int GVJWjkZWUKkBJxpBQdwP39648473 = -522809533;    int GVJWjkZWUKkBJxpBQdwP73218186 = -377682414;    int GVJWjkZWUKkBJxpBQdwP50720439 = -512890505;    int GVJWjkZWUKkBJxpBQdwP66546429 = -83778226;    int GVJWjkZWUKkBJxpBQdwP96663573 = -807794917;    int GVJWjkZWUKkBJxpBQdwP1697650 = 98410864;    int GVJWjkZWUKkBJxpBQdwP11637089 = -725703782;    int GVJWjkZWUKkBJxpBQdwP19847760 = 79028775;    int GVJWjkZWUKkBJxpBQdwP46929531 = -824762738;    int GVJWjkZWUKkBJxpBQdwP4294203 = -389294680;    int GVJWjkZWUKkBJxpBQdwP74114185 = -812997244;     GVJWjkZWUKkBJxpBQdwP79370197 = GVJWjkZWUKkBJxpBQdwP42585918;     GVJWjkZWUKkBJxpBQdwP42585918 = GVJWjkZWUKkBJxpBQdwP52256624;     GVJWjkZWUKkBJxpBQdwP52256624 = GVJWjkZWUKkBJxpBQdwP43822560;     GVJWjkZWUKkBJxpBQdwP43822560 = GVJWjkZWUKkBJxpBQdwP64670962;     GVJWjkZWUKkBJxpBQdwP64670962 = GVJWjkZWUKkBJxpBQdwP24762612;     GVJWjkZWUKkBJxpBQdwP24762612 = GVJWjkZWUKkBJxpBQdwP26203163;     GVJWjkZWUKkBJxpBQdwP26203163 = GVJWjkZWUKkBJxpBQdwP25817838;     GVJWjkZWUKkBJxpBQdwP25817838 = GVJWjkZWUKkBJxpBQdwP11024722;     GVJWjkZWUKkBJxpBQdwP11024722 = GVJWjkZWUKkBJxpBQdwP46260084;     GVJWjkZWUKkBJxpBQdwP46260084 = GVJWjkZWUKkBJxpBQdwP41332699;     GVJWjkZWUKkBJxpBQdwP41332699 = GVJWjkZWUKkBJxpBQdwP99211876;     GVJWjkZWUKkBJxpBQdwP99211876 = GVJWjkZWUKkBJxpBQdwP39060985;     GVJWjkZWUKkBJxpBQdwP39060985 = GVJWjkZWUKkBJxpBQdwP8006248;     GVJWjkZWUKkBJxpBQdwP8006248 = GVJWjkZWUKkBJxpBQdwP28947994;     GVJWjkZWUKkBJxpBQdwP28947994 = GVJWjkZWUKkBJxpBQdwP38396635;     GVJWjkZWUKkBJxpBQdwP38396635 = GVJWjkZWUKkBJxpBQdwP49864520;     GVJWjkZWUKkBJxpBQdwP49864520 = GVJWjkZWUKkBJxpBQdwP88829715;     GVJWjkZWUKkBJxpBQdwP88829715 = GVJWjkZWUKkBJxpBQdwP35460371;     GVJWjkZWUKkBJxpBQdwP35460371 = GVJWjkZWUKkBJxpBQdwP60643612;     GVJWjkZWUKkBJxpBQdwP60643612 = GVJWjkZWUKkBJxpBQdwP51650218;     GVJWjkZWUKkBJxpBQdwP51650218 = GVJWjkZWUKkBJxpBQdwP58795852;     GVJWjkZWUKkBJxpBQdwP58795852 = GVJWjkZWUKkBJxpBQdwP13503538;     GVJWjkZWUKkBJxpBQdwP13503538 = GVJWjkZWUKkBJxpBQdwP54486802;     GVJWjkZWUKkBJxpBQdwP54486802 = GVJWjkZWUKkBJxpBQdwP94856174;     GVJWjkZWUKkBJxpBQdwP94856174 = GVJWjkZWUKkBJxpBQdwP52827994;     GVJWjkZWUKkBJxpBQdwP52827994 = GVJWjkZWUKkBJxpBQdwP69084267;     GVJWjkZWUKkBJxpBQdwP69084267 = GVJWjkZWUKkBJxpBQdwP68561755;     GVJWjkZWUKkBJxpBQdwP68561755 = GVJWjkZWUKkBJxpBQdwP8740695;     GVJWjkZWUKkBJxpBQdwP8740695 = GVJWjkZWUKkBJxpBQdwP60988538;     GVJWjkZWUKkBJxpBQdwP60988538 = GVJWjkZWUKkBJxpBQdwP74289027;     GVJWjkZWUKkBJxpBQdwP74289027 = GVJWjkZWUKkBJxpBQdwP52320898;     GVJWjkZWUKkBJxpBQdwP52320898 = GVJWjkZWUKkBJxpBQdwP73975505;     GVJWjkZWUKkBJxpBQdwP73975505 = GVJWjkZWUKkBJxpBQdwP32262614;     GVJWjkZWUKkBJxpBQdwP32262614 = GVJWjkZWUKkBJxpBQdwP60222818;     GVJWjkZWUKkBJxpBQdwP60222818 = GVJWjkZWUKkBJxpBQdwP2300567;     GVJWjkZWUKkBJxpBQdwP2300567 = GVJWjkZWUKkBJxpBQdwP48490260;     GVJWjkZWUKkBJxpBQdwP48490260 = GVJWjkZWUKkBJxpBQdwP15512816;     GVJWjkZWUKkBJxpBQdwP15512816 = GVJWjkZWUKkBJxpBQdwP8506541;     GVJWjkZWUKkBJxpBQdwP8506541 = GVJWjkZWUKkBJxpBQdwP57375995;     GVJWjkZWUKkBJxpBQdwP57375995 = GVJWjkZWUKkBJxpBQdwP69278496;     GVJWjkZWUKkBJxpBQdwP69278496 = GVJWjkZWUKkBJxpBQdwP36924903;     GVJWjkZWUKkBJxpBQdwP36924903 = GVJWjkZWUKkBJxpBQdwP96965714;     GVJWjkZWUKkBJxpBQdwP96965714 = GVJWjkZWUKkBJxpBQdwP76265259;     GVJWjkZWUKkBJxpBQdwP76265259 = GVJWjkZWUKkBJxpBQdwP63125987;     GVJWjkZWUKkBJxpBQdwP63125987 = GVJWjkZWUKkBJxpBQdwP42236622;     GVJWjkZWUKkBJxpBQdwP42236622 = GVJWjkZWUKkBJxpBQdwP6025627;     GVJWjkZWUKkBJxpBQdwP6025627 = GVJWjkZWUKkBJxpBQdwP42834466;     GVJWjkZWUKkBJxpBQdwP42834466 = GVJWjkZWUKkBJxpBQdwP63997654;     GVJWjkZWUKkBJxpBQdwP63997654 = GVJWjkZWUKkBJxpBQdwP36755396;     GVJWjkZWUKkBJxpBQdwP36755396 = GVJWjkZWUKkBJxpBQdwP99213896;     GVJWjkZWUKkBJxpBQdwP99213896 = GVJWjkZWUKkBJxpBQdwP31219652;     GVJWjkZWUKkBJxpBQdwP31219652 = GVJWjkZWUKkBJxpBQdwP28927617;     GVJWjkZWUKkBJxpBQdwP28927617 = GVJWjkZWUKkBJxpBQdwP73501035;     GVJWjkZWUKkBJxpBQdwP73501035 = GVJWjkZWUKkBJxpBQdwP33690711;     GVJWjkZWUKkBJxpBQdwP33690711 = GVJWjkZWUKkBJxpBQdwP20574346;     GVJWjkZWUKkBJxpBQdwP20574346 = GVJWjkZWUKkBJxpBQdwP29082381;     GVJWjkZWUKkBJxpBQdwP29082381 = GVJWjkZWUKkBJxpBQdwP97769821;     GVJWjkZWUKkBJxpBQdwP97769821 = GVJWjkZWUKkBJxpBQdwP48966386;     GVJWjkZWUKkBJxpBQdwP48966386 = GVJWjkZWUKkBJxpBQdwP11842968;     GVJWjkZWUKkBJxpBQdwP11842968 = GVJWjkZWUKkBJxpBQdwP55678345;     GVJWjkZWUKkBJxpBQdwP55678345 = GVJWjkZWUKkBJxpBQdwP57641407;     GVJWjkZWUKkBJxpBQdwP57641407 = GVJWjkZWUKkBJxpBQdwP17077144;     GVJWjkZWUKkBJxpBQdwP17077144 = GVJWjkZWUKkBJxpBQdwP50036184;     GVJWjkZWUKkBJxpBQdwP50036184 = GVJWjkZWUKkBJxpBQdwP71971056;     GVJWjkZWUKkBJxpBQdwP71971056 = GVJWjkZWUKkBJxpBQdwP89011801;     GVJWjkZWUKkBJxpBQdwP89011801 = GVJWjkZWUKkBJxpBQdwP25236371;     GVJWjkZWUKkBJxpBQdwP25236371 = GVJWjkZWUKkBJxpBQdwP6798372;     GVJWjkZWUKkBJxpBQdwP6798372 = GVJWjkZWUKkBJxpBQdwP47783429;     GVJWjkZWUKkBJxpBQdwP47783429 = GVJWjkZWUKkBJxpBQdwP26647427;     GVJWjkZWUKkBJxpBQdwP26647427 = GVJWjkZWUKkBJxpBQdwP89906374;     GVJWjkZWUKkBJxpBQdwP89906374 = GVJWjkZWUKkBJxpBQdwP34351705;     GVJWjkZWUKkBJxpBQdwP34351705 = GVJWjkZWUKkBJxpBQdwP80323174;     GVJWjkZWUKkBJxpBQdwP80323174 = GVJWjkZWUKkBJxpBQdwP78084376;     GVJWjkZWUKkBJxpBQdwP78084376 = GVJWjkZWUKkBJxpBQdwP91365116;     GVJWjkZWUKkBJxpBQdwP91365116 = GVJWjkZWUKkBJxpBQdwP14725315;     GVJWjkZWUKkBJxpBQdwP14725315 = GVJWjkZWUKkBJxpBQdwP61830138;     GVJWjkZWUKkBJxpBQdwP61830138 = GVJWjkZWUKkBJxpBQdwP37238278;     GVJWjkZWUKkBJxpBQdwP37238278 = GVJWjkZWUKkBJxpBQdwP91360815;     GVJWjkZWUKkBJxpBQdwP91360815 = GVJWjkZWUKkBJxpBQdwP52619552;     GVJWjkZWUKkBJxpBQdwP52619552 = GVJWjkZWUKkBJxpBQdwP46802368;     GVJWjkZWUKkBJxpBQdwP46802368 = GVJWjkZWUKkBJxpBQdwP26249801;     GVJWjkZWUKkBJxpBQdwP26249801 = GVJWjkZWUKkBJxpBQdwP4564102;     GVJWjkZWUKkBJxpBQdwP4564102 = GVJWjkZWUKkBJxpBQdwP71985298;     GVJWjkZWUKkBJxpBQdwP71985298 = GVJWjkZWUKkBJxpBQdwP61774641;     GVJWjkZWUKkBJxpBQdwP61774641 = GVJWjkZWUKkBJxpBQdwP43069375;     GVJWjkZWUKkBJxpBQdwP43069375 = GVJWjkZWUKkBJxpBQdwP23393281;     GVJWjkZWUKkBJxpBQdwP23393281 = GVJWjkZWUKkBJxpBQdwP474471;     GVJWjkZWUKkBJxpBQdwP474471 = GVJWjkZWUKkBJxpBQdwP98571902;     GVJWjkZWUKkBJxpBQdwP98571902 = GVJWjkZWUKkBJxpBQdwP39648473;     GVJWjkZWUKkBJxpBQdwP39648473 = GVJWjkZWUKkBJxpBQdwP73218186;     GVJWjkZWUKkBJxpBQdwP73218186 = GVJWjkZWUKkBJxpBQdwP50720439;     GVJWjkZWUKkBJxpBQdwP50720439 = GVJWjkZWUKkBJxpBQdwP66546429;     GVJWjkZWUKkBJxpBQdwP66546429 = GVJWjkZWUKkBJxpBQdwP96663573;     GVJWjkZWUKkBJxpBQdwP96663573 = GVJWjkZWUKkBJxpBQdwP1697650;     GVJWjkZWUKkBJxpBQdwP1697650 = GVJWjkZWUKkBJxpBQdwP11637089;     GVJWjkZWUKkBJxpBQdwP11637089 = GVJWjkZWUKkBJxpBQdwP19847760;     GVJWjkZWUKkBJxpBQdwP19847760 = GVJWjkZWUKkBJxpBQdwP46929531;     GVJWjkZWUKkBJxpBQdwP46929531 = GVJWjkZWUKkBJxpBQdwP4294203;     GVJWjkZWUKkBJxpBQdwP4294203 = GVJWjkZWUKkBJxpBQdwP74114185;     GVJWjkZWUKkBJxpBQdwP74114185 = GVJWjkZWUKkBJxpBQdwP79370197;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void bzSuiZysRyTFkJRWyrHM91750068() {     int zjeRfagVqMzWBuhVrKTb13458989 = -42395646;    int zjeRfagVqMzWBuhVrKTb97938698 = -597322982;    int zjeRfagVqMzWBuhVrKTb23502201 = -517678894;    int zjeRfagVqMzWBuhVrKTb3947965 = -972724791;    int zjeRfagVqMzWBuhVrKTb18395759 = -161434817;    int zjeRfagVqMzWBuhVrKTb57588170 = -129887461;    int zjeRfagVqMzWBuhVrKTb33465000 = -58704484;    int zjeRfagVqMzWBuhVrKTb66129996 = -301766437;    int zjeRfagVqMzWBuhVrKTb23132532 = 49864707;    int zjeRfagVqMzWBuhVrKTb6805061 = -935515639;    int zjeRfagVqMzWBuhVrKTb30866522 = -967086232;    int zjeRfagVqMzWBuhVrKTb7581460 = -104317073;    int zjeRfagVqMzWBuhVrKTb85203594 = -39024535;    int zjeRfagVqMzWBuhVrKTb72038967 = -128973503;    int zjeRfagVqMzWBuhVrKTb89830097 = -678755392;    int zjeRfagVqMzWBuhVrKTb96555411 = -500673388;    int zjeRfagVqMzWBuhVrKTb51721788 = -736991726;    int zjeRfagVqMzWBuhVrKTb30458659 = 28602207;    int zjeRfagVqMzWBuhVrKTb61615928 = -23885550;    int zjeRfagVqMzWBuhVrKTb42484769 = 72989852;    int zjeRfagVqMzWBuhVrKTb87563768 = -68918026;    int zjeRfagVqMzWBuhVrKTb4758549 = -264270276;    int zjeRfagVqMzWBuhVrKTb9764543 = -963781962;    int zjeRfagVqMzWBuhVrKTb41498631 = -160621717;    int zjeRfagVqMzWBuhVrKTb13972694 = -36098122;    int zjeRfagVqMzWBuhVrKTb8976814 = -12421248;    int zjeRfagVqMzWBuhVrKTb87932518 = -278271861;    int zjeRfagVqMzWBuhVrKTb53338207 = -331215589;    int zjeRfagVqMzWBuhVrKTb25509884 = -341105765;    int zjeRfagVqMzWBuhVrKTb63587539 = -656990760;    int zjeRfagVqMzWBuhVrKTb42875998 = -453250343;    int zjeRfagVqMzWBuhVrKTb42814180 = -271766979;    int zjeRfagVqMzWBuhVrKTb91454751 = -255724719;    int zjeRfagVqMzWBuhVrKTb93048672 = -605781561;    int zjeRfagVqMzWBuhVrKTb52216145 = -738902871;    int zjeRfagVqMzWBuhVrKTb95188404 = -520313228;    int zjeRfagVqMzWBuhVrKTb94389547 = -115435549;    int zjeRfagVqMzWBuhVrKTb38042705 = -408888763;    int zjeRfagVqMzWBuhVrKTb39539511 = -220346154;    int zjeRfagVqMzWBuhVrKTb85686052 = 8767931;    int zjeRfagVqMzWBuhVrKTb95714451 = -111065686;    int zjeRfagVqMzWBuhVrKTb89239683 = -560739782;    int zjeRfagVqMzWBuhVrKTb99050269 = -408200643;    int zjeRfagVqMzWBuhVrKTb3016108 = -86998258;    int zjeRfagVqMzWBuhVrKTb46856466 = -554241010;    int zjeRfagVqMzWBuhVrKTb3181650 = -718526187;    int zjeRfagVqMzWBuhVrKTb28761924 = -724729715;    int zjeRfagVqMzWBuhVrKTb63670195 = -242284722;    int zjeRfagVqMzWBuhVrKTb97121257 = -989918389;    int zjeRfagVqMzWBuhVrKTb78399062 = -222992210;    int zjeRfagVqMzWBuhVrKTb92780223 = 8455974;    int zjeRfagVqMzWBuhVrKTb76491468 = -393786930;    int zjeRfagVqMzWBuhVrKTb39405655 = -441556405;    int zjeRfagVqMzWBuhVrKTb42212564 = -543032352;    int zjeRfagVqMzWBuhVrKTb4186300 = -184236605;    int zjeRfagVqMzWBuhVrKTb8700440 = -778125371;    int zjeRfagVqMzWBuhVrKTb88174155 = -633541021;    int zjeRfagVqMzWBuhVrKTb82003570 = -257057178;    int zjeRfagVqMzWBuhVrKTb89975271 = -836626669;    int zjeRfagVqMzWBuhVrKTb9418946 = -49013569;    int zjeRfagVqMzWBuhVrKTb69655652 = -851615601;    int zjeRfagVqMzWBuhVrKTb80126793 = -727488895;    int zjeRfagVqMzWBuhVrKTb40620113 = -960660672;    int zjeRfagVqMzWBuhVrKTb59544993 = -293144533;    int zjeRfagVqMzWBuhVrKTb63929062 = -382265296;    int zjeRfagVqMzWBuhVrKTb88052341 = -595319254;    int zjeRfagVqMzWBuhVrKTb16126709 = -848592355;    int zjeRfagVqMzWBuhVrKTb92154922 = -433242975;    int zjeRfagVqMzWBuhVrKTb19822822 = -390070633;    int zjeRfagVqMzWBuhVrKTb94641692 = -58442164;    int zjeRfagVqMzWBuhVrKTb2165864 = -285237839;    int zjeRfagVqMzWBuhVrKTb13679084 = -228102963;    int zjeRfagVqMzWBuhVrKTb90919148 = -751051639;    int zjeRfagVqMzWBuhVrKTb75929876 = 67346519;    int zjeRfagVqMzWBuhVrKTb46770317 = -815944462;    int zjeRfagVqMzWBuhVrKTb98324085 = -508178244;    int zjeRfagVqMzWBuhVrKTb5708279 = -856069634;    int zjeRfagVqMzWBuhVrKTb6748435 = -776783704;    int zjeRfagVqMzWBuhVrKTb94642164 = -606380708;    int zjeRfagVqMzWBuhVrKTb10791044 = -317571936;    int zjeRfagVqMzWBuhVrKTb80214890 = -287691534;    int zjeRfagVqMzWBuhVrKTb24262324 = 64012861;    int zjeRfagVqMzWBuhVrKTb56216950 = -341297201;    int zjeRfagVqMzWBuhVrKTb47110821 = -18113556;    int zjeRfagVqMzWBuhVrKTb70807315 = -565446734;    int zjeRfagVqMzWBuhVrKTb66384530 = 40536587;    int zjeRfagVqMzWBuhVrKTb3408526 = -830210575;    int zjeRfagVqMzWBuhVrKTb49242188 = -712692368;    int zjeRfagVqMzWBuhVrKTb88862373 = -321544956;    int zjeRfagVqMzWBuhVrKTb43515705 = -960777501;    int zjeRfagVqMzWBuhVrKTb7014250 = -886772208;    int zjeRfagVqMzWBuhVrKTb12385978 = -858378372;    int zjeRfagVqMzWBuhVrKTb48067434 = -572262094;    int zjeRfagVqMzWBuhVrKTb30120566 = -71332586;    int zjeRfagVqMzWBuhVrKTb16030401 = -139616469;    int zjeRfagVqMzWBuhVrKTb15587659 = -383576791;    int zjeRfagVqMzWBuhVrKTb48619571 = -600079111;    int zjeRfagVqMzWBuhVrKTb39505277 = -15056110;    int zjeRfagVqMzWBuhVrKTb39087046 = -704732963;    int zjeRfagVqMzWBuhVrKTb58804124 = -42395646;     zjeRfagVqMzWBuhVrKTb13458989 = zjeRfagVqMzWBuhVrKTb97938698;     zjeRfagVqMzWBuhVrKTb97938698 = zjeRfagVqMzWBuhVrKTb23502201;     zjeRfagVqMzWBuhVrKTb23502201 = zjeRfagVqMzWBuhVrKTb3947965;     zjeRfagVqMzWBuhVrKTb3947965 = zjeRfagVqMzWBuhVrKTb18395759;     zjeRfagVqMzWBuhVrKTb18395759 = zjeRfagVqMzWBuhVrKTb57588170;     zjeRfagVqMzWBuhVrKTb57588170 = zjeRfagVqMzWBuhVrKTb33465000;     zjeRfagVqMzWBuhVrKTb33465000 = zjeRfagVqMzWBuhVrKTb66129996;     zjeRfagVqMzWBuhVrKTb66129996 = zjeRfagVqMzWBuhVrKTb23132532;     zjeRfagVqMzWBuhVrKTb23132532 = zjeRfagVqMzWBuhVrKTb6805061;     zjeRfagVqMzWBuhVrKTb6805061 = zjeRfagVqMzWBuhVrKTb30866522;     zjeRfagVqMzWBuhVrKTb30866522 = zjeRfagVqMzWBuhVrKTb7581460;     zjeRfagVqMzWBuhVrKTb7581460 = zjeRfagVqMzWBuhVrKTb85203594;     zjeRfagVqMzWBuhVrKTb85203594 = zjeRfagVqMzWBuhVrKTb72038967;     zjeRfagVqMzWBuhVrKTb72038967 = zjeRfagVqMzWBuhVrKTb89830097;     zjeRfagVqMzWBuhVrKTb89830097 = zjeRfagVqMzWBuhVrKTb96555411;     zjeRfagVqMzWBuhVrKTb96555411 = zjeRfagVqMzWBuhVrKTb51721788;     zjeRfagVqMzWBuhVrKTb51721788 = zjeRfagVqMzWBuhVrKTb30458659;     zjeRfagVqMzWBuhVrKTb30458659 = zjeRfagVqMzWBuhVrKTb61615928;     zjeRfagVqMzWBuhVrKTb61615928 = zjeRfagVqMzWBuhVrKTb42484769;     zjeRfagVqMzWBuhVrKTb42484769 = zjeRfagVqMzWBuhVrKTb87563768;     zjeRfagVqMzWBuhVrKTb87563768 = zjeRfagVqMzWBuhVrKTb4758549;     zjeRfagVqMzWBuhVrKTb4758549 = zjeRfagVqMzWBuhVrKTb9764543;     zjeRfagVqMzWBuhVrKTb9764543 = zjeRfagVqMzWBuhVrKTb41498631;     zjeRfagVqMzWBuhVrKTb41498631 = zjeRfagVqMzWBuhVrKTb13972694;     zjeRfagVqMzWBuhVrKTb13972694 = zjeRfagVqMzWBuhVrKTb8976814;     zjeRfagVqMzWBuhVrKTb8976814 = zjeRfagVqMzWBuhVrKTb87932518;     zjeRfagVqMzWBuhVrKTb87932518 = zjeRfagVqMzWBuhVrKTb53338207;     zjeRfagVqMzWBuhVrKTb53338207 = zjeRfagVqMzWBuhVrKTb25509884;     zjeRfagVqMzWBuhVrKTb25509884 = zjeRfagVqMzWBuhVrKTb63587539;     zjeRfagVqMzWBuhVrKTb63587539 = zjeRfagVqMzWBuhVrKTb42875998;     zjeRfagVqMzWBuhVrKTb42875998 = zjeRfagVqMzWBuhVrKTb42814180;     zjeRfagVqMzWBuhVrKTb42814180 = zjeRfagVqMzWBuhVrKTb91454751;     zjeRfagVqMzWBuhVrKTb91454751 = zjeRfagVqMzWBuhVrKTb93048672;     zjeRfagVqMzWBuhVrKTb93048672 = zjeRfagVqMzWBuhVrKTb52216145;     zjeRfagVqMzWBuhVrKTb52216145 = zjeRfagVqMzWBuhVrKTb95188404;     zjeRfagVqMzWBuhVrKTb95188404 = zjeRfagVqMzWBuhVrKTb94389547;     zjeRfagVqMzWBuhVrKTb94389547 = zjeRfagVqMzWBuhVrKTb38042705;     zjeRfagVqMzWBuhVrKTb38042705 = zjeRfagVqMzWBuhVrKTb39539511;     zjeRfagVqMzWBuhVrKTb39539511 = zjeRfagVqMzWBuhVrKTb85686052;     zjeRfagVqMzWBuhVrKTb85686052 = zjeRfagVqMzWBuhVrKTb95714451;     zjeRfagVqMzWBuhVrKTb95714451 = zjeRfagVqMzWBuhVrKTb89239683;     zjeRfagVqMzWBuhVrKTb89239683 = zjeRfagVqMzWBuhVrKTb99050269;     zjeRfagVqMzWBuhVrKTb99050269 = zjeRfagVqMzWBuhVrKTb3016108;     zjeRfagVqMzWBuhVrKTb3016108 = zjeRfagVqMzWBuhVrKTb46856466;     zjeRfagVqMzWBuhVrKTb46856466 = zjeRfagVqMzWBuhVrKTb3181650;     zjeRfagVqMzWBuhVrKTb3181650 = zjeRfagVqMzWBuhVrKTb28761924;     zjeRfagVqMzWBuhVrKTb28761924 = zjeRfagVqMzWBuhVrKTb63670195;     zjeRfagVqMzWBuhVrKTb63670195 = zjeRfagVqMzWBuhVrKTb97121257;     zjeRfagVqMzWBuhVrKTb97121257 = zjeRfagVqMzWBuhVrKTb78399062;     zjeRfagVqMzWBuhVrKTb78399062 = zjeRfagVqMzWBuhVrKTb92780223;     zjeRfagVqMzWBuhVrKTb92780223 = zjeRfagVqMzWBuhVrKTb76491468;     zjeRfagVqMzWBuhVrKTb76491468 = zjeRfagVqMzWBuhVrKTb39405655;     zjeRfagVqMzWBuhVrKTb39405655 = zjeRfagVqMzWBuhVrKTb42212564;     zjeRfagVqMzWBuhVrKTb42212564 = zjeRfagVqMzWBuhVrKTb4186300;     zjeRfagVqMzWBuhVrKTb4186300 = zjeRfagVqMzWBuhVrKTb8700440;     zjeRfagVqMzWBuhVrKTb8700440 = zjeRfagVqMzWBuhVrKTb88174155;     zjeRfagVqMzWBuhVrKTb88174155 = zjeRfagVqMzWBuhVrKTb82003570;     zjeRfagVqMzWBuhVrKTb82003570 = zjeRfagVqMzWBuhVrKTb89975271;     zjeRfagVqMzWBuhVrKTb89975271 = zjeRfagVqMzWBuhVrKTb9418946;     zjeRfagVqMzWBuhVrKTb9418946 = zjeRfagVqMzWBuhVrKTb69655652;     zjeRfagVqMzWBuhVrKTb69655652 = zjeRfagVqMzWBuhVrKTb80126793;     zjeRfagVqMzWBuhVrKTb80126793 = zjeRfagVqMzWBuhVrKTb40620113;     zjeRfagVqMzWBuhVrKTb40620113 = zjeRfagVqMzWBuhVrKTb59544993;     zjeRfagVqMzWBuhVrKTb59544993 = zjeRfagVqMzWBuhVrKTb63929062;     zjeRfagVqMzWBuhVrKTb63929062 = zjeRfagVqMzWBuhVrKTb88052341;     zjeRfagVqMzWBuhVrKTb88052341 = zjeRfagVqMzWBuhVrKTb16126709;     zjeRfagVqMzWBuhVrKTb16126709 = zjeRfagVqMzWBuhVrKTb92154922;     zjeRfagVqMzWBuhVrKTb92154922 = zjeRfagVqMzWBuhVrKTb19822822;     zjeRfagVqMzWBuhVrKTb19822822 = zjeRfagVqMzWBuhVrKTb94641692;     zjeRfagVqMzWBuhVrKTb94641692 = zjeRfagVqMzWBuhVrKTb2165864;     zjeRfagVqMzWBuhVrKTb2165864 = zjeRfagVqMzWBuhVrKTb13679084;     zjeRfagVqMzWBuhVrKTb13679084 = zjeRfagVqMzWBuhVrKTb90919148;     zjeRfagVqMzWBuhVrKTb90919148 = zjeRfagVqMzWBuhVrKTb75929876;     zjeRfagVqMzWBuhVrKTb75929876 = zjeRfagVqMzWBuhVrKTb46770317;     zjeRfagVqMzWBuhVrKTb46770317 = zjeRfagVqMzWBuhVrKTb98324085;     zjeRfagVqMzWBuhVrKTb98324085 = zjeRfagVqMzWBuhVrKTb5708279;     zjeRfagVqMzWBuhVrKTb5708279 = zjeRfagVqMzWBuhVrKTb6748435;     zjeRfagVqMzWBuhVrKTb6748435 = zjeRfagVqMzWBuhVrKTb94642164;     zjeRfagVqMzWBuhVrKTb94642164 = zjeRfagVqMzWBuhVrKTb10791044;     zjeRfagVqMzWBuhVrKTb10791044 = zjeRfagVqMzWBuhVrKTb80214890;     zjeRfagVqMzWBuhVrKTb80214890 = zjeRfagVqMzWBuhVrKTb24262324;     zjeRfagVqMzWBuhVrKTb24262324 = zjeRfagVqMzWBuhVrKTb56216950;     zjeRfagVqMzWBuhVrKTb56216950 = zjeRfagVqMzWBuhVrKTb47110821;     zjeRfagVqMzWBuhVrKTb47110821 = zjeRfagVqMzWBuhVrKTb70807315;     zjeRfagVqMzWBuhVrKTb70807315 = zjeRfagVqMzWBuhVrKTb66384530;     zjeRfagVqMzWBuhVrKTb66384530 = zjeRfagVqMzWBuhVrKTb3408526;     zjeRfagVqMzWBuhVrKTb3408526 = zjeRfagVqMzWBuhVrKTb49242188;     zjeRfagVqMzWBuhVrKTb49242188 = zjeRfagVqMzWBuhVrKTb88862373;     zjeRfagVqMzWBuhVrKTb88862373 = zjeRfagVqMzWBuhVrKTb43515705;     zjeRfagVqMzWBuhVrKTb43515705 = zjeRfagVqMzWBuhVrKTb7014250;     zjeRfagVqMzWBuhVrKTb7014250 = zjeRfagVqMzWBuhVrKTb12385978;     zjeRfagVqMzWBuhVrKTb12385978 = zjeRfagVqMzWBuhVrKTb48067434;     zjeRfagVqMzWBuhVrKTb48067434 = zjeRfagVqMzWBuhVrKTb30120566;     zjeRfagVqMzWBuhVrKTb30120566 = zjeRfagVqMzWBuhVrKTb16030401;     zjeRfagVqMzWBuhVrKTb16030401 = zjeRfagVqMzWBuhVrKTb15587659;     zjeRfagVqMzWBuhVrKTb15587659 = zjeRfagVqMzWBuhVrKTb48619571;     zjeRfagVqMzWBuhVrKTb48619571 = zjeRfagVqMzWBuhVrKTb39505277;     zjeRfagVqMzWBuhVrKTb39505277 = zjeRfagVqMzWBuhVrKTb39087046;     zjeRfagVqMzWBuhVrKTb39087046 = zjeRfagVqMzWBuhVrKTb58804124;     zjeRfagVqMzWBuhVrKTb58804124 = zjeRfagVqMzWBuhVrKTb13458989;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void hYIJDUsHFkcMPWwCNQdD50401739() {     int oQriroyKhtWkFeGtgboG76889641 = -726169060;    int oQriroyKhtWkFeGtgboG23190278 = -357097597;    int oQriroyKhtWkFeGtgboG16092072 = -595753354;    int oQriroyKhtWkFeGtgboG9221943 = -986704518;    int oQriroyKhtWkFeGtgboG59510337 = -523157848;    int oQriroyKhtWkFeGtgboG35219399 = 28889309;    int oQriroyKhtWkFeGtgboG90895277 = -483030985;    int oQriroyKhtWkFeGtgboG18813824 = -620168752;    int oQriroyKhtWkFeGtgboG72115451 = -524064433;    int oQriroyKhtWkFeGtgboG41023089 = -415362156;    int oQriroyKhtWkFeGtgboG98471789 = -404928891;    int oQriroyKhtWkFeGtgboG84696148 = -738633840;    int oQriroyKhtWkFeGtgboG83371731 = -360669545;    int oQriroyKhtWkFeGtgboG6068874 = -861887020;    int oQriroyKhtWkFeGtgboG13164291 = -885920232;    int oQriroyKhtWkFeGtgboG96750096 = 12127174;    int oQriroyKhtWkFeGtgboG15578989 = -968263260;    int oQriroyKhtWkFeGtgboG74794642 = -946711313;    int oQriroyKhtWkFeGtgboG26491974 = 70557609;    int oQriroyKhtWkFeGtgboG37612980 = -341596933;    int oQriroyKhtWkFeGtgboG48010989 = -761427004;    int oQriroyKhtWkFeGtgboG67621746 = 20371965;    int oQriroyKhtWkFeGtgboG40960136 = -909169778;    int oQriroyKhtWkFeGtgboG38311030 = -799247562;    int oQriroyKhtWkFeGtgboG97407513 = -410707060;    int oQriroyKhtWkFeGtgboG51102130 = -365809265;    int oQriroyKhtWkFeGtgboG68741566 = -208023685;    int oQriroyKhtWkFeGtgboG65735958 = -686530009;    int oQriroyKhtWkFeGtgboG22815032 = -874177616;    int oQriroyKhtWkFeGtgboG3104314 = -308026123;    int oQriroyKhtWkFeGtgboG37803651 = -488813343;    int oQriroyKhtWkFeGtgboG73519859 = -870965232;    int oQriroyKhtWkFeGtgboG11441060 = -128816870;    int oQriroyKhtWkFeGtgboG88752594 = -714428104;    int oQriroyKhtWkFeGtgboG48543273 = -181985206;    int oQriroyKhtWkFeGtgboG70911173 = -564491891;    int oQriroyKhtWkFeGtgboG8438717 = -585117665;    int oQriroyKhtWkFeGtgboG77697785 = -768279716;    int oQriroyKhtWkFeGtgboG27352789 = -474144127;    int oQriroyKhtWkFeGtgboG90765704 = -140324408;    int oQriroyKhtWkFeGtgboG49932791 = 60041740;    int oQriroyKhtWkFeGtgboG98769133 = -243933017;    int oQriroyKhtWkFeGtgboG12451225 = -590501557;    int oQriroyKhtWkFeGtgboG82490554 = -634657559;    int oQriroyKhtWkFeGtgboG18650939 = -63041947;    int oQriroyKhtWkFeGtgboG98148810 = -285874811;    int oQriroyKhtWkFeGtgboG53064810 = -202096023;    int oQriroyKhtWkFeGtgboG94275295 = -413396056;    int oQriroyKhtWkFeGtgboG43461798 = -905689373;    int oQriroyKhtWkFeGtgboG31660666 = -644960587;    int oQriroyKhtWkFeGtgboG31416596 = -759228544;    int oQriroyKhtWkFeGtgboG94439036 = 77185064;    int oQriroyKhtWkFeGtgboG70256384 = -492716645;    int oQriroyKhtWkFeGtgboG30947058 = -786420800;    int oQriroyKhtWkFeGtgboG2390751 = -207883894;    int oQriroyKhtWkFeGtgboG9267895 = -646541025;    int oQriroyKhtWkFeGtgboG82230141 = -447927819;    int oQriroyKhtWkFeGtgboG77781041 = -796505793;    int oQriroyKhtWkFeGtgboG11814429 = -475997458;    int oQriroyKhtWkFeGtgboG8408208 = -57348583;    int oQriroyKhtWkFeGtgboG66477832 = -763087006;    int oQriroyKhtWkFeGtgboG25159319 = -796500977;    int oQriroyKhtWkFeGtgboG95998792 = -745991137;    int oQriroyKhtWkFeGtgboG69011137 = -116038311;    int oQriroyKhtWkFeGtgboG3219439 = -926548814;    int oQriroyKhtWkFeGtgboG24951930 = -533963659;    int oQriroyKhtWkFeGtgboG73255089 = -509816970;    int oQriroyKhtWkFeGtgboG94619137 = -646241442;    int oQriroyKhtWkFeGtgboG57525600 = -579901815;    int oQriroyKhtWkFeGtgboG42253117 = -221428342;    int oQriroyKhtWkFeGtgboG88311379 = -402755162;    int oQriroyKhtWkFeGtgboG37881204 = -99983545;    int oQriroyKhtWkFeGtgboG47441853 = -372567186;    int oQriroyKhtWkFeGtgboG35726270 = -789117984;    int oQriroyKhtWkFeGtgboG87680188 = -301638673;    int oQriroyKhtWkFeGtgboG49241855 = -417493987;    int oQriroyKhtWkFeGtgboG55170521 = -389126479;    int oQriroyKhtWkFeGtgboG58469582 = -174512220;    int oQriroyKhtWkFeGtgboG19660091 = -636205616;    int oQriroyKhtWkFeGtgboG99258703 = -24832249;    int oQriroyKhtWkFeGtgboG98037319 = -63713243;    int oQriroyKhtWkFeGtgboG74466271 = -794627629;    int oQriroyKhtWkFeGtgboG22274161 = -780840636;    int oQriroyKhtWkFeGtgboG91154366 = -129217029;    int oQriroyKhtWkFeGtgboG71687718 = -548797579;    int oQriroyKhtWkFeGtgboG43364614 = -465998407;    int oQriroyKhtWkFeGtgboG3263476 = -278248588;    int oQriroyKhtWkFeGtgboG80494001 = -342396070;    int oQriroyKhtWkFeGtgboG86361844 = -406544211;    int oQriroyKhtWkFeGtgboG39275379 = -535444181;    int oQriroyKhtWkFeGtgboG88681031 = -16564072;    int oQriroyKhtWkFeGtgboG30657676 = -788611873;    int oQriroyKhtWkFeGtgboG65883357 = -192282259;    int oQriroyKhtWkFeGtgboG18944582 = -316795544;    int oQriroyKhtWkFeGtgboG24287872 = -377237402;    int oQriroyKhtWkFeGtgboG24773473 = -143457284;    int oQriroyKhtWkFeGtgboG2770342 = -497941881;    int oQriroyKhtWkFeGtgboG43440088 = -374463246;    int oQriroyKhtWkFeGtgboG79271115 = -708108745;    int oQriroyKhtWkFeGtgboG93699009 = -726169060;     oQriroyKhtWkFeGtgboG76889641 = oQriroyKhtWkFeGtgboG23190278;     oQriroyKhtWkFeGtgboG23190278 = oQriroyKhtWkFeGtgboG16092072;     oQriroyKhtWkFeGtgboG16092072 = oQriroyKhtWkFeGtgboG9221943;     oQriroyKhtWkFeGtgboG9221943 = oQriroyKhtWkFeGtgboG59510337;     oQriroyKhtWkFeGtgboG59510337 = oQriroyKhtWkFeGtgboG35219399;     oQriroyKhtWkFeGtgboG35219399 = oQriroyKhtWkFeGtgboG90895277;     oQriroyKhtWkFeGtgboG90895277 = oQriroyKhtWkFeGtgboG18813824;     oQriroyKhtWkFeGtgboG18813824 = oQriroyKhtWkFeGtgboG72115451;     oQriroyKhtWkFeGtgboG72115451 = oQriroyKhtWkFeGtgboG41023089;     oQriroyKhtWkFeGtgboG41023089 = oQriroyKhtWkFeGtgboG98471789;     oQriroyKhtWkFeGtgboG98471789 = oQriroyKhtWkFeGtgboG84696148;     oQriroyKhtWkFeGtgboG84696148 = oQriroyKhtWkFeGtgboG83371731;     oQriroyKhtWkFeGtgboG83371731 = oQriroyKhtWkFeGtgboG6068874;     oQriroyKhtWkFeGtgboG6068874 = oQriroyKhtWkFeGtgboG13164291;     oQriroyKhtWkFeGtgboG13164291 = oQriroyKhtWkFeGtgboG96750096;     oQriroyKhtWkFeGtgboG96750096 = oQriroyKhtWkFeGtgboG15578989;     oQriroyKhtWkFeGtgboG15578989 = oQriroyKhtWkFeGtgboG74794642;     oQriroyKhtWkFeGtgboG74794642 = oQriroyKhtWkFeGtgboG26491974;     oQriroyKhtWkFeGtgboG26491974 = oQriroyKhtWkFeGtgboG37612980;     oQriroyKhtWkFeGtgboG37612980 = oQriroyKhtWkFeGtgboG48010989;     oQriroyKhtWkFeGtgboG48010989 = oQriroyKhtWkFeGtgboG67621746;     oQriroyKhtWkFeGtgboG67621746 = oQriroyKhtWkFeGtgboG40960136;     oQriroyKhtWkFeGtgboG40960136 = oQriroyKhtWkFeGtgboG38311030;     oQriroyKhtWkFeGtgboG38311030 = oQriroyKhtWkFeGtgboG97407513;     oQriroyKhtWkFeGtgboG97407513 = oQriroyKhtWkFeGtgboG51102130;     oQriroyKhtWkFeGtgboG51102130 = oQriroyKhtWkFeGtgboG68741566;     oQriroyKhtWkFeGtgboG68741566 = oQriroyKhtWkFeGtgboG65735958;     oQriroyKhtWkFeGtgboG65735958 = oQriroyKhtWkFeGtgboG22815032;     oQriroyKhtWkFeGtgboG22815032 = oQriroyKhtWkFeGtgboG3104314;     oQriroyKhtWkFeGtgboG3104314 = oQriroyKhtWkFeGtgboG37803651;     oQriroyKhtWkFeGtgboG37803651 = oQriroyKhtWkFeGtgboG73519859;     oQriroyKhtWkFeGtgboG73519859 = oQriroyKhtWkFeGtgboG11441060;     oQriroyKhtWkFeGtgboG11441060 = oQriroyKhtWkFeGtgboG88752594;     oQriroyKhtWkFeGtgboG88752594 = oQriroyKhtWkFeGtgboG48543273;     oQriroyKhtWkFeGtgboG48543273 = oQriroyKhtWkFeGtgboG70911173;     oQriroyKhtWkFeGtgboG70911173 = oQriroyKhtWkFeGtgboG8438717;     oQriroyKhtWkFeGtgboG8438717 = oQriroyKhtWkFeGtgboG77697785;     oQriroyKhtWkFeGtgboG77697785 = oQriroyKhtWkFeGtgboG27352789;     oQriroyKhtWkFeGtgboG27352789 = oQriroyKhtWkFeGtgboG90765704;     oQriroyKhtWkFeGtgboG90765704 = oQriroyKhtWkFeGtgboG49932791;     oQriroyKhtWkFeGtgboG49932791 = oQriroyKhtWkFeGtgboG98769133;     oQriroyKhtWkFeGtgboG98769133 = oQriroyKhtWkFeGtgboG12451225;     oQriroyKhtWkFeGtgboG12451225 = oQriroyKhtWkFeGtgboG82490554;     oQriroyKhtWkFeGtgboG82490554 = oQriroyKhtWkFeGtgboG18650939;     oQriroyKhtWkFeGtgboG18650939 = oQriroyKhtWkFeGtgboG98148810;     oQriroyKhtWkFeGtgboG98148810 = oQriroyKhtWkFeGtgboG53064810;     oQriroyKhtWkFeGtgboG53064810 = oQriroyKhtWkFeGtgboG94275295;     oQriroyKhtWkFeGtgboG94275295 = oQriroyKhtWkFeGtgboG43461798;     oQriroyKhtWkFeGtgboG43461798 = oQriroyKhtWkFeGtgboG31660666;     oQriroyKhtWkFeGtgboG31660666 = oQriroyKhtWkFeGtgboG31416596;     oQriroyKhtWkFeGtgboG31416596 = oQriroyKhtWkFeGtgboG94439036;     oQriroyKhtWkFeGtgboG94439036 = oQriroyKhtWkFeGtgboG70256384;     oQriroyKhtWkFeGtgboG70256384 = oQriroyKhtWkFeGtgboG30947058;     oQriroyKhtWkFeGtgboG30947058 = oQriroyKhtWkFeGtgboG2390751;     oQriroyKhtWkFeGtgboG2390751 = oQriroyKhtWkFeGtgboG9267895;     oQriroyKhtWkFeGtgboG9267895 = oQriroyKhtWkFeGtgboG82230141;     oQriroyKhtWkFeGtgboG82230141 = oQriroyKhtWkFeGtgboG77781041;     oQriroyKhtWkFeGtgboG77781041 = oQriroyKhtWkFeGtgboG11814429;     oQriroyKhtWkFeGtgboG11814429 = oQriroyKhtWkFeGtgboG8408208;     oQriroyKhtWkFeGtgboG8408208 = oQriroyKhtWkFeGtgboG66477832;     oQriroyKhtWkFeGtgboG66477832 = oQriroyKhtWkFeGtgboG25159319;     oQriroyKhtWkFeGtgboG25159319 = oQriroyKhtWkFeGtgboG95998792;     oQriroyKhtWkFeGtgboG95998792 = oQriroyKhtWkFeGtgboG69011137;     oQriroyKhtWkFeGtgboG69011137 = oQriroyKhtWkFeGtgboG3219439;     oQriroyKhtWkFeGtgboG3219439 = oQriroyKhtWkFeGtgboG24951930;     oQriroyKhtWkFeGtgboG24951930 = oQriroyKhtWkFeGtgboG73255089;     oQriroyKhtWkFeGtgboG73255089 = oQriroyKhtWkFeGtgboG94619137;     oQriroyKhtWkFeGtgboG94619137 = oQriroyKhtWkFeGtgboG57525600;     oQriroyKhtWkFeGtgboG57525600 = oQriroyKhtWkFeGtgboG42253117;     oQriroyKhtWkFeGtgboG42253117 = oQriroyKhtWkFeGtgboG88311379;     oQriroyKhtWkFeGtgboG88311379 = oQriroyKhtWkFeGtgboG37881204;     oQriroyKhtWkFeGtgboG37881204 = oQriroyKhtWkFeGtgboG47441853;     oQriroyKhtWkFeGtgboG47441853 = oQriroyKhtWkFeGtgboG35726270;     oQriroyKhtWkFeGtgboG35726270 = oQriroyKhtWkFeGtgboG87680188;     oQriroyKhtWkFeGtgboG87680188 = oQriroyKhtWkFeGtgboG49241855;     oQriroyKhtWkFeGtgboG49241855 = oQriroyKhtWkFeGtgboG55170521;     oQriroyKhtWkFeGtgboG55170521 = oQriroyKhtWkFeGtgboG58469582;     oQriroyKhtWkFeGtgboG58469582 = oQriroyKhtWkFeGtgboG19660091;     oQriroyKhtWkFeGtgboG19660091 = oQriroyKhtWkFeGtgboG99258703;     oQriroyKhtWkFeGtgboG99258703 = oQriroyKhtWkFeGtgboG98037319;     oQriroyKhtWkFeGtgboG98037319 = oQriroyKhtWkFeGtgboG74466271;     oQriroyKhtWkFeGtgboG74466271 = oQriroyKhtWkFeGtgboG22274161;     oQriroyKhtWkFeGtgboG22274161 = oQriroyKhtWkFeGtgboG91154366;     oQriroyKhtWkFeGtgboG91154366 = oQriroyKhtWkFeGtgboG71687718;     oQriroyKhtWkFeGtgboG71687718 = oQriroyKhtWkFeGtgboG43364614;     oQriroyKhtWkFeGtgboG43364614 = oQriroyKhtWkFeGtgboG3263476;     oQriroyKhtWkFeGtgboG3263476 = oQriroyKhtWkFeGtgboG80494001;     oQriroyKhtWkFeGtgboG80494001 = oQriroyKhtWkFeGtgboG86361844;     oQriroyKhtWkFeGtgboG86361844 = oQriroyKhtWkFeGtgboG39275379;     oQriroyKhtWkFeGtgboG39275379 = oQriroyKhtWkFeGtgboG88681031;     oQriroyKhtWkFeGtgboG88681031 = oQriroyKhtWkFeGtgboG30657676;     oQriroyKhtWkFeGtgboG30657676 = oQriroyKhtWkFeGtgboG65883357;     oQriroyKhtWkFeGtgboG65883357 = oQriroyKhtWkFeGtgboG18944582;     oQriroyKhtWkFeGtgboG18944582 = oQriroyKhtWkFeGtgboG24287872;     oQriroyKhtWkFeGtgboG24287872 = oQriroyKhtWkFeGtgboG24773473;     oQriroyKhtWkFeGtgboG24773473 = oQriroyKhtWkFeGtgboG2770342;     oQriroyKhtWkFeGtgboG2770342 = oQriroyKhtWkFeGtgboG43440088;     oQriroyKhtWkFeGtgboG43440088 = oQriroyKhtWkFeGtgboG79271115;     oQriroyKhtWkFeGtgboG79271115 = oQriroyKhtWkFeGtgboG93699009;     oQriroyKhtWkFeGtgboG93699009 = oQriroyKhtWkFeGtgboG76889641;}
// Junk Finished

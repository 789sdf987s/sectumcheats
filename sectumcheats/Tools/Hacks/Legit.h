#pragma once
#include "stdafx.h"
#include "../Utils/Hitbox.h"
#include "Misc.h"
#include "../Utils/LocalInfo.h"

#include <vector>
using namespace std;

#include "../Utils/Playerlist.h"

#define keystate(i) GetAsyncKeyState(i & 0x8000)

bool IsWeaponKnife(int weaponid)
{
	if (weaponid == WEAPON_KNIFE || weaponid == WEAPON_KNIFEGG)
		return true;

	return false;
}

bool IsWeaponGrenade(int weaponid)
{
	if (weaponid == WEAPON_FLASHBANG || weaponid == WEAPON_HEGRENADE || weaponid == WEAPON_SMOKEGRENADE || weaponid == WEAPON_MOLOTOV || weaponid == WEAPON_INCGRENADE || weaponid == WEAPON_DECOY)
		return true;

	return false;
}

bool IsWeaponBomb(int weaponid)
{
	if (weaponid == WEAPON_C4)
		return true;

	return false;
}

bool IsWeaponTaser(int weaponid)
{
	if (weaponid == WEAPON_TASER)
		return true;

	return false;
}

bool IsNonAimWeapon(int weaponid)
{
	if (IsWeaponKnife(weaponid) || IsWeaponGrenade(weaponid) || IsWeaponBomb(weaponid) || IsWeaponTaser(weaponid))
		return true;

	return false;
}
bool CanShoot(CBaseEntity* pLocalPlayer, CBaseCombatWeapon* weapon)
{
	if (!weapon)
		return false;

	if (IsNonAimWeapon(*weapon->GetItemDefinitionIndex()))
		return false;

	if (weapon->ammo() < 1)
		return false;


	float server_time = pLocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;

	float next_shot = weapon->NextPrimaryAttack() - server_time;
	if (next_shot > 0)
		return false;

	return true;
}
enum
{
	PITCH = 0,	// up / down
	YAW,		// left / right
	ROLL		// fall over
};
namespace LegitHelper
{

	void inline SinCos(float radians, float *sine, float *cosine)
	{
		*sine = sin(radians);
		*cosine = cos(radians);
	}
	void AngleVectors(const Vector &angles, Vector *forward)
	{
		float sp, sy, cp, cy;

		SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
		SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);

		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}




};


class Legit
{
private:
	float deltaTime;
	float curAimTime;

	int besttarget = -1;
	int besthitbox = -1;

	bool EntityIsValid(int i, int hitbox)
	{
		auto pEntity = static_cast<CBaseEntity*> (Interfaces.pEntList->GetClientEntity(i));

		if (!pEntity)
			return false;
		if (pEntity == Hacks.LocalPlayer)
			return false;
		if (pEntity->GetHealth() <= 0)
			return false;
		if (pEntity->HasGunGameImmunity())
			return false;
		if (pEntity->IsDormant())
			return false;
		if (pEntity->GetTeam() == Hacks.LocalPlayer->GetTeam() && !Settings.Aimbot.FriendlyFire)
			return false;
		if (Settings.Aimbot.JumpCheck && !(Hacks.LocalPlayer->GetFlags() & FL_ONGROUND))
			return false;
		if (Settings.aCacheAngle[i].bCustomSettsLegit && Settings.aCacheAngle[i].Ignore)
			return false;

		return true;
	}

	void GetBestTarget()
	{

		int cw = *Hacks.LocalWeapon->GetItemDefinitionIndex();
		float bestfov = 9999999.f;
		Vector vSrc = Hacks.LocalPlayer->GetEyePosition();
		for (int i = 0; i < Interfaces.pEntList->GetHighestEntityIndex(); i++)
		{
			if (!EntityIsValid(i, Settings.Weapons[cw].Bone + 1))
				continue;

			auto pEntity = static_cast<CBaseEntity*> (Interfaces.pEntList->GetClientEntity(i));
			if (!pEntity)
				continue;


			if (Settings.Weapons[cw].Nearest || Settings.aCacheAngle[i].bCustomSettsLegit && Settings.aCacheAngle[i].LegitNearest)
			{
				for (int j = 0; j <= 27; j++)
				{
					Vector vec = pEntity->GetBonePos(j);

		
					float fov = Misc::GetFov(Hacks.CurrentCmd->viewangles + LocalInfo.PunchAns * (Settings.Weapons[cw].RcsX / 50.f) / 5 * 4.6, Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), pEntity->GetBonePos(j)));
					if (fov > Settings.Weapons[cw].Fov)
						continue;

					if (fov < bestfov)
					{
						if (!Misc::pIsVisible(vSrc, vec, Hacks.LocalPlayer, pEntity, Settings.Aimbot.SmokeCheck))
							continue;
						if (Hacks.LocalPlayer->IsFlashed() > Settings.Aimbot.FlashAlpha && Settings.Aimbot.FlashCheck)
							continue;
						if (!(pEntity->GetFlags() & FL_ONGROUND) && Settings.Aimbot.FlashCheck)
							continue;
						bestfov = fov;
						besttarget = i;
						besthitbox = j;
					}
				}
			}
			else
			{

				Vector vec = pEntity->GetBonePos(Settings.Weapons[cw].Bone + 1);
				if (!Misc::pIsVisible(vSrc, vec, Hacks.LocalPlayer, pEntity, Settings.Aimbot.SmokeCheck))
					continue;
				if (Hacks.LocalPlayer->IsFlashed() && Settings.Aimbot.FlashCheck)
					continue;
				if (!(pEntity->GetFlags() & FL_ONGROUND) && Settings.Aimbot.JumpCheck)
					continue;
				float fov = Misc::GetFov(Hacks.CurrentCmd->viewangles + LocalInfo.PunchAns, Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), pEntity->GetBonePos(Settings.Weapons[cw].Bone + 1)));
				if (fov > Settings.Weapons[cw].Fov)
					continue;
				if (fov < bestfov)
				{
					bestfov = fov;
					besttarget = i;
					besthitbox = Settings.Weapons[cw].Bone + 1;
				}
			}

		}
		if (bestfov == 9999999.f)
		{
			besttarget = -1;
			besthitbox = -1;
		}
		return;

	}

	void GoToTarget(int target, int hitbox)
	{

		auto pEntity = static_cast<CBaseEntity*> (Interfaces.pEntList->GetClientEntity(target));
		if (!pEntity)
			return;



		int cw = *Hacks.LocalWeapon->GetItemDefinitionIndex();

		bool pSilent = Settings.Weapons[cw].pSilent;
		bool pSilentFov = Settings.Weapons[cw].pSilentFovNormal;

		if (Settings.aCacheAngle[pEntity->GetIndex()].bCustomSettsLegit)
		{
			pSilent = Settings.aCacheAngle[pEntity->GetIndex()].pSilent;
			pSilentFov = Settings.aCacheAngle[pEntity->GetIndex()].pSilentFov;
		}


		if (pSilent)
		{
			float fov1 = Misc::GetFov(Hacks.CurrentCmd->viewangles + LocalInfo.PunchAns, Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), pEntity->GetBonePos(Settings.Weapons[cw].Bone + 1)));
			if (fov1 < pSilentFov)
			{
				DoPerfectSilent(besttarget, besthitbox);
				return;
			}
		}



		Vector dst = Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), pEntity->GetBonePos(hitbox));
		Vector src = Hacks.CurrentCmd->viewangles;


		dst.x -= LocalInfo.PunchAns.x * (Settings.Weapons[cw].RcsX / 50.f);
		dst.y -= LocalInfo.PunchAns.y * (Settings.Weapons[cw].RcsY / 50.f);

		Vector delta = dst - src;

		delta.Normalize();

		if (!delta.IsZero())
		{

			float smoothX = Settings.Weapons[cw].Smooth;
			float finalTimeX = delta.Length();
			float smoothY = Settings.Weapons[cw].Smooth;
			float finalTimeY = delta.Length();

			if (smoothX != 0)
			{
				finalTimeX = delta.Length() / smoothX;
			}

			if (smoothY != 0)
			{
				finalTimeY = delta.Length() / smoothY;
			}

			float curAimTimeY = curAimTime;
			curAimTimeY += deltaTime;
			curAimTime += deltaTime;

			if (curAimTime > finalTimeX)
				curAimTime = finalTimeX;

			if (curAimTimeY > finalTimeY)
				curAimTimeY = finalTimeY;

			float percentX = curAimTime / finalTimeX;
			float percentY = curAimTimeY / finalTimeY;

			delta.x *= percentX;
			delta.y *= percentY;
			dst = src + delta;
		}

		Hacks.CurrentCmd->viewangles = dst.Normalize();
		Interfaces.pEngine->SetViewAngles(Hacks.CurrentCmd->viewangles);
	}





public:
	void Run()
	{
		CBaseEntity* pLocalEntity = Interfaces.pEntList->GetClientEntity(Interfaces.pEngine->GetLocalPlayer());
		if (!pLocalEntity)
			return;
		int cw = *Hacks.LocalWeapon->GetItemDefinitionIndex();
		double cur_time = clock();
		static double time = clock();
		int fire_delay = Settings.Aimbot.KillDelay;
		if (fire_delay > 0)
		{
			if (Hacks.CurrentCmd->buttons & IN_ATTACK)
			{
				if (cur_time - time < fire_delay)
				{
					Hacks.CurrentCmd->buttons &= ~IN_ATTACK;
				}
			}
			else
			{
				time = clock();
			}
		}


		curAimTime = 0.f;

		static float oldServerTime = Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;
		float serverTime = Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;
		deltaTime = serverTime - oldServerTime;
		oldServerTime = serverTime;

		if (Hacks.LocalWeapon->IsMiscWeapon())
			return;

		if (Settings.Weapons[cw].Bone == 0)
		{
			Settings.Weapons[cw].Bone = 1;
		}

		if ((!Settings.Aimbot.Enabled) || (Settings.Aimbot.OnKey && !(GetAsyncKeyState(Settings.Aimbot.Key) & 0x8000)) || (Settings.Aimbot.OnFire && !Hacks.CurrentCmd->buttons & IN_ATTACK))
			return;



		static int do_or_not = 0;
		if (do_or_not == 0)
		{
			GetBestTarget();
		}
		else
		{
			do_or_not++;
			if (do_or_not>15)
			{
				do_or_not = 0;
			}
		}




		if (besttarget == -1 || besthitbox == -1)
			return;



		GoToTarget(besttarget, besthitbox);






	}



	void DoPerfectSilent(int target, int hitbox)
	{

		int cw = *Hacks.LocalWeapon->GetItemDefinitionIndex();
		auto pEntity = static_cast<CBaseEntity*> (Interfaces.pEntList->GetClientEntity(target));
		Vector AimbotAngle = Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), pEntity->GetBonePos(hitbox));
		float flServerTime = (float)Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;
		float flNextPrimaryAttack = Hacks.LocalWeapon->NextPrimaryAttack();
		Vector localangs;
		Interfaces.pEngine->GetViewAngles(localangs);
		bool BulletTime = true;

		if (flNextPrimaryAttack > flServerTime)
			BulletTime = false;

		if (Hacks.CurrentCmd->buttons & IN_ATTACK && BulletTime)
		{
			Hacks.SendPacket = false;
			Hacks.CurrentCmd->viewangles = AimbotAngle - LocalInfo.PunchAns * (Settings.Weapons[cw].pSilentFov / 50.f);

		}
		else
		{
			Hacks.CurrentCmd->viewangles = localangs;
			std::chrono::milliseconds(2);
			Hacks.SendPacket = true;
			Hacks.CurrentCmd->buttons &= ~IN_ATTACK;

		}


	}
}Legitbot;

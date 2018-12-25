#pragma once
#include "stdafx.h"
#include "../Menu/Menu.h"
#include "Misc.h"
#include "Aimbot.h"
#include "../Utils/LocalInfo.h"
#include "../Menu/SettingsManager.h"
#define RandomInt(min, max) (rand() % (max - min + 1) + min)
#define TICKS_TO_TIME(t) (Interfaces.pGlobalVars->interval_per_tick * (t) )

int lagticks = 0;
int lagticksMax = 16;

namespace FakeLag {

	void FakeLag() {//гей лаги
		if (Settings.Ragebot.AntiAim.FakeLagAmount <= 0)
			return;
		if (Hacks.LocalPlayer->GetVecVelocity().Length2D() < 15 && Hacks.LocalPlayer->GetFlags() & FL_ONGROUND)
			return;
		if (!Interfaces.pEngine->IsInGame())
			return;
		if (Hacks.CurrentCmd->buttons & IN_ATTACK)
		{
			Hacks.SendPacket = true;
			return;
		}
		if (Hacks.LocalPlayer->GetFlags() & FL_ONGROUND && Settings.Ragebot.AntiAim.FakeLagInAirOnly)
			return;

		switch (Settings.Ragebot.AntiAim.FakeLag)
		{
		case 1: {
			if (lagticks >= lagticksMax){
				Hacks.SendPacket = true;
				lagticks = 0;
			}
			else{
				Hacks.SendPacket = lagticks < lagticksMax - Settings.Ragebot.AntiAim.FakeLagAmount;
			}
		}
		break;
		case 2: {
			static int FakelagFactor = Settings.Ragebot.AntiAim.FakeLagAmount;
			Hacks.SendPacket = 1 ? !(lagticks % (FakelagFactor + 1)) : 1;
			if (Hacks.SendPacket) {
				FakelagFactor = (rand() % 7) + 6;
			}
		}
		break;
		case 3: {
			static int FakelagFactor = 2;
			Hacks.SendPacket = 1 ? !(lagticks % (FakelagFactor + 1)) : 1;
			if (Hacks.SendPacket)
				FakelagFactor = max(1, min((int)(fabs(Hacks.LocalPlayer->GetVecVelocity().Length() / 80.f)), Settings.Ragebot.AntiAim.FakeLagAmount));
		}
	    break;
		}
		lagticks++;
	}

}

class AntiAim
{
public:
	bool ShouldAA = true;

private:
	void doFakeWalk()
	{
		if (GetAsyncKeyState(Settings.Ragebot.AntiAim.FakeWalkButton))
		{

			if (Hacks.LocalPlayer->GetVecVelocity().Length2D() > Settings.Ragebot.AntiAim.FakeWalkSpeed)
			{
				//Hacks.CurrentCmd->tick_count += 15;
				Hacks.CurrentCmd->buttons |= Hacks.LocalPlayer->GetMoveType() == IN_BACK;
				Hacks.CurrentCmd->forwardmove = Hacks.CurrentCmd->sidemove = 0.f;
			}
			else
			{
				//Interfaces.pGlobalVars->frametime *= Hacks.LocalPlayer->GetVecVelocity().Length2D();
				Hacks.CurrentCmd->buttons |= Hacks.LocalPlayer->GetMoveType() == IN_FORWARD;
			}
		}
	}

	static inline bool IsNearEqual(float v1, float v2, float Tolerance)
	{
		return std::abs(v1 - v2) <= std::abs(Tolerance);
	}

	void AtTargets(Vector& viewangles)
	{

	}

	void AntiAimBinds()
	{
		if (Hacks.LocalPlayer->GetVecVelocity().Length2D() < 15.f)
		{
			if (GetKeyState(Settings.Ragebot.AntiAim.ShuffleFlipKey) /*& 0x1*/)

			{
				Settings.Ragebot.AntiAim.RealYawAdd = 90;
				Settings.Ragebot.AntiAim.FakeYawAdd = -90;
			}
			else
			{
				Settings.Ragebot.AntiAim.RealYawAdd = -90;
				Settings.Ragebot.AntiAim.FakeYawAdd = 90;
			}
		}
		else
		{
			if (GetKeyState(Settings.Ragebot.AntiAim.ShuffleFlipKey) /*& 0x1*/)
			{
				Settings.Ragebot.AntiAim.Move.YawAdd = 90;
				Settings.Ragebot.AntiAim.Move.FakeYawAdd = -90;
			}
			else
			{
				Settings.Ragebot.AntiAim.Move.YawAdd = -90;
				Settings.Ragebot.AntiAim.Move.FakeYawAdd = 90;
			}
		}
	}

	bool lbyupdate() {
		/*------определения------*/
		static float next1 = 0;
		float tickbase = TICKS_TO_TIME(Hacks.LocalPlayer->GetTickBase());
		float flServerTime = (Hacks.LocalPlayer->GetTickBase()  * Interfaces.pGlobalVars->interval_per_tick);
		/*----------------------*/

		if (next1 - tickbase > 1.1) {
			next1 = 0;
			return false;
		}

		if (Hacks.LocalPlayer->GetVecVelocity().Length2D() > 2) {
			next1 = flServerTime;
			return false;
		}

		if ((next1 + 1.1 <= flServerTime) && (Hacks.LocalPlayer->GetFlags() & FL_ONGROUND) && (Hacks.LocalPlayer->GetVecVelocity().Length2D() < 2)) {
			next1 = flServerTime + Interfaces.pGlobalVars->interval_per_tick;
			return true;
		}
		return false;
	}

	void Yaw(int index, bool yFlip, bool sFlip) {

		float OldLBY;
		//float LBYBreakerTimer;
		bool bSwitch;
		static float LastLBYUpdateTime;
		static float NextLBYUpdate1;


		Vector views = Hacks.CurrentCmd->viewangles;

		if (Hacks.LocalPlayer->GetFlags() & FL_ONGROUND)
		{
			switch (index)
			{
			case 1: {
				yFlip ? views.y -= 179 : views.y += 179;
			}
			break;
			case 2: // Shuffle
			{
				if (!Hacks.SendPacket) {
					if (lbyupdate())
						views.y += Settings.Ragebot.AntiAim.lby_delta;
				}
					
			}
			break;

			case 3:
			{
				float spin = 1000 * Settings.Ragebot.AntiAim.SpinSpeed * (Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick);

				/*clamp*/
				while (spin > 180)
					spin -= 360;
				while (spin < 180)
					spin += 360;
				/*----*/
				views.y = spin;
			}
			break;
			case 4:
			{
				static bool minus;
				static bool flip;
				float original = views.y;
				flip = !flip;
				static int jitter_range = 0;

				if (!minus)
					jitter_range += Settings.Ragebot.AntiAim.JitterSpeed;
				else
					jitter_range -= Settings.Ragebot.AntiAim.JitterSpeed;

				if (jitter_range < -Settings.Ragebot.AntiAim.JitterRange)
					minus = false;
				else if (jitter_range > Settings.Ragebot.AntiAim.JitterRange)
					minus = true;

				if (flip)
					views.y -= jitter_range;
				else
					views.y += jitter_range;
			}
			break;
			case 5:
			{
				static int Ticks;
				views.y -= Ticks;
				Ticks += 2;

				if (Ticks > 240)
					Ticks = 120;
			}
			break;
			}
		}
		else
		{
			static int Ticks;
			views.y -= Ticks;
			Ticks += 2;

			if (Ticks > 240)
				Ticks = 120;

			Settings.Ragebot.AntiAim.RealYawAdd = 0;
			Settings.Ragebot.AntiAim.FakeYawAdd = 0;

			Settings.Ragebot.AntiAim.Move.YawAdd = 0;
			Settings.Ragebot.AntiAim.Move.FakeYawAdd = 0;
		}
		Hacks.CurrentCmd->viewangles = views;
	}

	void Pitch(bool yFlip, bool sFlip)
	{
		Vector views = Hacks.CurrentCmd->viewangles;
		int pitch = Settings.Ragebot.AntiAim.Pitch;
		switch (pitch)
		{
		case 1://down
		{
			if (Settings.Misc.UT){
				views.x += 89;
			}
			else {
				if (Hacks.SendPacket) {
					views.x -= 180;
				}
				else {
					views.x -= 991;
				}
			}
		}
		break;
		case 2: { // up
			if (Settings.Misc.UT) {
				views.x -= 89;
			}
			else {
				if (Hacks.SendPacket) {
					views.x += 180;
				}
				else {
					views.x += 991;
				}
			}
		}
		break;
		case 3: {// zero
			if (Settings.Misc.UT) {
				if (Hacks.SendPacket) {
					views.x += 0;
				}
				else {
					views.x += 89;
				}
			}
			else {
				if (Hacks.SendPacket) {
					views.x += 1080;
				}
				else {
					views.x -= 991;
				}
			}
		}
		break;
		}
		Hacks.CurrentCmd->viewangles = views;
	}

	void MovePitch(bool yFlip, bool sFlip)
	{
		Vector views = Hacks.CurrentCmd->viewangles;
		int pitch = Settings.Ragebot.AntiAim.Move.Pitch;
		switch (pitch)
		{
		case 1://down
		{
			if (Settings.Misc.UT) {
				views.x += 89;
			}
			else {
				if (Hacks.SendPacket) {
					views.x -= 180;
				}
				else {
					views.x -= 991;
				}
			}
		}
		break;
		case 2: { // up
			if (Settings.Misc.UT) {
				views.x -= 89;
			}
			else {
				if (Hacks.SendPacket) {
					views.x += 180;
				}
				else {
					views.x += 991;
				}
			}
		}
				break;
		case 3: {// zero
			if (Settings.Misc.UT) {
				if (Hacks.SendPacket) {
					views.x += 0;
				}
				else {
					views.x += 89;
				}
			}
			else {
				if (Hacks.SendPacket) {
					views.x += 1080;
				}
				else {
					views.x -= 991;
				}
			}
		}
				break;
		}
		Hacks.CurrentCmd->viewangles = views;
	}
	void AA(Vector& views)
	{
		static bool flip;
		flip = rand() % 2 ? false : true;
		static bool sFlip = false;
		static bool bSwitch;
		bSwitch = !bSwitch;


		Hacks.SendPacket = bSwitch;

		if ((Interfaces.pGlobalVars->tickcount % 100) > 1 && (Interfaces.pGlobalVars->tickcount % 100) < 50)
			sFlip = true;
		else
			sFlip = false;


		if (Settings.Ragebot.AntiAim.FakeWalk)
			doFakeWalk();


		FakeLag::FakeLag();
		if (!Misc::Shooting);

		//DoAntiAimbot

		if (Hacks.LocalPlayer->GetVecVelocity().Length2D() < 20.f || GetAsyncKeyState(Settings.Ragebot.AntiAim.FakeWalkButton))
		{
			if (Hacks.SendPacket)
			{
				Yaw(Settings.Ragebot.AntiAim.FakeYaw, flip, sFlip);
				if (Settings.Ragebot.AntiAim.FakeYaw == 4 || Settings.Ragebot.AntiAim.FakeYaw == 2)
					views.y -= Settings.Ragebot.AntiAim.FakeYawAdd;
				else
					views.y += Settings.Ragebot.AntiAim.FakeYawAdd;
			}
			else
			{
				Yaw(Settings.Ragebot.AntiAim.RealYaw, flip, sFlip);
				if (Settings.Ragebot.AntiAim.RealYaw == 4 || Settings.Ragebot.AntiAim.RealYaw == 2)
					views.y -= Settings.Ragebot.AntiAim.RealYawAdd;
				else
					views.y += Settings.Ragebot.AntiAim.RealYawAdd;
			}

			Pitch(flip, sFlip);
		}
		else
		{
			if (!Hacks.SendPacket)
			{
				Yaw(Settings.Ragebot.AntiAim.Move.Yaw, flip, sFlip); // Real yaw	
				if (Settings.Ragebot.AntiAim.Move.Yaw == 4 || Settings.Ragebot.AntiAim.Move.Yaw == 2)
					views.y -= Settings.Ragebot.AntiAim.Move.YawAdd;
				else
					views.y += Settings.Ragebot.AntiAim.Move.YawAdd;
			}
			else
			{
				Yaw(Settings.Ragebot.AntiAim.Move.FakeYaw, flip, sFlip); // Fake yaw
				if (Settings.Ragebot.AntiAim.Move.FakeYaw == 4 || Settings.Ragebot.AntiAim.Move.Yaw == 2)
					views.y -= Settings.Ragebot.AntiAim.Move.FakeYawAdd;
				else
					views.y += Settings.Ragebot.AntiAim.Move.FakeYawAdd;
			}

			MovePitch(flip, sFlip);
		}
		if (Settings.Ragebot.AntiAim.ShuffleFlipKey > 0 && Hacks.LocalPlayer->GetFlags() & FL_ONGROUND && Settings.Misc.UT)
			AntiAimBinds();

		if (!Hacks.SendPacket)
			Hacks.VisualAngle = views;
		else
			Hacks.FakeAnges = views;
	}
public:
	void Run()
	{
		if (!Settings.Ragebot.AntiAim.Enable)
			return;
		if (Hacks.CurrentCmd->buttons & IN_USE)
			return;
		if (Hacks.LocalPlayer->GetMoveType() == MOVETYPE_LADDER || Hacks.LocalPlayer->GetMoveType() == MOVETYPE_NOCLIP)
			return;
		if (Hacks.LocalWeapon->IsKnife() && !Settings.Ragebot.AntiAim.AAWithKnife)
			return;
		if (Hacks.LocalWeapon->IsGrenade() && !Settings.Ragebot.AntiAim.AAWithGrenades)
			return;
		if (GetAsyncKeyState(71))
			return;

	
		if (Settings.Ragebot.AntiAim.AtTarget)
			AtTargets(Hacks.CurrentCmd->viewangles);

		ShouldAA = true;

		if (ShouldAA)
			AA(Hacks.CurrentCmd->viewangles);
	}
} AA;

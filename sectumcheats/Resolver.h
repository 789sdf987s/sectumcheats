#pragma once
#include "Backtrack.h"
#include "Header.h"

#define TICK_INTERVAL			(Interfaces.pGlobalVars->interval_per_tick)
#define TIME_TO_TICKS2( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )


int CBaseEntity::GetSequenceActivity(int sequence)
{
	auto hdr = Interfaces.g_pModelInfo->GetStudioModel(this->GetModel());

	if (!hdr)
		return -1;

	static auto client2 = GetModuleHandleW(L"client_panorama.dll");
	static auto getSequenceActivity = (DWORD)(Utils.pattern_scan(client2, "55 8B EC 83 7D 08 FF 56 8B F1 74"));
	static auto GetSequenceActivity = reinterpret_cast<int(__fastcall*)(void*, studiohdr_t*, int)>(getSequenceActivity);

	return GetSequenceActivity(this, hdr, sequence);
}

static inline bool isNearEq(float v1, float v2, float Tolerance)
{
	return std::abs(v1 - v2) <= std::abs(Tolerance);
}

struct ResolverData
{
	float simtime, flcycle[13], flprevcycle[13], flweight[13], flweightdatarate[13], fakewalkdetection[2], fakeanglesimtimedetection[2], fakewalkdetectionsimtime[2];
	float yaw, addyaw, lbycurtime;
	float shotsimtime, oldlby, lastmovinglby, balanceadjustsimtime, balanceadjustflcycle;
	int fakeanglesimtickdetectionaverage[4], amountgreaterthan2, amountequal1or2, amountequal0or1, amountequal1, amountequal0, resetmovetick, resetmovetick2;
	int tick, balanceadjusttick, missedshots, activity[13];
	bool bfakeangle, bfakewalk, playerhurtcalled, weaponfirecalled;
	Vector shotaimangles, hitboxPos, balanceadjustaimangles;
	uint32_t norder[13];
	char* resolvermode = "NONE", *fakewalk = "Not Moving";
};

extern ResolverData pResolverData[64];

bool isPartOf(char *a, char *b) {
	if (std::strstr(b, a) != NULL) {    //Strstr says does b contain a
		return true;
	}
	return false;
}

float yawnormalize(float value)
{
	while (value > 180)
		value -= 360.f;

	while (value < -180)
		value += 360.f;
	return value;
}

void normalize(Vector& vecIn) // fuck this cod
{
	for (int axis = 0; axis < 3; ++axis)
	{
		while (vecIn[axis] > 180.f)
			vecIn[axis] -= 360.f;

		while (vecIn[axis] < -180.f)
			vecIn[axis] += 360.f;

	}

	vecIn[2] = 0.f;
}

class Resolver2
{
public:
	void AntiAimResolver()
	{
		for (auto i = 0; i <= Interfaces.pEntList->GetHighestEntityIndex(); i++)
		{
			auto pEntity = static_cast<CBaseEntity*>(Interfaces.pEntList->GetClientEntity(i));
			if (pEntity == nullptr)
				continue;
			if (pEntity == Hacks.LocalPlayer)
				continue;
			if (!pEntity->isAlive())
				continue;
			if (!(pEntity->GetHealth() > 0))
				continue;
			if (pEntity->GetTeam() == Hacks.LocalPlayer->GetTeam())
				continue;
			if (pEntity->IsDormant())
				continue;
			if (pEntity->HasGunGameImmunity())
				continue;

			if (Settings.Ragebot.Resolver == 1)
			{
				if (Global::weaponfirecalled)
				{
					if (!Global::playerhurtcalled)
					{
						if (isPartOf("Brute", pResolverData[i].resolvermode))
						{
							pResolverData[i].addyaw += 65.f;
							yawnormalize(pResolverData[i].addyaw);
						}
					}
					else
						Global::playerhurtcalled = false;

					Global::weaponfirecalled = false;
				}
				for (int w = 0; w < 13; w++)
				{
					CBaseEntity::AnimationLayer currentLayer = pEntity->GetAnimOverlay(w);
					const int activity = pEntity->GetSequenceActivity(currentLayer.m_nSequence);
					float flcycle = currentLayer.m_flCycle, flprevcycle = currentLayer.m_flPrevCycle, flweight = currentLayer.m_flWeight, flweightdatarate = currentLayer.m_flWeightDeltaRate;
					uint32_t norder = currentLayer.m_nOrder;
					Vector* pAngles = pEntity->GetEyeAnglesPointer();

					if (norder == 12)
					{
						pResolverData[pEntity->GetIndex()].fakewalkdetection[Hacks.CurrentCmd->command_number % 2] = flweight;
						pResolverData[pEntity->GetIndex()].fakewalkdetectionsimtime[Hacks.CurrentCmd->command_number % 2] = pEntity->GetSimulationTime();
						for (int t = 0; t < 2; t++)
						{
							int resetmovetick2{};
							if (pResolverData[pEntity->GetIndex()].fakewalkdetection[t] > 0.f)
								pResolverData[pEntity->GetIndex()].resetmovetick = t;
							else if (t == 1)
							{
								if (pEntity->GetVecVelocity().Length2D() < 0.50 && flweight == 0.f)
								{
									pResolverData[pEntity->GetIndex()].fakewalk = "Not Moving";
									pResolverData[pEntity->GetIndex()].bfakewalk = false;
								}
							}
							else {
								if (pResolverData[pEntity->GetIndex()].resetmovetick > 0)
									resetmovetick2 = pResolverData[pEntity->GetIndex()].resetmovetick - 1;
								else
									resetmovetick2 = pResolverData[pEntity->GetIndex()].resetmovetick + 1;

								if (pResolverData[pEntity->GetIndex()].fakewalkdetection[resetmovetick2] == 0.f)
								{
									pResolverData[pEntity->GetIndex()].fakewalk = "Fake Walking";
									pResolverData[pEntity->GetIndex()].bfakewalk = true;
								}
							}
						}
					}

					if (pEntity->GetVecVelocity().Length2D() >= 0.50 && norder == 6 && flweight >= 0.550000 || pEntity->GetVecVelocity().Length2D() >= 0.50 && norder == 5 && flweight >= 0.550000 || !pResolverData[pEntity->GetIndex()].bfakewalk && pEntity->GetVecVelocity().Length2D() >= 0.50)
					{
						pResolverData[pEntity->GetIndex()].lastmovinglby = pEntity->GetLowerBodyYaw();
						pResolverData[pEntity->GetIndex()].resolvermode = "LBY Move";
						pResolverData[pEntity->GetIndex()].fakewalk = "No Fake Walk";
						pAngles->y = pEntity->GetLowerBodyYaw();
					}
					else
					{

						if (activity == ACT_CSGO_IDLE_TURN_BALANCEADJUST && flweight <= 1.0f && flcycle <= 0.851166f) //[06:40AM] == > Activity > 979, Weight > 0.494118, Cycle > 0.851166
						{
							pResolverData[pEntity->GetIndex()].resolvermode = "Less BA Brute";
							//G::FakeDetection[i] = 1;
							pAngles->y = pResolverData[pEntity->GetIndex()].lastmovinglby;
							pResolverData[pEntity->GetIndex()].addyaw > 0.f ? pAngles->y = pEntity->GetLowerBodyYaw() - pResolverData[pEntity->GetIndex()].addyaw - 77.5 : pAngles->y = pEntity->GetLowerBodyYaw() - 77.5;
						}

						else if (activity == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING || activity == ACT_CSGO_IDLE_TURN_BALANCEADJUST && flweight == 0.000000f && flcycle >= 0.955994) // High delta
						{
							pResolverData[pEntity->GetIndex()].resolvermode = "LBY Brute";
							//G::FakeDetection[i] = 2;
							pAngles->y = pResolverData[pEntity->GetIndex()].lastmovinglby;
							pResolverData[pEntity->GetIndex()].addyaw > 0.f ? pAngles->y = pEntity->GetLowerBodyYaw() - pResolverData[pEntity->GetIndex()].addyaw : pAngles->y = pEntity->GetLowerBodyYaw() - 17.5;
						}
					}
					normalize(*pAngles);
				}
			}
		}
	}
};

ResolverData pResolverData[64];
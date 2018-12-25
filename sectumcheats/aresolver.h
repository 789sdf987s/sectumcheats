#pragma once
#include "Backtrack.h"

#define TICK_INTERVAL			(Interfaces.pGlobalVars->interval_per_tick)
#define TIME_TO_TICKS2( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )

/*int CBaseEntity::GetSequenceActivity(int sequence)
{
	auto hdr = Interfaces.g_pModelInfo->GetStudioModel(this->GetModel());

	if (!hdr)
		return -1;

	static auto client2 = GetModuleHandleW(L"client_panorama.dll");
	static auto getSequenceActivity = (DWORD)(Utils.pattern_scan(client2, "55 8B EC 83 7D 08 FF 56 8B F1 74"));
	static auto GetSequenceActivity = reinterpret_cast<int(__fastcall*)(void*, studiohdr_t*, int)>(getSequenceActivity);

	return GetSequenceActivity(this, hdr, sequence);
}*/

class Resolver3
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

			if (Settings.Ragebot.Resolver)
			{
				CBaseEntity::AnimationLayer layer3 = pEntity->GetAnimOverlay(3);
				int activity = pEntity->GetSequenceActivity(layer3.m_nSequence);
				auto yaw = pEntity->GetEyeAnglesPointer()->y;
				auto lby = pEntity->pelvisangs();
				static float lastlby[65];
				if (Hacks.LocalPlayer->isAlive())
				{

					if (pEntity->GetFlags() & FL_ONGROUND) {
						if (pEntity->GetVecVelocity().Length2D() > 11.f) {
							lastlby[i] = pEntity->pelvisangs();
							pEntity->GetEyeAnglesPointer()->y = pEntity->pelvisangs();
							float simtime = pEntity->GetSimulationTime();
							Hitbox box{};
							if (!box.GetHitbox(pEntity, 0))
								continue;
							Vector hitboxPos{};
							float damage = box.GetBestPoint(hitboxPos);
							RageBackData[pEntity->GetIndex()][Hacks.CurrentCmd->command_number % 16].damage = damage;
							RageBackData[pEntity->GetIndex()][Hacks.CurrentCmd->command_number % 16].simtime = simtime;
							RageBackData[pEntity->GetIndex()][Hacks.CurrentCmd->command_number % 16].hitboxPos = hitboxPos;
						}
						else {
							if (activity == ACT_CSGO_IDLE_TURN_BALANCEADJUST && layer3.m_flWeight == 0 && layer3.m_flCycle >= 0.97000) {
								pResolverData[pEntity->GetIndex()].balanceadjustsimtime = pEntity->GetSimulationTime();
								if (backtracking->IsTickValid(pResolverData[pEntity->GetIndex()].balanceadjustsimtime))
								{
									Hitbox box{};
									if (!box.GetHitbox(pEntity, 0))
										return;
									pResolverData[pEntity->GetIndex()].balanceadjustaimangles = box.GetCenter();

									if (fabsf(pEntity->GetEyeAnglesPointer()->y - pEntity->pelvisangs()) > 35.f) {
										pEntity->GetEyeAnglesPointer()->y = pEntity->GetEyeAnglesPointer()->y - pEntity->pelvisangs(); /*lby + g_Math.RandomFloat(-35, 35);*/
									}
									else {
										pEntity->GetEyeAnglesPointer()->y = lastlby[i];
									}

									if (Hacks.CurrentCmd && Hacks.CurrentCmd->buttons & IN_ATTACK)
										backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(pResolverData[pEntity->GetIndex()].balanceadjustsimtime));
								}
							}
							else if (backtracking->IsTickValid(pResolverData[pEntity->GetIndex()].balanceadjustsimtime))
							{
								if (Hacks.CurrentCmd && Hacks.CurrentCmd->buttons & IN_ATTACK)
									backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(pResolverData[pEntity->GetIndex()].balanceadjustsimtime));
							}
							else if (activity == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING && layer3.m_flWeight == 0.f && layer3.m_flCycle > 0.92f) {
								pEntity->GetEyeAnglesPointer()->y = pEntity->pelvisangs();
							}
							else {
								if (fabsf(pEntity->GetEyeAnglesPointer()->y - pEntity->pelvisangs()) > 35.f) {
									pEntity->GetEyeAnglesPointer()->y = pEntity->GetEyeAnglesPointer()->y - pEntity->pelvisangs(); /*lby + g_Math.RandomFloat(-35, 35);*/
								}
								else {
									pEntity->GetEyeAnglesPointer()->y = lastlby[i];
								}
							}
						}
					}
					else {
						lastlby[i] = 0;
						pEntity->GetEyeAnglesPointer()->y = fabsf(pEntity->GetEyeAnglesPointer()->y - pEntity->pelvisangs() + g_Math.RandomFloat(-35, 35));
					}
				}
				else
				{
					lastlby[i] = 0;
					pResolverData[pEntity->GetIndex()].balanceadjustsimtime = 0;
					RageBackData[pEntity->GetIndex()][Hacks.CurrentCmd->command_number % 16].damage = 0;
					RageBackData[pEntity->GetIndex()][Hacks.CurrentCmd->command_number % 16].simtime = 0;
				}
			}
		}
	}
};
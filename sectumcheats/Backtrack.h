#pragma once
#include "../../stdafx.h"

#include "./Tools/Utils/Hitbox.h"
#include "../../SDK/SDK.h"
//#include "Resolver.h"
#include <algorithm>

#define TICK_INTERVAL			(Interfaces.pGlobalVars->interval_per_tick)
#define TIME_TO_TICKS2( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define NOMINMAX
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#define max(a,b)            (((a) > (b)) ? (a) : (b))

inline Vector AngleVector(Vector meme)
{
	auto sy = sin(meme.y / 180.f * static_cast<float>(PI));
	auto cy = cos(meme.y / 180.f * static_cast<float>(PI));

	auto sp = sin(meme.x / 180.f * static_cast<float>(PI));
	auto cp = cos(meme.x / 180.f* static_cast<float>(PI));

	return Vector(cp*cy, cp*sy, -sp);
}

inline float DistPointToLine(Vector Point, Vector LineOrigin, Vector Dir)
{
	auto PointDir = Point - LineOrigin;

	auto TempOffset = PointDir.Dot(Dir) / (Dir.x*Dir.x + Dir.y*Dir.y + Dir.z*Dir.z);
	if (TempOffset < 0.000001f)
		return FLT_MAX;

	auto PerpendicularPoint = LineOrigin + (Dir * TempOffset);

	return (Point - PerpendicularPoint).Length();
}

struct lbyRecords
{
	int tick_count;
	float lby;
	Hitbox headPosition;
};
struct RageBackTrackData
{
	float simtime;
	float flcycle[13];
	float flprevcycle[13];
	float flweight[13];
	float flweightdatarate[13];
	float yaw;
	uint32_t norder[13];
	Vector hitboxPos;
	int tick;
	int balanceadjusttick;
	float balanceadjustsimtime;
	int activity[13];
	float damage;
};
struct LegitBackTrackData
{
	float simtime;
	Vector hitboxPos;
	int tick;
};

class BackTrack
{
	int latest_tick;
	bool Lowerbody(int tick);
	void UpdateRecord(int i);
public:
	lbyRecords records[65];
	float GetLerpTime();
	bool IsTickValid(int TargetIndex, int tick);
	bool IsTickValid(float simtime);
	bool BackTrackPlayer(CInput::CUserCmd* cmd, CBaseEntity* pEntity, int Tick);
	bool BackTrackPlayer(CInput::CUserCmd* cmd, int Tick);
	void RageBackTrack(CInput::CUserCmd* cmd, CBaseEntity* pEntity, float damage, Vector Aimspot, Vector& aimPoint);
	bool RunLBYBackTrack(int i, CInput::CUserCmd* cmd, Hitbox aimPoint);
	void Update(int tick_count);
	void legitBackTrack(CInput::CUserCmd* cmd, CBaseEntity* pLocal);
};

extern LegitBackTrackData LegitBackData[64][16];
extern RageBackTrackData RageBackData[64][16];
extern BackTrack* backtracking;

double clamp(double x, double upper, double lower)
{
	return min(upper, max(x, lower));
}

bool BackTrack::RunLBYBackTrack(int i, CInput::CUserCmd* cmd, Hitbox aimPoint)
{
	if (IsTickValid(records[i].tick_count))
	{
		aimPoint = records[i].headPosition;
		cmd->tick_count = records[i].tick_count;
		return true;
	}
	return false;
}

float BackTrack::GetLerpTime()
{
	float updaterate = Interfaces.g_ICVars->FindVar("cl_updaterate")->GetFloat();
	ConVar* minupdate = Interfaces.g_ICVars->FindVar("sv_minupdaterate");
	ConVar* maxupdate = Interfaces.g_ICVars->FindVar("sv_maxupdaterate");

	updaterate = static_cast<int>(clamp(updaterate, minupdate->GetFloat(), maxupdate->GetFloat()));

	float ratio = Interfaces.g_ICVars->FindVar("cl_interp_ratio")->GetFloat();

	if (ratio == 0)
		ratio = 1.0f;

	float lerp = Interfaces.g_ICVars->FindVar("cl_interp")->GetFloat();
	ConVar* cmin = Interfaces.g_ICVars->FindVar("sv_client_min_interp_ratio");
	ConVar* cmax = Interfaces.g_ICVars->FindVar("sv_client_max_interp_ratio");

	if (cmin && cmax && cmin->GetFloat() != 1)
		ratio = clamp(ratio, cmin->GetFloat(), cmax->GetFloat());

	return max(lerp, ratio / updaterate);
}

bool BackTrack::IsTickValid(int TargetIndex, int tick)
{
	float correct = 0;

	correct += Interfaces.pEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);

	ConVar* sv_maxunlag = Interfaces.g_ICVars->FindVar("sv_maxunlag");
	correct = clamp(correct, 0, sv_maxunlag->GetFloat());

	float deltaTime = correct - (Interfaces.pGlobalVars->curtime - LegitBackData[TargetIndex][tick].simtime);

	if (fabsf(deltaTime) > 0.2f)
	{
		return false;
	}

	return true;
}

bool BackTrack::IsTickValid(float simtime)
{
	float correct = 0;

	correct += Interfaces.pEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);

	ConVar* sv_maxunlag = Interfaces.g_ICVars->FindVar("sv_maxunlag");
	correct = clamp(correct, 0, sv_maxunlag->GetFloat());

	if (Interfaces.pGlobalVars)
	{
		float deltaTime = correct - (Interfaces.pGlobalVars->curtime - simtime);

		if (fabsf(deltaTime) > 0.2f)
		{
			return false;
		}
	}
	else
		return false;

	return true;
}

bool BackTrack::BackTrackPlayer(CInput::CUserCmd* cmd, CBaseEntity* pEntity, int Tick)
{
	if (pEntity && !pEntity->IsDormant() && pEntity->GetTeam() != Hacks.LocalPlayer->GetTeam() && pEntity->isAlive())
	{
		int i = pEntity->GetIndex();

		if (LegitBackData[i][Tick % 13].simtime == 0.0f)
			return TIME_TO_TICKS2(pEntity->GetSimulationTime() + GetLerpTime());

		int iLerpTicks = TIME_TO_TICKS2(GetLerpTime());
		int iTargetTickCount = Tick - iLerpTicks;

		cmd->tick_count = iTargetTickCount;
		return true;
	}
	return false;
}

bool BackTrack::BackTrackPlayer(CInput::CUserCmd* cmd, int Tick)
{
	//int iLerpTicks = TIME_TO_TICKS2(GetLerpTime());
	int iTargetTickCount = Tick;

	if (Hacks.CurrentCmd && Hacks.VerifiedCmd)
		cmd->tick_count = iTargetTickCount;
	else
		return false;
	return true;
}

//second: check if the animation state is 979, then do your flip however you want >120
//check if animation state is 973, then do your flip however you want <120

void BackTrack::RageBackTrack(CInput::CUserCmd* cmd, CBaseEntity* pEntity, float damage, Vector Aimspot, Vector& aimPoint)
{
	for (int t = 0; t < cmd->command_number % 16; t++)
	{
		int activity[13];
		float flcycle[13], flprevcycle[13], flweight[13], flweightdatarate[13];
		uint32_t norder[13];
		for (int w = 0; w < 13; w++)
		{
			CBaseEntity::AnimationLayer currentLayer = *pEntity->GetAnimOverlays();
			activity[w] = pEntity->GetSequenceActivity(currentLayer.m_nSequence);
			flcycle[w] = currentLayer.m_flCycle;
			flprevcycle[w] = currentLayer.m_flPrevCycle;
			flweight[w] = currentLayer.m_flWeight;
			flweightdatarate[w] = currentLayer.m_flWeightDeltaRate;
			norder[w] = currentLayer.m_nOrder;
		}
		int i = pEntity->GetIndex();
		RageBackData[i][t] = RageBackTrackData{
			pEntity->GetSimulationTime(),
		{ flcycle[0], flcycle[1], flcycle[2], flcycle[3], flcycle[4], flcycle[5], flcycle[6], flcycle[7], flcycle[8], flcycle[9], flcycle[10], flcycle[11], flcycle[12] },
		{ flprevcycle[0], flprevcycle[1], flprevcycle[2], flprevcycle[3], flprevcycle[4], flprevcycle[5], flprevcycle[6], flprevcycle[7], flprevcycle[8], flprevcycle[9], flprevcycle[10], flprevcycle[11], flprevcycle[12] },
		{ flweight[0], flweight[1], flweight[2], flweight[3], flweight[4], flweight[5], flweight[6], flweight[7], flweight[8], flweight[9], flweight[10], flweight[11], flweight[12] },
		{ flweightdatarate[0], flweightdatarate[1], flweightdatarate[2], flweightdatarate[3], flweightdatarate[4], flweightdatarate[5], flweightdatarate[6], flweightdatarate[7], flweightdatarate[8], flweightdatarate[9], flweightdatarate[10], flweightdatarate[11], flweightdatarate[12] },
		0.f,
		{ norder[0], norder[1], norder[2], norder[3], norder[4], norder[5], norder[6], norder[7], norder[8], norder[9], norder[10], norder[11], norder[12] },
		Aimspot,
		t,
		t,
		pEntity->GetSimulationTime(),
		{ activity[0], activity[1], activity[2], activity[3], activity[4], activity[5], activity[6], activity[7], activity[8], activity[9], activity[10], activity[11], activity[12] }
		};

	}
}

void BackTrack::legitBackTrack(CInput::CUserCmd* cmd, CBaseEntity* pLocal)
{
	int bestTargetIndex = -1;
	int tickxd = 16;
	float bestFov = FLT_MAX;
	player_info_t info{};
	if (!pLocal->isAlive())
		return;

	for (int i = 0; i < Interfaces.pEngine->GetMaxClients(); i++)
	{
		auto entity = Interfaces.pEntList->GetClientEntity(i);

		if (!entity || !pLocal)
			continue;

		if (entity == pLocal)
			continue;

		if (!Interfaces.pEngine->GetPlayerInfo(i, &info))
			continue;

		if (entity->IsDormant())
			continue;

		if (entity->GetTeam() == pLocal->GetTeam())
			continue;

		if (entity->isAlive())
		{
			float simtime = entity->GetSimulationTime();
			Hitbox box;
			if (!box.GetHitbox(entity, 0))
				continue;

			Vector hitboxPos = box.GetCenter();

			//headPositions[i][cmd->command_number % 13] = backtrackData{ simtime, hitboxPos };
			LegitBackData[i][cmd->command_number % tickxd] = LegitBackTrackData{ simtime, hitboxPos, cmd->command_number % tickxd };
			Vector ViewDir = AngleVector(cmd->viewangles + (pLocal->GetPunchAngle() * 2.f));
			float FOVDistance = DistPointToLine(hitboxPos, pLocal->GetEyePosition(), ViewDir);

			if (bestFov > FOVDistance)
			{
				bestFov = FOVDistance;
				bestTargetIndex = i;
			}
		}
	}

	float bestTargetSimTime{};
	int BestTick{};
	if (bestTargetIndex != -1)
	{
		float tempFloat = FLT_MAX;
		Vector ViewDir = AngleVector(cmd->viewangles + (pLocal->GetPunchAngle() * 2.f));
		for (int t = 0; t < 15; ++t)
		{
			float tempFOVDistance = DistPointToLine(LegitBackData[bestTargetIndex][t].hitboxPos, pLocal->GetEyePosition(), ViewDir);
			if (tempFloat > tempFOVDistance && LegitBackData[bestTargetIndex][t].simtime > pLocal->GetSimulationTime() - 1)
			{
				if (IsTickValid(bestTargetIndex, t))
				{
					BestTick = t;
					tempFloat = tempFOVDistance;
					bestTargetSimTime = LegitBackData[bestTargetIndex][t].simtime;
				}
			}
		}
		if (cmd->buttons & IN_ATTACK)
		{
			BackTrackPlayer(cmd, TIME_TO_TICKS2(bestTargetSimTime));
		}
		//Vector Aimangles{};
		//Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), LegitBackData[bestTargetIndex][BestTick].hitboxPos, Aimangles);
		//Interfaces.pEngine->SetViewAngles(Aimangles);
	}
}

BackTrack* backtracking = new BackTrack();
LegitBackTrackData LegitBackData[64][16];
RageBackTrackData RageBackData[64][16];
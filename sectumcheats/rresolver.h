#pragma once

#include "Header.h"

#include <deque>

#define MAX_LOGTICKS 32

struct PlayerSettings
{
	bool friendly;
	float threatMultiplier; //Used in GetBestTarget to prefer the player
	float healthMultiplier; //Used in HitScan to prefer headshots or bodyaim
	float sortMultiplier; //Used to make the sort function cycle through players
	int shotCount; //Per player shot count used by resolver
	float shotCountTime; //The last time shotcount was increased, used to reset it after some time
	bool shootOnlyLBY; //Should aimbot shoot only at lby updates?
	float lbyUpdateTime; //Set by resolver, predicted lby update time
	bool useLoggedAngles; //Should use logged angles in the resolver (set automatically)
	float loggedLBYDelta; //LBY delta to keep using for a few shots
	int logFailCount; //Counts how many times the logged delta fails on second use, used to determine randomized offsets
	int shotCountLog; //Shot count when the angle was logged
	float lastSimTimeDelta; //I added this comment here
	float averageSimTimeDelta; //Used in lagcomp
	bool inairFakelag; //B1g inair only fakelags, screw em
	bool breakingLC; //Breaks lag compensation

	PlayerSettings() : friendly(false), threatMultiplier(1.0f), healthMultiplier(1.0f), sortMultiplier(1.0f), shotCount(0), shotCountTime(0), shootOnlyLBY(false), lbyUpdateTime(0), useLoggedAngles(false), loggedLBYDelta(0), logFailCount(0), shotCountLog(0), lastSimTimeDelta(.0f), averageSimTimeDelta(.0f), inairFakelag(false), breakingLC(false) {}
};

extern struct PlayerSettings g_PlayerSettings[255];

struct angleLog
{
	float yaw;
	int shotCount;
	bool lbyUpdate;
	float m_flSimulationTime;

	struct angleLog()
	{
		yaw = 0;
		shotCount = 0;
		m_flSimulationTime = 0;
		lbyUpdate = false;
	};

	struct angleLog(float nYaw, int shotC, float simTime, bool lbyUpd)
	{
		yaw = nYaw;
		shotCount = shotC;
		m_flSimulationTime = simTime;
		lbyUpdate = lbyUpd;
	};
};

struct angleTickLog
{
	struct angleLog bestAngle;
	std::deque<angleLog> prevAngles;
};

extern float LerpTime();
extern float pelvisAverageVelocity[65];

namespace Resolver
{
	extern bool lbyUpdate[65];
	extern angleTickLog loggedAngles[65];
	bool IsFakewalking(IClientEntity * player, int entID);
	bool IsFakewalking(CBaseEntity * player, int entID);
	bool didLBYUpdate(CBaseEntity * player, int entID);
	bool didLBYUpdate(CBaseEntity* player);
	void FrameStageNotify(ClientFrameStage_t stage);
	void FireGameEvent(IGameEvent* event);
	bool lbyDeltaOver120(int plID);
	void BrokenLBY(CBaseEntity * player, int entID);
}


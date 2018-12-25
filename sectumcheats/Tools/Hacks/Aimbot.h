#pragma once
#include "../../stdafx.h"
#include "../Utils/Hitbox.h"
#include "../Menu/Menu.h"
#include "../Utils/Playerlist.h"
#include "../../Backtrack.h"
#include "../../Resolver.h"
#define TICK_INTERVAL			( Interfaces.pGlobalVars->interval_per_tick )
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )

vector<int> Hitboxes =
{
	(int)CSGOHitboxID::Head,
	(int)CSGOHitboxID::Neck,
	(int)CSGOHitboxID::Chest,
	(int)CSGOHitboxID::Pelvis,
	(int)CSGOHitboxID::Stomach,
};

class GOTV
{
public:
	class CHLTVFrame;

	class CHLTVDemoRecorder
	{
	public:
		char _pad[0x540];
		bool m_bIsRecording;
		int m_nFrameCount;
		float m_nStartTick;
		int m_SequenceInfo;
		int m_nDeltaTick;
		int m_nSignonTick;
		bf_write m_MessageData; // temp buffer for all network messages
	};

	class CHLTVServer
	{
	public:
		char _pad[0x5040];
		CHLTVDemoRecorder m_DemoRecorder;
	};
};

class Aimbot
{
private:
	struct Target_t
	{
		int OverrideHitbox;
		Vector aimspot;
		CBaseEntity* ent;
	};
	Vector TickPrediction(Vector AimPoint, CBaseEntity* pTarget)
	{
		return AimPoint + (pTarget->GetVecVelocity() * Interfaces.pGlobalVars->interval_per_tick);
	}
	Vector GetBestPoint(PlayerList::CPlayer* Player)
	{
		for (auto Pos : Player->ShootPos)
		{
			if (Pos != Vector(0, 0, 0))
			{
				Vector New = Pos + Player->box->points[1];
				Vector Aimangles;
				Misc::CalcAngle(Hacks.LocalPlayer->GetEyePosition(), New, Aimangles);
				//if (Misc::FovTo(Hacks.CurrentCmd->viewangles, Aimangles) > Menu::AimbotMenu::Selection::Fov.value) continue;
				float damage = Autowall::GetDamage(New);
				if (damage > Settings.Ragebot.Accuracy.Mindamage)
				{
					return New;
				}
			}
		}
		return Vector(0, 0, 0);
	}

	void HitboxScanning(PlayerList::CPlayer* Player)
	{
		Hitbox* box = Player->box;
		CTraceFilter filter;
		trace_t tr;
		Ray_t ray;
		int Dividens[3] = { (box->points[8].x - box->points[1].x) / 5 , (box->points[8].y - box->points[1].y) / 5, (box->points[8].z - box->points[1].z) / 5 };
		int Old = Player->ScannedNumber + 5;
		for (; Player->ScannedNumber < Old; Player->ScannedNumber++)
		{
			int x = Player->ScannedNumber;
			int y = 0;
			int z = 0;
			while (x >= 5)
			{
				x -= 5;
				y++;
			}
			while (y >= 5)
			{
				y -= 5;
				z++;
			}
			if (z >= 5)
			{
				Player->ScannedNumber = 0;
				break;
			}

			Vector Pos = Vector(box->points[1].x + (x * Dividens[0]), box->points[1].y + (y * Dividens[1]), box->points[1].z + (z * Dividens[2]));
			ray.Init(Pos, Pos);
			Interfaces.pTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);
			if (tr.m_pEnt != Player->entity)
				Player->ShootPos[Player->ScannedNumber] = Vector(0, 0, 0);
		}
	}

	void GetTargets(std::vector< CBaseEntity* >& possibletargets, std::vector< Target_t > possibleaimspots)
	{
		for (auto i = 0; i <= Interfaces.pEntList->GetHighestEntityIndex(); i++)
		{
			auto pEntity = static_cast< CBaseEntity* >(Interfaces.pEntList->GetClientEntity(i));
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
			PlayerList::CPlayer* Player = plist.FindPlayer(pEntity);
			Player->entity = pEntity;
			if (pEntity->HasGunGameImmunity())
				continue;
			player_info_t info;
			if (!(Interfaces.pEngine->GetPlayerInfo(pEntity->GetIndex(), &info)))
				continue;

			possibletargets.emplace_back(pEntity);
		}
	}

	void GetAimSpots(std::vector< Vector >& Targets, std::vector< Target_t >& possibleaimspots, std::vector< CBaseEntity* >& possibletargets)
	{
		if ((int)possibletargets.size())
		{
			for (CBaseEntity* pEntity : possibletargets)
			{
				Targets.emplace_back(pEntity->GetVecOrigin() - Hacks.LocalPlayer->GetVecOrigin());

				std::vector< int > iArray;

				float damage = 0;

				iArray.clear();
				if (Hacks.LocalWeapon->isAWP())
				{
					if (pEntity->IsMoving() && pEntity->GetFlags() & FL_ONGROUND)
						iArray = { (int)CSGOHitboxID::Head };
					else
						iArray = { Hitboxes[(int)Settings.Ragebot.Hitscan.PriorityHitbox] };
				}
				else
					iArray = { Hitboxes[(int)Settings.Ragebot.Hitscan.PriorityHitbox] };
				damage = ScanBoxList(pEntity, iArray, possibleaimspots);
				if (damage == 0)
				{
					if (Settings.Ragebot.Hitscan.Head)
						iArray.push_back((int)CSGOHitboxID::Head);

					if (Settings.Ragebot.Hitscan.Neck)
					{
						iArray.push_back((int)CSGOHitboxID::Neck);
						if (Settings.Ragebot.Hitscan.NeckFull)
							iArray.push_back((int)CSGOHitboxID::NeckLower);
					}

					if (Settings.Ragebot.Hitscan.Body)
					{
						iArray.push_back((int)CSGOHitboxID::Chest);
						iArray.push_back((int)CSGOHitboxID::Pelvis);
						iArray.push_back((int)CSGOHitboxID::Stomach);
						if (Settings.Ragebot.Hitscan.BodyFull)
						{
							iArray.push_back((int)CSGOHitboxID::UpperChest);
							iArray.push_back((int)CSGOHitboxID::LowerChest);
							iArray.push_back((int)CSGOHitboxID::RightThigh);
							iArray.push_back((int)CSGOHitboxID::LeftThigh);
						}
					}

					if (Settings.Ragebot.Hitscan.Arms)
					{
						iArray.push_back((int)CSGOHitboxID::LeftHand);
						iArray.push_back((int)CSGOHitboxID::RightHand);
						if (Settings.Ragebot.Hitscan.ArmsFull)
						{
							iArray.push_back((int)CSGOHitboxID::LeftLowerArm);
							iArray.push_back((int)CSGOHitboxID::RightLowerArm);
							iArray.push_back((int)CSGOHitboxID::LeftUpperArm);
							iArray.push_back((int)CSGOHitboxID::RightUpperArm);
						}
					}

					if (Settings.Ragebot.Hitscan.Legs)
					{
						iArray.push_back((int)CSGOHitboxID::LeftFoot);
						iArray.push_back((int)CSGOHitboxID::RightFoot);
						if (Settings.Ragebot.Hitscan.LegsFull)
						{
							iArray.push_back((int)CSGOHitboxID::LeftShin);
							iArray.push_back((int)CSGOHitboxID::RightShin);
						}
					}
					if (GetAsyncKeyState(Settings.Ragebot.Accuracy.bAimAtKeyButton))
					{
						iArray.clear();
						iArray.push_back((int)CSGOHitboxID::Chest);
						iArray.push_back((int)CSGOHitboxID::Pelvis);
						iArray.push_back((int)CSGOHitboxID::Stomach);
						iArray.push_back((int)CSGOHitboxID::UpperChest);
						iArray.push_back((int)CSGOHitboxID::LowerChest);
					}
				}
				ScanBoxList(pEntity, iArray, possibleaimspots);
			}
		}
	}

	float ScanBoxList(CBaseEntity* pEntity, std::vector< int > iArray, std::vector< Target_t > &possibleaimspots)
	{
		float flBestDamage = 0.f;

		if (iArray.size() > 0)
		{
			for (int i = 0; i < iArray.size(); i++)
			{
				Hitbox box;
				if (!box.GetHitbox(pEntity, iArray[i]))
					continue;

				Vector Aimspot;
				float flDamage = iArray[i] == Hitboxes[(int)Settings.Ragebot.Hitscan.PriorityHitbox] ? box.GetBestPoint(Aimspot) : box.ScanCenter(Aimspot);

				if (flDamage > Settings.Ragebot.Accuracy.Mindamage)
				{
					if (flDamage > flBestDamage)
					{
						flBestDamage = flDamage;
						//if (Misc::GetNumberOfTicksChoked(pEntity) > 5 && Settings.GetSetting(Tab_Ragebot, Ragebot_PositionAdjustment))
						//{
						//	Aimspot -= pEntity->GetAbsOrigin();
						//	Aimspot += pEntity->GetNetworkOrigin();
						//}
						float correct = Interfaces.pEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);

						float Backtrack[360];
						struct Hitbox
						{
							Hitbox(void)
							{
								hitbox = -1;
							}


							Hitbox(int newHitBox)
							{
								hitbox = newHitBox;
							}


							int  hitbox;
							Vector points[9];
						};


						enum
						{
							FL_HIGH = (1 << 0),
							FL_LOW = (1 << 6),
							FL_SPECIAL = (21 << 2)
						};


						struct BestPoint
						{
							BestPoint(void)
							{
								hitbox = -1;
								index = -1;
								dmg = -1;
								flags = 0;
							}


							BestPoint(int newHitBox)
							{
								hitbox = newHitBox;
								index = -1;
								dmg = -1;
								flags = 0;
							}
							Vector point;
							int  index;
							int  dmg;
							int  flags;
							int  untransformedBox;
							int  hitbox;
						};

						class HitboxS
						{
						public:
							Vector points[9];

							bool GetHitbox(CBaseEntity* Player, int HitboxID)
							{
								if (!Player)
									return false;

								matrix3x4_t matrix[35929324];
								if (!Player->SetupBones(matrix, 35929324, 0x00000100, GetTickCount64()))
									return false;
								const model_t* mod = Player->GetModel();
								if (!mod)
									return false;
								studiohdr_t* hdr = Interfaces.g_pModelInfo->GetStudioModel(mod);
								if (!hdr)
									return false;
								mstudiohitboxset_t* set = hdr->GetHitboxSet(0);
								if (!set)
									return false;
								mstudiobbox_t* hitbox = set->pHitbox(HitboxID);
								if (!hitbox)
									return false;

								Vector points[] = { ((hitbox->bbmin + hitbox->bbmax) * .9f),
									Vector(hitbox->bbmin.x, hitbox->bbmin.y, hitbox->bbmin.z),
									Vector(hitbox->bbmin.x, hitbox->bbmax.y, hitbox->bbmin.z),
									Vector(hitbox->bbmax.x, hitbox->bbmax.y, hitbox->bbmin.z),
									Vector(hitbox->bbmax.x, hitbox->bbmin.y, hitbox->bbmin.z),
									Vector(hitbox->bbmax.x, hitbox->bbmax.y, hitbox->bbmax.z),
									Vector(hitbox->bbmin.x, hitbox->bbmax.y, hitbox->bbmax.z),
									Vector(hitbox->bbmin.x, hitbox->bbmin.y, hitbox->bbmax.z),
									Vector(hitbox->bbmax.x, hitbox->bbmin.y, hitbox->bbmax.z) };

								for (int index = 0; index < 49; ++index)
								{
									float multiplier = 100.f / Settings.Ragebot.PointScale;
									g_Math.VectorTransform(points[index], matrix[hitbox->bone], this->points[index]);
								}

								return true;
							}


							float ScanCenter(Vector& Aimspot)
							{
								Aimspot = this->points[0];
								return this->points[0] == Vector(0, 0, 0) ? -1.f : Autowall::GetDamage(this->points[0]);
							}


							void FindActualHitbox(CBaseEntity* pEntity, Vector& best) {
								FireBulletData Bullet_Data = FireBulletData(Vector(best.x, best.y, best.z - 9.9));
								int bestdamage = 12;
								for (int i = -128; i < 45; i += 140) {
									Vector Angle = Vector(0, i, 0);
									g_Math.angleVectors(Angle, Bullet_Data.direction);
									if (Autowall::FireSimulatedBullet(Bullet_Data, Hacks.LocalPlayer, Hacks.LocalWeapon))
										if (Bullet_Data.current_damage > bestdamage) {
											bestdamage = Bullet_Data.current_damage;
											best = Bullet_Data.enter_trace.endpos;
										}
								}
							}
						};
						Target_t target;
						target.aimspot = Aimspot;
						target.ent = pEntity;
						possibleaimspots.emplace_back(target);

					}
				}
			}
		}

		return flBestDamage;
	}

	void GetAtTargetsSpot(std::vector< Vector >& Targets)
	{
		if (!(int)Targets.size())
			Target = Vector(0, 0, 0);
		else
		{
			Target = Vector(8128, 8128, 8128);
			for (Vector t : Targets)
			{
				if (t.Length() < Target.Length())
					Target = t;
			}
		}
	}

	bool Fire(CInput::CUserCmd* cmd, Vector vecCurPos, std::vector< Target_t >& possibleaimspots)
	{
		int closest = 8129;
		if ((!Settings.Ragebot.Accuracy.AutoShoot && !(cmd->buttons & IN_ATTACK)) || !Settings.Ragebot.EnableAim)
			return true;
		Vector Aimangles;
		int originaltick = Hacks.CurrentCmd->tick_count;
		int distance = -1;
		static int Cycle = 0;
		bool shot = false;
		if (!Misc::bullettime())
			return true;
		int selection = Settings.Ragebot.TargetDetection;
		if (selection != 2) {
			for (int i = 0; i < possibleaimspots.size(); i++)
			{
				Target_t Aimspot = possibleaimspots[i];
				Misc::CalcAngle(vecCurPos, Aimspot.aimspot, Aimangles);
				distance = vecCurPos.DistTo(Aimspot.aimspot);
				if (distance < closest)
				{
					int TempTick = originaltick;
					//Hacks.CurrentCmd->tick_count = TempTick;
					float flbestdamage = 0;
					float flbestsimtime{};
					int besttick{};
					if (backtracking->IsTickValid(pResolverData[Aimspot.ent->GetIndex()].balanceadjustsimtime))
					{
						Misc::CalcAngle(vecCurPos, pResolverData[Aimspot.ent->GetIndex()].balanceadjustaimangles, Aimangles);
						backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(pResolverData[Aimspot.ent->GetIndex()].balanceadjustsimtime));
					}
					else
					{
						for (int t = 0; t < 15; t++)
						{
							if (RageBackData[Aimspot.ent->GetIndex()][t].damage > flbestdamage)
							{
								if (Interfaces.pGlobalVars && Interfaces.pGlobalVars->curtime)
								{
									if (backtracking->IsTickValid(RageBackData[Aimspot.ent->GetIndex()][t].simtime))
									{
										flbestdamage = RageBackData[Aimspot.ent->GetIndex()][t].damage;
										flbestsimtime = RageBackData[Aimspot.ent->GetIndex()][t].simtime;
										besttick = t;
										Misc::CalcAngle(vecCurPos, RageBackData[Aimspot.ent->GetIndex()][t].hitboxPos, Aimangles);
									}
								}
							}
						}
						backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(flbestsimtime));
					}
					cmd->viewangles = Aimangles;
					pTarget = Aimspot.ent;
					cmd->buttons |= IN_ATTACK;
					Angles = cmd->viewangles;
					shot = true;
				}
			}
		}
		else if (possibleaimspots.size() > 0)
		{
			Cycle++;
			if (Cycle >= possibleaimspots.size())
				Cycle = 0;
			Target_t Aimspot = possibleaimspots[Cycle];
			Misc::CalcAngle(vecCurPos, Aimspot.aimspot, Aimangles);
			int TempTick = originaltick;
			float flbestdamage = 0;
			float flbestsimtime{};
			int besttick{};
			if (backtracking->IsTickValid(pResolverData[Aimspot.ent->GetIndex()].balanceadjustsimtime))
			{
				Misc::CalcAngle(vecCurPos, pResolverData[Aimspot.ent->GetIndex()].balanceadjustaimangles, Aimangles);
				backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(pResolverData[Aimspot.ent->GetIndex()].balanceadjustsimtime));
			}
			else
			{
				for (int t = 0; t < 15; t++)
				{
					if (RageBackData[Aimspot.ent->GetIndex()][t].damage > flbestdamage)
					{
						if (Interfaces.pGlobalVars && Interfaces.pGlobalVars->curtime)
						{
							if (backtracking->IsTickValid(RageBackData[Aimspot.ent->GetIndex()][t].simtime))
							{
								flbestdamage = RageBackData[Aimspot.ent->GetIndex()][t].damage;
								flbestsimtime = RageBackData[Aimspot.ent->GetIndex()][t].simtime;
								besttick = t;
								Misc::CalcAngle(vecCurPos, RageBackData[Aimspot.ent->GetIndex()][t].hitboxPos, Aimangles);
							}
						}
					}
				}
				backtracking->BackTrackPlayer(Hacks.CurrentCmd, TIME_TO_TICKS2(flbestsimtime));
			}
			cmd->viewangles = Aimangles;
			Angles = cmd->viewangles;
			cmd->buttons |= IN_ATTACK;
			shot = true;
		}
		return false;
	}

	bool AntiGOTV(int tick)
	{
		return true;
	}

public:
	Vector Target;
	CBaseEntity* pTarget;
	Vector Angles;
	bool Aim(CInput::CUserCmd* cmd)
	{
		Angles = cmd->viewangles;
		Vector vecCurPos = Hacks.LocalPlayer->GetEyePosition();
		std::vector< Vector > Targets;
		std::vector< Target_t > possibleaimspots;
		std::vector< CBaseEntity* > possibletargets;
		GetTargets(possibletargets, possibleaimspots);
		GetAimSpots(Targets, possibleaimspots, possibletargets);
		GetAtTargetsSpot(Targets);
		if (Hacks.LocalWeapon->IsMiscWeapon())
			return false;
		if (Settings.Ragebot.Accuracy.AutoScope && Hacks.LocalWeapon->isSniper() && !Hacks.LocalPlayer->m_bIsScoped() && possibleaimspots.size() > 0)
		{
			Hacks.CurrentCmd->buttons |= IN_ATTACK2;
			return false;
		}
		if (!Hacks.LocalPlayer->isAlive())
			return false;

		if (Hacks.LocalWeapon->hitchance() < Settings.Ragebot.Accuracy.Hitchance)
			return false;

		return !Fire(cmd, vecCurPos, possibleaimspots);
	}
} Aimbot;


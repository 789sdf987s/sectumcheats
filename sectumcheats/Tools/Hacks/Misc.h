#pragma once
#include "stdafx.h"
#include <chrono>
#include "..\..\Xor.h"
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

class Misc
{
	typedef bool ( __cdecl* ServerRankRevealAllFn)( float* );

public:

	static void CL_SendMove()
	{
		typedef void(__fastcall* CL_SendMoveFn)(void);
		static CL_SendMoveFn CL_SendMove = reinterpret_cast<CL_SendMoveFn>(Utils.PFindPattern("engine.dll", "55 8B EC A1 ?? ?? ?? ?? 81 EC ?? ?? ?? ?? B9 ?? ?? ?? ?? 53 8B 98"));

		CL_SendMove();
	}

	static void WriteUsercmd(void* buf, CInput::CUserCmd* in, CInput::CUserCmd* out)
	{
		typedef void(__fastcall* WriteUsercmdFn)(void*, CInput::CUserCmd*, CInput::CUserCmd*);
		static WriteUsercmdFn WriteUsercmd = reinterpret_cast<WriteUsercmdFn>(Utils.PFindPattern("client_panorama.dll", "55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

		__asm
		{
			mov     ecx, buf
			mov     edx, in
			push    out
			call    WriteUsercmd
			add     esp, 4
		}
	}

	static float GetFOV(Vector qAngles, Vector vecSource, Vector vecDestination, bool bDistanceBased)
	{
		auto MakeVector = [](Vector qAngles)
		{
			auto ret = Vector();
			auto pitch = float(qAngles[0] * M_PI / 180.f);
			auto yaw = float(qAngles[1] * M_PI / 180.f);
			auto tmp = float(cos(pitch));
			ret.x = float(-tmp * -cos(yaw));
			ret.y = float(sin(yaw)*tmp);
			ret.z = float(-sin(pitch));
			return ret;
		};

		Vector ang, aim;
		double fov;

		ang = g_Math.CalcAngle2(vecSource, vecDestination);
		aim = MakeVector(qAngles);
		ang = MakeVector(ang);

		auto mag_s = sqrt((aim[0] * aim[0]) + (aim[1] * aim[1]) + (aim[2] * aim[2]));
		auto mag_d = sqrt((aim[0] * aim[0]) + (aim[1] * aim[1]) + (aim[2] * aim[2]));
		auto u_dot_v = aim[0] * ang[0] + aim[1] * ang[1] + aim[2] * ang[2];

		fov = acos(u_dot_v / (mag_s*mag_d)) * (180.f / M_PI);

		if (bDistanceBased) {
			fov *= 1.4;
			float xDist = abs(vecSource[0] - vecDestination[0]);
			float yDist = abs(vecSource[1] - vecDestination[1]);
			float Distance = sqrt((xDist * xDist) + (yDist * yDist));

			Distance /= 650.f;

			if (Distance < 0.7f)
				Distance = 0.7f;

			if (Distance > 6.5)
				Distance = 6.5;

			fov *= Distance;
		}

		return (float)fov;
	}

	static std::vector<Vector> GetMultiplePointsForHitbox(CBaseEntity* pBaseEntity, int iHitbox, matrix3x4 BoneMatrix[128])
	{
		auto VectorTransform_Wrapper = [](const Vector& in1, const matrix3x4 &in2, Vector &out)
		{
			auto VectorTransform = [](const float *in1, const matrix3x4& in2, float *out)
			{
				auto DotProducts = [](const float *v1, const float *v2)
				{
					return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
				};
				out[0] = DotProducts(in1, in2[0]) + in2[0][3];
				out[1] = DotProducts(in1, in2[1]) + in2[1][3];
				out[2] = DotProducts(in1, in2[2]) + in2[2][3];
			};
			VectorTransform(&in1.x, in2, &out.x);
		};

		studiohdr_t* pStudioModel = Interfaces.g_pModelInfo->GetStudioModel(pBaseEntity->GetModel());
		mstudiohitboxset_t* set = pStudioModel->GetHitboxSet(0);
		mstudiobbox_t *hitbox = set->pHitbox(iHitbox);

		std::vector<Vector> vecArray;

		Vector max;
		Vector min;
		VectorTransform_Wrapper(hitbox->bbmax, BoneMatrix[hitbox->bone], max);
		VectorTransform_Wrapper(hitbox->bbmin, BoneMatrix[hitbox->bone], min);

		auto center = (min + max) * 0.5f;

		Vector CurrentAngles = g_Math.CalcAngle2(center, Hacks.LocalPlayer->GetEyePosition());

		Vector Forward;
		g_Math.angleVectors(CurrentAngles, Forward);

		Vector Right = Forward.Cross(Vector(0, 0, 1));
		Vector Left = Vector(-Right.x, -Right.y, Right.z);

		Vector Top = Vector(0, 0, 1);
		Vector Bot = Vector(0, 0, -1);

		switch (iHitbox) {
		case (int)CSGOHitboxID::Head:
			for (auto i = 0; i < 4; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Top * (hitbox->radius * Settings.Ragebot.Headscale);
			vecArray[2] += Right * (hitbox->radius * Settings.Ragebot.Headscale);
			vecArray[3] += Left * (hitbox->radius * Settings.Ragebot.Headscale);
			break;

		default:

			for (auto i = 0; i < 3; ++i)
			{
				vecArray.emplace_back(center);
			}
			vecArray[1] += Right * (hitbox->radius * Settings.Ragebot.Bodyscale);
			vecArray[2] += Left * (hitbox->radius * Settings.Ragebot.Bodyscale);
			break;
		}
		return vecArray;
	}
	static int TIME_TO_TICKS1(int dt)
	{
		return (int)(0.5f + (float)(dt) / Interfaces.pGlobalVars->interval_per_tick);
	}
	static bool pIsVisible(Vector& vecAbsStart, Vector& vecAbsEnd, CBaseEntity* pLocal, CBaseEntity* pBaseEnt, bool smokeCheck) throw()
	{
		trace_t tr;
		Ray_t ray;
		CTraceFilter filter;

		filter.pSkip = pLocal;

		ray.Init(vecAbsStart, vecAbsEnd);

		Interfaces.pTrace->TraceRay(ray, MASK_SHOT, &filter, &tr);

		bool visible = (tr.m_pEnt == pBaseEnt || tr.fraction >= 0.96f);

		if (visible && smokeCheck)
			visible = !LineGoesThroughSmoke(vecAbsStart, vecAbsEnd);

		return visible;
	}
	static bool LineGoesThroughSmoke(Vector pos1, Vector pos2) throw()
	{

		typedef bool(*LineGoesThroughSmokeFn)(float, float, float, float, float, float, short);
		LineGoesThroughSmokeFn LineGoesThroughSmokeEx;
		LineGoesThroughSmokeEx = (LineGoesThroughSmokeFn)(Interfaces.OffsetLineGoes);
		return LineGoesThroughSmokeEx(pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z, 1);
	}

	static Vector CalcAngle(Vector src, Vector dst)
	{
		Vector angles;
		Vector delta = src - dst;
		angles.x = (asinf(delta.z / delta.Length()) * 57.295779513082f);
		angles.y = (atanf(delta.y / delta.x) * 57.295779513082f);
		angles.z = 0.0f;
		if (delta.x >= 0.0) { angles.y += 180.0f; }

		return angles;
	}

	static void AutoZues()
	{
		if (Settings.Ragebot.Accuracy.AutoZeus)
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
				if (Hacks.LocalPlayer->isAlive() && Hacks.LocalWeapon) {
					float dist = g_Math.VectorDistance(Hacks.LocalPlayer->GetVecOrigin(), pEntity->GetVecOrigin());

					if (Hacks.LocalWeapon->istaser() && dist < 800.f)
						Hacks.CurrentCmd->buttons |= IN_ATTACK;
					else if (!Hacks.LocalWeapon->istaser())
						Hacks.CurrentCmd->buttons |= IN_ATTACK;


				}
			}
		}
	}

	static float GetFov(const Vector& viewAngle, const Vector& aimAngle)
	{
		Vector ang, aim;

		g_Math.angleVectors(viewAngle, aim);
		g_Math.angleVectors(aimAngle, ang);

		return RAD2DEG(acos(aim.Dot(ang) / aim.LengthSqr()));
	}
	static bool isVisible(CBaseEntity* lul, int bone)
	{
		Ray_t ray;
		trace_t tr;

		ray.Init(Hacks.LocalPlayer->GetEyePosition(), lul->GetBonePos(bone));

		CTraceFilter filter;
		filter.pSkip = Hacks.LocalPlayer;

		Interfaces.pTrace->TraceRay(ray, (0x1 | 0x4000 | 0x2000000 | 0x2 | 0x4000000 | 0x40000000), &filter, &tr);

		if (tr.m_pEnt == lul)
		{
			return true;
		}

		return false;
	}
	static vec_t Normalize_y( vec_t ang )
	{
		while( ang < -180.0f )
			ang += 360.0f;
		while( ang > 180.0f )
			ang -= 360.0f;
		return ang;
	}

	static void setName( const char* name )
	{
		auto namevar = Interfaces.g_ICVars->FindVar( "name" );
		*reinterpret_cast< int* >( reinterpret_cast< DWORD >( &namevar->fnChangeCallback ) + 0xC ) = 0;
		namevar->SetValue( name );
	}

	static void ServerRankReveal()
	{

	}

	static void SetClanTag( const char* tag )
	{
		static auto pSetClanTag = reinterpret_cast< void(__fastcall*)( const char*, const char* ) >( ( DWORD )( Utils.PFindPattern( "engine.dll", "53 56 57 8B DA 8B F9 FF 15" ) ) );
		pSetClanTag( tag, "AIMWARE.net" );
	}

	static void Clan_Tag() {

		if (Settings.Misc.ClanChanger) {
			static int counter = 0;

			static int motion = 0;
			int ServerTime = (float)Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick * 3;

			switch (int(Interfaces.pGlobalVars->curtime * 2.4) % 27)
			{
			case 0: SetClanTag(XorStr("                  ")); break;
			case 1: SetClanTag(XorStr("                 A")); break;
			case 2: SetClanTag(XorStr("                AI")); break;
			case 3: SetClanTag(XorStr("               AIM")); break;
			case 4: SetClanTag(XorStr("              AIMW")); break;
			case 5: SetClanTag(XorStr("             AIMWA")); break;
			case 6: SetClanTag(XorStr("            AIMWAR")); break;
			case 7: SetClanTag(XorStr("           AIMWARE")); break;
			case 8: SetClanTag(XorStr("          AIMWARE.")); break;
			case 9: SetClanTag(XorStr("         AIMWARE.n")); break;
			case 10:SetClanTag(XorStr("        AIMWARE.ne")); break;
			case 11:SetClanTag(XorStr("       AIMWARE.net")); break;
			case 12:SetClanTag(XorStr("                  ")); break;
			case 13:SetClanTag(XorStr("                  ")); break;
			case 14:SetClanTag(XorStr("                  ")); break;
			case 15:SetClanTag(XorStr("                  ")); break;
			case 16:SetClanTag(XorStr("                  ")); break;
			case 17:SetClanTag(XorStr("                  ")); break;
			case 18:SetClanTag(XorStr("                  ")); break;
			case 19:SetClanTag(XorStr("                  ")); break;
			case 20:SetClanTag(XorStr("                  ")); break;
			case 22:SetClanTag(XorStr("                  ")); break;
			case 23:SetClanTag(XorStr("                  ")); break;
			case 24:SetClanTag(XorStr("                  ")); break;
			case 25:SetClanTag(XorStr("                  ")); break;
			case 26:SetClanTag(XorStr("                  ")); break;
			case 27:SetClanTag(XorStr("                  ")); break;
			}
		}
	}

	static int TIME_TO_TICKS( int dt )
	{
		return ( int )( 0.5f + ( float )( dt ) / Interfaces.pGlobalVars->interval_per_tick );
	}

	static float GetNetworkLatency()
	{
		INetChannelInfo* nci = Interfaces.pEngine->GetNetChannelInfo();
		if( nci )
		{
			return nci->GetAvgLatency( FLOW_OUTGOING );
		}
		return 0.0f;
	}
	static void NameSpammer() {
		if (!Interfaces.pEngine->IsConnected() || !Interfaces.pEngine->IsInGame())
			return;
		static bool Spam;
		if (Settings.Misc.Namespam) {
			if (Spam)
			{
				setName("FXWARE--");
				Spam = false;
			}
			else
			{
				setName("--Enigma");
				Spam = true;
			}
			Spam != Spam;
		}
		else return;
	}


	static void ThirdPerson( void )
	{
		static bool bThirdPerson = false;
		static bool bSpoofed = false;
		static ConVar* not_sv_cheats;
		static SpoofedConvar* big_cheta;

		if( !bSpoofed )
		{
			not_sv_cheats = Interfaces.g_ICVars->FindVar( "sv_cheats" );
			big_cheta = new SpoofedConvar( not_sv_cheats );
			big_cheta->SetBool( TRUE );
			bSpoofed = true;
		}

		if( Hacks.LocalPlayer->isAlive() && ( !bThirdPerson && Settings.Visuals.Thirdperson) )
		{
			Interfaces.pEngine->ClientCmd_Unrestricted( "thirdperson", 0 );
			bThirdPerson = true;
		}
		else
		{
			Interfaces.pEngine->ClientCmd_Unrestricted( "firstperson", 0 );
			big_cheta->SetBool( FALSE );
			bThirdPerson = false;
		}
	}

	static double GetNumberOfTicksChoked( CBaseEntity* pEntity )
	{
		double flSimulationTime = pEntity->GetSimulationTime();
		double flSimDiff = ( ( double )Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick ) - flSimulationTime;
		return TIME_TO_TICKS( max(0.0f, flSimDiff) );
	}

	static bool Shooting()
	{
		 if (Hacks.LocalWeapon->IsNade())
		{
			CBaseCSGrenade* csGrenade = (CBaseCSGrenade*)Hacks.LocalWeapon;
			if (csGrenade->GetThrowTime() > 0.f)
			{
				return true;
			}
		}
		else if (Hacks.CurrentCmd->buttons & IN_ATTACK && bullettime())
		{
			if (*Hacks.LocalWeapon->GetItemDefinitionIndex() == weapon_revolver && Settings.Ragebot.AutoRevoler)
			{
				if (Hacks.LocalWeapon->GetPostponeFireReadyTime() - GetServerTime() <= 0.05f)
				{
					return true;
				}
			}
			else
				return true;
		}
		return false;
	}

	static void DrawScope()
	{
		CBaseCombatWeapon* pLocalWeapon = Hacks.LocalPlayer->GetActiveBaseCombatWeapon();

		if (pLocalWeapon)
		{
			if (pLocalWeapon->isSniper())
			{
				if (Hacks.LocalPlayer->GetScope())
				{
					int width = 0;
					int height = 0;
					Interfaces.pEngine->GetScreenSize(width, height);
					int centerX = static_cast<int>(width * 0.5f);
					int centerY = static_cast<int>(height * 0.5f);
					Interfaces.pSurface->DrawSetColor(0, 0, 0, 255);
					Interfaces.pSurface->DrawLine(0, centerY, width, centerY);
					Interfaces.pSurface->DrawLine(centerX, 0, centerX, height);
				}
			}
		}
	}

	static void CircleStrafer( float& OldAngle )
	{
		static int Angle = 0;
		if( OldAngle - Angle > 360 )
			Angle -= 360;
		static bool shouldspin = false;
		static bool enabled = false;
		static bool check = false;

		if( enabled )
		{
			shouldspin = true;
			Hacks.CurrentCmd->buttons |= IN_JUMP;
			Hacks.CurrentCmd->buttons |= IN_DUCK;
		}
		if( shouldspin )
		{
			Vector Dir;
			g_Math.angleVectors( Vector( 0, Angle, 0 ), Dir );
			Dir *= 8218;
			Ray_t ray;
			CTraceWorldOnly filter;
			trace_t trace;
			ray.Init( Hacks.LocalPlayer->GetEyePosition(), Hacks.LocalPlayer->GetVecOrigin() + Dir );
			Interfaces.pTrace->TraceRay( ray, MASK_SHOT, &filter, &trace );
			auto temp = 3.4f / ( ( trace.endpos - Hacks.LocalPlayer->GetVecOrigin() ).Length() / 100.f );
			if( temp < 3.4f )
				temp = 3.4f;
			if( enabled )
			{
				Angle += temp;
				Hacks.CurrentCmd->sidemove = -450;
			}
			else
			{
				if( OldAngle - Angle < temp )
				{
					Angle = OldAngle;
					shouldspin = false;
				}
				else
					Angle += temp;
			}
			OldAngle = Angle;
		}
		else
			Angle = OldAngle;
	}

	static Vector Normalize( Vector& angs )
	{
		while( angs.y < -180.0f )
			angs.y += 360.0f;
		while( angs.y > 180.0f )
			angs.y -= 360.0f;
		if( angs.x > 89.0f )
			angs.x = 89.0f;
		if( angs.x < -89.0f )
			angs.x = -89.0f;
		angs.z = 0;
		return angs;
	}

	static bool AimStep( Vector angSource, Vector& angDestination )
	{
		Vector angDelta = Normalize( angDestination - angSource );
		if( angDelta.Abs() > 40.f )
		{
			angDestination = Normalize( angSource + angDelta / angDelta.Abs() * 40.f );
			return false;
		}
		return true;
	}

	static float GetServerTime()
	{
		return ( float )Hacks.LocalPlayer->GetTickBase() * Interfaces.pGlobalVars->interval_per_tick;
	}

	static bool bullettime()
	{
		if( !Hacks.LocalWeapon )
			return false;
		float flNextPrimaryAttack = Hacks.LocalWeapon->NextPrimaryAttack();

		return flNextPrimaryAttack <= GetServerTime();
	}

	static void MoveFix( CInput::CUserCmd* cmd, Vector& realvec )
	{
		Vector vMove( cmd->forwardmove, cmd->sidemove, cmd->upmove );
		float flSpeed = sqrt( vMove.x * vMove.x + vMove.y * vMove.y ), flYaw;
		Vector vMove2;
		g_Math.vectorAnglesVec( vMove, vMove2 );
		Normalize( vMove2 );
		flYaw = DEG2RAD(cmd->viewangles.y - realvec.y + vMove2.y);
		cmd->forwardmove = cos( flYaw ) * flSpeed;
		cmd->sidemove = sin( flYaw ) * flSpeed;
		if( 90 < abs( cmd->viewangles.x ) && 180 > abs( cmd->viewangles.x ) )
			cmd->forwardmove *= -1;
	}

	static void CalcAngle( Vector src, Vector dst, Vector& angles )
	{
		Vector delta = src - dst;
		double hyp = delta.Length2D();
		angles.y = atan( delta.y / delta.x ) * 57.295779513082f;
		angles.x = atan( delta.z / hyp ) * 57.295779513082f;
		if( delta.x >= 0.0 )
			angles.y += 180.0f;
		angles.z = 0;
	}

	static int FovTo( Vector From, Vector To )
	{
		From -= To;
		Normalize( From );
		return ( abs( From.y ) + abs( From.x ) );
	}
};

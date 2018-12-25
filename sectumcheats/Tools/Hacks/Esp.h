
#pragma once
#include "../../stdafx.h"
#include "CreateMoveETC.h"
#include "../../SDK/Math/Math.h"
#include "Header.h"
#include "Resolver.h"

static bool faked;

float lineFakeAngle;
float lineRealAngle;
float lineLBY;

class CGlowObjectManager
{
public:
	class GlowObjectDefinition_t
	{
	public:
		void set(CColor color, int style)
		{
			int r, g, b, a;
			color.GetColor(r, g, b, a);
			m_vGlowColor = Vector(r / 255.f, g / 255.f, b / 255.f);
			m_flGlowAlpha = a / 255.f;
			m_bRenderWhenOccluded = true;
			m_bRenderWhenUnoccluded = false;
			m_flBloomAmount = 1.f;
			//iUnk = style;
		}

		CBaseEntity* getEntity()
		{
			return m_hEntity;
		}

		bool IsEmpty() const { return m_nNextFreeSlot != GlowObjectDefinition_t::ENTRY_IN_USE; }

	public:
		CBaseEntity * m_hEntity;
		Vector				m_vGlowColor;
		float				m_flGlowAlpha;

		char				unknown[4];
		float				flUnk;
		float				m_flBloomAmount;
		float				localplayeriszeropoint3;


		bool				m_bRenderWhenOccluded;
		bool				m_bRenderWhenUnoccluded;
		bool				m_bFullBloomRender;
		char				unknown1[1];


		int					m_nFullBloomStencilTestValue;
		int					iUnk;
		int					m_nSplitScreenSlot;
		int					m_nNextFreeSlot;

		static const int END_OF_FREE_LIST = -1;
		static const int ENTRY_IN_USE = -2;
	};

	GlowObjectDefinition_t* m_GlowObjectDefinitions;
	int		max_size;
	int		pad;
	int		size;
	GlowObjectDefinition_t* m_GlowObjectDefinitions2;
	int		currentObjects;
};

class Esp
{
public:

	static void PostProcess()
	{
		if (Settings.Visuals.PostProcessing)
		{
			ConVar *gg = Interfaces.g_ICVars->FindVar("mat_postprocess_enable");
			gg->SetValue(0);
		}
		else
		{
			ConVar *gg = Interfaces.g_ICVars->FindVar("mat_postprocess_enable");
			gg->SetValue(1);
		}
	}

	static void ThirdPerson()
	{
		static bool bSpoofed = false;
		static bool bThirdperson = false;
		MEMCHECK;

		static ConVar* sv_cheat;
		static SpoofedConvar* big_cheta;

		if (!bSpoofed)
		{
			sv_cheat = Interfaces.g_ICVars->FindVar("sv_cheats");
			big_cheta = new SpoofedConvar(sv_cheat);
			bSpoofed = true;
		}

		if (bSpoofed)
		{
			if (GetKeyState(Settings.Visuals.Thirdperson))
			{
				if (!bThirdperson &&  GetKeyState(Settings.Visuals.Thirdperson) && Hacks.LocalPlayer->GetHealth() > 0)
				{
					big_cheta->SetBool(true);
					Interfaces.pEngine->ClientCmd_Unrestricted("thirdperson", 0);
					bThirdperson = true;
				}
			}
			else
			{
				if (bThirdperson)
				{
					big_cheta->SetBool(false);
					Interfaces.pEngine->ClientCmd_Unrestricted("firstperson", 0);
					bThirdperson = false;
				}
			}
			if (Hacks.LocalPlayer->GetHealth() <= 0)
			{
				Interfaces.pEngine->ClientCmd_Unrestricted("firstperson", 0);
				bThirdperson = false;
			}
		}
	}

	static void Watermark()
	{
		int W, H;
		Interfaces.pEngine->GetScreenSize(W, H);
		Interfaces.pSurface->DrawT(5, 6, CColor(255, 255, 255, 255), Hacks.Font_Watermark, false, "sectumcheats");
		Interfaces.pSurface->DrawFilledRect(5, 6, false, (255, 255, 255, 255) );


		if (Interfaces.pEngine->IsConnected() && Interfaces.pEngine->IsInGame() && Hacks.LocalPlayer->isAlive() && Settings.Misc.lbyindicator)
		{
			if (!faked) {
				Interfaces.pSurface->DrawStringY(Hacks.Font_Logo, 10, H - 72, CColor(255, 0, 10, 150), FONT_LEFT, "LBY");
			}
			else {
				Interfaces.pSurface->DrawStringY(Hacks.Font_Logo, 10, H - 72, CColor(66, 244, 83, 255), FONT_LEFT, "LBY");
			}
		}
	}

	static void Hitmarker()
	{
		if (Global::hitmarkerAlpha < 0.f)
			Global::hitmarkerAlpha = 0.f;
		else if (Global::hitmarkerAlpha > 0.f)
			Global::hitmarkerAlpha -= 0.01f;

		int W, H;
		Interfaces.pEngine->GetScreenSize(W, H);

		if (Global::hitmarkerAlpha > 0.f)
		{
			DrawLine(W / 2 - 10, H / 2 - 10, W / 2 - 5, H / 2 - 5, CColor(240, 240, 240, (Global::hitmarkerAlpha * 255.f)));
			DrawLine(W / 2 - 10, H / 2 + 10, W / 2 - 5, H / 2 + 5, CColor(240, 240, 240, (Global::hitmarkerAlpha * 255.f)));
			DrawLine(W / 2 + 10, H / 2 - 10, W / 2 + 5, H / 2 - 5, CColor(240, 240, 240, (Global::hitmarkerAlpha * 255.f)));
			DrawLine(W / 2 + 10, H / 2 + 10, W / 2 + 5, H / 2 + 5, CColor(240, 240, 240, (Global::hitmarkerAlpha * 255.f)));
		}
	}

	static void Crosshair()
	{
		int screenW, screenH;
		Interfaces.pEngine->GetScreenSize(screenW, screenH);

		int crX = screenW / 2, crY = screenH / 2;
		int dy = screenH / Hacks.FOV;
		int dx = screenW / Hacks.FOV;
		int drX;
		int drY;

		if (Settings.Visuals.NoVisRecoil)
		{
			drX = crX - (int)(dx * (Hacks.LocalPlayer->GetPunchAngle().y * 2));
			drY = crY + (int)(dy * (Hacks.LocalPlayer->GetPunchAngle().x * 2));
		}
		else
		{
			drX = crX - (int)(dx * (((Hacks.LocalPlayer->GetPunchAngle().y * 2.f) * 0.45f) + Hacks.LocalPlayer->GetPunchAngle().y));
			drY = crY + (int)(dy * (((Hacks.LocalPlayer->GetPunchAngle().x * 2.f) * 0.45f) + Hacks.LocalPlayer->GetPunchAngle().x));
		}

		float r = (Settings.Visuals.Colors.ScopeCrosshair[0] * 255.f);
		float g = (Settings.Visuals.Colors.ScopeCrosshair[1] * 255.f);
		float b = (Settings.Visuals.Colors.ScopeCrosshair[2] * 255.f);

		auto accuracy = 0.f;
	//	Interfaces.pSurface->DrawSetColor(r, g, b, 255);
		Interfaces.pSurface->DrawFilledCircle(crX, crY, 2, r, g, b, 255);
		//Interfaces.pSurface->DrawLine(crX + accuracy, crY, crX + 8 + accuracy, crY);
		//Interfaces.pSurface->DrawLine(crX - 8 - accuracy, crY, crX - accuracy, crY);
		//Interfaces.pSurface->DrawLine(crX, crY + accuracy, crX, crY + 8 + accuracy);
		//Interfaces.pSurface->DrawLine(crX, crY - 8 - accuracy, crX, crY - accuracy);
	}


	static void DrawLines()
	{
		Vector src3D, dst3D, forward, src, dst;
		trace_t tr;
		Ray_t ray;
		CTraceFilter filter;

		filter.pSkip = Hacks.LocalPlayer;

		g_Math.AngleVectors(Vector(0, Hacks.LocalPlayer->pelvisangs(), 0), &forward);
		src3D = Hacks.LocalPlayer->GetVecOrigin();
		dst3D = src3D + (forward * 42.f); //replace 50 with the length you want the line to have 

		ray.Init(src3D, dst3D);

		Interfaces.pTrace->TraceRay(ray, 0, &filter, &tr);

		if (!g_Math.WorldToScreen(src3D, src) || !g_Math.WorldToScreen(tr.endpos, dst))
			return;

		Interfaces.pSurface->RenderLine(src.x, src.y, dst.x, dst.y, CColor(0, 0, 255, 255));

		g_Math.AngleVectors(Vector(0, lineFakeAngle, 0), &forward);
		dst3D = src3D + (forward * 42.f); //replace 50 with the length you want the line to have 

		ray.Init(src3D, dst3D);

		Interfaces.pTrace->TraceRay(ray, 0, &filter, &tr);

		if (!g_Math.WorldToScreen(src3D, src) || !g_Math.WorldToScreen(tr.endpos, dst))
			return;

		Interfaces.pSurface->RenderLine(src.x, src.y, dst.x, dst.y, CColor(255, 0, 0, 255));
	}


	static void DrawInfo(RECT rect, CBaseEntity* pPlayer)
	{
		float bar = 0;
		float top = 1.4;
		float bot = 0.8;
		float right = 0;
		float left = 0;
		player_info_t info;
		static class Text
		{
		public:
			string text;
			int side;
			int Font;

			Text(string text, int side, int Font) : text(text), side(side), Font(Font)
			{
			}
		};
		std::vector< Text > texts;
		int middle = ((rect.right - rect.left) / 2) + rect.left;
		if (Interfaces.pEngine->GetPlayerInfo(pPlayer->GetIndex(), &info))
		{
			if (Settings.Visuals.Health)
			{
				bar++;
				float HealthValue = pPlayer->GetHealth();
				int iHealthValue = HealthValue;
				int Red = 255 - (HealthValue * 2.00);
				int Green = HealthValue * 2.00;
				float flBoxes = std::ceil(pPlayer->GetHealth() / 10.f);

				float height = (rect.bottom - rect.top) * (HealthValue / 100);
				float height2 = (rect.bottom - rect.top) * (100 / 100);
				float flHeight = height2 / 10.f;

				DrawRect(rect.left - 5, rect.top - 1, rect.left - 1, rect.bottom + 1, CColor(0, 0, 0, 150));
				DrawRect(rect.left - 4, rect.bottom - height, rect.left - 2, rect.bottom, CColor(Red, Green, 0, 255));

				for (int i = 0; i < 10; i++)
					DrawLine(rect.left - 5, rect.top + i * flHeight, rect.left - 2, rect.top + i * flHeight, CColor(0, 0, 0, 255));
			}

			if (Settings.Visuals.Armor)
			{
				float ArmorValue = pPlayer->GetArmorValue();
				int iArmorValue = ArmorValue;
				int red = 255 - (ArmorValue * 2.0);
				int blue = ArmorValue * 2.0;

				float height = (rect.right - rect.left) * (ArmorValue / 100);


				DrawRect(rect.left - 1, rect.bottom + 1, rect.right + 1, rect.bottom + 5, CColor(10, 10, 10, 165));
				DrawRect(rect.left, rect.bottom + 2, rect.left + height, rect.bottom + 4, CColor(red, blue, blue, 255));

				int Armor = pPlayer->GetArmorValue();
				//				if (Armor > 0)
				//					Interfaces.pSurface->DrawT(rect.left + height, rect.bottom + 2, CColor(255, 255, 255, 255), Hacks.Font_ESP, true, "%i AP", Armor);
			}

			if (Settings.Visuals.Name)
				Interfaces.pSurface->DrawT(middle, rect.top - top++ * 9, CColor(244, 244, 244, 255), Hacks.Font_ESP, true, "%s", pPlayer->GetName().c_str());

			if (Settings.Visuals.Scoped && pPlayer->m_bIsScoped())
				Interfaces.pSurface->DrawT(middle, rect.top - top++ * 9, CColor(255, 255, 255, 200), Hacks.Font_ESP, true, "Scoped");

			if (Settings.Visuals.Flashed && pPlayer->IsFlashed())
				Interfaces.pSurface->DrawT(middle, rect.top + top++ * 9, CColor(255, 255, 255, 200), Hacks.Font_ESP, true, "Flashed");

			CBaseCombatWeapon* weapon = pPlayer->GetActiveBaseCombatWeapon();
			if (weapon)
			{
				if (Settings.Visuals.Weapon && weapon)
				{
					//char* weaponchar = weapon->GetGunName();
					std::string weaponname = weapon->GetGunName();
					std::string ammo = std::to_string(weapon->ammo());

					CCSWeaponInfo* data = weapon->GetCSWpnData();

					std::string str = "";

					str += weaponname.c_str();

					
					Interfaces.pSurface->DrawT(middle, rect.bottom + bot++ * 9 + 1, CColor(244, 244, 244, 255), Hacks.Font_ESP, true, "%s", str.c_str());

					//texts.push_back(Text(weapon->GetGunIcon(), 2, Hacks.Font_ESP));
				}
			}
			//delete[]weapon;
		}

	}


	static void DrawZoom() {
		int iScreenWidth, iScreenHeight;
		Interfaces.pEngine->GetScreenSize(iScreenWidth, iScreenHeight);

		Interfaces.pSurface->DrawSetColor(0, 0, 0, 255);
		Interfaces.pSurface->DrawLine(0, iScreenHeight / 2, iScreenWidth, iScreenHeight / 2);
		Interfaces.pSurface->DrawLine(iScreenWidth / 2, 0, iScreenWidth / 2, iScreenHeight);
	}

	static void Spread()
	{
		int W, H, cW, cH;
		Interfaces.pEngine->GetScreenSize(W, H);

		cW = W / 2;
		cH = H / 2;

		int dX = W / Hacks.FOV;
		int dY = H / Hacks.FOV;
		int drX, drY;

		drX = cW;
		drY = cH;

		float r = (Settings.Visuals.Colors.SpreadCrossheir[0] * 255.f);
		float g = (Settings.Visuals.Colors.SpreadCrossheir[1] * 255.f);
		float b = (Settings.Visuals.Colors.SpreadCrossheir[2] * 255.f);

		Interfaces.pSurface->DrawSetColor(r, g, b, 255);
		float radius = ((Hacks.LocalPlayer->GetActiveBaseCombatWeapon()->GetSpread()) + (Hacks.LocalPlayer->GetActiveBaseCombatWeapon()->GetInaccuracy())) * 500;
		Interfaces.pSurface->DrawOutlinedCircle(drX, drY, radius, 255);
	}

    static void DrawTriangle(Vector pos, float size, CColor color)
	{
		// Drawing a Triangle
		Vector up, right, left, up_right, up_left;
		Vector s_up, s_right, s_left, s_up_right, s_up_left;
		up[0] = pos[0];
		up[1] = pos[1];
		up[2] = pos[2] + size / 2;
		g_Math.WorldToScreen(up, s_up);
		up_left[0] = pos[0] - size / 2;
		up_left[1] = pos[1] + size / 2;
		up_left[2] = pos[2] - size / 2;
		g_Math.WorldToScreen(up_left, s_up_left);
		up_right[0] = pos[0] + size / 2;
		up_right[1] = pos[1] + size / 2;
		up_right[2] = pos[2] - size / 2;
		g_Math.WorldToScreen(up_right, s_up_right);
		right[0] = pos[0] + size / 2;
		right[1] = pos[1] - size / 2;
		right[2] = pos[2] - size / 2;
		g_Math.WorldToScreen(right, s_right);
		left[0] = pos[0] - size / 2;
		left[1] = pos[1] - size / 2;
		left[2] = pos[2] - size / 2;
		g_Math.WorldToScreen(left, s_left);
		DrawLine(s_right[0], s_right[1], s_left[0], s_left[1], color);
		DrawLine(s_right[0], s_right[1], s_up_right[0], s_up_right[1], color);
		DrawLine(s_left[0], s_left[1], s_up_left[0], s_up_left[1], color);
		DrawLine(s_up_right[0], s_up_right[1], s_up_left[0], s_up_left[1], color);
		DrawLine(s_right[0], s_right[1], s_up[0], s_up[1], color);
		DrawLine(s_left[0], s_left[1], s_up[0], s_up[1], color);
		DrawLine(s_up_left[0], s_up_left[1], s_up[0], s_up[1], color);
		DrawLine(s_up_right[0], s_up_right[1], s_up[0], s_up[1], color);
	}

	static void DoEsp()
	{
		CBaseEntity* pEntity;
		static bool bCrosshair;
	
		if (Hacks.LocalPlayer->isAlive() && Settings.Visuals.AntiAimLines)
		{
			DrawLines();
		}

		if (Settings.Visuals.SpreadCrosshair && Hacks.LocalPlayer->isAlive())
		{
			Spread();
		}

		ThirdPerson();

		PostProcess();

		if (Settings.Visuals.Hitmarker && Hacks.LocalPlayer->isAlive())
			Hitmarker();

		if (Settings.Visuals.RemoveScope)
		{
			if (Hacks.LocalPlayer->m_bIsScoped())
			{
				if (Hacks.LocalPlayer->m_bIsScoped())
					DrawZoom();
			}
		}
		if (Settings.Visuals.Crosshair )
		{
			ConVar *gg = Interfaces.g_ICVars->FindVar("crosshair");
			gg->SetValue(0);
			Crosshair();

		}
		else
		{
			ConVar *gg = Interfaces.g_ICVars->FindVar("crosshair");
			gg->SetValue(1);
		}

		int width = 0;
		int height = 0;
		Interfaces.pEngine->GetScreenSize(width, height);
		for (auto i = 0; i <= Interfaces.pEntList->GetHighestEntityIndex(); i++)
		{
			CBaseEntity* pEntity = Interfaces.pEntList->GetClientEntity(i);
			if (pEntity == nullptr)
				continue;
			if (pEntity == Hacks.LocalPlayer)
				continue;
			if (pEntity->IsDormant())
				continue;

			player_info_t info;
			const ModelRenderInfo_t pInfo;
			if (Interfaces.pEngine->GetPlayerInfo(pEntity->GetIndex(), &info))
			{
				CColor clr = pEntity->GetTeam() == Hacks.LocalPlayer->GetTeam() ? CColor(50, 150, 255, 255) : CColor(255, 0, 0, 255);
				float RS = (Settings.Visuals.Colors.SnapLines[0] * 255.f);
				float GS = (Settings.Visuals.Colors.SnapLines[1] * 255.f);
				float BS = (Settings.Visuals.Colors.SnapLines[2] * 255.f);

				float R = (Settings.Visuals.Colors.Box[0] * 255.f);
				float G = (Settings.Visuals.Colors.Box[1] * 255.f);
				float B = (Settings.Visuals.Colors.Box[2] * 255.f);

				CColor snap = CColor(RS, GS, BS, 255);
				CColor box = CColor(R, G, B, 255);
				if (pEntity->GetTeam() == Hacks.LocalPlayer->GetTeam() && !Settings.Visuals.EspTeam)
					continue;
				if (!pEntity->isAlive())
					continue;

				bool PVS = false;
				RECT rect = DynamicBox(pEntity, PVS);

				DrawInfo(rect, pEntity);

				bool EspBox = (Settings.Visuals.Box);
				if (EspBox)
					DrawBox(rect, pEntity, box);
				if (Settings.Visuals.pBox)
					Draw3DTriangleBox(pEntity, box);

				if (Settings.Visuals.Lines)
					DrawSnapLine(Vector(rect.left + ((rect.right - rect.left) / 2), rect.bottom, 0), snap);

				if (Settings.Visuals.Skeleton)
					Skeleton(pEntity);

				int x, y, height;

			}
		}
	}
	static void Draw3DTriangleBox(CBaseEntity* pPlayer, CColor color)
	{
		Vector BoxPos = pPlayer->GetBonePos((int)CSGOHitboxID::LowerChest);
		DrawTriangle(BoxPos, 60, color);
	}

	static RECT DynamicBox(CBaseEntity* pPlayer, bool& PVS)
	{
		Vector trans = pPlayer->GetVecOrigin();

		Vector min;
		Vector max;

		min = pPlayer->BBMin();
		max = pPlayer->BBMax();

		Vector pointList[] = {
			Vector(min.x, min.y, min.z),
			Vector(min.x, max.y, min.z),
			Vector(max.x, max.y, min.z),
			Vector(max.x, min.y, min.z),
			Vector(max.x, max.y, max.z),
			Vector(min.x, max.y, max.z),
			Vector(min.x, min.y, max.z),
			Vector(max.x, min.y, max.z)
		};

		Vector Distance = pointList[0] - pointList[1];
		int dst = Distance.Length();
		dst /= 1.3f;
		Vector angs;
		Misc::CalcAngle(trans, Hacks.LocalPlayer->GetEyePosition(), angs);

		Vector all[8];
		angs.y += 45;
		for (int i = 0; i < 4; i++)
		{
			g_Math.angleVectors(angs, all[i]);
			all[i] *= dst;
			all[i + 4] = all[i];
			all[i].z = max.z;
			all[i + 4].z = min.z;
			VectorAdd(all[i], trans, all[i]);
			VectorAdd(all[i + 4], trans, all[i + 4]);
			angs.y += 90;
		}

		Vector flb, brt, blb, frt, frb, brb, blt, flt;
		PVS = true;

		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[3], flb))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[0], blb))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[2], frb))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[6], blt))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[5], brt))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[4], frt))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[1], brb))
			PVS = false;
		if (!Interfaces.g_pDebugOverlay->ScreenPosition(all[7], flt))
			PVS = false;

		Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

		float left = flb.x;
		float top = flb.y;
		float right = flb.x;
		float bottom = flb.y;

		for (int i = 0; i < 8; i++)
		{
			if (left > arr[i].x)
				left = arr[i].x;
			if (top > arr[i].y)
				top = arr[i].y;
			if (right < arr[i].x)
				right = arr[i].x;
			if (bottom < arr[i].y)
				bottom = arr[i].y;
		}
		RECT rect;
		rect.left = left;
		rect.bottom = bottom;
		rect.right = right;
		rect.top = top;
		return rect;
	}

	static void Skeleton(CBaseEntity* pEntity)
	{


		studiohdr_t* pStudioHdr = Interfaces.g_pModelInfo->GetStudioModel(pEntity->GetModel());

		if (!pStudioHdr)
			return;

		Vector vParent, vChild, sParent, sChild;

		for (int j = 0; j < pStudioHdr->numbones; j++)
		{
			mstudiobone_t* pBone = pStudioHdr->GetBone(j);

			if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
			{
				vChild = pEntity->GetBonePos(j);
				vParent = pEntity->GetBonePos(pBone->parent);
				Interfaces.g_pDebugOverlay->ScreenPosition(vParent, sParent);
				Interfaces.g_pDebugOverlay->ScreenPosition(vChild, sChild);
				int RS = (Settings.Visuals.Colors.Skeleton[0] * 255.f);
				int GS = (Settings.Visuals.Colors.Skeleton[1] * 255.f);
				int BS = (Settings.Visuals.Colors.Skeleton[2] * 255.f);
				Interfaces.pSurface->DrawSetColor(RS, GS, BS, 255);
				Interfaces.pSurface->DrawLine(sParent[0], sParent[1], sChild[0], sChild[1]);
			}
			//delete[]pBone;
		}

	}

	static void DrawSnapLine(Vector to, CColor clr)
	{
		int width = 0;
		int height = 0;
		Interfaces.pEngine->GetScreenSize(width, height);
		Vector From((width / 2), height - 1, 0);
		DrawLine(From.x, From.y, to.x, to.y, clr);
	}

	static void DrawBox(RECT rect, CBaseEntity* pEntity, CColor Color)
	{
		DrawOutlinedRect(rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1, CColor(0, 0, 0, 150));
		DrawOutlinedRect(rect.left + 1, rect.top + 1, rect.right - 1, rect.bottom - 1, CColor(0, 0, 0, 125));
		DrawOutlinedRect(rect.left, rect.top, rect.right, rect.bottom, Color);
	}

	static void CallGlow()
	{
		CGlowObjectManager* GlowObjectManager = (CGlowObjectManager*)Interfaces.g_pGlowObjectManager;

		for (int i = 0; i < GlowObjectManager->size; ++i)
		{
			CGlowObjectManager::GlowObjectDefinition_t* glowEntity = &GlowObjectManager->m_GlowObjectDefinitions[i];
			CBaseEntity* Entity = glowEntity->getEntity();

			if (glowEntity->IsEmpty() || !Entity)
				continue;

			char* EntName = Entity->GetClientClass()->m_pNetworkName;

			if (strstr(EntName, ("Projectile")) && Settings.Visuals.Glow.GrenadeGlow)
			{
				glowEntity->set(CColor(Settings.Visuals.Colors.GrenadeGlow[0] * 255, Settings.Visuals.Colors.GrenadeGlow[1] * 255, Settings.Visuals.Colors.GrenadeGlow[2] * 255, Settings.Visuals.Glow.Alpha), 0);
			}
			if ((strstr(EntName, ("CWeapon")) || strstr(EntName, ("CAK47")) || strstr(EntName, ("CDEagle"))) && Settings.Visuals.Glow.WeaponGlow)
			{
				glowEntity->set(CColor(Settings.Visuals.Colors.WeaponGlow[0] * 255, Settings.Visuals.Colors.WeaponGlow[1] * 255, Settings.Visuals.Colors.WeaponGlow[2] * 255, Settings.Visuals.Glow.Alpha), 0);
			}
			if ((strstr(EntName, ("CPlantedC4")) || strstr(EntName, ("CC4"))) && Settings.Visuals.Glow.BombGlow)
			{
				glowEntity->set(CColor(Settings.Visuals.Colors.BombGlow[0] * 255, Settings.Visuals.Colors.BombGlow[1] * 255, Settings.Visuals.Colors.BombGlow[2] * 255, Settings.Visuals.Glow.Alpha), 0);
			}
			if (strstr(EntName, ("CCSPlayer")))
			{
				if (Entity->GetTeam() == Hacks.LocalPlayer->GetTeam() && Settings.Visuals.Glow.GlowTeam)
					glowEntity->set(CColor(Settings.Visuals.Colors.TeamGlow[0] * 255, Settings.Visuals.Colors.TeamGlow[1] * 255, Settings.Visuals.Colors.TeamGlow[2] * 255, Settings.Visuals.Glow.Alpha), 0);

				if (Entity->GetTeam() != Hacks.LocalPlayer->GetTeam() && Settings.Visuals.Glow.GlowEnemy)
				{
					if (!Settings.Visuals.Colors.EnemyGlowHB)
						glowEntity->set(CColor(Settings.Visuals.Colors.EnemyGlow[0] * 255, Settings.Visuals.Colors.EnemyGlow[1] * 255, Settings.Visuals.Colors.EnemyGlow[2] * 255, Settings.Visuals.Glow.Alpha), 0);
					else
						glowEntity->set(Entity->GetHealthColor(Settings.Visuals.Colors.EnemyGlow[3] * 255), 0);
				}
			}
		}
	}
};

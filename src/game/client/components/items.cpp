/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/demo.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/effects.h>

#include "items.h"


void CItems::RenderProjectile(const CNetObj_Projectile *pCurrent, int ItemID)
{
	// get positions
	float Curvature = 0;
	float Speed = 0;
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		Curvature = m_pClient->m_Tuning[Config()->m_ClDummy].m_GrenadeCurvature;
		Speed = m_pClient->m_Tuning[Config()->m_ClDummy].m_GrenadeSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_SHOTGUN)
	{
		Curvature = m_pClient->m_Tuning[Config()->m_ClDummy].m_ShotgunCurvature;
		Speed = m_pClient->m_Tuning[Config()->m_ClDummy].m_ShotgunSpeed;
	}
	else if(pCurrent->m_Type == WEAPON_GUN)
	{
		Curvature = m_pClient->m_Tuning[Config()->m_ClDummy].m_GunCurvature;
		Speed = m_pClient->m_Tuning[Config()->m_ClDummy].m_GunSpeed;
	}

	static float s_LastGameTickTime = Client()->GameTickTime();
	if(!m_pClient->IsWorldPaused() && !m_pClient->IsDemoPlaybackPaused())
		s_LastGameTickTime = Client()->GameTickTime();
	
	float Ct;
	if(m_pClient->ShouldUsePredicted() && Config()->m_ClPredictProjectiles)
		Ct = ((float)(Client()->PredGameTick() - 1 - pCurrent->m_StartTick) + Client()->PredIntraGameTick())/(float)SERVER_TICK_SPEED;
	else
		Ct = (Client()->PrevGameTick()-pCurrent->m_StartTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
	if(Ct < 0)
		return; // projectile haven't been shot yet

	vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
	vec2 StartVel(pCurrent->m_VelX/100.0f, pCurrent->m_VelY/100.0f);
	vec2 Pos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct);
	vec2 PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed, Ct-0.001f);


	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[clamp(pCurrent->m_Type, 0, NUM_WEAPONS-1)].m_pSpriteProj);
	vec2 Vel = Pos-PrevPos;
	//vec2 pos = mix(vec2(prev->x, prev->y), vec2(current->x, current->y), Client()->IntraGameTick());


	// add particle for this projectile
	if(pCurrent->m_Type == WEAPON_GRENADE)
	{
		m_pClient->m_pEffects->SmokeTrail(Pos, Vel*-1);
		const float Now = Client()->LocalTime();
		static float s_Time = 0.0f;
		static float s_LastLocalTime = Now;
		s_Time += (Now - s_LastLocalTime) * m_pClient->GetAnimationPlaybackSpeed();
		Graphics()->QuadsSetRotation(s_Time*pi*2*2 + ItemID);
		s_LastLocalTime = Now;
	}
	else
	{
		m_pClient->m_pEffects->BulletTrail(Pos);

		if(length(Vel) > 0.00001f)
			Graphics()->QuadsSetRotation(angle(Vel));
		else
			Graphics()->QuadsSetRotation(0);

	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 32, 32);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsSetRotation(0);
	Graphics()->QuadsEnd();
}

void CItems::RenderPickup(const CNetObj_Pickup *pPrev, const CNetObj_Pickup *pCurrent)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());
	float Angle = 0.0f;
	float Size = 64.0f;
	const int c[] = {
		SPRITE_PICKUP_HEALTH,
		SPRITE_PICKUP_ARMOR,
		SPRITE_PICKUP_GRENADE,
		SPRITE_PICKUP_SHOTGUN,
		SPRITE_PICKUP_LASER,
		SPRITE_PICKUP_NINJA,
		SPRITE_PICKUP_GUN,
		SPRITE_PICKUP_HAMMER
		};
	RenderTools()->SelectSprite(c[pCurrent->m_Type]);

	switch(pCurrent->m_Type)
	{
	case PICKUP_GRENADE:
		Size = g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_VisualSize;
		break;
	case PICKUP_SHOTGUN:
		Size = g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_VisualSize;
		break;
	case PICKUP_LASER:
		Size = g_pData->m_Weapons.m_aId[WEAPON_LASER].m_VisualSize;
		break;
	case PICKUP_NINJA:
		m_pClient->m_pEffects->PowerupShine(Pos, vec2(96,18));
		Size *= 2.0f;
		Pos.x -= 10.0f;
		break;
	case PICKUP_GUN:
		Size = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_VisualSize;
		break;
	case PICKUP_HAMMER:
		Size = g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_VisualSize;
		break;
	}
	

	Graphics()->QuadsSetRotation(Angle);

	const float Now = Client()->LocalTime();
	static float s_Time = 0.0f;
	static float s_LastLocalTime = Now;
	s_Time += (Now - s_LastLocalTime) * m_pClient->GetAnimationPlaybackSpeed();
	const float Offset = Pos.y/32.0f + Pos.x/32.0f;
	Pos.x += cosf(s_Time*2.0f+Offset)*2.5f;
	Pos.y += sinf(s_Time*2.0f+Offset)*2.5f;
	s_LastLocalTime = Now;
	RenderTools()->DrawSprite(Pos.x, Pos.y, Size);
	Graphics()->QuadsEnd();
}

void CItems::RenderFlag(const CNetObj_Flag *pPrev, const CNetObj_Flag *pCurrent, const CNetObj_GameDataFlag *pPrevGameDataFlag, const CNetObj_GameDataFlag *pCurGameDataFlag)
{
	float Angle = 0.0f;
	float Size = 42.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	if(pCurrent->m_Team == TEAM_RED)
		RenderTools()->SelectSprite(SPRITE_FLAG_RED);
	else
		RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);

	Graphics()->QuadsSetRotation(Angle);

	vec2 Pos = mix(vec2(pPrev->m_X, pPrev->m_Y), vec2(pCurrent->m_X, pCurrent->m_Y), Client()->IntraGameTick());

	if(pCurGameDataFlag)
	{
		// make sure that the flag isn't interpolated between capture and return
		if(pPrevGameDataFlag &&
			((pCurrent->m_Team == TEAM_RED && pPrevGameDataFlag->m_FlagCarrierRed != pCurGameDataFlag->m_FlagCarrierRed) ||
			(pCurrent->m_Team == TEAM_BLUE && pPrevGameDataFlag->m_FlagCarrierBlue != pCurGameDataFlag->m_FlagCarrierBlue)))
			Pos = vec2(pCurrent->m_X, pCurrent->m_Y);

		int FlagCarrier = -1;
		if(pCurrent->m_Team == TEAM_RED && pCurGameDataFlag->m_FlagCarrierRed >= 0)
			FlagCarrier = pCurGameDataFlag->m_FlagCarrierRed;
		else if(pCurrent->m_Team == TEAM_BLUE && pCurGameDataFlag->m_FlagCarrierBlue >= 0)
			FlagCarrier = pCurGameDataFlag->m_FlagCarrierBlue;

		// make sure to use predicted position
		if(FlagCarrier >= 0 && FlagCarrier < MAX_CLIENTS && m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(FlagCarrier))
			Pos = m_pClient->GetCharPos(FlagCarrier, true);
	}

	IGraphics::CQuadItem QuadItem(Pos.x, Pos.y-Size*0.75f, Size, Size*2);
	Graphics()->QuadsDraw(&QuadItem, 1);
	Graphics()->QuadsEnd();
}


void CItems::RenderLaser(const struct CNetObj_Laser *pCurrent)
{
	vec2 Pos = vec2(pCurrent->m_X, pCurrent->m_Y);
	vec2 From = vec2(pCurrent->m_FromX, pCurrent->m_FromY);
	vec2 Dir = normalize(Pos-From);

	float Ticks = (Client()->GameTick() - pCurrent->m_StartTick) + Client()->IntraGameTick();
	float Ms = (Ticks/50.0f) * 1000.0f;
	float a = Ms / m_pClient->m_Tuning[Config()->m_ClDummy].m_LaserBounceDelay;
	a = clamp(a, 0.0f, 1.0f);
	float Ia = 1-a;

	vec2 Out, Border;

	Graphics()->BlendNormal();
	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	//vec4 inner_color(0.15f,0.35f,0.75f,1.0f);
	//vec4 outer_color(0.65f,0.85f,1.0f,1.0f);

	// do outline
	vec4 OuterColor(0.075f, 0.075f, 0.25f, 1.0f);
	Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (7.0f*Ia);

	IGraphics::CFreeformItem Freeform(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	// do inner
	vec4 InnerColor(0.5f, 0.5f, 1.0f, 1.0f);
	Out = vec2(Dir.y, -Dir.x) * (5.0f*Ia);
	Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f); // center

	Freeform = IGraphics::CFreeformItem(
			From.x-Out.x, From.y-Out.y,
			From.x+Out.x, From.y+Out.y,
			Pos.x-Out.x, Pos.y-Out.y,
			Pos.x+Out.x, Pos.y+Out.y);
	Graphics()->QuadsDrawFreeform(&Freeform, 1);

	Graphics()->QuadsEnd();

	// render head
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
		Graphics()->QuadsBegin();

		int Sprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(Sprites[Client()->GameTick()%3]);
		Graphics()->QuadsSetRotation(Client()->GameTick());
		Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
		IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f);
		QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	Graphics()->BlendNormal();
}

CNetObj_Projectile ProjectileDDNetStripExtraInfo(const CNetObj_DDNetProjectile *pProj)
{
	CNetObj_Projectile Result = {0};
	Result.m_X = pProj->m_X / 100.0f;
	Result.m_Y = pProj->m_Y / 100.0f;
	float Angle = pProj->m_Angle / 1000000.0f;
	Result.m_VelX = sin(-Angle) * 100.0f;
	Result.m_VelY = cos(-Angle) * 100.0f;
	Result.m_Type = pProj->m_Type;
	Result.m_StartTick = pProj->m_StartTick;
	return Result;
}

CNetObj_Projectile ProjectileStripExtraInfo(const CNetObj_Projectile *pProj)
{
	if(!(pProj->m_VelY >= 0 && (pProj->m_VelY & PROJECTILEFLAG_IS_DDNET) != 0))
	{
		return *pProj;
	}
	CNetObj_DDNetProjectile Proj;
	mem_copy(&Proj, pProj, sizeof(Proj));
	return ProjectileDDNetStripExtraInfo(&Proj);
}

void CItems::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_PROJECTILE || Item.m_Type == NETOBJTYPE_DDNETPROJECTILE)
		{
			CNetObj_Projectile Normal;
			if(Item.m_Type == NETOBJTYPE_PROJECTILE)
			{
				Normal = ProjectileStripExtraInfo((const CNetObj_Projectile *)pData);
			}
			else
			{
				Normal = ProjectileDDNetStripExtraInfo((const CNetObj_DDNetProjectile *)pData);
			}
			RenderProjectile(&Normal, Item.m_ID);
		}
		else if(Item.m_Type == NETOBJTYPE_PICKUP)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if(pPrev)
				RenderPickup((const CNetObj_Pickup *)pPrev, (const CNetObj_Pickup *)pData);
		}
		else if(Item.m_Type == NETOBJTYPE_LASER)
		{
			RenderLaser((const CNetObj_Laser *)pData);
		}
	}

	// render flag
	for(int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if(Item.m_Type == NETOBJTYPE_FLAG)
		{
			const void *pPrev = Client()->SnapFindItem(IClient::SNAP_PREV, Item.m_Type, Item.m_ID);
			if(pPrev)
			{
				const void *pPrevGameDataFlag = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_GAMEDATAFLAG, m_pClient->m_Snap.m_GameDataFlagSnapID);
				RenderFlag(static_cast<const CNetObj_Flag *>(pPrev), static_cast<const CNetObj_Flag *>(pData),
							static_cast<const CNetObj_GameDataFlag *>(pPrevGameDataFlag), m_pClient->m_Snap.m_pGameDataFlag);
			}
		}
	}
}


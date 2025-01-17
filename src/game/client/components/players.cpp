/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/render.h>

#include <game/client/components/flow.h>
#include <game/client/components/skins.h>
#include <game/client/components/effects.h>
#include <game/client/components/sounds.h>
#include <game/client/components/controls.h>

#include "players.h"

inline float NormalizeAngular(float f)
{
	return fmod(f+pi*2, pi*2);
}

inline float AngularMixDirection (float Src, float Dst) { return sinf(Dst-Src) >0?1:-1; }
inline float AngularDistance(float Src, float Dst) { return asinf(sinf(Dst-Src)); }

inline float AngularApproach(float Src, float Dst, float Amount)
{
	float d = AngularMixDirection (Src, Dst);
	float n = Src + Amount*d;
	if(AngularMixDirection (n, Dst) != d)
		return Dst;
	return n;
}

void CPlayers::RenderHook(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CTeeRenderInfo *pRenderInfo,
	int ClientID
	) const
{
	CNetObj_Character Prev = *pPrevChar;
	CNetObj_Character Player = *pPlayerChar;
	CTeeRenderInfo RenderInfo = *pRenderInfo;

	float IntraTick = Client()->IntraGameTick();

	// set size
	RenderInfo.m_Size = 64.0f;

	if(m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(ClientID))
	{
		m_pClient->UsePredictedChar(&Prev, &Player, &IntraTick, ClientID);
	}

	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);

	// draw hook
	if(Prev.m_HookState>0 && Player.m_HookState>0)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		//Graphics()->QuadsBegin();

		vec2 Pos = Position;
		vec2 HookPos;

		if(Player.m_HookedPlayer != -1 && m_pClient->m_Snap.m_aCharacters[Player.m_HookedPlayer].m_Active)
		{
			// `HookedPlayer != -1` means that a player is being hooked
			bool Predicted = m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(Player.m_HookedPlayer);
			HookPos = m_pClient->GetCharPos(Player.m_HookedPlayer, Predicted);
		}
		else
		{
			// The hook is in the air, on a hookable tile or the hooked player is out of range
			HookPos = mix(vec2(Prev.m_HookX, Prev.m_HookY), vec2(Player.m_HookX, Player.m_HookY), IntraTick);
		}

		float d = distance(Pos, HookPos);
		vec2 Dir = normalize(Pos-HookPos);

		Graphics()->QuadsSetRotation(angle(Dir)+pi);

		// render head
		RenderTools()->SelectSprite(SPRITE_HOOK_HEAD);
		IGraphics::CQuadItem QuadItem(HookPos.x, HookPos.y, 24,16);
		Graphics()->QuadsDraw(&QuadItem, 1);

		// render chain
		RenderTools()->SelectSprite(SPRITE_HOOK_CHAIN);
		IGraphics::CQuadItem Array[1024];
		int i = 0;
		for(float f = 16; f < d && i < 1024; f += 16, i++)
		{
			vec2 p = HookPos + Dir*f;
			Array[i] = IGraphics::CQuadItem(p.x, p.y,16,16);
		}

		Graphics()->QuadsDraw(Array, i);
		Graphics()->QuadsSetRotation(0);
		Graphics()->QuadsEnd();

		RenderTools()->RenderTeeHand(&RenderInfo, Position, normalize(HookPos-Pos), -pi/2, vec2(20, 0));
	}
}

void CPlayers::RenderPlayer(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CNetObj_PlayerInfo *pPlayerInfo,
	const CTeeRenderInfo *pRenderInfo,
	int ClientID
	) const
{
	CNetObj_Character Prev = *pPrevChar;
	CNetObj_Character Player = *pPlayerChar;
	CTeeRenderInfo RenderInfo = *pRenderInfo;

	const CGameClient::CClientData *pClientData = &m_pClient->m_aClients[ClientID];

	// set size
	RenderInfo.m_Size = 64.0f;

	float IntraTick = Client()->IntraGameTick();

	if(Prev.m_Angle < pi*-128 && Player.m_Angle > pi*128)
		Prev.m_Angle += 2*pi*256;
	else if(Prev.m_Angle > pi*128 && Player.m_Angle < pi*-128)
		Player.m_Angle += 2*pi*256;
	float Angle = mix((float)Prev.m_Angle, (float)Player.m_Angle, IntraTick)/256.0f;

	//float angle = 0;

	if(m_pClient->m_LocalClientID[Config()->m_ClDummy] == ClientID && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		// just use the direct input if it's local player we are rendering
		Angle = angle(m_pClient->m_pControls->m_MousePos[Config()->m_ClDummy]);
	}
	else
	{
		/*
		float mixspeed = Client()->FrameTime()*2.5f;
		if(player.attacktick != prev.attacktick) // shooting boosts the mixing speed
			mixspeed *= 15.0f;

		// move the delta on a constant speed on a x^2 curve
		float current = g_GameClient.m_aClients[info.cid].angle;
		float target = player.angle/256.0f;
		float delta = angular_distance(current, target);
		float sign = delta < 0 ? -1 : 1;
		float new_delta = delta - 2*mixspeed*sqrt(delta*sign)*sign + mixspeed*mixspeed;

		// make sure that it doesn't vibrate when it's still
		if(fabs(delta) < 2/256.0f)
			angle = target;
		else
			angle = angular_approach(current, target, fabs(delta-new_delta));

		g_GameClient.m_aClients[info.cid].angle = angle;*/
	}

	if(m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(ClientID))
	{
		m_pClient->UsePredictedChar(&Prev, &Player, &IntraTick, ClientID);
	}
	const bool Paused = m_pClient->IsWorldPaused() || m_pClient->IsDemoPlaybackPaused();

	vec2 Direction = direction(Angle);
	vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), IntraTick);
	vec2 Vel = mix(vec2(Prev.m_VelX/256.0f, Prev.m_VelY/256.0f), vec2(Player.m_VelX/256.0f, Player.m_VelY/256.0f), IntraTick);

	m_pClient->m_pFlow->Add(Position, Vel*100.0f, 10.0f);

	RenderInfo.m_GotAirJump = Player.m_Jumped&2?0:1;

	bool Stationary = Player.m_VelX <= 1 && Player.m_VelX >= -1;
	bool InAir = !Collision()->CheckPoint(Player.m_X, Player.m_Y+16);
	bool WantOtherDir = (Player.m_Direction == -1 && Vel.x > 0) || (Player.m_Direction == 1 && Vel.x < 0);

	// evaluate animation
	const float WalkTimeMagic = 100.0f;
	float WalkTime =
		((Position.x >= 0)
			? fmod(Position.x, WalkTimeMagic)
			: WalkTimeMagic - fmod(-Position.x, WalkTimeMagic))
		/ WalkTimeMagic;
	CAnimState State;
	State.Set(&g_pData->m_aAnimations[ANIM_BASE], 0);

	if(InAir)
		State.Add(&g_pData->m_aAnimations[ANIM_INAIR], 0, 1.0f); // TODO: some sort of time here
	else if(Stationary)
		State.Add(&g_pData->m_aAnimations[ANIM_IDLE], 0, 1.0f); // TODO: some sort of time here
	else if(!WantOtherDir)
		State.Add(&g_pData->m_aAnimations[ANIM_WALK], WalkTime, 1.0f);

	static float s_LastGameTickTime = Client()->GameTickTime();
	if(!Paused)
		s_LastGameTickTime = Client()->GameTickTime();
	if (Player.m_Weapon == WEAPON_HAMMER)
	{
		float ct = (Client()->PrevGameTick()-Player.m_AttackTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
		State.Add(&g_pData->m_aAnimations[ANIM_HAMMER_SWING], clamp(ct*5.0f,0.0f,1.0f), 1.0f);
	}
	if (Player.m_Weapon == WEAPON_NINJA)
	{
		float ct = (Client()->PrevGameTick()-Player.m_AttackTick)/(float)SERVER_TICK_SPEED + s_LastGameTickTime;
		State.Add(&g_pData->m_aAnimations[ANIM_NINJA_SWING], clamp(ct*2.0f,0.0f,1.0f), 1.0f);
	}

	// do skidding
	if(!InAir && WantOtherDir && length(Vel*50) > 500.0f)
	{
		static int64 SkidSoundTime = 0;
		if(time_get()-SkidSoundTime > time_freq()/10)
		{
			m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_SKID, 0.25f, Position);
			SkidSoundTime = time_get();
		}

		m_pClient->m_pEffects->SkidTrail(
			Position+vec2(-Player.m_Direction*6,12),
			vec2(-Player.m_Direction*100*length(Vel),-50)
		);
	}

	bool Local = m_pClient->m_LocalClientID[Config()->m_ClDummy] == ClientID;

	// hook collision
	bool Aim = (pPlayerInfo->m_PlayerFlags&PLAYERFLAG_AIM);
	if (!Client()->ServerCapAnyPlayerFlag())
	{
		for (int i = 0; i < NUM_CLIENTS; i++)
			if (ClientID == m_pClient->m_LocalClientID[i])
			{
				Aim = GameClient()->m_pControls->m_ShowHookColl[i];
				break;
			}
	}

	if(ClientID >= 0 && (Config()->m_ClShowHookCollAlways || (Aim && ((!Local && Config()->m_ClShowHookCollOther) || (Local && Config()->m_ClShowHookCollOwn)))))
	{
		vec2 ExDirection = Direction;

		if(Local && Client()->State() != IClient::STATE_DEMOPLAYBACK)
			ExDirection = normalize(vec2(m_pClient->m_pControls->m_InputData[Config()->m_ClDummy].m_TargetX, m_pClient->m_pControls->m_InputData[Config()->m_ClDummy].m_TargetY));

		Graphics()->TextureClear();
		vec2 InitPos = Position;
		vec2 FinishPos = InitPos + ExDirection * (m_pClient->m_Tuning[Config()->m_ClDummy].m_HookLength-42.0f);

		Graphics()->LinesBegin();
		vec3 HookCollColor(1.0f, 0.0f, 0.0f);

		float PhysSize = 28.0f;

		vec2 OldPos = InitPos + ExDirection * PhysSize * 1.5f;
		vec2 NewPos = OldPos;

		bool DoBreak = false;
		int Hit = 0;

		do {
			OldPos = NewPos;
			NewPos = OldPos + ExDirection * m_pClient->m_Tuning[Config()->m_ClDummy].m_HookFireSpeed;

			if(distance(InitPos, NewPos) > m_pClient->m_Tuning[Config()->m_ClDummy].m_HookLength)
			{
				NewPos = InitPos + normalize(NewPos-InitPos) * m_pClient->m_Tuning[Config()->m_ClDummy].m_HookLength;
				DoBreak = true;
			}

			int TeleNr = 0;
			Hit = Collision()->IntersectLineTeleHook(OldPos, NewPos, &FinishPos, 0x0, &TeleNr);

			if(!DoBreak && Hit)
			{
				if(Hit != TILE_NOHOOK)
				{
					HookCollColor.r = 130.0f/255.0f;
					HookCollColor.g = 232.0f/255.0f;
					HookCollColor.b = 160.0f/255.0f;
				}
			}

			if(m_pClient->IntersectCharacter(OldPos, FinishPos, FinishPos, ClientID) != -1)
			{
				HookCollColor.r = 1.0f;
				HookCollColor.g = 1.0f;
				HookCollColor.b = 0.0f;
				break;
			}

			if(Hit)
				break;

			NewPos.x = round_to_int(NewPos.x);
			NewPos.y = round_to_int(NewPos.y);

			if(OldPos == NewPos)
				break;

			ExDirection.x = round_to_int(ExDirection.x*256.0f) / 256.0f;
			ExDirection.y = round_to_int(ExDirection.y*256.0f) / 256.0f;
		} while (!DoBreak);

		if(Config()->m_ClShowHookCollAlways && Aim)
		{
			// invert the hook coll colors when using cl_show_hook_coll_always and +showhookcoll is pressed
			HookCollColor.r = 1.0f - HookCollColor.r;
			HookCollColor.g = 1.0f - HookCollColor.g;
			HookCollColor.b = 1.0f - HookCollColor.b;
		}
		Graphics()->SetColor(HookCollColor.r, HookCollColor.g, HookCollColor.b, 1.0f);
		IGraphics::CLineItem LineItem(InitPos.x, InitPos.y, FinishPos.x, FinishPos.y);
		Graphics()->LinesDraw(&LineItem, 1);
		Graphics()->LinesEnd();
	}

	// draw gun
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation(State.GetAttach()->m_Angle*pi*2+Angle);

		// normal weapons
		int iw = clamp(Player.m_Weapon, 0, NUM_WEAPONS-1);
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_pSpriteBody, Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);

		vec2 Dir = Direction;
		float Recoil = 0.0f;
		vec2 p;
		if (Player.m_Weapon == WEAPON_HAMMER)
		{
			// Static position for hammer
			p = Position + vec2(State.GetAttach()->m_X, State.GetAttach()->m_Y);
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
			// if attack is under way, bash stuffs
			if(Direction.x < 0)
			{
				Graphics()->QuadsSetRotation(-pi/2-State.GetAttach()->m_Angle*pi*2);
				p.x -= g_pData->m_Weapons.m_aId[iw].m_Offsetx;
			}
			else
			{
				Graphics()->QuadsSetRotation(-pi/2+State.GetAttach()->m_Angle*pi*2);
			}
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
		}
		else if (Player.m_Weapon == WEAPON_NINJA)
		{
			p = Position;
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;

			if(Direction.x < 0)
			{
				Graphics()->QuadsSetRotation(-pi/2-State.GetAttach()->m_Angle*pi*2);
				p.x -= g_pData->m_Weapons.m_aId[iw].m_Offsetx;
				m_pClient->m_pEffects->PowerupShine(p+vec2(32,0), vec2(32,12));
			}
			else
			{
				Graphics()->QuadsSetRotation(-pi/2+State.GetAttach()->m_Angle*pi*2);
				m_pClient->m_pEffects->PowerupShine(p-vec2(32,0), vec2(32,12));
			}
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);

			// HADOKEN
			if ((Client()->GameTick()-Player.m_AttackTick) <= (SERVER_TICK_SPEED / 6) && g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles)
			{
				int IteX = random_int() % g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles;
				static int s_LastIteX = IteX;
				if(Paused)
					IteX = s_LastIteX;
				else
					s_LastIteX = IteX;
				if(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[IteX])
				{
					vec2 Dir = vec2(pPlayerChar->m_X,pPlayerChar->m_Y) - vec2(pPrevChar->m_X, pPrevChar->m_Y);
					Dir = normalize(Dir);
					float HadokenAngle = angle(Dir);
					Graphics()->QuadsSetRotation(HadokenAngle );
					//float offsety = -data->weapons[iw].muzzleoffsety;
					RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[IteX], 0);
					vec2 DirY(-Dir.y,Dir.x);
					p = Position;
					float OffsetX = g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsetx;
					p -= Dir * OffsetX;
					RenderTools()->DrawSprite(p.x, p.y, 160.0f);
				}
			}
		}
		else
		{
			// TODO: should be an animation
			Recoil = 0;
			static float s_LastIntraTick = IntraTick;
			if(!Paused)
				s_LastIntraTick = IntraTick;

			float a = (Client()->GameTick()-Player.m_AttackTick+s_LastIntraTick)/5.0f;
			if(a < 1)
				Recoil = sinf(a*pi);
			p = Position + Dir * g_pData->m_Weapons.m_aId[iw].m_Offsetx - Dir*Recoil*10.0f;
			p.y += g_pData->m_Weapons.m_aId[iw].m_Offsety;
			if(Player.m_Weapon == WEAPON_GUN && !Config()->m_ClOldGunPosition)
				p.y += 8;
			RenderTools()->DrawSprite(p.x, p.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
		}

		if (Player.m_Weapon == WEAPON_GUN || Player.m_Weapon == WEAPON_SHOTGUN)
		{
			// check if we're firing stuff
			if(g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles)//prev.attackticks)
			{
				float Alpha = 0.0f;
				int Phase1Tick = (Client()->GameTick() - Player.m_AttackTick);
				if (Phase1Tick < (g_pData->m_Weapons.m_aId[iw].m_Muzzleduration + 3))
				{
					float t = ((((float)Phase1Tick) + IntraTick)/(float)g_pData->m_Weapons.m_aId[iw].m_Muzzleduration);
					Alpha = mix(2.0f, 0.0f, min(1.0f,max(0.0f,t)));
				}

				int IteX = random_int() % g_pData->m_Weapons.m_aId[iw].m_NumSpriteMuzzles;
				static int s_LastIteX = IteX;
				if(Paused)
					IteX = s_LastIteX;
				else
					s_LastIteX = IteX;
				if (Alpha > 0.0f && g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[IteX])
				{
					float OffsetY = -g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsety;
					RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[iw].m_aSpriteMuzzles[IteX], Direction.x < 0 ? SPRITE_FLAG_FLIP_Y : 0);
					if(Direction.x < 0)
						OffsetY = -OffsetY;

					vec2 DirY(-Dir.y,Dir.x);
					vec2 MuzzlePos = p + Dir * g_pData->m_Weapons.m_aId[iw].m_Muzzleoffsetx + DirY * OffsetY;

					RenderTools()->DrawSprite(MuzzlePos.x, MuzzlePos.y, g_pData->m_Weapons.m_aId[iw].m_VisualSize);
				}
			}
		}
		Graphics()->QuadsEnd();

		switch (Player.m_Weapon)
		{
			case WEAPON_GUN: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -3*pi/4, vec2(-15, 4)); break;
			case WEAPON_SHOTGUN: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -pi/2, vec2(-5, 4)); break;
			case WEAPON_GRENADE: RenderTools()->RenderTeeHand(&RenderInfo, p, Direction, -pi/2, vec2(-4, 7)); break;
		}

	}

	// render the "shadow" tee
	if(m_pClient->m_LocalClientID[Config()->m_ClDummy] == ClientID && Config()->m_Debug)
	{
		vec2 GhostPosition = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());
		CTeeRenderInfo Ghost = RenderInfo;
		for(int p = 0; p < NUM_SKINPARTS; p++)
			Ghost.m_aColors[p].a *= 0.5f;
		RenderTools()->RenderTee(&State, &Ghost, Player.m_Emote, Direction, GhostPosition); // render ghost
	}

	RenderTools()->RenderTee(&State, &RenderInfo, Player.m_Emote, Direction, Position);

	if(pPlayerInfo->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_DOTDOT);
		IGraphics::CQuadItem QuadItem(Position.x + 24, Position.y - 40, 64,64);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	if(Config()->m_ClAfkEmote && pClientData->m_Afk && !(pPlayerInfo->m_PlayerFlags&PLAYERFLAG_CHATTING) && !(Client()->DummyConnected() && ClientID == m_pClient->m_LocalClientID[!Config()->m_ClDummy]))
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_ZZZ);
		IGraphics::CQuadItem QuadItem(Position.x + 24, Position.y - 40, 64,64);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	if(pClientData->m_EmoticonStart != -1 && pClientData->m_EmoticonStart + 2 * Client()->GameTickSpeed() > Client()->GameTick())
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
		Graphics()->QuadsBegin();

		int SinceStart = Client()->GameTick() - pClientData->m_EmoticonStart;
		int FromEnd = pClientData->m_EmoticonStart + 2 * Client()->GameTickSpeed() - Client()->GameTick();

		float a = 1;

		if (FromEnd < Client()->GameTickSpeed() / 5)
			a = FromEnd / (Client()->GameTickSpeed() / 5.0);

		float h = 1;
		if (SinceStart < Client()->GameTickSpeed() / 10)
			h = SinceStart / (Client()->GameTickSpeed() / 10.0);

		float Wiggle = 0;
		if (SinceStart < Client()->GameTickSpeed() / 5)
			Wiggle = SinceStart / (Client()->GameTickSpeed() / 5.0);

		float WiggleAngle = sinf(5*Wiggle);

		Graphics()->QuadsSetRotation(pi/6*WiggleAngle);

		Graphics()->SetColor(1.0f * a, 1.0f * a, 1.0f * a, a);
		// client_datas::emoticon is an offset from the first emoticon
		RenderTools()->SelectSprite(SPRITE_OOP + pClientData->m_Emoticon);
		IGraphics::CQuadItem QuadItem(Position.x, Position.y - 23 - 32*h, 64, 64*h);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CPlayers::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	static const CNetObj_PlayerInfo *s_apInfo[MAX_CLIENTS];
	static CTeeRenderInfo s_aRenderInfo[MAX_CLIENTS];
	static CTeeRenderInfo s_RenderInfoSpec;

	// update RenderInfo for ninja
	bool IsTeamplay = (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) != 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		s_apInfo[i] = (const CNetObj_PlayerInfo *)Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);
		s_aRenderInfo[i] = m_pClient->m_aClients[i].m_RenderInfo;

		if(m_pClient->m_Snap.m_aCharacters[i].m_Cur.m_Weapon == WEAPON_NINJA && Config()->m_ClShowNinja)
		{
			// change the skin for the player to the ninja
			int Skin = m_pClient->m_pSkins->Find("x_ninja", true);
			if(Skin != -1)
			{
				const CSkins::CSkin *pNinja = m_pClient->m_pSkins->Get(Skin);
				for(int p = 0; p < NUM_SKINPARTS; p++)
				{
					if(IsTeamplay)
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_ColorTexture;
						int ColorVal = m_pClient->m_pSkins->GetTeamColor(true, pNinja->m_aPartColors[p], m_pClient->m_aClients[i].m_Team, p);
						s_aRenderInfo[i].m_aColors[p] = m_pClient->m_pSkins->GetColorV4(ColorVal, p==SKINPART_MARKING);
					}
					else if(pNinja->m_aUseCustomColors[p])
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_ColorTexture;
						s_aRenderInfo[i].m_aColors[p] = m_pClient->m_pSkins->GetColorV4(pNinja->m_aPartColors[p], p==SKINPART_MARKING);
					}
					else
					{
						s_aRenderInfo[i].m_aTextures[p] = pNinja->m_apParts[p]->m_OrgTexture;
						s_aRenderInfo[i].m_aColors[p] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
					}
				}
			}
		}
	}

	int Skin = m_pClient->m_pSkins->Find("x_spec", true);
	const CSkins::CSkin *pSkin = m_pClient->m_pSkins->Get(Skin);
	s_RenderInfoSpec.m_Size = 64.f;
	s_RenderInfoSpec.m_aTextures[SKINPART_BODY] = pSkin->m_apParts[SKINPART_BODY]->m_OrgTexture;
	s_RenderInfoSpec.m_aColors[SKINPART_BODY] = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	// render other players in three passes, first pass we render spectees,
	// then everyone but us, and finally we render ourselves
	for (int p = 0; p < 6; p++)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if (p % 3 == 0 && !m_pClient->m_aClients[i].m_SpecCharPresent)
				continue;
			// only render active characters
			if(p % 3 != 0 && (!m_pClient->m_Snap.m_aCharacters[i].m_Active || !s_apInfo[i]))
				continue;

			if (p % 3 == 0)
			{
				if (p < 3)
					continue;
				vec2 Pos = m_pClient->m_aClients[i].m_SpecChar;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &s_RenderInfoSpec, EMOTE_BLINK, vec2(1, 0), Pos, true);
			}
			else
			{
				//
				bool Local = m_pClient->m_LocalClientID[Config()->m_ClDummy] == i;
				if((p % 3) == 1 && Local) continue;
				if((p % 3) == 2 && !Local) continue;

				CNetObj_Character *pPrevChar = &m_pClient->m_Snap.m_aCharacters[i].m_Prev;
				CNetObj_Character *pCurChar = &m_pClient->m_Snap.m_aCharacters[i].m_Cur;

				if(p<3)
				{
					RenderHook(pPrevChar, pCurChar, &s_aRenderInfo[i], i);
				}
				else
				{
					RenderPlayer(pPrevChar, pCurChar, s_apInfo[i], &s_aRenderInfo[i], i);
				}
			}
		}
	}
}

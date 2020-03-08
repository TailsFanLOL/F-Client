/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/localization.h>
#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/stats.h>

#include "menus.h"
#include "stats.h"
#include "scoreboard.h"
#include "stats.h"


CScoreboard::CScoreboard()
{
	OnReset();
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	CScoreboard *pScoreboard = (CScoreboard *)pUserData;
	int Result = pResult->GetInteger(0);
	if(!Result)
	{
		pScoreboard->m_Activate = false;
		pScoreboard->m_Active = false;
	}
	else if(!pScoreboard->m_Active)
		pScoreboard->m_Activate = true;	
}

void CScoreboard::OnReset()
{
	m_Active = false;
	m_Activate = false;
}

void CScoreboard::OnRelease()
{
	m_Active = false;
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
	float h = 20.0f;

	Graphics()->BlendNormal();
	CUIRect Rect = {x, y, w, h};
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);

	// render goals
	y += 2.0f;
	if(m_pClient->m_GameInfo.m_ScoreLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_GameInfo.m_ScoreLimit);
		TextRender()->Text(0, x+10.0f, y, 12.0f, aBuf, -1.0f);
	}
	if(m_pClient->m_GameInfo.m_TimeLimit)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_GameInfo.m_TimeLimit);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1, -1.0f);
		TextRender()->Text(0, x+w/2-tw/2, y, 12.0f, aBuf, -1.0f);
	}
	if(m_pClient->m_GameInfo.m_MatchNum && m_pClient->m_GameInfo.m_MatchCurrent)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Match", "rounds (scoreboard)"), m_pClient->m_GameInfo.m_MatchCurrent, m_pClient->m_GameInfo.m_MatchNum);
		float tw = TextRender()->TextWidth(0, 12.0f, aBuf, -1, -1.0f);
		TextRender()->Text(0, x+w-tw-10.0f, y, 12.0f, aBuf, -1.0f);
	}
}

float CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 20.0f;

	int NumSpectators = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(m_pClient->m_aClients[i].m_Active && m_pClient->m_aClients[i].m_Team == TEAM_SPECTATORS)
			NumSpectators++;

	char aBuf[64];
	char SpectatorBuf[64];
	str_format(SpectatorBuf, sizeof(SpectatorBuf), "%s (%d):", Localize("Spectators"), NumSpectators);
	float tw = TextRender()->TextWidth(0, 12.0f, SpectatorBuf, -1, -1.0f);

	// do all the text without rendering it once
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x, y, 12.0f, TEXTFLAG_ALLOW_NEWLINE);
	Cursor.m_LineWidth = w-17.0f;
	Cursor.m_StartX -= tw+3.0f;
	Cursor.m_MaxLines = 4;
	bool Multiple = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			TextRender()->TextEx(&Cursor, ", ", -1);
		if(Config()->m_ClShowUserId && Cursor.m_LineCount <= Cursor.m_MaxLines)
		{
			Cursor.m_X += Cursor.m_FontSize;
		}
		if(m_pClient->m_aClients[i].m_aClan[0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", m_pClient->m_aClients[i].m_aClan);
			TextRender()->TextEx(&Cursor, aBuf, -1);
		}
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[i].m_aName, -1);
		Multiple = true;
	}

	// background
	float RectHeight = 3*h+(Cursor.m_Y-Cursor.m_StartY);
	Graphics()->BlendNormal();
	CUIRect Rect = {x, y, w, RectHeight};
	RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);

	// Headline
	y += 30.0f;
	TextRender()->Text(0, x+10.0f, y, 12.0f, SpectatorBuf, w-20.0f);

	// spectator names and now render everything
	x += tw+2.0f+10.0f;
	Multiple = false;
	TextRender()->SetCursor(&Cursor, x, y, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_ALLOW_NEWLINE);
	Cursor.m_LineWidth = w-17.0f;
	Cursor.m_StartX -= tw+3.0f;
	Cursor.m_MaxLines = 4;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || m_pClient->m_aClients[i].m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			TextRender()->TextEx(&Cursor, ", ", -1);

		if(Config()->m_ClShowUserId && Cursor.m_LineCount <= Cursor.m_MaxLines)
		{
			RenderTools()->DrawClientID(TextRender(), &Cursor, i);
		}

		if(m_pClient->m_aClients[i].m_aClan[0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", m_pClient->m_aClients[i].m_aClan);
			TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f : 1.0f, 0.7f);
			TextRender()->TextEx(&Cursor, aBuf, -1);
		}

		if(pInfo->m_PlayerFlags&PLAYERFLAG_ADMIN)
		{
			vec4 Color = m_pClient->m_pSkins->GetColorV4(Config()->m_ClAuthedPlayerColor, false);
			TextRender()->TextColor(Color.r, Color.g, (pInfo->m_PlayerFlags & PLAYERFLAG_WATCHING) ? 0.0f : Color.b, Color.a);
		}
		else
			TextRender()->TextColor(1.0f, 1.0f, (pInfo->m_PlayerFlags&PLAYERFLAG_WATCHING) ? 0.0f :	 1.0f, 1.0f);
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[i].m_aName, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Multiple = true;
	}

	return RectHeight;
}

float CScoreboard::RenderScoreboard(float x, float y, float w, int Team, const char *pTitle, int Align)
{
	if(Team == TEAM_SPECTATORS)
		return 0.0f;

	bool lower16 = false;
	bool upper16 = false;
	bool lower24 = false;
	bool upper24 = false;
	bool lower32 = false;
	bool upper32 = false;

	if(Team == -3)
		upper16 = true;
	else if(Team == -4)
		lower32 = true;
	else if(Team == -5)
		upper32 = true;
	else if(Team == -6)
		lower16 = true;
	else if(Team == -7)
		lower24 = true;
	else if(Team == -8)
		upper24 = true;

	if(Team < -1)
		Team = 0;

	// ready mode
	const CGameClient::CSnapState& Snap = m_pClient->m_Snap;
	const bool ReadyMode = Snap.m_pGameData && (Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED|GAMESTATEFLAG_WARMUP)) && Snap.m_pGameData->m_GameStateEndTick == 0;

	bool Race = m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_RACE;

	float HeadlineHeight = 40.0f;
	float TitleFontsize = 20.0f;

	float HeadlineFontsize = 12.0f;
	if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 48)
		HeadlineFontsize = 8.0f;
	else if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 32)
		HeadlineFontsize = 9.0f;
	else if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 16)
		HeadlineFontsize = 10.0f;

	float LineHeight = 20.0f;
	float TeeSizeMod = 1.0f;
	float Spacing = 2.0f;
	float PingOffset = x+Spacing, PingLength = 35.0f;
	float CountryFlagOffset = PingOffset+PingLength, CountryFlagLength = 20.f;
	float IdSize = Config()->m_ClShowUserId ? LineHeight : 0.0f;
	float ReadyLength = ReadyMode ? 10.f : 0.f;
	float TeeOffset = CountryFlagOffset+CountryFlagLength+4.0f, TeeLength = 25*TeeSizeMod;
	float NameOffset = CountryFlagOffset+CountryFlagLength+IdSize, NameLength = 128.0f-IdSize/2-ReadyLength;
	float ClanOffset = NameOffset+NameLength+ReadyLength, ClanLength = 88.0f-IdSize/2;
	float KillOffset = ClanOffset+ClanLength, KillLength = Race ? 0.0f : 24.0f;
	float DeathOffset = KillOffset+KillLength, DeathLength = Race ? 0.0f : 24.0f;
	float ScoreOffset = DeathOffset+DeathLength, ScoreLength = Race ? 83.0f : 35.0f;
	float tw = 0.0f;

	float CountrySpacing = 3.0f;
	int Clamp = 16;
	if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 48)
	{
		LineHeight = 11.0f;
		TeeSizeMod = 0.5f;
		Spacing = 0.0f;
		CountrySpacing = 1.3f;
		Clamp = 32;
	}
	else if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 32)
	{
		LineHeight = 14.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
		CountrySpacing = 1.8f;
		Clamp = 24;
	}
	else if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 16)
	{
		LineHeight = 16.0f;
		TeeSizeMod = 0.9f;
		Spacing = 0.0f;
		CountrySpacing = 2.5f;
	}


	bool NoTitle = pTitle? false : true;

	// count players
	dbg_assert(Team == TEAM_RED || Team == TEAM_BLUE, "Unknown team id");
	int NumPlayers = m_pClient->m_GameInfo.m_aTeamSize[Team];
	int PlayerLines = NumPlayers;
	if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		PlayerLines = max(m_pClient->m_GameInfo.m_aTeamSize[Team^1], PlayerLines);

	// clamp
	if(PlayerLines > Clamp)
		PlayerLines = Clamp;

	char aBuf[128] = {0};

	// background
	Graphics()->BlendNormal();
	vec4 Color;
	if(Team == TEAM_RED && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
		Color = vec4(0.975f, 0.17f, 0.17f, 0.75f);
	else if(Team == TEAM_BLUE)
		Color = vec4(0.17f, 0.46f, 0.975f, 0.75f);
	else
		Color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
	CUIRect Rect = {x, y, w, HeadlineHeight};
	if(upper16 || upper32 || upper24)
		RenderTools()->DrawUIRect(&Rect, Color, CUI::CORNER_R, 5.0f);
	else if(lower16 || lower32 || lower24)
		RenderTools()->DrawUIRect(&Rect, Color, CUI::CORNER_L, 5.0f);
	else
		RenderTools()->DrawRoundRect(&Rect, Color, 5.0f);

	// render title
	if(NoTitle)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
			pTitle = Localize("Round over");
		else
			pTitle = Localize("Scoreboard");
	}
	else
	{
		if(Team == TEAM_BLUE)
			str_format(aBuf, sizeof(aBuf), "(%d) %s", NumPlayers, pTitle);
		else
			str_format(aBuf, sizeof(aBuf), "%s (%d)", pTitle, NumPlayers);
	}

	if(!upper16 && !upper24 && !upper32)
	{
		if(Align == -1)
		{
			tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1, -1.0f);
			TextRender()->Text(0, x+20.0f, y+5.0f, TitleFontsize, pTitle, -1.0f);
			if(!NoTitle)
			{
				str_format(aBuf, sizeof(aBuf), " (%d)", NumPlayers);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
				TextRender()->Text(0, x+20.0f+tw, y+5.0f, TitleFontsize, aBuf, -1.0f);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
		}
		else
		{
			tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1, -1.0f);	
			if(!NoTitle)
			{
				str_format(aBuf, sizeof(aBuf), "(%d) ", NumPlayers);
				float PlayersTextWidth = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1, -1.0f);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
				TextRender()->Text(0, x+w-tw-PlayersTextWidth-20.0f, y+5.0f, TitleFontsize, aBuf, -1.0f);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
			tw = TextRender()->TextWidth(0, TitleFontsize, pTitle, -1, -1.0f);
			TextRender()->Text(0, x+w-tw-20.0f, y+5.0f, TitleFontsize, pTitle, -1.0f);
		}
	}

	if(m_pClient->m_GameInfo.m_aTeamSize[0] <= 16 || (upper16 || upper24 || upper32))
	{
		if(Race)
		{
			if(m_pClient->m_Snap.m_pGameDataRace && Team != TEAM_BLUE)
			{
				float MapRecordFontsize = 16.0f;
				const char *pMapRecordStr = Localize("Map record");
				FormatTime(aBuf, sizeof(aBuf), m_pClient->m_Snap.m_pGameDataRace->m_BestTime, m_pClient->RacePrecision());
				if(Align == -1)
				{
					tw = TextRender()->TextWidth(0, HeadlineFontsize, pMapRecordStr, -1, -1.0f);
					TextRender()->Text(0, x+w-tw-20.0f, y+3.0f, HeadlineFontsize, pMapRecordStr, -1.0f);
					tw = TextRender()->TextWidth(0, MapRecordFontsize, aBuf, -1, -1.0f);
					TextRender()->Text(0, x+w-tw-20.0f, y+HeadlineFontsize+3.0f, MapRecordFontsize, aBuf, -1.0f);
				}
				else
				{
					TextRender()->Text(0, x+20.0f, y+3.0f, HeadlineFontsize, pMapRecordStr, -1.0f);
					TextRender()->Text(0, x+20.0f, y+HeadlineFontsize+3.0f, MapRecordFontsize, aBuf, -1.0f);
				}
			}
		}
		else
		{
			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
			{
				int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue;
				str_format(aBuf, sizeof(aBuf), "%d", Score);
			}
			else
			{
				if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID >= 0 &&
					m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
				{
					int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
					str_format(aBuf, sizeof(aBuf), "%d", Score);
				}
				else if(m_pClient->m_Snap.m_pLocalInfo)
				{
					int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
					str_format(aBuf, sizeof(aBuf), "%d", Score);
				}
			}
			if(Align == -1)
			{
				tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1, -1.0f);
				TextRender()->Text(0, x+w-tw-20.0f, y+5.0f, TitleFontsize, aBuf, -1.0f);
			}
			else
				TextRender()->Text(0, x+20.0f, y+5.0f, TitleFontsize, aBuf, -1.0f);
		}
	}

	// render headlines
	y += HeadlineHeight;

	Graphics()->BlendNormal();
	{
		CUIRect Rect = {x, y, w, LineHeight*(PlayerLines+1)};
		if(upper16 || upper32 || upper24)
			RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_R, 5.0f);
		else if(lower16 || lower32 || lower24)
			RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_L, 5.0f);
		else
			RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);
	}
	if(PlayerLines)
	{
		CUIRect Rect = {x, y+LineHeight, w, LineHeight*(PlayerLines)};
		if(upper16 || upper32 || upper24)
			RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_R, 5.0f);
		else if(lower16 || lower32 || lower24)
			RenderTools()->DrawUIRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_L, 5.0f);
		else
			RenderTools()->DrawRoundRect(&Rect, vec4(0.0f, 0.0f, 0.0f, 0.25f), 5.0f);
	}

	const char *pPingStr = Localize("Ping");
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, pPingStr, -1, -1.0f);
	TextRender()->Text(0, PingOffset+PingLength-tw, y+Spacing, HeadlineFontsize, pPingStr, -1.0f);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->Text(0, NameOffset+ TeeLength, y+Spacing, HeadlineFontsize, Localize("Name"), -1.0f);

	const char *pClanStr = Localize("Clan");
	tw = TextRender()->TextWidth(0, HeadlineFontsize, pClanStr, -1, -1.0f);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y+Spacing, HeadlineFontsize, pClanStr, -1.0f);

	if(!Race)
	{
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.5f);
		tw = TextRender()->TextWidth(0, HeadlineFontsize, "K", -1, -1.0f);
		TextRender()->Text(0, KillOffset+KillLength/2-tw/2, y+Spacing, HeadlineFontsize, "K", -1.0f);

		tw = TextRender()->TextWidth(0, HeadlineFontsize, "D", -1, -1.0f);
		TextRender()->Text(0, DeathOffset+DeathLength/2-tw/2, y+Spacing, HeadlineFontsize, "D", -1.0f);
	}

	const char *pScoreStr = Race ? Localize("Time") : Localize("Score");
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, pScoreStr, -1, -1.0f);
	TextRender()->Text(0, ScoreOffset + (Race ? ScoreLength-tw-3.f : ScoreLength/2-tw/2), y+Spacing, HeadlineFontsize, pScoreStr, -1.0f);

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render player entries
	y += LineHeight;
	float FontSize = HeadlineFontsize;
	CTextCursor Cursor;

	int RenderScoreIDs[MAX_CLIENTS];
	int NumRenderScoreIDs = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
		RenderScoreIDs[i] = -1;

	for (int RenderDead = 0; RenderDead < 2; ++RenderDead)
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			// make sure that we render the correct team
			const CGameClient::CPlayerInfoItem* pInfo = &m_pClient->m_Snap.m_aInfoByScore[i];
			if (!pInfo->m_pPlayerInfo || m_pClient->m_aClients[pInfo->m_ClientID].m_Team != Team || (!RenderDead && (pInfo->m_pPlayerInfo->m_PlayerFlags & PLAYERFLAG_DEAD)) ||
				(RenderDead && !(pInfo->m_pPlayerInfo->m_PlayerFlags & PLAYERFLAG_DEAD)))
				continue;

			RenderScoreIDs[NumRenderScoreIDs] = i;
			NumRenderScoreIDs++;
		}
	}

	int rendered = 0;
	if (upper16)
		rendered = -16;
	if (upper32)
		rendered = -32;
	if (upper24)
		rendered = -24;
	for(int i = 0 ; i < NumRenderScoreIDs ; i++)
	{
		if (rendered++ < 0) continue;
		if(RenderScoreIDs[i] >= 0)
		{
			const CGameClient::CPlayerInfoItem *pInfo = &m_pClient->m_Snap.m_aInfoByScore[RenderScoreIDs[i]];
			bool RenderDead = pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_DEAD;
			float ColorAlpha = RenderDead ? 0.5f : 1.0f;
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, ColorAlpha);

			// color for text
			vec3 TextColor = vec3(1.0f, 1.0f, 1.0f);
			vec4 OutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
			const bool HighlightedLine = m_pClient->m_LocalClientID[Config()->m_ClDummy] == pInfo->m_ClientID || (Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == Snap.m_SpecInfo.m_SpectatorID);

			// background so it's easy to find the local player or the followed one in spectator mode
			if(HighlightedLine)
			{
				CUIRect Rect = {x, y, w, LineHeight};
				if(m_pClient->m_GameInfo.m_aTeamSize[Team] > 32)
					RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.75f*ColorAlpha), 3.0f);
				else
					RenderTools()->DrawRoundRect(&Rect, vec4(1.0f, 1.0f, 1.0f, 0.75f*ColorAlpha), 5.0f);

				// make color for own entry black
				TextColor = vec3(0.0f, 0.0f, 0.0f);
				OutlineColor = vec4(1.0f, 1.0f, 1.0f, 0.25f);
			}
			else
				OutlineColor = vec4(0.0f, 0.0f, 0.0f, 0.3f);

			// set text color
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextOutlineColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// ping
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Latency, 0, 999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
			TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = PingLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);

			// country flag
			const vec4 CFColor(1, 1, 1, 0.75f * ColorAlpha);
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &CFColor,
				CountryFlagOffset, y + CountrySpacing, LineHeight*1.5f, LineHeight*0.75f);

			// custom client recognition
			if(Config()->m_ClClientRecognition)
			{
				int Sprite = m_pClient->GetClientIconSprite(pInfo->m_ClientID);
				if (Sprite != -1)
				{
					Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CLIENTICONS].m_Id);
					Graphics()->QuadsBegin();
					RenderTools()->SelectSprite(Sprite);
					IGraphics::CQuadItem QuadItem(CountryFlagOffset + 30.0f - 10.0f, y, 10, 10);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}

			// flag
			if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_FLAGS && m_pClient->m_Snap.m_pGameDataFlag &&
				(m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierRed == pInfo->m_ClientID ||
				m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue == pInfo->m_ClientID))
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				RenderTools()->SelectSprite(pInfo->m_ClientID == m_pClient->m_Snap.m_pGameDataFlag->m_FlagCarrierBlue ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

				float Size = LineHeight;
				IGraphics::CQuadItem QuadItem(TeeOffset+4.0f, y-2.0f-Spacing/2.0f, Size/2.0f, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// avatar
			if(RenderDead)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_DEADTEE].m_Id);
				Graphics()->QuadsBegin();
				if(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
				{
					vec4 Color = m_pClient->m_pSkins->GetColorV4(m_pClient->m_pSkins->GetTeamColor(true, 0, m_pClient->m_aClients[pInfo->m_ClientID].m_Team, SKINPART_BODY), false);
					Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);
				}
				IGraphics::CQuadItem QuadItem(TeeOffset+TeeLength/2 - 10*TeeSizeMod, y-2.0f+Spacing, 20*TeeSizeMod, 20*TeeSizeMod);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
			else
			{
				CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
				TeeInfo.m_Size = 20*TeeSizeMod;
				RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+LineHeight/2+Spacing));
			}

			// TODO: make an eye icon or something
			if(RenderDead && pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_WATCHING)
				TextRender()->TextColor(1.0f, 1.0f, 0.0f, ColorAlpha);

			// id
			if(Config()->m_ClShowUserId)
			{
				TextRender()->SetCursor(&Cursor, NameOffset+TeeLength-IdSize+Spacing, y+Spacing, FontSize, TEXTFLAG_RENDER);
				RenderTools()->DrawClientID(TextRender(), &Cursor, pInfo->m_ClientID);
			}

			// name
			TextRender()->SetCursor(&Cursor, NameOffset+TeeLength, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			if(pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_ADMIN)
			{
				vec4 Color = m_pClient->m_pSkins->GetColorV4(Config()->m_ClAuthedPlayerColor, false);
				TextRender()->TextColor(Color.r, Color.g, Color.b, Color.a);
			}
			Cursor.m_LineWidth = NameLength-TeeLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, str_length(m_pClient->m_aClients[pInfo->m_ClientID].m_aName));
			// ready / watching
			if(ReadyMode && (pInfo->m_pPlayerInfo->m_PlayerFlags&PLAYERFLAG_READY))
			{
				if(HighlightedLine)
					TextRender()->TextOutlineColor(0.0f, 0.1f, 0.0f, 0.5f);
				TextRender()->TextColor(0.1f, 1.0f, 0.1f, ColorAlpha);
				TextRender()->SetCursor(&Cursor, Cursor.m_X, y+Spacing, FontSize, TEXTFLAG_RENDER);
				TextRender()->TextEx(&Cursor, "\xE2\x9C\x93", str_length("\xE2\x9C\x93"));
			}
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->TextOutlineColor(OutlineColor.r, OutlineColor.g, OutlineColor.b, OutlineColor.a);

			// clan
			tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1, -1.0f);
			TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			if (str_comp(m_pClient->m_aClients[pInfo->m_ClientID].m_aClan,
				m_pClient->m_aClients[GameClient()->m_LocalClientID[Config()->m_ClDummy]].m_aClan) == 0)
			{
				vec4 Color = m_pClient->m_pSkins->GetColorV4(Config()->m_ClSameClanColor, false);
				TextRender()->TextColor(Color.r, Color.g, Color.b, Color.a);
			}
			else
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			Cursor.m_LineWidth = ClanLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			if(!Race)
			{
				// K
				TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, 0.5f*ColorAlpha);
				str_format(aBuf, sizeof(aBuf), "%d", clamp(m_pClient->m_pStats->GetPlayerStats(pInfo->m_ClientID)->m_Frags, 0, 999));
				tw = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
				TextRender()->SetCursor(&Cursor, KillOffset+KillLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER);
				Cursor.m_LineWidth = KillLength;
				TextRender()->TextEx(&Cursor, aBuf, -1);

				// D
				str_format(aBuf, sizeof(aBuf), "%d", clamp(m_pClient->m_pStats->GetPlayerStats(pInfo->m_ClientID)->m_Deaths, 0, 999));
				tw = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
				TextRender()->SetCursor(&Cursor, DeathOffset+DeathLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER);
				Cursor.m_LineWidth = DeathLength;
				TextRender()->TextEx(&Cursor, aBuf, -1);
			}

			// score
			if(Race)
			{
				aBuf[0] = 0;
				if(pInfo->m_pPlayerInfo->m_Score >= 0)
					FormatTime(aBuf, sizeof(aBuf), pInfo->m_pPlayerInfo->m_Score, m_pClient->RacePrecision());
			}
			else
			{
				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_pPlayerInfo->m_Score, -999, 9999));
			}

			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1, -1.0f);
			TextRender()->TextColor(TextColor.r, TextColor.g, TextColor.b, ColorAlpha);
			TextRender()->SetCursor(&Cursor, ScoreOffset + (Race ? ScoreLength-tw-3.f : ScoreLength/2-tw/2), y+Spacing, FontSize, TEXTFLAG_RENDER);
			Cursor.m_LineWidth = ScoreLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			y += LineHeight;
		}

		if(lower32 || upper32)
		{
			if(rendered == 32) break;
		}
		else if (lower24 || upper24)
		{
			if (rendered == 24) break;
		}
		else
		{
			if(rendered == 16) break;
		}
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);

	return HeadlineHeight+LineHeight*(PlayerLines+1);
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	CUIRect RectBox = {x, 0.0f, 180.0f, 50.0f};
	vec4 Color = vec4(0.0f, 0.0f, 0.0f, 0.4f);
	Graphics()->BlendNormal();
	RenderTools()->DrawUIRect(&RectBox, Color, CUI::CORNER_B, 15.0f);

	//draw the red dot
	CUIRect RectRedDot = {x+20, 15.0f, 20.0f, 20.0f};
	Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(&RectRedDot, Color, 10.0f);

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+50.0f, 10.0f, 20.0f, aBuf, -1.0f);
}

void CScoreboard::OnRender()
{
	// don't render scoreboard if menu or statboard is open
	if(m_pClient->m_pMenus->IsActive() || m_pClient->m_pStats->IsActive())
		return;

	// postpone the active state till the render area gets updated during the rendering
	if(m_Activate)
	{
		m_Active = true;
		m_Activate = false;
	}

	// close the motd if we actively wanna look on the scoreboard
	if(m_Active)
		m_pClient->m_pMotd->Clear();

	if(!IsActive())
		return;

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	float Width = Screen.w;
	float y = 85.f;
	float w = 364.0f;
	float FontSize = 86.0f;

	const char* pCustomRedClanName = GetClanName(TEAM_RED);
	const char* pCustomBlueClanName = GetClanName(TEAM_BLUE);
	const char* pRedClanName = pCustomRedClanName ? pCustomRedClanName : Localize("Red team");
	const char* pBlueClanName = pCustomBlueClanName ? pCustomBlueClanName : Localize("Blue team");

	if(m_pClient->m_Snap.m_pGameData)
	{
		if(!(m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS))
		{
			float ScoreboardHeight;
			if(m_pClient->m_GameInfo.m_aTeamSize[0] > 48)
			{
				ScoreboardHeight = RenderScoreboard(Width/2-w, y, w, -4, 0, -1);
				RenderScoreboard(Width/2, y, w, -5, 0, -1);
			}
			else if(m_pClient->m_GameInfo.m_aTeamSize[0] > 32)
			{
				ScoreboardHeight = RenderScoreboard(Width/2-w, y, w, -7, 0, -1);
				RenderScoreboard(Width/2, y, w, -8, 0, -1);
			}
			else if(m_pClient->m_GameInfo.m_aTeamSize[0] > 16)
			{
				ScoreboardHeight = RenderScoreboard(Width/2-w, y, w, -6, 0, -1);
				RenderScoreboard(Width/2, y, w, -3, 0, -1);
			}
			else
			{
				ScoreboardHeight = RenderScoreboard(Width/2-w/2, y, w, 0, 0, -1);
			}

			float SpectatorHeight = RenderSpectators(Width/2-w/2, y+3.0f+ScoreboardHeight, w);
			RenderGoals(Width/2-w/2, y+3.0f+ScoreboardHeight, w);

			// scoreboard size
			m_TotalRect.x = Width/2-w/2;
			m_TotalRect.y = y;
			m_TotalRect.w = w;
			m_TotalRect.h = ScoreboardHeight+SpectatorHeight+3.0f;
		}
		else if(m_pClient->m_Snap.m_pGameDataTeam)
		{
			float ScoreboardHeight = RenderScoreboard(Width/2-w-1.5f, y, w, TEAM_RED, pRedClanName, -1);
			RenderScoreboard(Width/2+1.5f, y, w, TEAM_BLUE, pBlueClanName, 1);

			float SpectatorHeight = RenderSpectators(Width/2-w-1.5f, y+3.0f+ScoreboardHeight, w*2.0f+3.0f);
			RenderGoals(Width/2-w-1.5f, y+3.0f+ScoreboardHeight, w*2.0f+3.0f);

			// scoreboard size
			m_TotalRect.x = Width/2-w-1.5f;
			m_TotalRect.y = y;
			m_TotalRect.w = w*2.0f+3.0f;
			m_TotalRect.h = ScoreboardHeight+SpectatorHeight+3.0f;
		}
	}

	Width = 400*3.0f*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, 400*3.0f);
	if(m_pClient->m_Snap.m_pGameData && (m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS) && m_pClient->m_Snap.m_pGameDataTeam)
	{
		if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
		{
			char aText[256];

			if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue)
				str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
			else if(m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataTeam->m_TeamscoreRed)
				str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
			else
				str_copy(aText, Localize("Draw!"), sizeof(aText));

			float tw = TextRender()->TextWidth(0, FontSize, aText, -1, -1.0f);
			TextRender()->Text(0, Width/2-tw/2, 39, FontSize, aText, -1.0f);
		}
		else if(m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
		{
			char aText[256];
			str_copy(aText, Localize("Round over!"), sizeof(aText));

			float tw = TextRender()->TextWidth(0, FontSize, aText, -1, -1.0f);
			TextRender()->Text(0, Width/2-tw/2, 39, FontSize, aText, -1.0f);
		}
	}

	RenderRecordingNotification((Width/7)*4);
}

bool CScoreboard::IsActive() const
{
	// if we actively wanna look on the scoreboard
	if(m_Active)
		return true;

	if(m_pClient->m_LocalClientID[Config()->m_ClDummy] != -1 && m_pClient->m_aClients[m_pClient->m_LocalClientID[Config()->m_ClDummy]].m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead, don't follow a player and the game isn't paused
		if(!m_pClient->m_Snap.m_pLocalCharacter && !m_pClient->m_Snap.m_SpecInfo.m_Active &&
			!(m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameData && m_pClient->m_Snap.m_pGameData->m_GameStateFlags&(GAMESTATEFLAG_ROUNDOVER|GAMESTATEFLAG_GAMEOVER))
		return true;

	return false;
}

const char *CScoreboard::GetClanName(int Team)
{
	int ClanPlayers = 0;
	const char *pClanName = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_aClients[i].m_Active || m_pClient->m_aClients[i].m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = m_pClient->m_aClients[i].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(m_pClient->m_aClients[i].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return 0;
		}
	}

	if(ClanPlayers > 1 && pClanName[0])
		return pClanName;
	else
		return 0;
}

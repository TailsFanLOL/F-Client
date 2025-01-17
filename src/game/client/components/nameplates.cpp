/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <generated/protocol.h>
#include <generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "nameplates.h"
#include "controls.h"

#include <base/color.h>

void CNamePlates::RenderNameplate(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	int ClientID
	) const
{
	bool Predicted = m_pClient->ShouldUsePredicted() && m_pClient->ShouldUsePredictedChar(ClientID);
	vec2 Position = m_pClient->GetCharPos(ClientID, Predicted);

	RenderNameplatePos(Position, ClientID, pPlayerChar, 1.0f);
}

void CNamePlates::RenderNameplatePos(vec2 Position, int ClientID, const CNetObj_Character *pPlayerChar, float Alpha) const
{
	float FontSize = 18.0f + 20.0f * Config()->m_ClNameplatesSize / 100.0f;
	// render name plate

	float a = Alpha;
	if(Config()->m_ClNameplatesAlways == 0)
		a = clamp(1-powf(distance(m_pClient->m_pControls->m_TargetPos[Config()->m_ClDummy], Position)/200.0f,16.0f), 0.0f, 1.0f);

	char aName[64];
	str_format(aName, sizeof(aName), "%s", Config()->m_ClShowsocial ? m_pClient->m_aClients[ClientID].m_aName: "");

	TextRender()->TextSecondaryColor(0.0f, 0.0f, 0.0f, 0.5f);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	if(Config()->m_ClNameplatesTeamcolors && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_aClients[ClientID].m_Team == TEAM_RED)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, a);
		else if(m_pClient->m_aClients[ClientID].m_Team == TEAM_BLUE)
			TextRender()->TextColor(0.7f, 0.7f, 1.0f, a);
	}

	const vec4 IdTextColor(0.1f, 0.1f, 0.1f, a);
	vec4 BgIdColor(1.0f, 1.0f, 1.0f, a * 0.5f);
	if(Config()->m_ClNameplatesTeamcolors && m_pClient->m_GameInfo.m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_aClients[ClientID].m_Team == TEAM_RED)
			BgIdColor = vec4(1.0f, 0.5f, 0.5f, a * 0.5f);
		else if(m_pClient->m_aClients[ClientID].m_Team == TEAM_BLUE)
			BgIdColor = vec4(0.7f, 0.7f, 1.0f, a * 0.5f);
	}

	if (Config()->m_ClNameplatesTeamcolors && m_pClient->m_Teams.Team(ClientID))
	{
		vec3 rgb = HslToRgb(vec3(m_pClient->m_Teams.Team(ClientID) / 64.0f, 1.0f, 0.75f));
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, a);
		BgIdColor = vec4(rgb.r, rgb.g, rgb.b, a * 0.5f);
	}

	// TODO: cache nameplates
	static CTextCursor s_Cursor;
	s_Cursor.m_FontSize = FontSize;
	s_Cursor.Reset();
	TextRender()->TextDeferred(&s_Cursor, aName, -1);

	float tw = s_Cursor.Width() + (Config()->m_ClShowUserId == 2 ? 0.f : UI()->GetClientIDRectWidth(FontSize));

	if(a > 0.001f)
	{
		vec2 CursorPosition = vec2(Position.x-tw/2.0f, Position.y-FontSize-38.0f);
		if(Config()->m_ClShowUserId != 2)
			CursorPosition.x += UI()->DrawClientID(s_Cursor.m_FontSize, CursorPosition, ClientID, BgIdColor, IdTextColor);
		s_Cursor.MoveTo(CursorPosition.x, CursorPosition.y);
		TextRender()->DrawTextOutlined(&s_Cursor, a);
	}

	TextRender()->TextColor(CUI::ms_DefaultTextColor);
	TextRender()->TextSecondaryColor(CUI::ms_DefaultTextOutlineColor);
}

void CNamePlates::OnRender()
{
	if(!Config()->m_ClNameplates || Client()->State() < IClient::STATE_ONLINE)
		return;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!m_pClient->m_aClients[i].m_Active || m_pClient->m_LocalClientID[Config()->m_ClDummy] == i)
			continue;

		if (m_pClient->m_aClients[i].m_SpecCharPresent)
		{
			RenderNameplatePos(m_pClient->m_aClients[i].m_SpecChar, i, &m_pClient->m_Snap.m_aCharacters[i].m_Cur, 0.4f);
		}

		// only render active characters
		if(m_pClient->m_Snap.m_aCharacters[i].m_Active)
		{
			RenderNameplate(&m_pClient->m_Snap.m_aCharacters[i].m_Prev, &m_pClient->m_Snap.m_aCharacters[i].m_Cur, i);
		}
	}
}

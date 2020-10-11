/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CAMERA_H
#define GAME_CLIENT_COMPONENTS_CAMERA_H
#include <base/vmath.h>
#include <game/bezier.h>
#include <game/client/component.h>

class CCamera : public CComponent
{
public:
	enum
	{
		POS_START = 0,
		POS_INTERNET,
		POS_LAN,
		POS_DEMOS,
		POS_SETTINGS_GENERAL, // order here should be the same like enum for settings pages in menu
		POS_SETTINGS_PLAYER,
		POS_SETTINGS_TBD, // TODO: change removed tee page to something else
		POS_SETTINGS_CONTROLS,
		POS_SETTINGS_GRAPHICS,
		POS_SETTINGS_SOUND,
		POS_SETTINGS_FCLIENT,

		NUM_POS,
	};

	CCamera();
	virtual void OnRender();

	void ChangePosition(int PositionNumber);
	int GetCurrentPosition();
	const vec2 *GetCenter() const { return &m_Center; };
	float GetZoom() const { return m_Zoom; };
	float GetZoomSmoothingTarget() const { return m_ZoomSmoothingTarget; };

	static void ConSetPosition(IConsole::IResult *pResult, void *pUserData);

	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);

	virtual void OnReset();

private:
	enum
	{
		CAMTYPE_UNDEFINED=-1,
		CAMTYPE_SPEC,
		CAMTYPE_PLAYER,
	};

	vec2 m_Center;
	vec2 m_MenuCenter;
	vec2 m_RotationCenter;
	float m_Zoom;
	int m_CamType;
	vec2 m_PrevCenter;
	vec2 m_Positions[NUM_POS];
	int m_CurrentPosition;
	vec2 m_AnimationStartPos;
	float m_MoveTime;

	bool m_ZoomSet;

	bool m_Zooming;
	float m_ZoomSmoothingTarget;
	CCubicBezier m_ZoomSmoothing;
	float m_ZoomSmoothingStart;
	float m_ZoomSmoothingEnd;

	void ScaleZoom(float Factor);
	void ChangeZoom(float Target);
	float ZoomProgress(float CurrentTime) const;

	static void ConZoomPlus(IConsole::IResult *pResult, void *pUserData);
	static void ConZoomMinus(IConsole::IResult *pResult, void *pUserData);
	static void ConZoomReset(IConsole::IResult *pResult, void *pUserData);
};

#endif

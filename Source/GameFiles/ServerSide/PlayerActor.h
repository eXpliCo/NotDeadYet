/*
Made by Ed�nge Simon Datum(30/11/12 created) 
for project desperation* at Blekinge tekniska h�gskola.
*/

#pragma once

#include "GameFiles/ServerSide/BioActor.h"
#include "GameFiles/KeyUtil/KeyValues.h"
#include "GameFiles/KeyUtil/KeyStates.h"

/*This class is used to save player information such as position and states.
  This information is sent to clients.
*/
class PlayerActor : public BioActor
{
public:
	PlayerActor();
	PlayerActor(const int ID);
	PlayerActor(const int ID, const Vector3& startPos);
	PlayerActor(const int ID, const Vector3& startPos, const Vector4& startRot);
	virtual ~PlayerActor();

	/*! Updates players pos, states etc.*/
	void Update(float deltaTime);

	float GetLatency() const {return this->zLatency;}
	inline float GetFrameTime() const {return this->zFrameTime;}
	/*! Gets the current key state. This function is used
		to see which buttons are pressed right now.
		Key is an enum defined in header KeyValues.
	*/
	inline bool GetkeyState(const unsigned int key)
	{return zKeyStates.GetKeyState(key);}

	/* ! Sets key states.
		This one is used to define which buttons are being pressed.
		Key is an enum defined in header KeyValues.
	*/
	inline void SetKeyState(const unsigned int key, const bool value)
	{zKeyStates.SetKeyState(key,value);}

	inline void SetFrameTime(const float frameTime){this->zFrameTime = frameTime;}
	inline void SetLatency(const float latency){this->zLatency = latency;}
	

private:
	float	zLatency;
	float	zFrameTime;

	std::string zPlayerModel;
	KeyStates zKeyStates;
};
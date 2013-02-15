#pragma once

#include "GameMode.h"


class GameModeFFA : public GameMode
{
private:
	//int zKillLimit;
	//std::map<Player*, int> zScoreBoard;
	std::set<Player*> zPlayers;
public:
	GameModeFFA(Game* game);
	virtual ~GameModeFFA();

	bool Update(float dt);

private:
	virtual void OnEvent( Event* e );
	void OnPlayerDeath(PlayerActor* pActor);
};
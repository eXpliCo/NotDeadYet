#include "Game.h"
#include "time.h"
#include "GameModeTest.h"
#include "BioActor.h"
#include "PlayerActor.h"
#include "AnimalActor.h"
#include "GhostActor.h"
#include "GameEvents.h"
#include "PlayerHumanBehavior.h"
#include "Physics.h"

GameModeTest::GameModeTest(Game* game, int killLimit) : GameMode(game)
{
	srand((unsigned int)time(0));
	zKillLimit = killLimit;
}

GameModeTest::~GameModeTest()
{

}

bool GameModeTest::Update( float dt )
{
	for(auto it = zPlayers.begin(); it != zPlayers.end(); it++)
	{
		if(zScoreBoard[(*it)] >= zKillLimit)
		{
			for(auto i = zPlayers.begin(); i != zPlayers.end(); i++)
			{
				MaloW::Debug("Kills: " + MaloW::convertNrToString((float)zScoreBoard[(*i)]));
			}
			return false;
		}
	}
	return true;
}

void GameModeTest::OnEvent( Event* e )
{
	if( BioActorTakeDamageEvent* ATD = dynamic_cast<BioActorTakeDamageEvent*>(e) )
	{
		if( PlayerActor* pActor = dynamic_cast<PlayerActor*>(ATD->zActor) )
		{
			if(pActor->GetHealth() - ATD->zDamage->GetTotal() <= 0)
			{
				// Set new spawn pos
				//int maxPlayers = zPlayers.size();
				//int rand = 1 + (std::rand() % (maxPlayers+1));
				//pa->SetPosition(zGame->CalcPlayerSpawnPoint(rand));
				//pa->SetHealth(pa->GetHealthMax());
				//pa->SetStamina(pa->GetStaminaMax());
				//pa->SetFullness(pa->GetFullnessMax());
				//pa->SetHydration(pa->GetHydrationMax());

				//ATD->zDamage->blunt = 0;
				//ATD->zDamage->fallingDamage = 0;
				//ATD->zDamage->piercing = 0;
				//ATD->zDamage->slashing = 0;

				//Add to scoreboard
				if( PlayerActor* dpa = dynamic_cast<PlayerActor*>(ATD->zDealer) )
					if(dpa != pActor)
						zScoreBoard[dpa->GetPlayer()]++;
					else if( dpa == pActor )
						zScoreBoard[pActor->GetPlayer()]--;

				OnPlayerDeath(pActor);
			}
			else
			{
				unsigned int ID = ATD->zDealer->GetID();
				float damage = ATD->zDamage->GetTotal();

				NetworkMessageConverter NMC;
				std::string msg = NMC.Convert(MESSAGE_TYPE_ACTOR_TAKE_DAMAGE, (float)ID);
				msg += NMC.Convert(MESSAGE_TYPE_HEALTH, damage);
				ClientData* cd = pActor->GetPlayer()->GetClientData();
				cd->Send(msg);
			}
		}
		else if( AnimalActor* aActor = dynamic_cast<AnimalActor*>(ATD->zActor) )
		{
			if(aActor->GetHealth() - ATD->zDamage->GetTotal() <= 0)
			{
				this->zGame->RemoveAIBehavior(aActor);
			}
		}
	}
	else if( PlayerAddEvent* PAE = dynamic_cast<PlayerAddEvent*>(e) )
	{
		this->zScoreBoard[PAE->player] = 0;
		zPlayers.insert(PAE->player);
	}
	else if( PlayerRemoveEvent* PRE = dynamic_cast<PlayerRemoveEvent*>(e) )
	{
		this->zScoreBoard.erase(PRE->player);
		this->zPlayers.erase(PRE->player);
	}
}

void GameModeTest::OnPlayerDeath(PlayerActor* pActor)
{
	NetworkMessageConverter NMC;
	//Kill player on Client
	std::string msg;

	Player* player = pActor->GetPlayer();
	pActor->SetPlayer(NULL);

	ClientData* cd = player->GetClientData();

	//Create new Player
	int maxPlayers = zPlayers.size();
	int rand = 1 + (std::rand() % (maxPlayers+1));
	Vector3 position = this->zGame->CalcPlayerSpawnPoint(rand);

	Vector3 direction = pActor->GetDir();

	PhysicsObject* pObj = GetPhysics()->CreatePhysicsObject(pActor->GetModel(), position);

	PlayerActor* newPActor = new PlayerActor(player, pObj);
	newPActor->SetPosition(position);
	newPActor->SetDir(direction);

	//Create New Human Behavior
	PlayerHumanBehavior* pHumanBehavior = new PlayerHumanBehavior(newPActor, this->zGame->GetWorld(), player);

	this->zGame->SetPlayerBehavior(player, pHumanBehavior);

	//Tell The Client his ID and Actor Type
	ActorManager* aManager = this->zGame->GetActorManager();
	msg = NMC.Convert(MESSAGE_TYPE_SELF_ID, newPActor->GetID());
	msg += NMC.Convert(MESSAGE_TYPE_ACTOR_TYPE, 1);

	cd->Send(msg);

	aManager->AddActor(newPActor);
}
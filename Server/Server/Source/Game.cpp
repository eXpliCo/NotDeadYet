#include "Game.h"
#include <time.h>
#include "GameModeFFA.h"
#include "GameModeTest.h"
#include "Behavior.h"
#include <world/World.h>
#include <World/EntityList.h>
#include <World/Entity.h>
#include "GameEvents.h"
#include "PlayerHumanBehavior.h"
#include "ProjectileArrowBehavior.h"
#include "PlayerWolfBehavior.h"
#include "PlayerActor.h"
#include "ProjectileActor.h"
#include "DeerActor.h"
#include "WolfActor.h"
#include "BearActor.h"
#include "GhostActor.h"
#include "ItemActor.h"
#include "PlayerDeerBehavior.h"
#include "PlayerBearBehavior.h"
#include "AIDeerBehavior.h"
#include "AIBearBehavior.h"
#include "WorldActor.h"
#include "ItemActor.h"
#include "Physics.h"
#include "ClientServerMessages.h"
#include "ItemLookup.h"
#include "PlayerGhostBehavior.h"
#include <Packets\NewActorPacket.h>
#include <Packets\PhysicalConditionPacket.h>
#include "AnimationFileReader.h"
#include "PlayerConfigReader.h"
#include "CraftingManager.h"
#include "MaterialSpawnManager.h"
#include "sounds.h"
#include "SupplyActor.h"

static const float PI = 3.14159265358979323846f;
//Total Degrees for the sun to rotate (160 degrees atm)
static const float TOTAL_SUN_DEGREE_SHIFT = 140 * PI / 180;

//Wait time in Seconds for sun Update. (5s atm)
static const float SUN_UPDATE_DELAY = 0.5f;
//Total Sun Update Time in Seconds (6h atm) 
static const float TOTAL_SUN_UPDATE_TIME = 60.0f * 60.0f * 6.0f;

//Expected playtime
static const float EXPECTED_PLAYTIME = 60.0f * 60.0f * 2.0f;

Game::Game(const int maxClients, PhysicsEngine* physics, ActorSynchronizer* syncher, const std::string& mode, const std::string& worldFile ) :
	zSyncher(syncher),
	zPhysicsEngine(physics),
	zMaterialSpawnManager(0)
{	

	this->zPerf = NULL;
	this->zCameraOffset["media/models/temp_guy_movement_anims.fbx"] = Vector3(0.0f, 1.9f, 0.0f);	
	this->zCameraOffset["media/models/temp_guy.obj"] = Vector3(0.0f, 1.9f, 0.0f);
	this->zCameraOffset["media/models/deer_temp.obj"] = Vector3(0.0f, 1.7f, 0.0f);
	this->zCameraOffset["media/models/ball.obj"] = Vector3(0.0f, 0.0f, 0.0f);
	
// Create World
	if(worldFile != "")
		this->zWorld = new World(this, worldFile.c_str());
	else
		this->zWorld = new World(this, 10, 10);  // Handle Error.

//Create Crafting Manager
	this->zCraftingManager = new CraftingManager();

// Load Entities
	LoadEntList("Entities.txt");

	// Create soundhandler and let it observe game.
	this->zSoundHandler = new SoundHandler();
	this->AddObserver(this->zSoundHandler);

// Actor Manager
	this->zActorManager = new ActorManager(syncher, this->zSoundHandler);
	
	// Material Spawner
	zMaterialSpawnManager = new MaterialSpawnManager(zWorld, zActorManager);

	InitItemLookup();
	InitCraftingRecipes();
//Initialize Player Configuration file
	InitPlayerConfig();

	this->zMaxNrOfPlayers = maxClients;
	
//Create GameMode
	if (mode.find("FFA") == 0 )
	{
		this->zGameMode = new GameModeFFA(this);
	}
	else if (mode.find("TestMode") == 0)
	{
		this->zGameMode = new GameModeTest(this, 10);
	}
	else
	{
		this->zGameMode = new GameModeFFA(this);
	}
// Game Mode Observes
	this->AddObserver(this->zGameMode);

//DEBUG;
	//this->SpawnItemsDebug();
	this->SpawnAnimalsDebug();
	//this->SpawnHumanDebug();
	//this->SpawnItemsDebug();
	//this->SpawnAnimalsDebug();
	//this->SpawnHumanDebug();

//Initialize Sun Direction
	Vector2 mapCenter2D = this->zWorld->GetWorldCenter();

	float radius = mapCenter2D.x;
	float angle = TOTAL_SUN_DEGREE_SHIFT * 0.5f;
	float x = mapCenter2D.x + radius * sin(angle);

	this->zMapCenter = Vector3(mapCenter2D.x, 0.0f, mapCenter2D.y);
	this->zCurrentSunPosition = Vector3(x, 10000.0f, 0.0f);

	this->zCurrentSunDirection =  zMapCenter - this->zCurrentSunPosition;
	this->zCurrentSunDirection.Normalize();

	this->zSunTimer = 0.0f;

	this->zTotalSunRadiansShift = 0.0f;
	this->zSunRadiansShiftPerUpdate = TOTAL_SUN_DEGREE_SHIFT / (SUN_UPDATE_DELAY * TOTAL_SUN_UPDATE_TIME);

	//Fog Enclosement
	this->zPlayersAlive = 0;

	Vector2 worldSize = this->zWorld->GetWorldSize();

	radius = (worldSize.x + worldSize.y) * 0.5f;

	float percent = 0.1f;
	if (worldFile.find("3x3.map"))
		percent = 0.5f;

	this->zInitalFogEnclosement = radius * percent;
	this->zIncrementFogEnclosement = (radius - this->zInitalFogEnclosement) / this->zMaxNrOfPlayers;

	this->zFogUpdateDelay = 30.0f;
	this->zFogDecreaseCoeff = this->zFogUpdateDelay / EXPECTED_PLAYTIME;
	this->zFogTotalDecreaseCoeff = 1.0f;
	this->zFogTimer = 0.0f;

	this->zCurrentFogEnclosement = ( this->zInitalFogEnclosement + (this->zIncrementFogEnclosement * this->zPlayersAlive) ) * this->zFogTotalDecreaseCoeff;

}

Game::~Game()
{
	this->zPerf->PreMeasure("Deleting Game", 3);
	// Delete Players
	for( auto i = this->zPlayers.begin(); i != this->zPlayers.end(); ++i )
	{
		Player* data = i->second;
		if (data)
		{
			delete data;
			data = NULL;
		}
	}
	this->zPlayers.clear();

	// Delete Subsystems
	SAFE_DELETE(zWorld);
	SAFE_DELETE(zActorManager);
	SAFE_DELETE(zGameMode);
	SAFE_DELETE(zSoundHandler);
	SAFE_DELETE(zCraftingManager);
	SAFE_DELETE(zMaterialSpawnManager);

	FreeItemLookup();
	FreePlayerConfig();
	FreeCraftingRecipes();

	this->zPerf->PostMeasure("Deleting Game", 3);
}

void Game::SpawnAnimalsDebug()
{
	srand((unsigned int)time(0));
	int increment = 10;
	Vector3 position = this->CalcPlayerSpawnPoint(increment++);
	PhysicsObject* deerPhysics = GetPhysics()->CreatePhysicsObject("Media/Models/deer_temp.obj");
	DeerActor* dActor = new DeerActor(deerPhysics);
	dActor->AddObserver(this->zGameMode);

	/*Vector3 position2 = this->CalcPlayerSpawnPoint(increment++);
	PhysicsObject* bearPhysics = GetPhysics()->CreatePhysicsObject("Media/Models/deer_temp.obj");
	BearActor* bActor = new BearActor(bearPhysics);
	bActor->AddObserver(this->zGameMode);*/

	AIDeerBehavior* aiDeerBehavior = new AIDeerBehavior(dActor, this->zWorld);
	//AIBearBehavior* aiBearBehavior = new AIBearBehavior(bActor, this->zWorld);

	zActorManager->AddBehavior(aiDeerBehavior);
	//zActorManager->AddBehavior(aiBearBehavior);

	dActor->SetPosition(position);
	dActor->SetScale(Vector3(0.05f, 0.05f, 0.05f));

	//bActor->SetPosition(position2);
	//bActor->SetScale(Vector3(0.08f, 0.08f, 0.08f));

	//const Food* temp_Bear_food = GetItemLookup()->GetFood(ITEM_SUB_TYPE_WOLF_FOOD);
	
	int lootSize = (rand() % 5) + 1;
	Food* new_Food = NULL;

	Inventory* inv;// = bActor->GetInventory();
	bool stacked = false;
	/*if (temp_Bear_food)
	{
		for (int i = 0; i < lootSize; i++)
		{
			new_Food = new Food((*temp_Bear_food));

			inv->AddItem(new_Food, stacked);
			if( stacked && new_Food->GetStackSize() == 0 )
				SAFE_DELETE(new_Food);
		}
	}*/

	const Food* temp_Deer_Food = GetItemLookup()->GetFood(ITEM_SUB_TYPE_DEER_FOOD);

	lootSize = (rand() % 7) + 1;
	new_Food = NULL;
	inv = dActor->GetInventory();
	stacked = false;
	if (temp_Deer_Food)
	{
		for (int i = 0; i < lootSize; i++)
		{
			new_Food = new Food((*temp_Deer_Food));

			inv->AddItem(new_Food, stacked);
			if( stacked && new_Food->GetStackSize() == 0 )
				SAFE_DELETE(new_Food);
		}
	}
	
	this->zActorManager->AddActor(dActor);
	//this->zActorManager->AddActor(bActor);
}

void Game::SpawnItemsDebug()
{
	//ITEMS
	const Food*			temp_food		= GetItemLookup()->GetFood(ITEM_SUB_TYPE_DEER_FOOD);
	const RangedWeapon* temp_R_weapon	= GetItemLookup()->GetRangedWeapon(ITEM_SUB_TYPE_BOW);
	const Projectile*	temp_Arrow		= GetItemLookup()->GetProjectile(ITEM_SUB_TYPE_ARROW);
	const MeleeWeapon*	temp_M_weapon	= GetItemLookup()->GetMeleeWeapon(ITEM_SUB_TYPE_MACHETE);
	const Material*		temp_material_S	= GetItemLookup()->GetMaterial(ITEM_SUB_TYPE_SMALL_STICK);
	const Material*		temp_material_M	= GetItemLookup()->GetMaterial(ITEM_SUB_TYPE_MEDIUM_STICK);
	const Material*		temp_material_T	= GetItemLookup()->GetMaterial(ITEM_SUB_TYPE_THREAD);
	const Material*		temp_material_L	= GetItemLookup()->GetMaterial(ITEM_SUB_TYPE_LARGE_STICK);
	const Material*		temp_material_D	= GetItemLookup()->GetMaterial(ITEM_SUB_TYPE_DISENFECTANT_LEAF);
	const Bandage*		temp_bandage	= GetItemLookup()->GetBandage(ITEM_SUB_TYPE_BANDAGE_POOR);
	const Bandage*		temp_bandage_G	= GetItemLookup()->GetBandage(ITEM_SUB_TYPE_BANDAGE_GREAT);
	const Container*	temp_waterBottle= GetItemLookup()->GetContainer(ITEM_SUB_TYPE_CANTEEN);
	const Misc*			temp_Trap		= GetItemLookup()->GetMisc(ITEM_SUB_TYPE_REGULAR_TRAP);

	unsigned int increment = 0;
	int maxPoints = 10;
	float radius = 3.5f;
	int numberOfObjects = 12;
	int total = 0;
	Vector3 center;
	Vector3 position;
	Vector2 tempCenter = this->zWorld->GetWorldCenter();
	for (int i = 0; i < maxPoints; i++)
	{
		center = Vector3(tempCenter.x, 0, tempCenter.y);
		int currentPoint = i % maxPoints;

		center = this->CalcPlayerSpawnPoint(currentPoint, maxPoints, 17.0f, center);

		//Food
		if (temp_food)
		{
			Food* new_Item = new Food((*temp_food));
			ItemActor* actor = new ItemActor(new_Item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Weapon_ranged
		if(temp_R_weapon)
		{
			RangedWeapon* new_item = new RangedWeapon((*temp_R_weapon));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Arrows
		if(temp_Arrow)
		{
			Projectile* new_item = new Projectile((*temp_Arrow));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Melee_weapon
		if(temp_M_weapon)
		{
			MeleeWeapon* new_item = new MeleeWeapon((*temp_M_weapon));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Small_stick
		if(temp_material_S)
		{
			Material* new_item = new Material((*temp_material_S));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Medium_stick
		if(temp_material_M)
		{
			Material* new_item = new Material((*temp_material_M));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Large Stick
		if(temp_material_L)
		{
			Material* new_item = new Material((*temp_material_L));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Disenfectant Leaf
		if(temp_material_D)
		{
			Material* new_item = new Material((*temp_material_D));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Thread
		if(temp_material_T)
		{
			Material* new_item = new Material((*temp_material_T));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		//Bandage
		if(temp_bandage)
		{
			Bandage* new_item = new Bandage((*temp_bandage));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}
		if(temp_bandage_G)
		{
			Bandage* new_item = new Bandage((*temp_bandage_G));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}

		if(temp_Trap)
		{
			Misc* new_item = new Misc((*temp_Trap));
			ItemActor* actor = new ItemActor(new_item);
			//center = CalcPlayerSpawnPoint(increment++);
			position = this->CalcPlayerSpawnPoint(increment++, numberOfObjects, radius, center);
			actor->SetPosition(position);
			actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
			this->zActorManager->AddActor(actor);
		}

		total += increment;
		increment = 0;
	}
	Container* new_item = new Container((*temp_waterBottle));
	new_item->SetRemainingUses(3);
	ItemActor* actor = new ItemActor(new_item);
	//center = CalcPlayerSpawnPoint(increment++);
	Vector2 tempVetc = this->zWorld->GetWorldCenter();
	position = Vector3(tempVetc.x, 0, tempVetc.y);
	actor->SetPosition(position);
	actor->SetScale(Vector3(0.05f, 0.05f, 0.05f));
	this->zActorManager->AddActor(actor);
}

void Game::SpawnHumanDebug()
{
	srand((unsigned int)time(0));
	int increment = 10;
	Vector3 position = this->CalcPlayerSpawnPoint(increment++);
	PhysicsObject* humanPhysics = GetPhysics()->CreatePhysicsObject("Media/Models/temp_guy.obj");
	PlayerActor* pActor = new PlayerActor(NULL, humanPhysics, this);
	pActor->SetModel("Media/Models/temp_guy_movement_anims.fbx");
	pActor->AddObserver(this->zGameMode);
	pActor->SetPosition(position);
	pActor->SetHealth(5000);
	pActor->SetScale(pActor->GetScale());
	this->zActorManager->AddActor(pActor);
}

void Game::UpdateSunDirection(float dt)
{
	//Update Sun
	this->zSunTimer += dt;

	if (this->zSunTimer >= SUN_UPDATE_DELAY)
	{
		Vector2 worldCenter = this->zWorld->GetWorldCenter();
		float radius = worldCenter.x;
		float angle = TOTAL_SUN_DEGREE_SHIFT * 0.5f + this->zTotalSunRadiansShift;
		float x = worldCenter.x + radius * sin(angle);

		this->zTotalSunRadiansShift += this->zSunRadiansShiftPerUpdate;

		if (this->zTotalSunRadiansShift >= TOTAL_SUN_DEGREE_SHIFT)
			this->zTotalSunRadiansShift = 0.0f;

		this->zCurrentSunPosition.x = x;

		this->zCurrentSunDirection = this->zMapCenter - this->zCurrentSunPosition;
		this->zCurrentSunDirection.Normalize();

		NetworkMessageConverter NMC;
		std::string msg = NMC.Convert(MESSAGE_TYPE_SUN_DIRECTION, this->zCurrentSunDirection);

		this->SendToAll(msg);

		this->zSunTimer = 0.0f;
	}
}

void Game::UpdateFogEnclosement( float dt )
{
	this->zFogTimer += dt;

	if (this->zFogTimer >= this->zFogUpdateDelay)
	{
		if (this->zCurrentFogEnclosement > 10.0f)
		{
			this->zFogTotalDecreaseCoeff -= this->zFogDecreaseCoeff;

			this->zCurrentFogEnclosement = (this->zInitalFogEnclosement + (this->zIncrementFogEnclosement * this->zPlayersAlive) ) * this->zFogTotalDecreaseCoeff;
			
			NetworkMessageConverter NMC;
			this->SendToAll(NMC.Convert(MESSAGE_TYPE_FOG_ENCLOSEMENT, this->zCurrentFogEnclosement));
		}
		this->zFogTimer = 0.0f;
	}
}

bool Game::Update( float dt )
{
	this->UpdateSunDirection(dt);
	this->UpdateFogEnclosement(dt);
	NetworkMessageConverter NMC;
	std::string msg;

	this->zPerf->PreMeasure("Updating Behaviors", 1);
	std::set<Behavior*> &behaviors = this->zActorManager->GetBehaviors();

	// Update Behaviors
	auto i = behaviors.begin();
	int counter = 0;
	while( i != behaviors.end() )
	{
		if (!(*i)->Removed())
		{
			if( PlayerBehavior* playerBehavior = dynamic_cast<PlayerBehavior*>((*i)) )
			{
				playerBehavior->RefreshNearCollideableActors(zActorManager->GetCollideableActors());
			}
			else if( ProjectileArrowBehavior* projectileArrowBehavior = dynamic_cast<ProjectileArrowBehavior*>(*i) )
			{
				projectileArrowBehavior->RefreshNearCollideableActors(zActorManager->GetCollideableActors());
			}

			if ( (*i)->IsAwake() && (*i)->Update(dt) )
			{
				Behavior* temp = (*i);
				Actor* oldActor = NULL;
				ItemActor* newActor = ConvertToItemActor(temp, oldActor);

				if( oldActor )
				{
					PhysicsObject* pObject = oldActor->GetPhysicsObject();
					if (pObject)
					{
						this->zPhysicsEngine->DeletePhysicsObject(pObject);
						oldActor->SetPhysicsObject(NULL);
					}

					this->zActorManager->RemoveActor(oldActor);
				}

				this->zActorManager->AddActor(newActor);

				i = behaviors.erase(i);
				this->zActorManager->RemoveBehavior(temp);

			}
			else
			{
				i++;
				counter++;
			}
		}
		else
		{
			Behavior* temp = (*i);
			i = behaviors.erase(i);
			this->zActorManager->RemoveBehavior(temp);
		}
		
	}
	this->zPerf->PostMeasure("Updating Behaviors", 1);

	this->zPerf->PreMeasure("Updating GameMode", 4);
	// Update Game Mode, Might Notify That GameMode is Finished
	if ( this->zGameMode->Update(dt) )
		return false;
	this->zPerf->PostMeasure("Updating GameMode", 4);

	this->zPerf->PreMeasure("Updating World", 4);
	// Update World
	this->zWorld->Update();
	this->zPerf->PostMeasure("Updating World", 4);

	//Updating animals and Check fog.
	static float testUpdater = 0.0f;

	testUpdater += dt;

	if(testUpdater > 4.0f)
	{
		this->zPerf->PreMeasure("Updating animal targets", 2);
		//Creating targets to insert into the animals' behaviors
		std::set<Actor*> aSet;

		for(i = behaviors.begin(); i != behaviors.end(); i++)
		{
			if(dynamic_cast<BioActor*>((*i)->GetActor()))
			{
				aSet.insert( (*i)->GetActor());
			}
		}

		//Updating animals' targets and Check if Players Are in Fog.
		for(i = behaviors.begin(); i != behaviors.end(); i++)
		{
			if(AIBehavior* animalBehavior = dynamic_cast<AIBehavior*>( (*i) ))
			{
				animalBehavior->SetTargets(aSet);
			}
			else if (PlayerBehavior* playerBehavior = dynamic_cast<PlayerBehavior*>( (*i) ))
			{
				if (BioActor* bActor = dynamic_cast<BioActor*>( (*i)->GetActor() ))
				{
					Vector2 center = this->zWorld->GetWorldCenter();

					float radiusFromCenter = (Vector3(center.x, 0.0f, center.y) - bActor->GetPosition()).GetLength();

					if (radiusFromCenter > this->zCurrentFogEnclosement)
					{
						Damage dmg;
						dmg.fogDamage = 1.0f * dt;
						bActor->TakeDamage(dmg, bActor);
					}
				}
			}
		}
		testUpdater = 0.0f;
		this->zPerf->PostMeasure("Updating animal targets", 2);
	}

	// Game Still Active
	return true;
}

void Game::OnEvent( Event* e )
{
	// TODO: Incoming Message
	if (this->zPerf)
		this->zPerf->PreMeasure("Game Event Handling", 2);

	if ( PlayerConnectedEvent* PCE = dynamic_cast<PlayerConnectedEvent*>(e) )
	{
		this->zPerf->PreMeasure("Player Connecting", 2);
		this->HandleConnection(PCE->clientData);
	}
	else if( UserReadyEvent* URE = dynamic_cast<UserReadyEvent*>(e) )
	{
		PlayerReadyEvent playerReady;
		playerReady.player = this->zPlayers[URE->clientData];
		
		NotifyObservers(&playerReady);
	}
	else if( KeyDownEvent* KDE = dynamic_cast<KeyDownEvent*>(e) )
	{
		zPlayers[KDE->clientData]->GetKeys().SetKeyState(KDE->key, true);
	}
	else if( KeyUpEvent* KUE = dynamic_cast<KeyUpEvent*>(e) )
	{
		zPlayers[KUE->clientData]->GetKeys().SetKeyState(KUE->key, false);
	}
	else if ( PlayerSwapEquippedWeaponsEvent* SEQWE = dynamic_cast<PlayerSwapEquippedWeaponsEvent*>(e) )
	{
		Actor* actor = this->zPlayers[SEQWE->clientdData]->GetBehavior()->GetActor();

		if(BioActor* pActor = dynamic_cast<BioActor*>(actor))
		{
			Inventory* inv = pActor->GetInventory();
			if(inv->SwapWeapon())
			{
				//NetworkMessageConverter NMC;
				//std::string msg;

				//Item* item = inv->GetSecondaryEquip();

				//msg = NMC.Convert(MESSAGE_TYPE_MESH_UNBIND, (float)pActor->GetID());
				//msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, item->GetModel());
				//SEQWE->clientdData->Send(msg);

				//Item* newPrimary = inv->GetPrimaryEquip();

				//if (newPrimary)
				//	this->HandleBindings(newPrimary, pActor->GetID());
			}
		}
	}
	else if( ClientDataEvent* CDE = dynamic_cast<ClientDataEvent*>(e) )
	{
		Player* player = zPlayers[CDE->clientData];
		if (player)
			if( PlayerBehavior* dCastBehavior = dynamic_cast<PlayerBehavior*>(player->GetBehavior()))
				dCastBehavior->ProcessClientData(CDE->direction, CDE->rotation);
	}
	else if ( PlayerDisconnectedEvent* PDCE = dynamic_cast<PlayerDisconnectedEvent*>(e) )
	{
		this->HandleDisconnect(PDCE->clientData);
	}
	else if(PlayerLootObjectEvent* PLOE = dynamic_cast<PlayerLootObjectEvent*>(e))
	{
		this->HandleLootObject(PLOE->clientData, PLOE->actorID);
	}
	else if ( PlayerLootItemEvent* PLIE = dynamic_cast<PlayerLootItemEvent*>(e) )
	{
		this->zPerf->PreMeasure("Loot Event Handling", 3);
		this->HandleLootItem(PLIE->clientData, PLIE->itemID, PLIE->itemType, PLIE->objID, PLIE->subType);
		this->zPerf->PostMeasure("Loot Event Handling", 3);	
	}
	else if ( PlayerDropItemEvent* PDIE = dynamic_cast<PlayerDropItemEvent*>(e) )
	{
		this->HandleDropItem(PDIE->clientData, PDIE->itemID);
	}
	else if (PlayerUseItemEvent* PUIE = dynamic_cast<PlayerUseItemEvent*>(e))
	{
		this->zPerf->PreMeasure("Use Event Handling", 3);
		this->HandleUseItem(PUIE->clientData, PUIE->itemID);
		this->zPerf->PostMeasure("Use Event Handling", 3);
	}
	else if (PlayerCraftItemEvent* PCIE = dynamic_cast<PlayerCraftItemEvent*>(e))
	{
		this->zPerf->PreMeasure("Craft Event Handling", 3);
		this->HandleCraftItem(PCIE->clientData, PCIE->craftedItemType, PCIE->craftedItemSubType);
		this->zPerf->PostMeasure("Craft Event Handling", 3);
	}
	else if (PlayerFillItemEvent* PFIE = dynamic_cast<PlayerFillItemEvent*>(e))
	{
		this->HandleFillItem(PFIE->clientData, PFIE->itemID);
	}
	else if ( PlayerUseEquippedWeaponEvent* PUEWE = dynamic_cast<PlayerUseEquippedWeaponEvent*>(e) )
	{
		this->zPerf->PreMeasure("Weapon Use Event Handling", 3);
		this->HandleUseWeapon(PUEWE->clientData, PUEWE->itemID);
		this->zPerf->PostMeasure("Weapon Use Event Handling", 3);
	}
	else if(PlayerAnimalAttackEvent* PAAE = dynamic_cast<PlayerAnimalAttackEvent*>(e))
	{
		auto playerIterator = this->zPlayers.find(PAAE->clientData);
		Player* player = playerIterator->second;
		auto it = playerIterator->second->GetBehavior();
		Actor* self = it->GetActor();

		float range = 0.0f;
		Damage damage;
		
		if(BearActor* bActor = dynamic_cast<BearActor*>(self))
		{
			if(player)
			{
				if(PAAE->mouseButton == MOUSE_LEFT_PRESS)
				{
					range = 2.2f;
					damage.blunt = 10.0f;
				}
				else if(PAAE->mouseButton == MOUSE_RIGHT_PRESS)
				{
					range = 1.5f;
					damage.piercing = 5.0f;
					damage.slashing = 25.0f;
				}
				if(Actor* target = this->zActorManager->CheckCollisions(self, range))
				{
					BioActor* bActor = dynamic_cast<BioActor*>(target);
					if(bActor->IsAlive())
					{
						bActor->TakeDamage(damage,self);
					}
				}
			}
		}

	}
	else if (PlayerEquipItemEvent* PEIE = dynamic_cast<PlayerEquipItemEvent*>(e) )
	{
		this->zPerf->PreMeasure("Equip Event Handling", 3);
		this->HandleEquipItem(PEIE->clientData, PEIE->itemID);
		this->zPerf->PostMeasure("Equip Event Handling", 3);
	}
	else if (PlayerUnEquipItemEvent* PUEIE = dynamic_cast<PlayerUnEquipItemEvent*>(e) )
	{
		this->zPerf->PreMeasure("UnEquip Event Handling", 3);
		this->HandleUnEquipItem(PUEIE->clientData, PUEIE->itemID);
		this->zPerf->PostMeasure("UnEquip Event Handling", 3);
	}
	else if(PlayerAnimalSwapEvent* PASE = dynamic_cast<PlayerAnimalSwapEvent*>(e))
	{
		auto playerIterator = this->zPlayers.find(PASE->clientData);
		Player* player = playerIterator->second;

		if (player)
		{
			Behavior* playerBehavior = player->GetBehavior();
			if (playerBehavior)
			{
				PASE->zActor = playerBehavior->GetActor();
			}
		}
	}
	else if(PlayerAnimalPossessEvent* POSSESSE = dynamic_cast<PlayerAnimalPossessEvent*>(e))
	{
		auto playerIterator = this->zPlayers.find(POSSESSE->clientData);
		Player* player = playerIterator->second;

		if (player)
		{
			Behavior* playerBehavior = player->GetBehavior();
			if (playerBehavior)
			{
				POSSESSE->zActor = playerBehavior->GetActor();
			}
		}
	}
	else if (PlayerLeaveAnimalEvent* PLAE = dynamic_cast<PlayerLeaveAnimalEvent*>(e))
	{
		auto playerIterator = this->zPlayers.find(PLAE->clientData);

		Player* player = playerIterator->second;
		NetworkMessageConverter NMC;
		std::string msg;
		if (player)
		{
			Behavior* playerBehavior = player->GetBehavior();
			if (playerBehavior)
			{
				Actor* actor = playerBehavior->GetActor();

				if(DeerActor* dActor = dynamic_cast<DeerActor*>(actor))
				{
					dActor->SetPlayer(NULL);
					AIDeerBehavior* behavior = new AIDeerBehavior(dActor, this->zWorld);

					this->zActorManager->AddBehavior(behavior);

					//Create Ghost behavior And Ghost Actor
					GhostActor* gActor = new GhostActor(player);
					gActor->SetPosition(dActor->GetPosition());

					PlayerGhostBehavior* playerGhostBehavior = new PlayerGhostBehavior(gActor, this->zWorld, player);

					this->SetPlayerBehavior(player, playerGhostBehavior);

					//Tell Client his new ID and actor type
					msg = NMC.Convert(MESSAGE_TYPE_SELF_ID, (float)gActor->GetID());
					msg += NMC.Convert(MESSAGE_TYPE_ACTOR_TYPE, 2);
					PLAE->clientData->Send(msg);

					//Add the actor to the list
					this->zActorManager->AddActor(gActor);

					NetworkMessageConverter NMC;
					std::string msg = NMC.Convert(MESSAGE_TYPE_SUN_DIRECTION, this->zCurrentSunDirection);
				}
			}
		}
	}
	else if (PlayerDeerEatObjectEvent* PDEOE = dynamic_cast<PlayerDeerEatObjectEvent*>(e))
	{
		//ID's of the Object deer is trying to eat
		auto playerIterator = this->zPlayers.find(PDEOE->clientData);
		auto playerBehavior = playerIterator->second->GetBehavior();
		Actor* actor = playerBehavior->GetActor();

		NetworkMessageConverter NMC;
		std::string msg = NMC.Convert(MESSAGE_TYPE_LOOT_OBJECT_RESPONSE);
		unsigned int ID = 0;
		bool bEaten = false;

		std::set<Actor*> actors = this->zActorManager->GetActors();

		DeerActor* thePlayerActor;

		ItemActor* toBeRemoved = NULL;

		Damage idiotDamage;

		thePlayerActor = dynamic_cast<DeerActor*>(actor);
		if(thePlayerActor)
		{
			auto it_Id_begin = PDEOE->actorID.begin();
			auto it_Id_End = PDEOE->actorID.end();
			auto it_actors_end = actors.end();
			for (auto it_actor = actors.begin(); it_actor != it_actors_end && !bEaten; it_actor++)
			{
				if (dynamic_cast<WorldActor*>(*it_actor))
					continue;

				//Loop through all ID's of all actors the client tried to loot.
				for (auto it_ID = it_Id_begin; it_ID != it_Id_End; it_ID++)
				{
					//Check if the ID is the same.
					if ((*it_ID) == (*it_actor)->GetID())
					{
						//Check if the distance between the actors are too far to be able to loot.
						if ((actor->GetPosition() - (*it_actor)->GetPosition()).GetLength() > 5.0f)
							continue;

						//Check if the Actor is an ItemActor
						if (ItemActor* iActor = dynamic_cast<ItemActor*>(*it_actor))
						{
							ID = iActor->GetID();
							Item* theItem = iActor->GetItem();
							if(theItem->GetItemType() == ITEM_TYPE_WEAPON_MELEE && theItem->GetItemSubType() == ITEM_SUB_TYPE_MACHETE)
							{
								idiotDamage.piercing = 25.0f;
						
								thePlayerActor->TakeDamage(idiotDamage,actor);
							}
							else if(theItem->GetItemType() == ITEM_TYPE_WEAPON_MELEE && theItem->GetItemSubType() == ITEM_SUB_TYPE_POCKET_KNIFE)
							{
								Damage idiotDamage;
								idiotDamage.piercing = 10.0f;
						
								thePlayerActor->TakeDamage(idiotDamage,actor);
							}
							else if(theItem->GetItemType() == ITEM_TYPE_PROJECTILE && theItem->GetItemSubType() == ITEM_SUB_TYPE_ROCK)
							{
								Damage idiotDamage;
								idiotDamage.piercing = 5.0f;
						
								thePlayerActor->TakeDamage(idiotDamage,actor);
							}
							else if(theItem->GetItemType() == ITEM_TYPE_PROJECTILE && theItem->GetItemSubType() == ITEM_SUB_TYPE_ARROW)
							{
								Damage idiotDamage;
								idiotDamage.piercing = 5.0f;

								thePlayerActor->TakeDamage(idiotDamage,actor);
							}
							toBeRemoved = iActor;
							bEaten = true;
						}
					}
				}
			}
		}

		if(bEaten)
			this->zActorManager->RemoveActor(toBeRemoved);
	}
	else if ( EntityUpdatedEvent* EUE = dynamic_cast<EntityUpdatedEvent*>(e) )
	{
		//auto i = zWorldActors.find(EUE->entity);
		//if ( i != zWorldActors.end() )
		//{
		//	i->second->SetPosition(EUE->entity->GetPosition());
		//	// TODO: Rotation
		//	i->second->SetScale(EUE->entity->GetScale());
		//}
	}
	else if ( EntityLoadedEvent* ELE = dynamic_cast<EntityLoadedEvent*>(e) )
	{
		PhysicsObject* phys = NULL;
		float blockRadius = GetEntBlockRadius(ELE->entity->GetType());
		if ( blockRadius > 0.0f )
		{
			// Create Physics Object
			phys = zPhysicsEngine->CreatePhysicsObject(GetEntModel(ELE->entity->GetType()));
		}
		WorldActor* actor = new WorldActor(phys, blockRadius);
		actor->SetPosition(ELE->entity->GetPosition());
		actor->SetScale(actor->GetScale());
		actor->AddObserver(this->zGameMode);
		
		this->zWorldActors[ELE->entity] = actor;
		this->zActorManager->AddActor(actor);
		
	}
	else if ( EntityRemovedEvent* ERE = dynamic_cast<EntityRemovedEvent*>(e) )
	{
		auto i = zWorldActors.find(ERE->entity);
		if ( i != zWorldActors.end() )
		{
			PhysicsObject* Pobj = i->second->GetPhysicsObject();
			zPhysicsEngine->DeletePhysicsObject(Pobj);
			i->second->SetPhysicsObject(NULL);
			this->zActorManager->RemoveActor(i->second);
			this->zWorldActors.erase(i);
		}
	}
	else if ( UserDataEvent* UDE = dynamic_cast<UserDataEvent*>(e) )
	{
		// Filter Player Models
		static const std::string defaultModel = "media/models/temp_guy_movement_anims.fbx";
		const std::string* selectedModel = &defaultModel;

		if ( UDE->playerModel == "media/models/temp_guy_movement_anims.fbx" )
		{
			selectedModel = &UDE->playerModel;
		}

		// Create Player Actor
		PhysicsObject* pObj = this->zPhysicsEngine->CreatePhysicsObject("Media/Models/temp_guy.obj");
		
		PlayerActor* pActor = new PlayerActor(zPlayers[UDE->clientData], pObj, this);
		pActor->SetModel(*selectedModel);
		zPlayers[UDE->clientData]->zUserName = UDE->playerName;
		zPlayers[UDE->clientData]->zUserModel = *selectedModel;

		
		pActor->AddObserver(this->zGameMode);
		Vector3 center;

		// Start Position
		center = this->CalcPlayerSpawnPoint(32, zWorld->GetWorldCenter());
		pActor->SetPosition(center, false);
		pActor->SetScale(pActor->GetScale(), false);
		Inventory* inv = pActor->GetInventory();
		inv->AddObserver(this);
		inv->SetPlayer(zPlayers[UDE->clientData]);

		auto offsets = this->zCameraOffset.find(*selectedModel);
		
		if(offsets != this->zCameraOffset.end())
			pActor->SetCameraOffset(offsets->second);

		// Apply Default Player Behavior
		SetPlayerBehavior(zPlayers[UDE->clientData], new PlayerHumanBehavior(pActor, zWorld, zPlayers[UDE->clientData]));

		//Add actor
		zActorManager->AddActor(pActor);

		//Tells the client which Actor he owns.
		std::string message;
		NetworkMessageConverter NMC;
		unsigned int selfID;

		selfID = pActor->GetID();
		message = NMC.Convert(MESSAGE_TYPE_SELF_ID, (float)selfID);
		message += NMC.Convert(MESSAGE_TYPE_ACTOR_TYPE, (float)1);

		UDE->clientData->Send(message);

		NewActorPacket* NAP = new NewActorPacket();
		PhysicalConditionPacket* PCP = new PhysicalConditionPacket();

		//Gather Actor Physical Conditions
		PCP->zBleedingLevel = pActor->GetBleeding();
		PCP->zEnergy = pActor->GetEnergy();
		PCP->zHealth = pActor->GetHealth();
		PCP->zHunger = pActor->GetFullness();
		PCP->zHydration = pActor->GetHydration();
		PCP->zStamina = pActor->GetStamina();

		UDE->clientData->Send(*PCP);
		delete PCP;

		//Gather Actors Information and send to client
		std::set<Actor*>& actors = this->zActorManager->GetActors();
		auto it_actors_end = actors.end();
		for (auto it = actors.begin(); it != it_actors_end; it++)
		{
			if(pActor == (*it))
				continue;

			if(dynamic_cast<WorldActor*>(*it))
				continue;

			NAP->actorPosition[(*it)->GetID()] = (*it)->GetPosition();
			NAP->actorRotation[(*it)->GetID()] = (*it)->GetRotation();
			NAP->actorScale[(*it)->GetID()] = (*it)->GetScale();
			NAP->actorModel[(*it)->GetID()] = (*it)->GetModel();

			if (BioActor* bActor = dynamic_cast<BioActor*>( (*it) ))
				NAP->actorState[bActor->GetID()] = bActor->GetState();
		}
		UDE->clientData->Send(*NAP);
		delete NAP;

		this->zPlayersAlive++;
		this->zCurrentFogEnclosement = this->zInitalFogEnclosement + ( this->zIncrementFogEnclosement * this->zPlayersAlive);

		message = NMC.Convert(MESSAGE_TYPE_FOG_ENCLOSEMENT, this->zCurrentFogEnclosement);
		this->SendToAll(message);

		this->zPerf->PostMeasure("Player Connecting", 2);
	}
	else if ( WorldLoadedEvent* WLE = dynamic_cast<WorldLoadedEvent*>(e) )
	{

	}
	else if ( PlayerKillEvent* PKE = dynamic_cast<PlayerKillEvent*>(e) )
	{
		ClientData* cd = PKE->clientData;

		Actor* actor = this->zPlayers[cd]->GetBehavior()->GetActor();

		BioActor* bActor = dynamic_cast<BioActor*>(actor);

		if (bActor)
			bActor->Kill();
	}
	else if (InventoryAddItemEvent* IAIE = dynamic_cast<InventoryAddItemEvent*>(e))
	{
		NetworkMessageConverter NMC;
		std::string msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);
		msg += IAIE->item->ToMessageString(&NMC);

		IAIE->cd->Send(msg);
	}
	else if (InventoryRemoveItemEvent* IRIE = dynamic_cast<InventoryRemoveItemEvent*>(e))
	{
		NetworkMessageConverter NMC;
		std::string msg = NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)IRIE->ID);

		IRIE->cd->Send(msg);
	}
	else if (InventoryEquipItemEvent* IEIE = dynamic_cast<InventoryEquipItemEvent*>(e))
	{
		NetworkMessageConverter NMC;

		std::string msg = NMC.Convert(MESSAGE_TYPE_EQUIP_ITEM, (float)IEIE->id);
		msg += NMC.Convert(MESSAGE_TYPE_EQUIPMENT_SLOT, (float)IEIE->slot);
		IEIE->cd->Send(msg);
	}
	else if (InventoryUnEquipItemEvent* IUIE = dynamic_cast<InventoryUnEquipItemEvent*>(e))
	{
		NetworkMessageConverter NMC;

		std::string msg = NMC.Convert(MESSAGE_TYPE_UNEQUIP_ITEM, (float)IUIE->id);
		msg += NMC.Convert(MESSAGE_TYPE_EQUIPMENT_SLOT, (float)IUIE->slot);
		IUIE->cd->Send(msg);
	}
	else if (InventoryBindPrimaryWeapon* IBPW = dynamic_cast<InventoryBindPrimaryWeapon*>(e))
	{
		this->HandleBindings(IBPW->ID, IBPW->model, IBPW->type, IBPW->subType);
	}
	else if (InventoryUnBindPrimaryWeapon* IUBPW = dynamic_cast<InventoryUnBindPrimaryWeapon*>(e))
	{
		NetworkMessageConverter NMC;
		std::string msg;

		msg = NMC.Convert(MESSAGE_TYPE_MESH_UNBIND, (float)IUBPW->ID);
		msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, IUBPW->model);
		this->SendToAll(msg);
	}
	NotifyObservers(e);
	if (this->zPerf)
		this->zPerf->PostMeasure("Game Event Handling", 2);
}

void Game::SetPlayerBehavior( Player* player, PlayerBehavior* behavior )
{
	// Find Old Behavior
	Behavior* curPlayerBehavior = player->GetBehavior();

	// Find In Behaviors
	if ( curPlayerBehavior )
	{
		curPlayerBehavior->Remove();
		//this->zActorManager->RemoveBehavior(curPlayerBehavior);
		//curPlayerBehavior = NULL;
	}

	// Set New Behavior
	if ( behavior )	
	{
		this->zActorManager->AddBehavior(behavior);
	}

	player->zBehavior = behavior;
}

Vector3 Game::CalcPlayerSpawnPoint(int currentPoint, int maxPoints, float radius, Vector3 center)
{
	float slice  = 2 * PI / maxPoints;

	float angle = slice * currentPoint;

	float x = center.x + radius * cos(angle);
	float z = center.z + radius * sin(angle);
	float y = 0.0f;

	if ( x >= 0.0f && y >= 0.0f && x<zWorld->GetWorldSize().x && y<zWorld->GetWorldSize().y )
	{
		y = this->zWorld->CalcHeightAtWorldPos(Vector2(x, z));
	}

	return Vector3(x, y, z);
}

Vector3 Game::CalcPlayerSpawnPoint(int maxPoints, Vector2 center)
{
	int point = this->zPlayers.size();

	static const float radius = 20.0f;
	float slice  = 2 * PI / maxPoints;

	float angle = slice * point;

	float x = center.x + radius * cos(angle);
	float z = center.y + radius * sin(angle);
	float y = 0.0f;

	if ( x >= 0.0f && z >= 0.0f && x < zWorld->GetWorldSize().x && z < zWorld->GetWorldSize().y )
	{
		y = this->zWorld->CalcHeightAtWorldPos(Vector2(x, z));
	}

	return Vector3(x, y, z);
}

Vector3 Game::CalcPlayerSpawnPoint(int nr)
{
	int point = nr;

	static const float radius = 20.0f;
	float slice  = 2 * PI / this->zMaxNrOfPlayers;

	float angle = slice * point;

	float x = zWorld->GetWorldCenter().x + radius * cos(angle);
	float z = zWorld->GetWorldCenter().y + radius * sin(angle);
	float y = 0.0f;

	if ( x >= 0.0f && z >= 0.0f && x < zWorld->GetWorldSize().x && z < zWorld->GetWorldSize().y )
	{
		y = this->zWorld->CalcHeightAtWorldPos(Vector2(x, z));
	}

	return Vector3(x, y, z);
}

ItemActor* Game::ConvertToItemActor(Behavior* behavior, Actor*& oldActorOut)
{
	int itemType = 0;
	ProjectileArrowBehavior* projBehavior = dynamic_cast<ProjectileArrowBehavior*>(behavior);

	//Check what kind of projectile
	if(projBehavior)
		itemType = ITEM_SUB_TYPE_ARROW;
	//else if(false) //Else if stone
	//	itemType = ITEM_SUB_TYPE_ROCK;
	else
		return NULL;
	
	//Fetch the actor
	ProjectileActor* projActor = dynamic_cast<ProjectileActor*>(projBehavior->GetActor());

	if(!projActor)
		return NULL;

	//Get the item based on type
	const Projectile* item = GetItemLookup()->GetProjectile(itemType);

	if(!item)
		return NULL;
	Projectile* projectile = new Projectile((*item));
	projectile->SetStackSize(1);

	ItemActor* itemActor = new ItemActor(projectile);
	itemActor->SetPosition(projActor->GetPosition(), false);
	itemActor->SetRotation(projActor->GetRotation(), false);
	itemActor->SetScale(projActor->GetScale(), false);
	itemActor->SetDir(projActor->GetDir(), false);
	oldActorOut = projActor;

	return itemActor;
}

void Game::HandleConnection( ClientData* cd )
{
	// Create Player
	Player* player = new Player(cd);
	zPlayers[cd] = player;

	//Lets gamemode observe players.
	player->AddObserver(this->zGameMode);

	//Tells the client it has been connected.
	NetworkMessageConverter NMC;
	std::string message;
	
	message = NMC.Convert(MESSAGE_TYPE_CONNECTED);
	cd->Send(message);

	// Sends the world name
	message = NMC.Convert(MESSAGE_TYPE_LOAD_MAP, zWorld->GetFileName());
	cd->Send(message);

	//Send event to game so it knows what players there are.
	PlayerAddEvent PAE;
	PAE.player = player;
	NotifyObservers(&PAE);
}

void Game::HandleDisconnect( ClientData* cd )
{
	// Delete Player Behavior
	auto playerIterator = zPlayers.find(cd);
	auto playerBehavior = playerIterator->second->GetBehavior();
		
	// Create AI Behavior For Players That Disconnected
	if ( PlayerDeerBehavior* playerDeer = dynamic_cast<PlayerDeerBehavior*>(playerBehavior) )
	{
		AIDeerBehavior* aiDeer = new AIDeerBehavior(playerDeer->GetActor(), zWorld);
		this->zActorManager->AddBehavior(aiDeer);
	}
	else if ( PlayerBearBehavior* playerBear = dynamic_cast<PlayerBearBehavior*>(playerBehavior) )
	{
		AIBearBehavior* aiDeer = new AIBearBehavior(playerBear->GetActor(), zWorld);
		this->zActorManager->AddBehavior(aiDeer);
	}
	//Kills actor if human
	else if ( PlayerHumanBehavior* pHuman = dynamic_cast<PlayerHumanBehavior*>(playerBehavior))
	{
		Actor* pActor = pHuman->GetActor();
		dynamic_cast<BioActor*>(pActor)->Kill();
	}
		
	this->SetPlayerBehavior(playerIterator->second, NULL);

	PlayerRemoveEvent PRE;
	PRE.player = playerIterator->second;
	NotifyObservers(&PRE);

	Player* temp = playerIterator->second;
	delete temp;
	temp = NULL;
	zPlayers.erase(playerIterator);
}

void Game::HandleLootObject( ClientData* cd, std::vector<unsigned int>& actorID )
{
	std::set<Actor*> actors = this->zActorManager->GetLootableActors();
	std::vector<Item*> lootedItems;

	auto playerIterator = this->zPlayers.find(cd);
	auto playerBehavior = playerIterator->second->GetBehavior();
	Actor* actor = playerBehavior->GetActor();
	NetworkMessageConverter NMC;
	std::string msg;
	unsigned int ID = 0;
	bool bLooted = false;
	//Loop through all actors.
	auto it_actors_end = actors.end();
	auto it_actorID_begin = actorID.begin();
	auto it_actorID_end = actorID.end();

	for (auto it_actor = actors.begin(); it_actor != it_actors_end && !bLooted; it_actor++)
	{
		if ((*it_actor)->GetID() == actor->GetID())
			continue;

		//Loop through all ID's of all actors the client tried to loot.
		for (auto it_ID = it_actorID_begin; it_ID != it_actorID_end && !bLooted; it_ID++)
		{
			//Check if the ID is the same.
			if ((*it_ID) == (*it_actor)->GetID())
			{
				//Check if the distance between the actors are to far to be able to loot.
				if ((actor->GetPosition() - (*it_actor)->GetPosition()).GetLength() > 7.0f)
					continue;

				//Check if the Actor is an ItemActor
				if (ItemActor* iActor = dynamic_cast<ItemActor*>(*it_actor))
				{
					msg = NMC.Convert(MESSAGE_TYPE_LOOT_OBJECT_RESPONSE, (float)iActor->GetID());
					msg += iActor->GetItem()->ToMessageString(&NMC);
					msg += NMC.Convert(MESSAGE_TYPE_ITEM_FINISHED);
					bLooted = true;
				}
				//Check if the Actor is an SupplyActor
				else if(SupplyActor* sActor = dynamic_cast<SupplyActor*>(*it_actor))
				{
					Inventory* inv = sActor->GetInventory();
					ID = sActor->GetID();

					std::vector<Item*> items = inv->GetItems();
					if (items.size() > 0)
					{
						msg = NMC.Convert(MESSAGE_TYPE_LOOT_OBJECT_RESPONSE, (float)sActor->GetID());
						auto it_items_end = items.end();
						for (auto it_Item = items.begin(); it_Item != it_items_end; it_Item++)
						{
							msg += (*it_Item)->ToMessageString(&NMC);
							msg += NMC.Convert(MESSAGE_TYPE_ITEM_FINISHED);
						}
						bLooted = true;
					}
				}
				//Check if the Actor is a PlayerActor
				else if(PlayerActor* pActor = dynamic_cast<PlayerActor*>(*it_actor))
				{
					//Check if the PlayerActor is Dead.
					if (!pActor->IsAlive())
					{
						Inventory* inv = pActor->GetInventory();
						ID = pActor->GetID();

						std::vector<Item*> items = inv->GetItems();
						if (items.size() > 0)
						{
							msg = NMC.Convert(MESSAGE_TYPE_LOOT_OBJECT_RESPONSE, (float)pActor->GetID());
							auto it_items_end = items.end();
							for (auto it_Item = items.begin(); it_Item != it_items_end; it_Item++)
							{
								msg += (*it_Item)->ToMessageString(&NMC);
								msg += NMC.Convert(MESSAGE_TYPE_ITEM_FINISHED);
							}
							bLooted = true;
						}
						
					}
				}
				//Check if the Actor is an AnimalActor.
				else if(AnimalActor* aActor = dynamic_cast<AnimalActor*>(*it_actor))
				{
					//Check if the AnimalActor is alive.
					if (!aActor->IsAlive())
					{
						//Verify that a PlayerActor was trying to loot.
						if (PlayerActor* player = dynamic_cast<PlayerActor*>(actor))
						{
							//Get the PlayerActors inventory.
							if (Inventory* playerInventory = player->GetInventory())
							{
								//Check if the PlayerActor has a Pocket Knife to be able to loot Animals.
								Item* item = playerInventory->SearchAndGetItemFromType(ITEM_TYPE_WEAPON_MELEE, ITEM_SUB_TYPE_POCKET_KNIFE);

								if (item)
								{
									Inventory* inv = aActor->GetInventory();
									ID = aActor->GetID();

									std::vector<Item*> items = inv->GetItems();
									if (items.size() > 0)
									{
										msg = NMC.Convert(MESSAGE_TYPE_LOOT_OBJECT_RESPONSE, (float)aActor->GetID());
										auto it_items_end = items.end();
										for (auto it_Item = items.begin(); it_Item != it_items_end; it_Item++)
										{
											msg += (*it_Item)->ToMessageString(&NMC);
											msg += NMC.Convert(MESSAGE_TYPE_ITEM_FINISHED, (float)ID);
										}
										bLooted = true;
									}
									
								}
							}
						}
					}
				}
			}
		}
	}
	if (bLooted)
		cd->Send(msg);
	else
		cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "No Lootable Objects Found."));
}

void Game::HandleLootItem(ClientData* cd, unsigned int itemID, unsigned int itemType, unsigned int objID, unsigned int subType )
{
	Actor* actor = this->zActorManager->GetActor(objID);
	NetworkMessageConverter NMC;
	Item* item = NULL;
	bool stacked = false;
	bool itemScattered = false;
	auto playerActor = this->zPlayers.find(cd);
	auto pBehaviour = playerActor->second->GetBehavior();

	PlayerActor* pActor = dynamic_cast<PlayerActor*>(pBehaviour->GetActor());
	
	if(!pActor)
	{
		cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "You Are Dead Loot Failed"));
		return;
	}

	//Check if the Actor being looted is an ItemActor.
	if (ItemActor* iActor = dynamic_cast<ItemActor*>(actor))
	{
		item = iActor->GetItem();

		if (!item)
			return;

		//std::string msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);

		if (item->GetID() == itemID && item->GetItemType() == itemType)// && item->GetItemSubType() == subType)
		{
			if( item->GetStacking() && !pActor->GetInventory()->IsStacking(item) )
			{
				int slots = pActor->GetInventory()->CalcMaxAvailableSlots(item);

				if(item->GetStackSize() > (unsigned int)slots)
				{
					Item* new_item = NULL;

					if( Projectile* projItem  = dynamic_cast<Projectile*>(item) )
						new_item = new Projectile((*projItem));
					else if( Material* matItem = dynamic_cast<Material*>(item) )
						new_item = new Material((*matItem));
					else if( Food* foodItem = dynamic_cast<Food*>(item) )
						new_item = new Food((*foodItem));
					else if( Bandage* bandageItem = dynamic_cast<Bandage*>(item) )
						new_item = new Bandage((*bandageItem));
					else if( Misc* miscItem = dynamic_cast<Misc*>(item) )
						new_item = new Misc((*miscItem));

					if(new_item)
					{
					 	//To generate an ID. For now.
					 	Projectile projectile;
					 	new_item->SetItemID( projectile.GetID() );
					 	new_item->SetStackSize(slots);
					 	item->DecreaseStackSize(slots);

					 	item = new_item;
						itemScattered = true;
					}
					else
					{
						return;
					}
				}
			}

			//msg += item->ToMessageString(&NMC);

			//add item
			if(pActor->GetInventory()->AddItem(item, stacked))
			{
				if( stacked && item->GetStackSize() == 0 )
				{
					iActor->RemoveItem();
					this->zActorManager->RemoveActor(iActor);

					SAFE_DELETE(item);
				}
				else if ( !stacked && !itemScattered )
				{
					iActor->RemoveItem();
					this->zActorManager->RemoveActor(iActor);
				}

			}
			else
			{
				cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Inventory Is Full"));
				return;
			}

			//cd->Send(msg);
		}
		
	}

	//Check if the Actor being looted is a BioActor.
	else if (BioActor* bActor = dynamic_cast<BioActor*>(actor))
	{
		Inventory* inv = bActor->GetInventory();
		if (inv)
		{
			item = inv->SearchAndGetItem(itemID);

			if( !item )
				return;
		
			//std::string msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);
			if (item->GetItemType() == itemType)// && item->GetItemSubType() == subType)
			{
				//msg += item->ToMessageString(&NMC);
				//Add item
				if(pActor->GetInventory()->AddItem(item, stacked))
				{
					bActor->GetInventory()->RemoveItem(item);

					if( stacked && item->GetStackSize() == 0 )
					{
						SAFE_DELETE(item);
					}

					if (bActor->GetInventory()->GetItems().size() <= 0)
						this->zActorManager->RemoveActor(bActor);
				}
				else
				{
					cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Inventory is Full"));
					return;
				}

				//cd->Send(msg);
			}
		}
	}

	//Check if the Actor being looted is a SupplyActor.
	else if (SupplyActor* supplyActor = dynamic_cast<SupplyActor*>(actor))
	{
		Inventory* inv = supplyActor->GetInventory();
		if (inv)
		{
			item = inv->SearchAndGetItem(itemID);

			if( !item )
				return;

			std::string msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);
			if (item->GetItemType() == itemType)// && item->GetItemSubType() == subType)
			{
				msg += item->ToMessageString(&NMC);
				//Add item
				if(pActor->GetInventory()->AddItem(item, stacked))
				{
					supplyActor->GetInventory()->RemoveItem(item);

					if( stacked && item->GetStackSize() == 0 )
					{
						SAFE_DELETE(item);
					}

					if (supplyActor->GetInventory()->GetItems().size() <= 0)
						this->zActorManager->RemoveActor(supplyActor);
				}
				else
				{
					cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Inventory is Full"));
					return;
				}

				cd->Send(msg);
			}
		}
	}
}

void Game::HandleDropItem(ClientData* cd, unsigned int objectID)
{
	NetworkMessageConverter NMC;
	std::string msg;

	Actor* actor  = NULL;
	PlayerActor* pActor = NULL;
	Item* item = NULL;

	auto i = this->zPlayers.find(cd);

	actor = i->second->GetBehavior()->GetActor();
	pActor = dynamic_cast<PlayerActor*>(actor);

	if(!pActor)
		return;

	Inventory* inventory = pActor->GetInventory();
	
	item = inventory->SearchAndGetItem(objectID);

	if(!item)
	{
		MaloW::Debug("Failed Item=NULL ID: " + MaloW::convertNrToString((float)objectID));
		return;
	}

	RangedWeapon* rwp = inventory->GetRangedWeapon();
	MeleeWeapon* mwp = inventory->GetMeleeWeapon();
	Projectile* proj = inventory->GetProjectile();

	if(rwp && dynamic_cast<RangedWeapon*>(item) == rwp)
		inventory->UnEquipRangedWeapon();
	
	else if(mwp && dynamic_cast<MeleeWeapon*>(item) == mwp)
		inventory->UnEquipMeleeWeapon();
	
	else if(proj && dynamic_cast<Projectile*>(item) == proj)
		inventory->UnEquipProjectile();

	item = inventory->RemoveItem(item);

	if (item && Messages::FileWrite())	
		Messages::Debug("Removed successes: " + MaloW::convertNrToString((float)objectID));
	
	//if (wasPrimary)
	//{
	//	msg = NMC.Convert(MESSAGE_TYPE_MESH_UNBIND, (float)pActor->GetID());
	//	msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, item->GetModel());
	//	cd->Send(msg);

	//	Item* newPrimary = inventory->GetPrimaryEquip();

	//	if (newPrimary)
	//		this->HandleBindings(newPrimary, pActor->GetID());
	//}

	if(!item)
		return;

	actor = NULL;
	actor = new ItemActor(item);
	actor->SetPosition(pActor->GetPosition(), false);
	this->zActorManager->AddActor(actor);

	//NetworkMessageConverter NMC;
	//cd->Send(NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)item->GetID()));
}

void Game::HandleUseItem(ClientData* cd, unsigned int itemID)
{
	auto playerIterator = this->zPlayers.find(cd);
	auto playerBehavior = playerIterator->second->GetBehavior();

	Actor* actor = playerBehavior->GetActor();

	//Check if the actor trying to use an item is a PlayerActor
	if(PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor))
	{
		//Check if he has an inventory
		if (Inventory* inv = pActor->GetInventory())
		{
			NetworkMessageConverter NMC;
			Item* item = inv->SearchAndGetItem(itemID);
			std::string msg;
			if (item)
			{
				unsigned int ID = 0;
				if (Food* food = dynamic_cast<Food*>(item))
				{
					ID = food->GetID();
					if (food->Use())
					{
						//To do fix Values and stuff.
						float value = food->GetHunger();

						float fullness = pActor->GetFullness();

						pActor->SetFullness(fullness + value);

						//Sending Message to client And removing stack from inventory.
						inv->RemoveItemStack(ID, 1);
						msg = NMC.Convert(MESSAGE_TYPE_ITEM_USE, (float)ID);

						cd->Send(msg);
					}
					else
					{
						msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Food_Stack_is_Empty");
						cd->Send(msg);
					}
					if (food->GetStackSize() <= 0)
					{
						item = inv->RemoveItem(food);

						if(item)
						{
							//msg = NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)ID);
							//cd->Send(msg);
							
							delete item, item = NULL;
						}
					}
				}
				else if (Container* container = dynamic_cast<Container*>(item))
				{
			
					if (container->Use())
					{
						//To do fix values and stuff
						float hydration = 2.0f + pActor->GetHydration();

						pActor->SetHydration(hydration);
						ID = container->GetID();
						msg = NMC.Convert(MESSAGE_TYPE_ITEM_USE, (float)ID);
						cd->Send(msg);
					}
					else
					{
						msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Container_Stack_is_Empty");
						cd->Send(msg);
					}
				}
				else if(Bandage* bandage = dynamic_cast<Bandage*>(item))
				{
					ID = bandage->GetID();
					
					if (bandage->Use())
					{				
						float bleedingLevel = pActor->GetBleeding();
						if(bandage->GetItemSubType() == 0) //Used poor bandage
						{
							pActor->SetBleeding(bleedingLevel - 1);
						}
						else if(bandage->GetItemSubType() == 1) //Used great bandage
						{
							pActor->SetBleeding(bleedingLevel - 3);
						}
						//pActor->SetBleeding(false);
						
						//Sending Message to client And removing stack from inventory.
						inv->RemoveItemStack(ID, 1);
						msg = NMC.Convert(MESSAGE_TYPE_ITEM_USE, (float)ID);

						cd->Send(msg);
					}
					else
					{
						msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Bandage_Stack_is_Empty");
						cd->Send(msg);
					}
					if (bandage->GetStackSize() <= 0)
					{
						item = inv->RemoveItem(bandage);

						if(item)
						{
							//msg = NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)ID);
							//cd->Send(msg);
							delete item, item = NULL;
						}
					}
				}
			}
		}
	}
}

void Game::HandleUseWeapon(ClientData* cd, unsigned int itemID)
{
	Actor* actor = NULL;

	auto playerIterator = zPlayers.find(cd);
	actor = playerIterator->second->GetBehavior()->GetActor();

	PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor);
	if ( !(pActor) )
	{
		MaloW::Debug("Actor cannot be found in Game.cpp, onEvent, PlayerUseEquippedWeaponEvent.");
		return;
	}

	Inventory* inventory = pActor->GetInventory();
	if( !(inventory) )
	{
		MaloW::Debug("Inventory is null in Game.cpp, onEvent, PlayerUseEquippedWeaponEvent.");
		return;
	}

	Item* item = inventory->GetPrimaryEquip();
	if ( !(item ) )
	{
		MaloW::Debug("Item is null in Game.cpp, onEvent, PlayerUseEquippedWeaponEvent.");
		return;
	}
	if( item->GetID() != itemID )
	{
		MaloW::Debug("Item ID do not match in Game.cpp, onEvent, PlayerUseEquippedWeaponEvent.");
		return;
	}
	NetworkMessageConverter NMC;

	if(RangedWeapon* ranged = dynamic_cast<RangedWeapon*>(item))
	{
		if (ranged->GetItemSubType() == ITEM_SUB_TYPE_BOW)
		{
			//Check if arrows are equipped
			Projectile* arrow = inventory->GetProjectile();
			if(arrow && arrow->GetItemSubType() == ITEM_SUB_TYPE_ARROW)
			{
				//create projectileActor
				PhysicsObject* pObj = this->zPhysicsEngine->CreatePhysicsObject(arrow->GetModel());
				ProjectileActor* projActor = new ProjectileActor(pActor, pObj);
		
				ProjectileArrowBehavior* projBehavior = NULL;
				Damage damage;

				//Sets damage
				damage.piercing = ranged->GetDamage() + arrow->GetDamage();
				projActor->SetDamage(damage);
				//Set other values
				projActor->SetScale(projActor->GetScale(), false);
				projActor->SetPosition( pActor->GetPosition() + pActor->GetCameraOffset(), false);
				projActor->SetDir(pActor->GetDir(), false);

				//Create behavior
				projBehavior = new ProjectileArrowBehavior(projActor, this->zWorld);
				projBehavior->AddObserver(this->zSoundHandler);

				//Set Nearby actors
				projBehavior->SetNearBioActors( dynamic_cast<PlayerBehavior*>(zPlayers[cd]->GetBehavior())->GetNearBioActors() );
				projBehavior->SetNearWorldActors( dynamic_cast<PlayerBehavior*>(zPlayers[cd]->GetBehavior())->GetNearWorldActors() );
				
				//Adds the actor and Behavior
				this->zActorManager->AddActor(projActor);
				this->zActorManager->AddBehavior(projBehavior);
				//Decrease stack
				arrow->Use();
				inventory->RemoveItemStack(arrow->GetID(), 1);

				//if arrow stack is empty
				if (arrow->GetStackSize() <= 0)
				{
					//std::string msg = NMC.Convert(MESSAGE_TYPE_REMOVE_EQUIPMENT, (float)arrow->GetID());
					//msg += NMC.Convert(MESSAGE_TYPE_EQUIPMENT_SLOT, (float)EQUIPMENT_SLOT_PROJECTILE);
					//cd->Send(msg);
					inventory->UnEquipProjectile();
					item = inventory->RemoveItem(arrow);
					
					SAFE_DELETE(item);
				}
				//Send feedback message
				cd->Send(NMC.Convert(MESSAGE_TYPE_WEAPON_USE, (float)ranged->GetID()));

				std::string msg = NMC.Convert(MESSAGE_TYPE_PLAY_SOUND, EVENTID_NOTDEADYET_BOW_BOWSHOT);
				msg += NMC.Convert(MESSAGE_TYPE_POSITION, pActor->GetPosition());
				this->SendToAll(msg);

				pActor->SetState(STATE_ATTACK_BOW);
			}
			else
				cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "No_Arrows_Equipped"));
		}
	}
	else if(Projectile* proj = dynamic_cast<Projectile*>(item))
	{
		//TODO: Implement rocks
	}
	else if(MeleeWeapon* meele = dynamic_cast<MeleeWeapon*>(item))
	{
		float range = 0.0f; 
		BioActor* victim = NULL;
		PlayerBehavior* pBehavior = dynamic_cast<PlayerBehavior*>(playerIterator->second->GetBehavior());

		if( !pBehavior )
		{
			MaloW::Debug("In Game, OnEvent, HandleWeaponUse: pBehavior is null.");
			return;
		}

		//Check Collisions
		range = meele->GetRange();
		victim = dynamic_cast<BioActor*>( zActorManager->CheckCollisions( actor, range, pBehavior->GetNearBioActors() ) );
		
		if(victim)
		{
			Damage dmg;

			if(meele->GetItemSubType() == ITEM_SUB_TYPE_MACHETE)
				dmg.slashing = meele->GetDamage();

			victim->TakeDamage(dmg, pActor);

			pActor->SetState(STATE_ATTACK_MACHETE);
		}
	}
}

void Game::HandleCraftItem(ClientData* cd, const unsigned int itemType, const unsigned int itemSubType)
{
	auto playerIterator = this->zPlayers.find(cd);
	auto playerBehavior = playerIterator->second->GetBehavior();

	Actor* actor = playerBehavior->GetActor();

	if(PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor))
	{
		if (Inventory* inv = pActor->GetInventory())
		{
			NetworkMessageConverter NMC;
			std::string msg;
			
			CraftedTypes craftedType = CraftedTypes();
			craftedType.type = itemType;
			craftedType.subType = itemSubType;
			//Items used for crafting and the required stacks.
			std::map<Item*, unsigned int> item_stack_out;

			//Check if there are enough materials to Craft.
			if(this->zCraftingManager->Craft(inv, &craftedType, item_stack_out))
			{
				Item* craftedItem = NULL;
				if (craftedType.type == ITEM_TYPE_PROJECTILE && craftedType.subType == ITEM_SUB_TYPE_ARROW)
				{
					const Projectile* temp_Item = GetItemLookup()->GetProjectile(craftedType.subType);
					craftedItem = new Projectile((*temp_Item));
				}
				else if (craftedType.type == ITEM_TYPE_WEAPON_RANGED && craftedType.subType == ITEM_SUB_TYPE_BOW)
				{
					const RangedWeapon* temp_Item = GetItemLookup()->GetRangedWeapon(craftedType.subType);
					craftedItem = new RangedWeapon((*temp_Item));
				}
				else if (craftedType.type == ITEM_TYPE_BANDAGE && craftedType.subType == ITEM_SUB_TYPE_BANDAGE_POOR)
				{
					const Bandage* temp_Item = GetItemLookup()->GetBandage(craftedType.subType);
					craftedItem = new Bandage((*temp_Item));
				}
				else if (craftedType.type == ITEM_TYPE_MISC && craftedType.subType == ITEM_SUB_TYPE_REGULAR_TRAP)
				{
					const Misc* temp_Item = GetItemLookup()->GetMisc(craftedType.subType);
					craftedItem = new Misc((*temp_Item));
				}
				else if (craftedType.type == ITEM_TYPE_MISC && craftedType.subType == ITEM_SUB_TYPE_CAMPFIRE)
				{
					const Misc* temp_Item = GetItemLookup()->GetMisc(craftedType.subType);
					craftedItem = new Misc((*temp_Item));
				}
				if (craftedItem)
				{
					int newWeightChange = craftedItem->GetStackSize() * craftedItem->GetWeight();
					auto item_it_end = item_stack_out.end();
					for (auto it = item_stack_out.begin(); it != item_it_end; it++)
					{
						newWeightChange -= (it->first->GetWeight() * it->second);
					}

					//Check if the new Weight is less or equal to the max Weight.
					if(inv->GetTotalWeight() + newWeightChange <= inv->GetInventoryCapacity())
					{
						for (auto it = item_stack_out.begin(); it != item_it_end; it++)
						{
							//Decrease stacks for the Material.
							it->first->DecreaseStackSize(it->second);
							//Check if material should be removed or just remove stack.
							if (it->first->GetStackSize() > 0)
							{
								inv->RemoveItemStack(it->first->GetID(), it->second);
							}
							else
							{
								inv->RemoveItem(it->first);
								//cd->Send(NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)it->first->GetID()));
							}
						}
						//Send Add Inventory Msg to the Player.
						//std::string add_msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);
						//add_msg += craftedItem->ToMessageString(&NMC);

						//Try to add the crafted item to the inventory.
						bool stacked = false;
						if (inv->AddItem(craftedItem, stacked))
						{					
							if (stacked)
							{
								if (craftedItem->GetStackSize() <= 0)
									SAFE_DELETE(craftedItem);
							}

							//Loop through items again and send Craft msg
							for (auto it = item_stack_out.begin(); it != item_it_end; it++)
							{
								msg = NMC.Convert(MESSAGE_TYPE_ITEM_CRAFT, (float)it->first->GetID());
								msg += NMC.Convert(MESSAGE_TYPE_ITEM_STACK_SIZE, (float)it->second);
								cd->Send(msg);
								if (it->first->GetStackSize() <= 0)
								{
									Item* temp = it->first;
									SAFE_DELETE(temp);
								}
							}

							//cd->Send(add_msg);
						}
						else
						{
							SAFE_DELETE(craftedItem);
							for (auto it = item_stack_out.begin(); it != item_it_end; it++)
							{
								inv->RemoveItem(it->first);
								//cd->Send(NMC.Convert(MESSAGE_TYPE_REMOVE_INVENTORY_ITEM, (float)it->first->GetID()));
								it->first->IncreaseStackSize(it->second);
								if(inv->AddItem(it->first, stacked))
								{
									//std::string msg = NMC.Convert(MESSAGE_TYPE_ADD_INVENTORY_ITEM);
									//msg += craftedItem->ToMessageString(&NMC);
									//cd->Send(msg);
									if (stacked)
									{
										MaloW::Debug("Weird Error When Crafting, item stacked but shouldn't have");
										Item* temp = it->first;
										SAFE_DELETE(temp);
									}
								}
								else
									MaloW::Debug("Weird Error When Crafting, item Can't be re added to inventory");
										
							}
							cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Not enough space in inventory"));
						}
					}
					else
					{
						SAFE_DELETE(craftedItem);
						cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Not enough space in inventory"));
					}
				}
			}
			else
			{
				cd->Send(NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Not enough materials to craft"));
			}
		}
	}
}

void Game::HandleFillItem( ClientData* cd, const unsigned int itemID )
{
	Actor* actor = this->zPlayers[cd]->GetBehavior()->GetActor();
	PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor);

	if (!pActor)
		return;

	Item* item = pActor->GetInventory()->SearchAndGetItem(itemID);

	if (!item)
	{
		MaloW::Debug("Failed to find item in Game::HandleFillItem");
		return;
	}

	//Logic for filling container here.
	Vector2 position = Vector2(pActor->GetPosition().x, pActor->GetPosition().z);
	float depth = this->zWorld->GetWaterDepthAt(position);
	if(depth > 0.3f)
	{
		dynamic_cast<Container*>(item)->SetRemainingUses(dynamic_cast<Container*>(item)->GetMaxUses());
	}
	//Sending Message to client
	NetworkMessageConverter NMC;
	std::string msg = NMC.Convert(MESSAGE_TYPE_ITEM_FILL, (float)itemID);

	if (Container* container = dynamic_cast<Container*>(item))
	{
		msg += NMC.Convert(MESSAGE_TYPE_CONTAINER_CURRENT, (float)container->GetRemainingUses());
		cd->Send(msg);
	}
}

void Game::HandleEquipItem( ClientData* cd, unsigned int itemID )
{
	std::string msg;
	int slot = -1;

	NetworkMessageConverter NMC;
	Actor* actor = this->zPlayers[cd]->GetBehavior()->GetActor();
	PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor);
	if (!pActor)
	{
		msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Equipping items is something only humans can do");
		cd->Send(msg);
		return;
	}
	Inventory* inventory = pActor->GetInventory();
	Item* item = inventory->SearchAndGetItem(itemID);
	Item* ret = NULL;
	bool success = false;

	if(Projectile* proj = dynamic_cast<Projectile*>(item))
	{
		int weight = inventory->GetTotalWeight();

		ret = inventory->EquipProjectile(proj);
		
		if(weight > inventory->GetTotalWeight())
		{
			Item* temp = inventory->RemoveItem(proj);
			SAFE_DELETE(temp);
		}

		slot = EQUIPMENT_SLOT_PROJECTILE;
	}
	else if(MeleeWeapon* meele = dynamic_cast<MeleeWeapon*>(item))
	{
		ret = inventory->EquipMeleeWeapon(meele, success);
		slot = EQUIPMENT_SLOT_MELEE_WEAPON;
	}
	else if(RangedWeapon* ranged = dynamic_cast<RangedWeapon*>(item))
	{
		ret = inventory->EquipRangedWeapon(ranged, success);
		slot = EQUIPMENT_SLOT_RANGED_WEAPON;
	}

	if(!success && slot != EQUIPMENT_SLOT_PROJECTILE)
	{
		msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Cannot Equip Item");
		cd->Send(msg);
		return;
	}
	////Check if the Equipped Item is the Primary one Then Add it to the Mesh
	//Item* primaryWpn = inventory->GetPrimaryEquip();
	//if (primaryWpn == item)
	//	this->HandleBindings(item, pActor->GetID());
	
	//msg = NMC.Convert(MESSAGE_TYPE_EQUIP_ITEM, (float)item->GetID());
	//msg += NMC.Convert(MESSAGE_TYPE_EQUIPMENT_SLOT, (float)slot);
	//cd->Send(msg);
}

void Game::HandleUnEquipItem( ClientData* cd, unsigned int itemID )
{
	std::string msg;
	NetworkMessageConverter NMC;
	Actor* actor = this->zPlayers[cd]->GetBehavior()->GetActor();
	PlayerActor* pActor = dynamic_cast<PlayerActor*>(actor);
	if (!pActor)
		return;

	Inventory* inventory = pActor->GetInventory();
	Item* item = inventory->SearchAndGetItem(itemID);

	bool wasPrimary = false;

	if (item == inventory->GetPrimaryEquip())
		wasPrimary = true;
	
	if(!item)
	{
		msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "Item Was Not Found");
		cd->Send(msg);
		return;
	}

	if(dynamic_cast<Projectile*>(item))
	{
		inventory->UnEquipProjectile();
	}
	else if(dynamic_cast<MeleeWeapon*>(item))
	{
		inventory->UnEquipMeleeWeapon();
	}
	else if(dynamic_cast<RangedWeapon*>(item))
	{
		inventory->UnEquipRangedWeapon();
	}
	else
	{
		msg = NMC.Convert(MESSAGE_TYPE_ERROR_MESSAGE, "No Such slot");
		cd->Send(msg);
		return;
	}

	//if (wasPrimary)
	//{
	//	msg = NMC.Convert(MESSAGE_TYPE_MESH_UNBIND, (float)pActor->GetID());
	//	msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, item->GetModel());
	//	cd->Send(msg);

	//	Item* newPrimary = inventory->GetPrimaryEquip();

	//	if (newPrimary)
	//		this->HandleBindings(newPrimary, pActor->GetID());
	//}
	
	//msg = NMC.Convert(MESSAGE_TYPE_UNEQUIP_ITEM, (float)itemID);
	//msg += NMC.Convert(MESSAGE_TYPE_EQUIPMENT_SLOT, (float)eq_slot);
	//cd->Send(msg);
}

void Game::HandleBindings( const unsigned int ID, const std::string& model, const unsigned int type, const unsigned int subType )
{
	std::string msg;
	NetworkMessageConverter NMC;

	if (type == ITEM_TYPE_WEAPON_RANGED)
	{
		if (subType == ITEM_SUB_TYPE_BOW)
		{
			msg = NMC.Convert(MESSAGE_TYPE_MESH_BINDING, BONE_L_WEAPON);
			msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, model);
			msg += NMC.Convert(MESSAGE_TYPE_OBJECT_ID, (float)ID);
			this->SendToAll(msg);
		}
	}
	else if (type == ITEM_TYPE_WEAPON_MELEE)
	{
		if (subType == ITEM_SUB_TYPE_MACHETE)
		{
			msg = NMC.Convert(MESSAGE_TYPE_MESH_BINDING, BONE_L_WEAPON);
			msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, model);
			msg += NMC.Convert(MESSAGE_TYPE_OBJECT_ID, (float)ID);
			this->SendToAll(msg);
		}
		else if (subType == ITEM_SUB_TYPE_POCKET_KNIFE)
		{
			msg = NMC.Convert(MESSAGE_TYPE_MESH_BINDING, BONE_L_WEAPON);
			msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, model);
			msg += NMC.Convert(MESSAGE_TYPE_OBJECT_ID, (float)ID);
			this->SendToAll(msg);
		}
	}
	else if (type == ITEM_TYPE_PROJECTILE && subType == ITEM_SUB_TYPE_ROCK)
	{
		msg = NMC.Convert(MESSAGE_TYPE_MESH_BINDING, BONE_L_WEAPON);
		msg += NMC.Convert(MESSAGE_TYPE_MESH_MODEL, model);
		msg += NMC.Convert(MESSAGE_TYPE_OBJECT_ID, (float)ID);
		this->SendToAll(msg);
	}
}

void Game::SendToAll( std::string msg)
{
	auto it_zPlayers_end = this->zPlayers.end();
	for(auto it = this->zPlayers.begin(); it != it_zPlayers_end; it++)
	{
		it->first->Send(msg);
	}
}

void Game::ModifyLivingPlayers( const int value )
{
	this->zPlayersAlive += value;

	this->zCurrentFogEnclosement = ( this->zInitalFogEnclosement + (this->zIncrementFogEnclosement * this->zPlayersAlive) ) * this->zFogTotalDecreaseCoeff;
	
	NetworkMessageConverter NMC;
	std::string message = NMC.Convert(MESSAGE_TYPE_FOG_ENCLOSEMENT, this->zCurrentFogEnclosement);
	this->SendToAll(message);
}

void Game::RestartGame()
{
	NetworkMessageConverter NMC;
	std::string msg = NMC.Convert(MESSAGE_TYPE_SERVER_ANNOUNCEMENT, "ServerRestarting");
	SendToAll(msg);
	msg = NMC.Convert(MESSAGE_TYPE_SERVER_RESTART);
	SendToAll(msg);

	//Delete All Actors
	this->zActorManager->ClearAll();
	//Remove old messages
	this->zSyncher->ClearAll();
	//Remove loaded entities
	this->zWorldActors.clear();
	
	//Recreate Actors
	std::string message = "";
	auto it_zPlayers_end = zPlayers.end();
	for (auto it = zPlayers.begin(); it != it_zPlayers_end; it++)
	{
		/*Delete old Behavior*/
		SetPlayerBehavior( (*it).second, 0 );
		(*it).second->GetKeys().ClearStates();

		PhysicsObject* physObj = zPhysicsEngine->CreatePhysicsObject("Media/Models/temp_guy.obj");
		
		PlayerActor* pActor = new PlayerActor((*it).second, physObj, this);
		pActor->SetModel( (*it).second->GetModelPath() );
		PlayerHumanBehavior* pBehavior = new PlayerHumanBehavior(pActor, zWorld, (*it).second);

		pActor->SetPosition(CalcPlayerSpawnPoint(32), false);
		pActor->SetScale(pActor->GetScale(), false);
		pActor->AddObserver(this->zGameMode);

		SetPlayerBehavior((*it).second, pBehavior);
		this->zActorManager->AddActor(pActor);

		//Should be changed Later
		auto offsets = this->zCameraOffset.find((*it).second->GetModelPath());

		if(offsets != this->zCameraOffset.end())
			dynamic_cast<PlayerActor*>(pActor)->SetCameraOffset(offsets->second);

		message = NMC.Convert(MESSAGE_TYPE_SELF_ID, (float)pActor->GetID());
		message += NMC.Convert(MESSAGE_TYPE_ACTOR_TYPE, (float)1);
		(*it).first->Send(message);
	}
	//Debug
	//SpawnAnimalsDebug();

	//Set everyone to false
	for (auto it = zPlayers.begin(); it != zPlayers.end(); it++)
	{
		(*it).second->SetReady(false);
	}

	SpawnItemsDebug();
	//SpawnAnimalsDebug();
	SpawnHumanDebug();

}
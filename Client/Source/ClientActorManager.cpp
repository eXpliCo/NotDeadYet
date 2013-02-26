#include "ClientActorManager.h"
#include "Safe.h"
#include "Graphics.h"
#include "sounds.h"

#define PI 3.14159265358979323846f

ClientActorManager::ClientActorManager()
{
	AudioManager* am = AudioManager::GetInstance();
	am->GetEventHandle(EVENTGROUP_NOTDEADYET_WALK, footStep);
	zInterpolationVelocity = 100.0f;
}

ClientActorManager::~ClientActorManager()
{
	GraphicsEngine* ge = GetGraphics();
	for(auto it = this->zActors.begin(); it != this->zActors.end(); it++)
	{
		if(it->second)
		{
			iMesh* mesh = it->second->GetMesh();
			if (mesh)
				ge->DeleteMesh(mesh);

			delete it->second;
			it->second = NULL;
		}
	}
	this->zActors.clear();

	for(auto it = this->zUpdates.begin(); it != this->zUpdates.end(); it++)
	{
		if(it->second)
		{
			delete it->second;
			it->second = NULL;
		}
	}
	this->zUpdates.clear();
}

void ClientActorManager::UpdateObjects( float deltaTime, unsigned int clientID )
{
	float t = GetInterpolationType(deltaTime, IT_SMOOTH_STEP);
	static GraphicsEngine* gEng = GetGraphics();
	
	auto it_Update = this->zUpdates.begin();
	while( it_Update != this->zUpdates.end() )
	{
		Updates* update = it_Update->second;
		Actor* actor = this->GetActor(update->GetID());

		if (actor)
		{
			if (update->HasStateChanged())
			{
				auto actorIterator = this->zState.find(actor);
				if (actorIterator != this->zState.end())
					actorIterator->second = update->GetState();

				update->SetStateChange(false);
			}
			if (update->HasPositionChanged())
			{
				Vector3 position;
				Vector3 oldPosition;
				if(update->GetID() == clientID)
				{
					position = this->InterpolatePosition(gEng->GetCamera()->GetPosition() - this->zCameraOffset, update->GetPosition(), t);
					
					AudioManager::GetInstance()->SetPlayerPosition(&ConvertToFmodVector(position), &ConvertToFmodVector(gEng->GetCamera()->GetForward()), &ConvertToFmodVector(gEng->GetCamera()->GetUpVector()));
					footStep->Setposition(&ConvertToFmodVector(Vector3(0.0f, 5.0f, 0.0f)));
					footStep->Play();
					gEng->GetCamera()->SetPosition(position + this->zCameraOffset);
				}
				else 
				{
					position = this->InterpolatePosition(actor->GetPosition(), update->GetPosition(), t);
					actor->SetPosition(position);
				}
				update->ComparePosition(position);
			}
			//if((*it_Update)->GetID() != clientID)
			//{
			//	if ((*it_Update)->HasRotationChanged())
			//	{
			//		Vector4 rotation = this->InterpolateRotation(actor->GetRotation(), (*it_Update)->GetRotation(), t);
			//		actor->SetRotation(rotation);
			//		(*it_Update)->CompareRotation(rotation);
			//	}
			//}
			//else
			//{
			//	(*it_Update)->SetRotationChanged(false);
			//}

			if (!update->HasPositionChanged() )//&& !(*it_Update)->HasRotationChanged() && !(*it_Update)->HasStateChanged())
			{
				Updates* temp = update; 
				it_Update = zUpdates.erase(it_Update);
				SAFE_DELETE(temp);
			}
			else
			{
				it_Update++;
			}
		}
		else
		{
			if(update->GetID() == clientID)
			{
				footStep->Stop();
			}
			Updates* temp = update; 
			it_Update = zUpdates.erase(it_Update);
			SAFE_DELETE(temp);
		}
	}
}

std::map<unsigned int, Actor*> ClientActorManager::GetActors()
{
	return this->zActors;
}

Actor* ClientActorManager::GetActor( unsigned int ID )
{
	auto actorIterator = this->zActors.find(ID);

	if (actorIterator != this->zActors.end())
		return actorIterator->second;

	return NULL;
}

Updates* ClientActorManager::GetUpdate(const int ID)
{
	auto updateIterator = this->zUpdates.find(ID);

	if (updateIterator != this->zUpdates.end())
		return updateIterator->second;

	return NULL;
	
}

void ClientActorManager::RemoveActor( const unsigned int ID )
{
	auto actorIterator = this->zActors.find(ID);

	Actor* actor = actorIterator->second;

	auto actorStateIterator = this->zState.find(actor);
	if (actorStateIterator != this->zState.end())
	{
		this->zState.erase(actorStateIterator);
	}

	SAFE_DELETE(actor);

	this->zActors.erase(actorIterator);
}

void ClientActorManager::AddActorState( Actor* actor, unsigned int state )
{
	if (actor)
		this->zState[actor] = state;
}

unsigned int ClientActorManager::GetState( Actor* actor )
{
	if (actor)
	{
		auto stateIterator = this->zState.find(actor);
		if (stateIterator != this->zState.end())
			return stateIterator->second;
	}	

	return 500;
}

bool ClientActorManager::AddActor(Actor* actor)
{
	if (actor)
	{
		this->zActors[actor->GetID()] = actor;
		return true;
	}
	return false;
}

void ClientActorManager::AddUpdate( Updates* update )
{
	if (update)
		this->zUpdates[update->GetID()] = update;
}

float ClientActorManager::GetInterpolationType(const float deltaTime, const unsigned int type)
{
	float t;
	switch (type)
	{
	case IT_LINEAR:
		t = deltaTime;
		break;
	case IT_COSINE:
		t = (-cos(PI * deltaTime) * 0.5f) + 0.5f;
		break;
	case IT_ACCELERATION:
		t = pow(deltaTime, 2);
		break;
	case IT_SMOOTH_STEP:
		t = pow(deltaTime, 2) * (3 - 2 * deltaTime);
		break;
	case IT_DECELERATION:
		t = (1 - pow(1 - deltaTime, 2));
		break;
	default:
		t = deltaTime;
		break;
	}

	return t;
}

Vector3 ClientActorManager::InterpolatePosition(const Vector3& currentPosition, const Vector3& newPosition, float t )
{
	float oldlength = (currentPosition - newPosition).GetLength();

	if (oldlength > 200.0f)
		return newPosition;

	Vector3 returnPosition = currentPosition + (newPosition - currentPosition) * t * zInterpolationVelocity;

	float newLength = (returnPosition - newPosition).GetLength();

	if (newLength > oldlength)
		returnPosition = newPosition;


	return returnPosition;
}

Vector4 ClientActorManager::InterpolateRotation( const Vector4& currentRotation, const Vector4& newRotation, float t )
{
	float oldlength = (currentRotation - newRotation).GetLength();

	Vector4 returnRotation = currentRotation + (newRotation - currentRotation) * t * zInterpolationVelocity;

	float newLength = (returnRotation - newRotation).GetLength();

	if (newLength > oldlength)
		returnRotation = newRotation;

	return returnRotation;
}

FMOD_VECTOR ClientActorManager::ConvertToFmodVector( Vector3 v )
{
	FMOD_VECTOR temp;
	temp.x = v.x;
	temp.y = v.y;
	temp.z = v.z;
	return temp;
}
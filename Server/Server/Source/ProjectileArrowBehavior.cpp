#include <World/World.h>
#include "ProjectileArrowBehavior.h"
#include "Actor.h"
#include "BioActor.h"
#include "ProjectileActor.h"
#include "Physics.h"

static const Vector3 GRAVITY = Vector3(0, -9.82f, 0);

ProjectileArrowBehavior::ProjectileArrowBehavior( Actor* actor, World* world ) : Behavior(actor, world)
{
	this->zSpeed = 30.0f;
	this->zVelocity = actor->GetDir();
	this->zDamping = 0.01f;
	this->zMoving = true;
	//this->zLength = 16.396855f;

	PhysicsObject* actorPhys = actor->GetPhysicsObject();
	Vector3 center = actorPhys->GetBoundingSphere().center;
	center = actorPhys->GetWorldMatrix() * center;

	this->zLength = ( ( center - actor->GetPosition() ) * 2).GetLength();
	this->zNearActorsIndex = 0;
	this->zCollisionRadius = 370.0f;
}

bool ProjectileArrowBehavior::Update( float dt )
{
	if(!zMoving)
		return true;

	Vector3 newPos;
	Vector3 newDir;
	
	// Update linear position.
	newPos =  this->zActor->GetPosition();
	zVelocity.Normalize();
	zVelocity *= zSpeed;
	newPos += (zVelocity * dt);
	newDir = zVelocity;
	newDir.Normalize();

	//Rotate Mesh
	Vector3 ProjectileStartDirection = Vector3(0,0,-1);
	Vector3 ProjectileMoveDirection = newDir;

	ProjectileStartDirection.Normalize();

	Vector3 around = ProjectileStartDirection.GetCrossProduct(ProjectileMoveDirection);
	float angle = acos(ProjectileStartDirection.GetDotProduct(ProjectileMoveDirection));

	//Set Values
	this->zActor->SetPosition(newPos);
	this->zActor->SetRotation(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	this->zActor->SetRotation(around, angle);
	this->zActor->SetDir(newDir);

	/**Check If the arrow is outside the world.**/
	if( !this->zWorld->IsInside(newPos.GetXZ()) )
	{
		Stop();
		return true;
	}
	//**Check if the projectile has hit the ground**
	float yValue = std::numeric_limits<float>::lowest();
	try
	{
		yValue = this->zWorld->CalcHeightAtWorldPos(newPos.GetXZ());
	}
	catch(...)
	{
		Stop();
		return true;
	}

	// If true, stop the projectile and return.
	Vector3 scale = this->zActor->GetScale();
	float middle = zLength * 0.5f;
	float yTip = newPos.y - middle;
	if(yTip <= yValue )
	{
 		middle += yValue;
 		newPos.y = middle;

		ProjectileActor* test = dynamic_cast<ProjectileActor*>(zActor);
		float testDistance = (test->GetOwner()->GetPosition() - test->GetPosition()).GetLength();

		this->zActor->SetPosition(newPos);
		this->zMoving = false;
		
		return true;
	}

	//**Update Velocity for next update**

	// Update linear velocity from the acceleration.
	this->zVelocity += (GRAVITY * dt);

	// Impose drag.
	this->zVelocity *= pow(zDamping, dt);

	//Check collisions
	Actor* collide = CheckCollision();
	if(collide)
	{
		this->zMoving = false;
		if( BioActor* bioActor = dynamic_cast<BioActor*>(collide) )
		{
			if( ProjectileActor* projActor = dynamic_cast<ProjectileActor*>(this->zActor) )
				bioActor->TakeDamage( projActor->GetDamage(), projActor->GetOwner() );
		}

		return true;
	}

	return false;
}

void ProjectileArrowBehavior::SetNearActors( std::set<Actor*> actors )
{
	this->zNearActors = actors;
	this->zNearActors.erase(this->zActor);
}

bool ProjectileArrowBehavior::RefreshNearCollideableActors( const std::set<Actor*>& actors )
{
	if( !this->zActor )
		return false;

	bool canCollide = this->zActor->CanCollide();
	if(!canCollide)
		return false;

	unsigned int size = actors.size();
	// Increment 10%
	unsigned int increment = size * 0.1;
	unsigned int counter = 0;

	Vector3 pos = this->zActor->GetPosition();

	auto it = actors.begin();
	std::advance(it, zNearActorsIndex);

	//Check if the new actors is within range.
	for (unsigned int i = 0; i < increment; i++)
	{

		if( it == actors.end() )
		{
			zNearActorsIndex = 0;
			it = actors.begin();
		}

		if( (*it) != this->zActor && (*it)->CanCollide() )
		{
			Vector3 vec = (*it)->GetPosition() - pos;
			auto found = zNearActors.find(*it);

			if( found == zNearActors.end() && vec.GetLength() <= zCollisionRadius )
			{
				counter++;
				zNearActors.insert(*it);
			}
		}

		zNearActorsIndex++;
		it++;
	}

	//Remove old actors that is not nearBy
	auto begin = zNearActors.begin();
	std::advance(begin, counter);

	while( begin != zNearActors.end() )
	{
		if(!(*begin))
		{
			begin = zNearActors.erase(begin);
			continue;
		}

		Vector3 vec = (*begin)->GetPosition() - pos;
		if( vec.GetLength() > zCollisionRadius)
			begin = zNearActors.erase(begin);
		else
		{
			begin++;
		}
	}

	return true;
}

Actor* ProjectileArrowBehavior::CheckCollision()
{
	if( !this->zActor && !this->zActor->CanCollide() )
		return NULL;

	float range = this->zLength;
	float rangeWithin = 2.0f + range;
	PhysicsCollisionData data;
	ProjectileActor* projActor = dynamic_cast<ProjectileActor*>(this->zActor); 
	Actor* collide = NULL;
	Actor* owner = NULL;

	if(projActor)
		owner = projActor->GetOwner();

	
	for (auto it = this->zNearActors.begin(); it != this->zNearActors.end(); it++)
	{

		if( *it == this->zActor )
			continue;
		if( *it == owner )
			continue;
		
		float distance = ( this->zActor->GetPosition() - (*it)->GetPosition() ).GetLength();
		
		if( distance > rangeWithin )
			continue;

		PhysicsObject* targetObject = (*it)->GetPhysicsObject();
		data = GetPhysics()->GetCollisionRayMesh(this->zActor->GetPosition(), this->zActor->GetDir(), targetObject);

		if(data.collision && data.distance < range)
		{
			range = data.distance;
			collide = (*it);
		}

	}

	return collide;
}

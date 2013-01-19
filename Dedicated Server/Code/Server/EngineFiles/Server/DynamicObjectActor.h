/*
Made by Ed�nge Simon Datum(19/12/12 created) 
for project Not Dead Yet at Blekinge tekniska h�gskola.
*/

#pragma once

#include "Actor.h"
#include "../../../../../Source/PhysicsEngine/PhysicsObject.h"

class DynamicObjectActor : public Actor
{
public:
	DynamicObjectActor(bool genID = false);
	DynamicObjectActor(const std::string& meshModel, bool genID = false);
	DynamicObjectActor(const std::string& meshModel, int objOwner, bool genID = false);
	virtual ~DynamicObjectActor();

	virtual void Update(float deltaTime) = 0;

	int GetWeight() const {return this->zWeight;}
	int GetType() const {return this->zType;}
	std::string GetDescription() const {return this->zDescription;}
	std::string GetIconPath() const {return this->zIconPath;}
	Vector3 GetDirection() const {return this->zDirection;}
	int GetStackSize() const {return this->zStackSize;}
	inline  PhysicsObject* GetPhysicObject() const {return this->zPhysicObj;}
	/*! Returns the ID of the player who created this object.
		Returns -1 if this object has no owner.
	*/
	int GetObjPlayerOwner(){return this->zObjPlayerOwner;}

	/*! Sets the player owner of this object.*/
	void SetObjOwner(int ID){this->zObjPlayerOwner = ID;}
	void SetIconPath(const std::string& path) {this->zIconPath = path;}
	void SetDescription(const std::string& description) {this->zDescription = description;}
	void SetWeight(const int weight) {this->zWeight = weight;}
	void SetType(const int TYPE) {this->zType = TYPE;}
	void SetDirection(Vector3 dir) {this->zDirection = dir;}
	void SetPhysicObject(PhysicsObject* pObj){this->zPhysicObj = pObj;}
	void SetStackSize(const int size) {this->zStackSize = size;}
	void ModifyStackSize(const int size) {this->zStackSize += size;}

private:
	void InitValues();

protected:
	int zType;
	int zWeight;
	int zStackSize;
	int zObjPlayerOwner;
	Vector3 zDirection;
	std::string zDescription;
	std::string zIconPath;
	PhysicsObject* zPhysicObj;
	
private:
};

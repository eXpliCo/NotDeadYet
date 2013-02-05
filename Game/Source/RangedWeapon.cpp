#include "RangedWeapon.h"

RangedWeapon::RangedWeapon(const long id, const unsigned int itemType, const float damage, const float range)
						   : Weapon(id, itemType, damage, range)
{
	this->zStacking = false;
}

RangedWeapon::RangedWeapon(const RangedWeapon& other)
{
	this->zID = other.zID;
	this->zRange = other.zRange;
	this->zStacks = other.zStacks;
	this->zWeight = other.zWeight;
	this->zDamage = other.zDamage;
	this->zItemName = other.zItemName;
	this->zIconPath = other.zIconPath;
	this->zItemType = other.zItemType;
	this->zItemSubType = other.zItemSubType;
	this->zItemDescription = other.zItemDescription;
}

RangedWeapon::RangedWeapon(const RangedWeapon* other)
{
	this->zID = other->zID;
	this->zRange = other->zRange;
	this->zStacks = other->zStacks;
	this->zWeight = other->zWeight;
	this->zDamage = other->zDamage;
	this->zItemName = other->zItemName;
	this->zIconPath = other->zIconPath;
	this->zItemType = other->zItemType;
	this->zItemSubType = other->zItemSubType;
	this->zItemDescription = other->zItemDescription;
}

RangedWeapon::RangedWeapon(const long ID, const unsigned int itemType, const int itemSubType, 
						   const float damage, const float range) : Weapon(ID, itemType, itemSubType, damage, range)
{
	this->zStacking = false;
}

RangedWeapon::~RangedWeapon()
{

}

void RangedWeapon::UseWeapon(float& range, float& damage)
{
	range = this->zRange;
	damage = this->zDamage;
}

bool RangedWeapon::Use()
{
	return true;
}
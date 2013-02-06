#include "Material.h"

Material::Material(const Material& other)
{
	this->zID = other.zID;
	this->zStacks = other.zStacks;
	this->zWeight = other.zWeight;
	this->zItemName = other.zItemName;
	this->zIconPath = other.zIconPath;
	this->zItemType = other.zItemType;
	this->zItemSubType = other.zItemSubType;
	this->zCraftingType = other.zCraftingType;
	this->zItemDescription = other.zItemDescription;
	this->zRequiredStackToCraft = other.zRequiredStackToCraft;
}

Material::Material(const Material* other)
{
	this->zID = other->zID;
	this->zStacks = other->zStacks;
	this->zWeight = other->zWeight;
	this->zItemName = other->zItemName;
	this->zIconPath = other->zIconPath;
	this->zItemType = other->zItemType;
	this->zItemSubType = other->zItemSubType;
	this->zCraftingType = other->zCraftingType;
	this->zItemDescription = other->zItemDescription;
	this->zRequiredStackToCraft = other->zRequiredStackToCraft;
}

Material::Material(const long ID, const unsigned int itemType, const unsigned int itemSubType, 
				   const unsigned int craftingType, const int stacksRequiredToCraft) : Item(ID, itemType, itemSubType)
{
	this->zStacking = true;
	this->zCraftingType = craftingType;
	this->zRequiredStackToCraft = stacksRequiredToCraft;
}

Material::~Material()
{

}

bool Material::Use()
{
	if (this->zStacks >= this->zRequiredStackToCraft)
	{
		this->zStacks -= this->zRequiredStackToCraft;

		return true;
	}
	return false;
}

bool Material::IsUsable()
{
	if (this->zStacks >= this->zRequiredStackToCraft)
		return true;

	return false;
}

std::string Material::ToMessageString( NetworkMessageConverter* NMC )
{
	std::string msg = Item::ToMessageString(NMC);

	msg += NMC->Convert(MESSAGE_TYPE_MATERIAL_STACKS_REQUIRED, (float)this->zRequiredStackToCraft);
	msg += NMC->Convert(MESSAGE_TYPE_MATERIAL_CRAFTING_TYPE, (float)this->zCraftingType);
	
	return msg;
}
#include "Inventory.h"
#include "ClientServerMessages.h"
#include <MaloWFileDebug.h>
#include <Safe.h>

const unsigned int GEAR_SLOTS = 4;
const unsigned int MAX_QUIVER_SLOTS = 30;

Inventory::Inventory()
{
	this->zInventoryCap = 49;
	this->zWeightTotal = 0;
	
	this->zInventorySlotBlocked = std::vector<bool>();
	this->zItems = std::vector<Item*>();

	this->zSlotsAvailable = this->zInventoryCap;

	this->zRangedWeapon = NULL;
	this->zMeleeWeapon = NULL;
	this->zProjectile = NULL;
	this->zPrimaryEquip = NULL;
	this->zSecondaryEquip = NULL;

	for (unsigned int i = 0; i < GEAR_SLOTS; i++)
	{
		this->zGear.push_back(NULL);
	}
}

Inventory::Inventory(const unsigned int inventorySize)
{
	this->zInventoryCap = inventorySize;
	this->zWeightTotal = 0;

	this->zInventorySlotBlocked = std::vector<bool>();
	this->zItems = std::vector<Item*>();

	this->zSlotsAvailable = this->zInventoryCap;

	this->zRangedWeapon = NULL;
	this->zMeleeWeapon = NULL;
	this->zProjectile = NULL;
	this->zPrimaryEquip = NULL;
	this->zSecondaryEquip = NULL;
}

Inventory::~Inventory()
{
	//Remove Item Arrays
	for (auto x = this->zItems.begin(); x != this->zItems.end(); x++)
	{
		SAFE_DELETE((*x));
	}
	this->zItems.clear();

	for (auto x = this->zGear.begin(); x != this->zGear.end(); x++)
	{
		SAFE_DELETE((*x));
	}
}

std::vector<Item*> Inventory::GetItems() const
{
	return this->zItems;
}

bool Inventory::AddItem(Item* item, bool &stacked)
{
	if (!item)
		return false;

	unsigned int weight = item->GetWeight();
	int available_slots = CalcMaxAvailableSlots(item);
	stacked = false;
	
	//No slots, return false
	if( available_slots <= 0 )
		return false;

	if (this->zSlotsAvailable - item->GetSlotSize() <= 0)
		return false;

	if ( item->GetStacking() )
	{
		if( IsStacking(item) )
		{
			Item* existingItem = this->SearchAndGetItemFromType(item->GetItemType(), item->GetItemSubType());

			int max_stack = MAX_QUIVER_SLOTS - existingItem->GetStackSize();
			unsigned int stack;

			if(available_slots < max_stack)
				max_stack = available_slots;

			if( max_stack < item->GetStackSize() )
			{
				stack = max_stack;
			}
			else if( max_stack >= item->GetStackSize() )
			{
				stack = item->GetStackSize();
			}

			existingItem->IncreaseStackSize(stack);
			item->DecreaseStackSize(stack);

			if (Messages::FileWrite())
				Messages::Debug("Added Stack to inventory " + item->GetItemName());

			stacked = true;
			this->zWeightTotal += weight * stack;
			this->zSlotsAvailable -= item->GetSlotSize();
			return true;
		}
	}
	
	if( available_slots >= item->GetStackSize() )
	{
		this->zItems.push_back(item);

		if (Messages::FileWrite())
			Messages::Debug("Added Item " + item->GetItemName() + " ID: " + MaloW::convertNrToString((float)item->GetID()));

		this->zWeightTotal += weight * item->GetStackSize();
		this->zSlotsAvailable -= item->GetSlotSize();
		return true;
	}

	return false;
}

int Inventory::CalcMaxAvailableSlots( Item* item )
{
	if(!item)
		return 0;

	int slotsLeft = this->zInventoryCap - this->zWeightTotal;
	unsigned int weight = item->GetWeight();

	int available_slots = slotsLeft / weight;

	return available_slots;
}

bool Inventory::IsStacking( Item* item )
{
	if( !item->GetStacking() )
		return false;

	Item* existingItem = this->SearchAndGetItemFromType(item->GetItemType(), item->GetItemSubType());

	if( !existingItem )
		return false;

	int max_stack = MAX_QUIVER_SLOTS - existingItem->GetStackSize();

	if( max_stack <= 0)
		return false;

	return true;
}

Item* Inventory::SearchAndGetItem(unsigned int ID) const
{
	int position = this->Search(ID);

	return this->GetItem(position);
}

Item* Inventory::GetItem(const unsigned int Index) const
{
	if (Index < this->zItems.size())
		return this->zItems[Index];
	
	return NULL;
}

int Inventory::Search(const unsigned int ID) const
{
	Item* temp = NULL;
	for (unsigned int i = 0; i < this->zItems.size(); i++)
	{
		temp = this->zItems[i];
		if (temp)
			if (ID == temp->GetID())
				return i;
	}
	return -1;
}

int Inventory::Search( Item* item ) const
{
	for (auto it = zItems.begin(); it < zItems.end(); it++)
	{
		if(item == (*it))
			return std::distance(zItems.begin(), it);
	}

	return -1;
}

bool Inventory::RemoveItemStack(const unsigned int ID, const unsigned int numberOfStacks)
{
	int index = this->Search(ID);
	if (index == -1)
		return false;

	if ((unsigned int)index < this->zItems.size())
	{
		int weight = GetItem(index)->GetWeight() * numberOfStacks;
		this->zWeightTotal -= weight;

		if (Messages::FileWrite())
			Messages::Debug("Removed " + MaloW::convertNrToString((float)numberOfStacks) + " Stacks, TotalWeight: " + MaloW::convertNrToString((float)zWeightTotal));
		
		return true;
	}
	return false;
}

Item* Inventory::RemoveItem(const unsigned int ID)
{
	int index = this->Search(ID);
	if (index != -1)
		return this->Erase(index);

	return NULL;
}

Item* Inventory::RemoveItem(Item* item)
{
	int index = this->Search(item);
	if (index != -1)
		return this->Erase(index);
	
	return NULL;
}

Item* Inventory::Erase( const unsigned int Index )
{
	if (Index < this->zItems.size())
	{
		Item* item = this->GetItem(Index);
		int weight = item->GetWeight() * item->GetStackSize();
		this->zWeightTotal -= weight;

		this->zItems.erase(this->zItems.begin() + Index);
		if(zRangedWeapon && dynamic_cast<RangedWeapon*>(item) == this->zRangedWeapon)
		{
			UnEquipRangedWeapon();
		}
		else if(zMeleeWeapon && dynamic_cast<MeleeWeapon*>(item) == this->zMeleeWeapon)
		{
			UnEquipMeleeWeapon();
		}
		else if(zProjectile && dynamic_cast<Projectile*>(item) == this->zProjectile)
		{
			UnEquipProjectile();
		}

		return item;
	}
	return NULL;
}

std::vector<bool> Inventory::GetBlockedSlots() const
{
	return this->zInventorySlotBlocked;
}

bool Inventory::GetBlockedSlot(unsigned int index) const
{
	return this->zInventorySlotBlocked[index];
}

int Inventory::GetInventoryCapacity() const
{
	return this->zInventoryCap;
}

int Inventory::SearchForItemType(const int Type, const int SubType)
{
	int counter = 0;

	for (auto it = this->zItems.begin(); it < this->zItems.end(); it++)
	{
		if ((*it)->GetItemType() == Type && (*it)->GetItemSubType() == SubType)
		{
			counter += (*it)->GetStackSize();
		}
	}

	return counter;
}

Item* Inventory::SearchAndGetItemFromType(const int Type, const int SubType)
{
	for (auto it = this->zItems.begin(); it < this->zItems.end(); it++)
	{
		if ((*it)->GetItemType() == Type && (*it)->GetItemSubType() == SubType)
		{
			return (*it);
		}
	}
	return NULL;
}

//Equipment
Item* Inventory::EquipRangedWeapon(RangedWeapon* weapon, bool& success)
{
	Item* ret = NULL;
	success = true;

	if(!this->zPrimaryEquip)
		zPrimaryEquip = weapon;

	else if(zPrimaryEquip->GetItemSubType() == weapon->GetItemSubType())
	{
		ret = zPrimaryEquip;
		zPrimaryEquip = weapon;
	}

	else if(!this->zSecondaryEquip)
		zSecondaryEquip = weapon;

	else if (zSecondaryEquip->GetItemSubType() == weapon->GetItemSubType())
	{
		ret = zSecondaryEquip;
		zSecondaryEquip = weapon;
	}
	else
	{
		MaloW::Debug("Failed to Equip Ranged Weapon");
		success = false;
	}

	if(success)
	{
		if (Messages::FileWrite())
			Messages::Debug("Equipped Ranged Weapon");
		this->zRangedWeapon = weapon;
	}

	return ret;
}

Item* Inventory::EquipMeleeWeapon(MeleeWeapon* weapon, bool& success)
{
	Item* ret = NULL;
	success = true;

	if(!this->zPrimaryEquip)
		zPrimaryEquip = weapon;

	else if(zPrimaryEquip->GetItemSubType() == weapon->GetItemSubType())
	{
		ret = zPrimaryEquip; 
		zPrimaryEquip = weapon;
	}

	else if(!this->zSecondaryEquip)
		zSecondaryEquip = weapon;

	else if(zSecondaryEquip->GetItemSubType() == weapon->GetItemSubType())
	{
		ret = zSecondaryEquip;
		zSecondaryEquip = weapon;
	}
	else
	{
		MaloW::Debug("Failed to Equip Melee Weapon");
		success = false;
	}

	if(success)
	{
		if (Messages::FileWrite())
			Messages::Debug("Equipped Melee Weapon");
		this->zMeleeWeapon = weapon;
	}

	return ret;
}

Item* Inventory::EquipProjectile(Projectile* projectile)
{

	Item* ret = NULL;

	if (this->zProjectile)
	{
		if (this->zProjectile->GetItemSubType() == projectile->GetItemSubType())
		{
			int totalStacks = this->zProjectile->GetStackSize() + projectile->GetStackSize();
			int weigth = projectile->GetStackSize() * projectile->GetWeight(); 
			this->zWeightTotal += weigth;

			this->zProjectile->SetStackSize(totalStacks);

			ret = zProjectile;
			if (Messages::FileWrite())
				Messages::Debug("Equipped Projectile and added stacks");
		}
		else
		{
			if (Messages::FileWrite())
				Messages::Debug("Equipped Projectile and replaced old one");
			ret = this->zProjectile;
			this->zProjectile = projectile;
		}
	}
	else
	{
		if (Messages::FileWrite())
			Messages::Debug("Equipped Projectile");
		this->zProjectile = projectile;
	}

	return ret;
}

bool Inventory::EquipGear(const unsigned int type, Gear* item)
{
	if (type < GEAR_SLOTS)
	{
		this->zGear[type] = item;
		return true;
	}
	return false;
}

Gear* Inventory::GetGear(const unsigned int type)
{
	if (type < GEAR_SLOTS)
	{
		return this->zGear[type];
	}
	return NULL;
}

void Inventory::UnEquipGear(const unsigned int type)
{
	if (type < GEAR_SLOTS)
		this->zGear[type] = NULL;
}

MeleeWeapon* Inventory::GetMeleeWeapon()
{
	return this->zMeleeWeapon;
}

RangedWeapon* Inventory::GetRangedWeapon()
{
	return this->zRangedWeapon;
}

void Inventory::UnEquipRangedWeapon()
{
	if (Messages::FileWrite())
		Messages::Debug("UnEquipped Ranged Weapon");

	Item* item = dynamic_cast<RangedWeapon*>(this->zRangedWeapon);

	if(zPrimaryEquip == item)
	{
		this->zPrimaryEquip = NULL;
		this->zPrimaryEquip = zSecondaryEquip;
		this->zSecondaryEquip = NULL;
	}
	else if(this->zSecondaryEquip == item)
	{
		this->zSecondaryEquip = NULL;
	}

	this->zRangedWeapon = NULL;
}

void Inventory::UnEquipMeleeWeapon()
{
	if (Messages::FileWrite())
		Messages::Debug("UnEquipped Melee Weapon");

	Item* item = dynamic_cast<MeleeWeapon*>(this->zMeleeWeapon);

	if(this->zPrimaryEquip == item)
	{
		this->zPrimaryEquip = NULL;
		this->zPrimaryEquip = zSecondaryEquip;
		this->zSecondaryEquip = NULL;
	}
	else if(this->zSecondaryEquip == item)
	{
		this->zSecondaryEquip = NULL;
	}

	this->zMeleeWeapon = NULL;
}

void Inventory::UnEquipProjectile()
{
	if (Messages::FileWrite())
		Messages::Debug("UnEquipped Projectile");

	Item* item = dynamic_cast<Projectile*>(this->zProjectile);

	if(this->zPrimaryEquip == item)
	{
		this->zPrimaryEquip = NULL;
		this->zPrimaryEquip = zSecondaryEquip;
		this->zSecondaryEquip = NULL;
	}
	else if(this->zSecondaryEquip == item)
	{
		this->zSecondaryEquip = NULL;
	}

	this->zProjectile = NULL;
}

Projectile* Inventory::GetProjectile()
{
	return this->zProjectile;
}

void Inventory::SetPrimaryEquip(unsigned int ID)
{
	Item* item = SearchAndGetItem(ID);

	if(!item)
	{
		MaloW::Debug("Cannot find item in SetPrimaryEquip.");
		return;
	}

	this->zPrimaryEquip = item;
}

void Inventory::SetSecondaryEquip(unsigned int ID)
{
	Item* item = SearchAndGetItem(ID);

	if(!item)
	{
		MaloW::Debug("Cannot find item in SetSecondaryEquip.");
		return;
	}

	this->zSecondaryEquip = item;
}

bool Inventory::SwapWeapon()
{
	if (!this->zPrimaryEquip && !this->zSecondaryEquip)
		return false;
	else if(!this->zSecondaryEquip)
		return false;

	
	Item* item = this->zPrimaryEquip;

	this->zPrimaryEquip = this->zSecondaryEquip;
	this->zSecondaryEquip = item;

	return true;
}



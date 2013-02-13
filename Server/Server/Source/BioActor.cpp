#include "BioActor.h"


BioActor::BioActor() : Actor()
{
	zInventory = new Inventory();
	this->zState = STATE_IDLE;
	this->zVelocity = V_WALK_SPEED;

	this->zAlive = true;
	this->zHealthMax = 100;
	this->zHealth = zHealthMax;
	this->zHealthChanged = true;

	this->zStaminaMax = 100;
	this->zStamina = zStaminaMax;
	this->zStaminaCof = 0.10f;
	this->zStaminaChanged = true;
}

BioActor::~BioActor()
{
	SAFE_DELETE(this->zInventory);
}


bool BioActor::TakeDamage(const Damage& dmg, Actor* dealer)
{
	this->zHealth -= dmg.GetTotal();
	this->zHealthChanged = true;

	if(dmg.GetTotal() / this->zHealth > 0.20 && dmg.GetBleedFactor() > 0.6)
	{
		this->zBleeding = true;
	}

	if(this->zHealth <= 0.0f)
	{
		this->zAlive = false;
		this->zHealth = 0.0f;
	}

	if(!zAlive)
	{
		//RotateMesh
		Vector3 up = Vector3(0, 1, 0);
		Vector3 forward = Vector3(0, 0, 1);
		Vector3 around = up.GetCrossProduct(forward);
		around.Normalize();
		float angle = 3.14f * 0.5f;
		
		this->GetPhysicsObject()->SetQuaternion(Vector4(.0f,.0f,.0f,.1f));
		this->SetRotation(around,angle);
	}
	
	// Notify Damage
	BioActorTakeDamageEvent BATD;
	BATD.zActor = this;
	BATD.zDamage = dmg;
	BATD.zDealer = dealer;
	NotifyObservers(&BATD);

	return this->zAlive;
}

void BioActor::Kill()
{
	Damage dmg;
	dmg.fallingDamage = zHealth;

	TakeDamage(dmg, this);
}

bool BioActor::IsAlive() const
{
	return this->zAlive;
}

bool BioActor::Sprint(float dt)
{
	float temp = this->zStamina;
	temp -= dt * this->zStaminaCof;

	if(temp < 0.0f)
		return false;
	else
		this->zStamina = temp;
	
	this->zStaminaChanged = true;

	return true;
}

void BioActor::RewindPosition()
{
	SetPosition(zPreviousPos);
}

bool BioActor::HasMoved()
{
	Vector3 curPos = GetPosition();

	if(curPos == this->zPreviousPos)
		return false;	
	return true;
}

std::string BioActor::ToMessageString( NetworkMessageConverter* NMC )
{
	string msg = "";
	msg += NMC->Convert(MESSAGE_TYPE_STATE, (float)this->zState);

	if(zHealthChanged)
	{
		msg += NMC->Convert(MESSAGE_TYPE_HEALTH, this->zHealth);
		this->zHealthChanged = false;
	}

	if(zStaminaChanged)
	{
		msg += NMC->Convert(MESSAGE_TYPE_STAMINA, this->zStamina);
		this->zStaminaChanged = false;
	}

	return msg;
}

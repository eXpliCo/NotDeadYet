#pragma once

#include <Observer.h>
class Actor;

class ActorPositionEvent : public Event
{
public:
	Actor *zActor;
};

class ActorRotationEvent : public Event
{
public:
	Actor *zActor;
};

class ActorScaleEvent : public Event
{
public:
	Actor *zActor;
};

class ActorDeleteEvent : public Event
{
public:
	Actor* zActor;
};

class ActorDirEvent : public Event
{
public:
	Actor *zActor;
};

class ActorPhysicalConditionEnergyEvent : public Event
{
public:
	Actor *zActor;
};

class ProjectileArrowCollide : public Event
{
public:
	Actor *zActor;
};
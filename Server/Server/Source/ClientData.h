/*
Made by Ed�nge Simon 2013-01-24
for project Not Dead Yet at Blekinge tekniska h�gskola.
*/
#pragma once

#include <ClientChannel.h>
#include <Safe.h>
#include <vector>
#include <Packets\Packet.h>


class ClientData
{
	ClientData(MaloW::ClientChannel* cc);
	virtual ~ClientData();

	bool zPinged;
	float zCurrentPingTime;
	float zTotalPingTime;
	float zMaxPingTime;
	int zNrOfPings;
	bool zReady;

	MaloW::ClientChannel* zClient;
public:

	inline float GetCurrentPingTime() const {return zCurrentPingTime;}
	inline float GetTotalPingTime() const {return zTotalPingTime;}
	inline int GetNrOfPings() const {return zNrOfPings;}
	inline bool GetReady(){ return zReady; }
	
	inline void SetReady(bool ready){ zReady = ready; }
	inline void SetPinged(const bool pinged) {zPinged = pinged;}
	inline void SetCurrentPingTime(float const cpt) {zCurrentPingTime = cpt;}
	inline bool HasBeenPinged() const {return zPinged;}
	inline void IncPingTime(float dt) {zCurrentPingTime += dt;}
	inline void ResetPingCounter() {zPinged = 0; zTotalPingTime = 0.0f;}

	/*! Sends a message to the client.*/
	inline void Send(const std::string& msg)
	{
		if ( zClient ) zClient->TrySend(msg);
	}

	inline void Send( const Packet& packet )
	{
		if ( zClient )
		{
			std::stringstream ss;
			
			// Notify Data Type
			ss << "PACKET";

			// Packet
			std::string typeName = typeid(packet).name();
			typeName.erase( typeName.begin(), typeName.begin() + 6 );

			// Write Packet Type Name
			unsigned int typeNameSize = typeName.length();
			ss.write( reinterpret_cast<const char*>(&typeNameSize), sizeof(unsigned int) );
			ss.write( &typeName[0], typeNameSize );

			// Write Packet Data
			if ( !packet.Serialize(ss) ) throw("Failed Packet Serialization!");			

			// Send
			Send(ss.str());
		}
	}

	/*! Handle the ping from client.*/
	void HandlePingMsg();
	/*! Updates the latency of this client.*/
	bool CalculateLatency(float& latencyOut);
	/*! kicks the client.*/
	void Kick();

	friend class Host;
};
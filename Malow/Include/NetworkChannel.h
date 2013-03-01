#pragma once

#include <string>
#include "Winsock.h"

namespace MaloW
{
	class NetworkChannel
	{
	protected:
		SOCKET zSocket;
		unsigned int zPacketNumberIn;
		unsigned int zPacketNumberOut;
		unsigned int zNumBytesIn;
		unsigned int zNumBytesOut;

		NetworkChannel( SOCKET socket );

		// Receive Message
		virtual bool Receive(std::string& msg) throw(...);

	public:
		// Amount of Packets Sent
		unsigned int GetNumPacketsSent() const { return zPacketNumberOut; }

		// Amount of Packets Received
		unsigned int GetNumPacketsReceived() const { return zPacketNumberIn; }
		
		// Amount of Bytes Sent
		unsigned int GetNumBytesSent() const { return zNumBytesIn; }

		// Amount of Bytes Received
		unsigned int GetNumBytesReceived() const { return zNumBytesOut; }

		// Send Message
		virtual bool Send(const std::string& msg) throw(...);

		// Try Sending A message, does not throw when failing
		virtual bool TrySend(const std::string& msg);

		// Disconnects the channel.
		virtual void Disconnect();
	};
}
#ifndef NETWORKSERVER_H
#define NETWORKSERVER_H

#include "ClientChannel.h"
#include "../../../../../Source/MaloWLib/Array.h"
#include "../../../../../Source/MaloWLib/MaloWFileDebug.h"

using namespace std;

/*
INHERIT THIS CLASS AND CREATE YOUR OWN NETWORK SERVER!
This class will listen for new connections on specified port with it's own process.
When a connection is found it will create a ClientChannel and then call the function 
ClientConnected(cc) with cc being the ClientChannel, 
so you have to implement this function yourself and do what you want with cc.
Dont forget to ->Start() it!
*/

namespace MaloW
{
	class NetworkServer : public MaloW::Process
	{
	private:
		SOCKET sock;
		int port;
		bool stayAlive;

	protected:
		virtual void CloseSpecific();

	public:
		NetworkServer();
		virtual ~NetworkServer();
		
		int InitConnection(int port);
		ClientChannel* ListenForNewClients();

		void Life();
		
		virtual bool IsAlive() const;

		virtual void ClientConnected(ClientChannel* cc) = 0;
	};
}

#endif
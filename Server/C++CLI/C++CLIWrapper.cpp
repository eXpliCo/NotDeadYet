//File: C++CLIWrapper.cpp

#include "C++CLIWrapper.h"
#include <sstream>

namespace System { namespace Windows { namespace Interop
{
	CppCLI::CppCLI()
	{
		zServer = new ServerEngineWrapper();
	}

	CppCLI::~CppCLI()
	{
		if(zServer)
			delete zServer;
	}

	String^ CppCLI::Init(int port, int nrOfPlayers, String^ gameMode, String^ mapName)
	{
		String^ returnText;
		char* gameModeName = nullptr;
		char* mapNameChar = nullptr;
		try
		{
			gameModeName = (char*)Marshal::StringToHGlobalAnsi(gameMode).ToPointer();
			mapNameChar = (char*)Marshal::StringToHGlobalAnsi(mapName).ToPointer();
		}
		catch(...)
		{
			return "Failed";
		}
		returnText = gcnew String(zServer->Init(port, nrOfPlayers, gameModeName, mapNameChar));

		return returnText;
	}

	int CppCLI::GetNrOfPlayers()
	{
		return zServer->GetNrOfPlayers();
	}

	bool CppCLI::StartHost()
	{
		return zServer->StartHost();
	}

	void CppCLI::ShutdownHost()
	{
		return zServer->ShutdownHost();
	}

	bool CppCLI::IsRunning()
	{
		return zServer->IsRunning();
	}



}}}
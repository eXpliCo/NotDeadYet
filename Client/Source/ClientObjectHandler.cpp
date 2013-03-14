#include "Client.h"
#include <ClientServerMessages.h>

bool Client::AddActor(const std::vector<std::string>& msgArray, const unsigned int ID)
{
	if (this->zActorManager->GetActor(ID))
	{
		MaloW::Debug("Cant create a new Player Object. ID: " + MaloW::convertNrToString((float)ID) + " already exists");
		return false;
	}
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(0.05f, 0.05f, 0.05f);
	Vector4 rotation = Vector4(0, 0, 0, 0);
	std::string filename = "";

	Actor* actor = new Actor(ID);

	char key[512];
	for(auto it = msgArray.begin() + 1; it < msgArray.end(); it++)
	{
		sscanf_s((*it).c_str(), "%s ", key, sizeof(key));

		if(strcmp(key, M_POSITION.c_str()) == 0)
		{
			position = this->zMsgHandler.ConvertStringToVector(M_POSITION, (*it));
		}
		else if(strcmp(key, M_ROTATION.c_str()) == 0)
		{
			rotation = this->zMsgHandler.ConvertStringToQuaternion(M_ROTATION, (*it));
		}
		else if(strcmp(key, M_STATE.c_str()) == 0)
		{
			int state = this->zMsgHandler.ConvertStringToInt(M_STATE, (*it));
			this->zActorManager->AddActorState(actor, state);
		}
		else if(strcmp(key, M_SCALE.c_str()) == 0)
		{
			scale = this->zMsgHandler.ConvertStringToVector(M_SCALE, (*it));
		}
		else if(strcmp(key, M_MESH_MODEL.c_str()) == 0)
		{
			filename = this->zMsgHandler.ConvertStringToSubstring(M_MESH_MODEL, (*it));
		}
		else
		{
			MaloW::Debug("C: Unknown Message Was sent from server " + (*it) + " in AddNewPlayerObject");
		}
	}
	
	if (Messages::FileWrite())
		Messages::Debug("Actor ID: " + MaloW::convertNrToString((float)ID) +" Added");

	//Creates a StaticMesh from the given Filename
	iMesh* mesh = this->zEng->CreateStaticMesh(filename.c_str(), position);
	mesh->ResetRotation();
	mesh->SetQuaternion(rotation);
	mesh->SetScale(scale);

	//Create player data
	actor->SetStaticMesh(mesh);
	actor->SetModel(filename);

	if (!this->zCreated)
	{
		if (ID == this->zID)
		{
			if (this->zGuiManager)
				SAFE_DELETE(this->zGuiManager);
					
			this->zGuiManager = new GuiManager(this->zEng);
			this->zCreated = true;

			auto meshOffsetsIterator = this->zMeshCameraOffsets.find(filename);
			if (meshOffsetsIterator != this->zMeshCameraOffsets.end())
			{
				this->zMeshOffset = meshOffsetsIterator->second;
			}
			else
			{
				this->zMeshOffset = Vector3(0.0f, 0.5f, 0.0f);
			}

			this->zActorManager->SetCameraOffset(this->zMeshOffset);
			this->zEng->GetCamera()->SetMesh(mesh, this->zMeshOffset);
			this->zEng->GetCamera()->SetPosition(position + this->zMeshOffset);

			if (this->zActorType == GHOST)
			{
				this->zPam->ToggleMenu(); // Shows the menu and sets Show to true.
				if(this->zPam->GetShow())
					zShowCursor = true;
				else
					zShowCursor = false;
			}
		}
	}

	this->zActorManager->AddActor(actor);

	return true;
}

void Client::AddActor( NewActorPacket* NAP )
{
	Actor* actor = NULL;
	unsigned int ID = 0;
	
	std::string model;
	auto it_actorModel_end = NAP->actorModel.end();
	for (auto it = NAP->actorModel.begin(); it != it_actorModel_end; it++)
	{
		ID = it->first;
		model = it->second;

		if (!this->zActorManager->GetActor(ID))
		{
			iMesh* mesh = NULL;
			//Creates a Mesh from the given Filename
			if (model.length() > 4)
			{
				std::string substring = model.substr(model.length() - 4);
				if (substring == ".obj")
					mesh = this->zEng->CreateStaticMesh(model.c_str(), Vector3());
				else if (substring == ".fbx")
					mesh = this->zEng->CreateFBXMesh(model.c_str(), Vector3());
				else if (substring == ".ani")
					mesh = this->zEng->CreateFBXMesh(model.c_str(), Vector3());
			}
			
			if(mesh)
			{
				actor = new Actor(ID);

				actor->SetStaticMesh(mesh);
				actor->SetModel(model);

				if (Messages::FileWrite())
					Messages::Debug("Actor ID: " + MaloW::convertNrToString((float)ID) +" Added with Model: " + model);

				this->zActorManager->AddActor(actor);

				if (ID == this->zID)
				{
					this->zGuiManager->ResetGui();
					this->zPlayerInventory->ClearAll();

					this->zCreated = true;

					auto meshOffsetsIterator = this->zMeshCameraOffsets.find(model);
					if (meshOffsetsIterator != this->zMeshCameraOffsets.end())
						this->zMeshOffset = meshOffsetsIterator->second;
					else
						this->zMeshOffset = Vector3(0.0f, 1.0f, 0.0f);

					this->zActorManager->SetCameraOffset(this->zMeshOffset);
					this->zEng->GetCamera()->SetMesh(mesh, this->zMeshOffset, Vector3(0.0f, 0.0f, 1.0f));
					this->zEng->GetCamera()->SetPosition(mesh->GetPosition() + this->zMeshOffset);

					if (this->zActorType == GHOST)
					{
						this->zPam->ToggleMenu(); // Shows the menu and sets Show to true.
						if(this->zPam->GetShow())
							zShowCursor = true;
						else
							zShowCursor = false;
					}
				}
			}
			else
				MaloW::Debug("Failed to load mesh with Model: " + model);
		}
		else
			MaloW::Debug("Cant create a new Actor. ID: " + MaloW::convertNrToString((float)ID) + " already exists");
	}

	Vector3 position;
	auto it_actorPosition_end = NAP->actorPosition.end();
	for (auto it = NAP->actorPosition.begin(); it != it_actorPosition_end; it++)
	{
		ID = it->first;
		position = it->second;

		actor = this->zActorManager->GetActor(ID);

		if (actor)
		{
			actor->SetPosition(position);

			if (this->zID == ID)
				this->zEng->GetCamera()->SetPosition(position + this->zMeshOffset);
		}
		else
			MaloW::Debug("Failed to find Actor with ID: " + MaloW::convertNrToString((float)ID));
	}

	Vector4 rotation;
	auto it_actorRotation_end = NAP->actorRotation.end();
	for (auto it = NAP->actorRotation.begin(); it != it_actorRotation_end; it++)
	{
		ID = it->first;
		rotation = it->second;

		actor = this->zActorManager->GetActor(ID);
		if (actor)
		{
			if (actor->GetMesh())
				actor->SetRotation(rotation);
			else
				MaloW::Debug("Failed to find Mesh for Actor with ID: " + MaloW::convertNrToString((float)ID));
		}
		else
			MaloW::Debug("Failed to find Actor with ID: " + MaloW::convertNrToString((float)ID));
	}

	Vector3 scale;
	auto it_actorScale_end = NAP->actorScale.end();
	for (auto it = NAP->actorScale.begin(); it != it_actorScale_end; it++)
	{
		ID = it->first;
		scale = it->second;

		actor = this->zActorManager->GetActor(ID);
		if (actor)
		{
			if (actor->GetMesh())
				actor->SetScale(scale);
			else
				MaloW::Debug("Failed to find Mesh for Actor with ID: " + MaloW::convertNrToString((float)ID));
		}
		else
			MaloW::Debug("Failed to find Actor with ID: " + MaloW::convertNrToString((float)ID));
	}

	unsigned int state;
	auto it_actorState_end = NAP->actorState.end();
	for (auto it = NAP->actorState.begin(); it != it_actorState_end; it++)
	{
		ID = it->first;
		state = it->second;

		actor = this->zActorManager->GetActor(ID);
		if (actor)
			this->zActorManager->AddActorState(actor, state);
		else
			MaloW::Debug("Failed to find Actor with ID: " + MaloW::convertNrToString((float)ID));
	}
 	if (!this->zReady)
	{		
		this->zEng->DeleteImage(this->zBlackImage);
		this->zBlackImage = NULL;

		this->zEng->LoadingScreen("Media/LoadingScreen/LoadingScreenBG.png", "Media/LoadingScreen/LoadingScreenPB.png", 0.0f, 1.0f, 0.2f, 0.2f);

		this->zEng->DeleteText(this->zConnectingText);
		this->zConnectingText = NULL;

		this->zReady = true;

		//this->zServerChannel->Send(this->zMsgHandler.Convert(MESSAGE_TYPE_PLAYER_READY));
 	}
}

void Client::UpdateActors(ServerFramePacket* SFP)
{
	Updates* update = NULL;
	Actor* actor = NULL;
	unsigned int ID = 0;

	Vector3 actorPosition;
	auto it_position_end = SFP->newPositions.end();
	for(auto positionIterator = SFP->newPositions.begin(); positionIterator != it_position_end; positionIterator++)
	{
		ID = positionIterator->first;
		actorPosition = positionIterator->second;
		actor = this->zActorManager->GetActor(ID);

		update = this->zActorManager->GetUpdate(ID);
		if (update)
		{
			update->SetPosition(actorPosition);
		}
		else
		{
			update = new Updates(ID);
			update->SetPosition(actorPosition);
			this->zActorManager->AddUpdate(update);
		}

	}

	Vector4 actorRotation;
	auto it_rotation_end = SFP->newRotations.end();
	for(auto rotationIterator = SFP->newRotations.begin(); rotationIterator != it_rotation_end; rotationIterator++)
	{
		ID = rotationIterator->first;
		actorRotation = rotationIterator->second;

		actor = this->zActorManager->GetActor(ID);
		if (actor && ID != this->zID)
			actor->SetRotation(actorRotation);
	}

	Vector3 actorScale;
	auto it_scale_end = SFP->newScales.end();
	for(auto scaleIterator = SFP->newScales.begin(); scaleIterator != it_scale_end; scaleIterator++)
	{
		ID = scaleIterator->first;
		actorScale = scaleIterator->second;

		actor = this->zActorManager->GetActor(ID);
		if (actor)
			actor->SetScale(actorScale);
	}

	unsigned int actorState;
	auto it_state_end = SFP->newStates.end();
	for (auto stateIterator = SFP->newStates.begin(); stateIterator != it_state_end; stateIterator++)
	{
		ID = stateIterator->first;
		actorState = stateIterator->second;

		actor = this->zActorManager->GetActor(ID);

		update = this->zActorManager->GetUpdate(ID);
		if (update)
		{
			update->SetState(actorState);
		}
		else
		{
			update = new Updates(ID);
			update->SetState(actorState);
			this->zActorManager->AddUpdate(update);
		}
		if (actor)
		{
			if (this->zID == ID)
				this->UpdateCameraOffset(actorState);
		}
	}

	auto it_animation_end = SFP->newAnimations.end();
	for (auto animIterator = SFP->newAnimations.begin(); animIterator != it_animation_end; animIterator++)
	{
		if (this->zReady)
		{
			unsigned int id = animIterator->first;
			std::string animationName = animIterator->second;

			Actor* actor = this->zActorManager->GetActor(id);
			if (actor)
			{
				iFBXMesh* mesh = dynamic_cast<iFBXMesh*>(actor->GetMesh());
				if (mesh)
				{
					std::string model = actor->GetModel();
					auto it = this->zModelToReaderMap.find(model);
					if (it != this->zModelToReaderMap.end())
					{
						std::string animation = it->second.GetAnimation(animationName);

						if (animation != "")
							mesh->SetAnimation(animation.c_str());
					}
				}
			}
		}
	}
}

bool Client::RemoveActor(const unsigned int ID)
{
	if (ID == -1)
		return false;

	Actor* actor = this->zActorManager->GetActor(ID);

	//Check if object was found in the array
	if(!actor)
		return false;

	if(this->zID == ID)
	{
		this->CloseConnection("Unknown reason possible Kicked");
	}

	iMesh* mesh = actor->GetMesh();

	if(mesh)
	{
		
		this->zEng->DeleteMesh(mesh);
	}

	this->zActorManager->RemoveActor(ID);

	return true;
}
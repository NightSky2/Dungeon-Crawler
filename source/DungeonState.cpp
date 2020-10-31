#include "DungeonState.h"

#include "DebugMacro.h"

DungeonState::DungeonState(irr::IrrlichtDevice &device, SoundManager &soundManager, const std::string &dungeonMapPath)
	: dungeonMap(device, dungeonMapPath), soundManager(soundManager)
{
	nextState = GameStateType::DungeonState;
	finished = false;
	this->device = &device;

	eventReceiver = nullptr;

	camera = nullptr;

	keybinds = Keybinds();
}

void DungeonState::initializeScene()
{
	using namespace irr;
	using namespace scene;
	using namespace core;
	using namespace video;

	// set the mouse cursor invisible in this scene
	device->getCursorControl()->setVisible(false);
	
	// initialize the dungeon map
	dungeonMap.initializeScene();

	// play the background music
	soundManager.playMusic(MusicType::bgm);

	// set the event receiver
	eventReceiver = new MyEventReceiver(*this, *device);
	device->setEventReceiver(eventReceiver);

	// setup the camera
	camera = this->device->getSceneManager()->getActiveCamera();
	camera = device->getSceneManager()->addCameraSceneNode();
	camera->setPosition(dungeonMap.getStartPosition());

	// set the camera target
	vector3df initialTarget;

	Directions::Value initialDirection = dungeonMap.getStartFacing();

	camera->setTarget(camera->getPosition() + directions.getVector(initialDirection));

	// initial player state
	playerState = PlayerState::standing;
	playerFacing = dungeonMap.getStartFacing();
	radiansRotated = 0;
	moveProgress = 0;
	wallBumped = false;
}


void DungeonState::update(float deltaTime)
{
	nextState = type();

	handleInput(deltaTime);

	// update the player/camera
	switch (playerState)
	{
	case PlayerState::standing:
		break;

	case PlayerState::turningLeft:
		turnLeft(dungeonMap.getTurnSpeed(), deltaTime);
		break;

	case PlayerState::turningRight:
		turnRight(dungeonMap.getTurnSpeed(), deltaTime);
		break;

	case PlayerState::movingForward:
		move(dungeonMap.getMoveSpeed(), deltaTime, playerFacing);
		break;

	case PlayerState::movingBackward:
		move(dungeonMap.getMoveSpeed(), deltaTime, directions.getReverse(playerFacing));
		break;

	case PlayerState::wallBumpForward:
		// TODO don't harcode the distance
		wallBump(dungeonMap.getBumpDistance(), dungeonMap.getBumpSpeed(), deltaTime, playerFacing);
		break;

	case PlayerState::wallBumpBackward:
		// TODO don't harcode the distance
		wallBump(dungeonMap.getBumpDistance(), dungeonMap.getBumpSpeed(), deltaTime, directions.getReverse(playerFacing));
		break;
	}
}

void DungeonState::cleanup()
{

}


GameStateType DungeonState::type()
{
	return GameStateType::DungeonState;
}


void DungeonState::preRender()
{

}

// camera functions ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DungeonState::translateCamera(irr::core::vector3df displacement)
{
	camera->setPosition(camera->getPosition() + displacement);
	camera->setTarget(camera->getTarget() + displacement);
}

void DungeonState::rotateCamera(float angle)
{
	float currentAngle = getCameraAngle();

	angle += currentAngle;

	camera->setTarget(camera->getPosition() + irr::core::vector3df({cosf(angle), 0, sinf(angle)}));
}

float DungeonState::getCameraAngle()
{
	return atan2f(camera->getTarget().Z - camera->getPosition().Z, camera->getTarget().X - camera->getPosition().X);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DungeonState::handleInput(float deltaTime)
{
	using namespace irr;

	// TODO add some sort of action buffer or stack so that keys hit during player actions aren't totally ignored

	if (eventReceiver->IsKeyDown(keybinds.turnLeft))
	{
		if (playerState == PlayerState::standing)
		{
			soundManager.makeSound(SoundType::turn);
			playerState = PlayerState::turningLeft;
		}
	}
	if (eventReceiver->IsKeyDown(keybinds.turnRight))
	{
		if (playerState == PlayerState::standing)
		{
			soundManager.makeSound(SoundType::turn);
			playerState = PlayerState::turningRight;
		}
	}
	if (eventReceiver->IsKeyDown(keybinds.moveForward) && playerState == PlayerState::standing)
	{
		if (isMoveValid(playerFacing))
		{
			soundManager.makeSound(SoundType::move);
			playerState = PlayerState::movingForward;
		}
		else
		{
			playerState = PlayerState::wallBumpForward;
			soundManager.makeSound(SoundType::wallBump);
		}
	}
	if (eventReceiver->IsKeyDown(keybinds.moveBackward) && playerState == PlayerState::standing)
	{
		if (isMoveValid(directions.getReverse(playerFacing)))
		{
			soundManager.makeSound(SoundType::move);
			playerState = PlayerState::movingBackward;
		}
		else
		{
			playerState = PlayerState::wallBumpBackward;
			soundManager.makeSound(SoundType::wallBump);
		}
	}
	if (eventReceiver->IsKeyDown(keybinds.exitGame))
	{
		device->closeDevice();
	}

}

// player action functions /////////////////////////////////////////////////////////////

void DungeonState::move(float speed, float deltaTime, Directions::Value direction)
{
	irr::core::vector3df vec = directions.getVector(direction);
	vec.normalize();
	vec = vec * dungeonMap.getScaling() * (deltaTime / speed);

	if (vec.getLength() + moveProgress >= dungeonMap.getScaling())
	{
		// were done moving
		vec = directions.getVector(direction).normalize();
		vec = vec * (dungeonMap.getScaling() - moveProgress);

		translateCamera(vec);
		playerState = PlayerState::standing;
		moveProgress = 0;
	}
	else
	{
		translateCamera(vec);
		moveProgress += vec.getLength();
	}

}

bool DungeonState::isMoveValid(Directions::Value direction)
{
	irr::core::vector3df vec = directions.getVector(direction);

	vec = vec * dungeonMap.getScaling();

	translateCamera(vec);

	// if the player is trying to move into a wall
	if (!dungeonMap.isWallsPositionEmpty( {camera->getPosition().X, camera->getPosition().Z} ))
	{
		DEBUG_MODE(printf("there's a wall in the way\n"));
		// make sure to fix the camera first
		translateCamera(-vec); 
		return false;
	}
	else
	{
		// make sure to fix the camera first
		translateCamera(-vec);
		return true;
	}
}

void DungeonState::turnLeft(float speed, float deltaTime)
{
	float angle = irr::core::HALF_PI * (deltaTime / speed);

	if (radiansRotated + angle >= irr::core::HALF_PI)
	{
		// we're done turning

		// update the player facing and the camera target

		playerFacing = directions.getLeft(playerFacing);

		irr::core::vector3df target = directions.getVector(playerFacing);

		camera->setTarget(camera->getPosition() + target);

		playerState = PlayerState::standing;

		radiansRotated = 0; 
	}
	else
	{
		rotateCamera(angle);
		radiansRotated += angle;
	}
}

void DungeonState::turnRight(float speed, float deltaTime)
{
	float angle = irr::core::HALF_PI * (deltaTime / speed);

	if (radiansRotated + angle >= irr::core::HALF_PI)
	{
		// we're done turning

		// update the player facing and the camera target

		playerFacing = directions.getRight(playerFacing);

		irr::core::vector3df target = directions.getVector(playerFacing);
		
		camera->setTarget(camera->getPosition() + target);

		playerState = PlayerState::standing;

		radiansRotated = 0; 
	}
	else
	{
		rotateCamera(-angle);
		radiansRotated += angle;
	}

}

void DungeonState::wallBump(float distance, float speed, float deltaTime, Directions::Value direction)
{
	irr::core::vector3df vec = directions.getVector(direction);
	vec.normalize();
	vec = vec * distance * (deltaTime / speed);

	if (!wallBumped)
	{
		if (vec.getLength() + moveProgress >= distance)
		{
			// were done moving forward
			vec = directions.getVector(direction).normalize();
			vec = vec * (distance - moveProgress);

			translateCamera(vec);
			moveProgress = 0;

			wallBumped = true;
		}
		else
		{
			translateCamera(vec);
			moveProgress += vec.getLength();
		}
	}
	if (wallBumped)
	{
		if (vec.getLength() + moveProgress >= distance)
		{
			// were done moving backward
			vec = directions.getVector(direction).normalize();
			vec = vec * (distance - moveProgress);

			translateCamera(-vec);
			playerState = PlayerState::standing;
			moveProgress = 0;
			wallBumped = false;;

			std::cout << camera->getPosition().X << ", " << camera->getPosition().Z << std::endl;
		}
		else
		{
			translateCamera(-vec);
			moveProgress += vec.getLength();
		}
	}
}

// MyEventReceiver functions ////////////////////////////////////////////////////////////

DungeonState::MyEventReceiver::MyEventReceiver(DungeonState &parent, irr::IrrlichtDevice &device)
	: parent(parent), device(&device)
{
	for (irr::u32 i = 0; i < irr::KEY_KEY_CODES_COUNT; ++i)
	{
		keyIsDown[i] = false;
	}
}

bool DungeonState::MyEventReceiver::OnEvent(const irr::SEvent &event)
{
	using namespace irr;
	using namespace io;

	if (event.EventType == EET_KEY_INPUT_EVENT)
	{
		keyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
	}

	return false;
}

bool DungeonState::MyEventReceiver::IsKeyDown(irr::EKEY_CODE keyCode) const
{
	return keyIsDown[keyCode];
}

/////////////////////////////////////////////////////////////////////////////////////////

// Keybinds functions ///////////////////////////////////////////////////////////////////

DungeonState::Keybinds::Keybinds()
{
	using namespace irr;

	// TODO load keybinds from a settings file, don't hardcode them
	moveForward = KEY_UP;
	moveBackward = KEY_DOWN;
	turnLeft = KEY_LEFT;
	turnRight = KEY_RIGHT;
	exitGame = KEY_ESCAPE;
}

/////////////////////////////////////////////////////////////////////////////////////////
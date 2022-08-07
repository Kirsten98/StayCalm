// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StayCalmGameMode.h"
#include "StayCalmHUD.h"
#include "StayCalmCharacter.h"
#include "UObject/ConstructorHelpers.h"

AStayCalmGameMode::AStayCalmGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter_Nadia"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AStayCalmHUD::StaticClass();
}

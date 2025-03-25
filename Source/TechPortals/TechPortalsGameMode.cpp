// Copyright Epic Games, Inc. All Rights Reserved.

#include "TechPortalsGameMode.h"
#include "TechPortalsCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATechPortalsGameMode::ATechPortalsGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

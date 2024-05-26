// Copyright Epic Games, Inc. All Rights Reserved.

#include "GlassFractureGameMode.h"
#include "GlassFractureCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGlassFractureGameMode::AGlassFractureGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

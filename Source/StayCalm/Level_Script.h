// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Level_Script.generated.h"

/**
 * 
 */
UCLASS()
class STAYCALM_API ULevel_Script : public ULevelScriptBlueprint
{
	GENERATED_BODY()
	
protected:

	virtual void BeginPlay();
};

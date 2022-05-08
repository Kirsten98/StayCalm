// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "PanicProcessVolume.generated.h"

/**
 * 
 */
UCLASS()
class STAYCALM_API APanicProcessVolume : public APostProcessVolume
{
	GENERATED_BODY()
protected:

	UFUNCTION(BlueprintImplementableEvent)
	void updateCameraBlur(int level);

	
};

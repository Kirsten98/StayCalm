// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "StayCalmCameraComponent.generated.h"

/**
 * 
 */
UCLASS()
class STAYCALM_API UStayCalmCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

		//Create methods to update camera settings

		void updateBlurLevel(int level);
};

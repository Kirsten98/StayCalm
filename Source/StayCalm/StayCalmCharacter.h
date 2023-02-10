// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PanicTrigger.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StayCalmCharacter.generated.h"


class UInputComponent;

UCLASS(config=Game)
class AStayCalmCharacter : public ACharacter
{
	GENERATED_BODY()

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Panic, meta = (AllowPrivateAccess = "true"))
	class UPostProcessComponent* PanicPostProcess;
public:
	AStayCalmCharacter();

protected:
	virtual void BeginPlay();

	void Tick(float DeltaTime);

	int panicLevel = 0;

	//Constant level variables for the different stages of movement time delay for movement_time_delay
	const float level_one_movement_time_delay = .5f;
	const float level_two_movement_time_delay = .75f;
	const float level_three_movement_time_delay = 1.0f;
	//The delay that the character experiences during panic. Default = 0.0, Level 1 = .5 , Level 2 = .75, Level 3 = 1
	UPROPERTY ()
		float movement_time_delay = 0.0f;


	//Constant level variables for the different stages of movement slowed speed for movement_speed
	const float level_one_movement_speed = 1.5f;
	const float level_two_movement_speed = 2.0f;
	const float level_three_movement_speed = 3.0f;
	//This is the denominator for the movement speed 1. When set to 1 the movement is 1/1 and when set to 2 the speed is 1/2 etc. Default = 1.0, Level 1 = 1.5 , Level 2 = 2.0, Level 3 = 3
	UPROPERTY()
		float movement_speed = 1.0f;

	void setMovementTimeDelay(float time_delay);

	//Timer handle for Delayed movement
	FTimerHandle ftimer_movement_delay;

	enum e_movement_direction {
		FORWARD,
		RIGHT,
		LOOKUP,
		LOOKRIGHT,
	};


	struct movement
	{
		int direction;
		float value;
	};

	void executeDelayedMovement();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Panic)
		class UAudioComponent* HeartBeatAudioCue;		

	void playPanicHeartBeat(float level);
	void stopPlayingPanicHeartBeat();
	void startPanic(int level);
	void stopPanic();

	TQueue<movement> *q_movement_input = new TQueue<movement>;

	TArray<APanicTrigger*> found_triggers;

	void addAllPanicTriggers();

	void panicLineTrace();

	//Updates intesity of the blur a user will experience. Level 0 - No Blur, Level 3 Max Blur
	UFUNCTION(BlueprintImplementableEvent, Category=Panic)
		void updatePanicBlur(int level);

	//Updates intesity of the depth perception change user will experience. Level 0 - No change, Level 3 Max distance the camera will view.
	UFUNCTION(BlueprintImplementableEvent, Category = Panic)
		void updateDepthPerception(int level);

						

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint32 bUsingMotionControllers : 1;

	

protected:
	

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/** Handles camera movement, left and right */
	void LookRight(float Value);

	/** Handles camera up/down */
	void LookUp(float Val);
	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:

	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};


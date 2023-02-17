// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StayCalmCharacter.h"
#include "StayCalmProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "PanicProcessVolume.h"
#include "Components/PostProcessComponent.h"
#include "Components/AudioComponent.h"
#include "DrawDebugHelpers.h"


DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AStayCalmCharacter

AStayCalmCharacter::AStayCalmCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;


	PanicPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PanicPostProcess"));
	PanicPostProcess->SetupAttachment(GetCapsuleComponent());

	HeartBeatAudioCue = CreateDefaultSubobject<UAudioComponent>(TEXT("HeartBeatAudio"));

}

void AStayCalmCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Stops the heartbeat cue from playing
	stopPlayingPanicHeartBeat();

	//Retrieves a list of all of the panic triggers
	addAllPanicTriggers();

	//Activates the first Panic Trigger
	UE_LOG(LogTemp, Warning, TEXT("Found All Triggers %d"), found_triggers.Num());
	if (found_triggers.Num() >= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Activated First Trigger"));
		found_triggers[0]->set_is_visible(true);
		found_triggers[0]->set_panic_trigger_active(true);
		found_triggers.RemoveAt(0);
	}
	
	if (BP_PauseWidgetMenu != nullptr)
	{
		PauseMenu = CreateWidget<UPauseMenuWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0), BP_PauseWidgetMenu);
	}

	
}

void AStayCalmCharacter::Tick(float DeltaTime)
{
	panicLineTrace();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AStayCalmCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AStayCalmCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AStayCalmCharacter::MoveRight);

	//Bind Pause Menu HUD
	PlayerInputComponent->BindAction("Pause", IE_Pressed,this, &AStayCalmCharacter::Pause_Game);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AStayCalmCharacter::LookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AStayCalmCharacter::LookUp);
}


void AStayCalmCharacter::executeDelayedMovement()
{
	//UE_LOG(LogTemp, Warning, TEXT("Execute Delayed Movement"));
	if (q_movement_input->Peek() != nullptr)
	{
		movement *this_action = q_movement_input->Peek();
		float value = this_action->value;

		if (this_action->direction == e_movement_direction::FORWARD)
		{
			AddMovementInput(GetActorForwardVector(), value);
			q_movement_input->Pop();

			//Checks if the character is strafing
			if (q_movement_input->Peek() != nullptr && q_movement_input->Peek()->direction == e_movement_direction::RIGHT) {
				AddMovementInput(GetActorRightVector(), q_movement_input->Peek()->value);
				q_movement_input->Pop();
			}
			//UE_LOG(LogTemp, Warning, TEXT("Moved Forward: %f"), value);
		}
		else if (this_action->direction == e_movement_direction::RIGHT)
		{
			AddMovementInput(GetActorRightVector(), this_action->value);
			q_movement_input->Pop();
			//UE_LOG(LogTemp, Warning, TEXT("Moved Right: %f"), value);
			
			//Checks if the character is strafing
			if (q_movement_input->Peek() != nullptr && q_movement_input->Peek()->direction == e_movement_direction::FORWARD) {
				AddMovementInput(GetActorForwardVector(), q_movement_input->Peek()->value);
				q_movement_input->Pop();
			}
		}
		/*
		else if (this_action->direction == e_movement_direction::LOOKUP)
		{
			AddControllerPitchInput(value);
			q_movement_input->Pop();
			//UE_LOG(LogTemp, Warning, TEXT("Look Up: %d"), value);
			
		}
		else if (this_action->direction == e_movement_direction::LOOKRIGHT)
		{
			AddControllerYawInput(value);
			q_movement_input->Pop();
			//UE_LOG(LogTemp, Warning, TEXT("Look Right: %d"), value);
			
		}
		*/
		GetWorldTimerManager().SetTimer(ftimer_movement_delay, this, &AStayCalmCharacter::executeDelayedMovement, movement_time_delay, true, 0);
	}
}




void AStayCalmCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		if (movement_time_delay > 0) {
			movement characterMovement;
			characterMovement.direction = e_movement_direction::FORWARD;
			characterMovement.value = Value / movement_speed;
			q_movement_input->Enqueue(characterMovement);


			if (!GetWorldTimerManager().IsTimerActive(ftimer_movement_delay))
			{
				AddMovementInput(GetActorForwardVector(), Value / movement_speed);
				executeDelayedMovement();
			}
		}
		else {
			AddMovementInput(GetActorForwardVector(), Value/ movement_speed);
		}
		
	}
}

void AStayCalmCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		if (movement_time_delay > 0) {
			movement characterMovement;
			characterMovement.direction = e_movement_direction::RIGHT;
			characterMovement.value = Value / movement_speed;
			q_movement_input->Enqueue(characterMovement);


			if (!GetWorldTimerManager().IsTimerActive(ftimer_movement_delay))
			{
				AddMovementInput(GetActorRightVector(), Value / movement_speed);
				executeDelayedMovement();
			}
		}
		else {
			AddMovementInput(GetActorRightVector(), Value / movement_speed);
		}
	}
}

void AStayCalmCharacter::LookRight(float Value) 
{
	/*
	if (Value != 0.0f) {
		if (movement_speed > 0) {
			movement characterMovement;
			characterMovement.direction = e_movement_direction::LOOKRIGHT;
			characterMovement.value = Value / movement_speed;
			q_movement_input->Enqueue(characterMovement);


			if (!GetWorldTimerManager().IsTimerActive(ftimer_movement_delay))
			{
				AddControllerYawInput(Value / movement_speed);
				executeDelayedMovement();
			}
		}
		else {
			AddControllerYawInput(Value / movement_speed);
		}
	}
	*/
	AddControllerYawInput(Value / movement_speed);
	
}

void AStayCalmCharacter::LookUp(float Value) 
{
	/*
	if (Value != 0.0f) {

		if (movement_speed > 0) {
			movement characterMovement;
			characterMovement.direction = e_movement_direction::LOOKUP;
			characterMovement.value = Value / movement_speed;
			q_movement_input->Enqueue(characterMovement);


			if (!GetWorldTimerManager().IsTimerActive(ftimer_movement_delay))
			{
				AddControllerPitchInput(Value / movement_speed);
				executeDelayedMovement();
			}
		}
		else {
			AddControllerPitchInput(Value / movement_speed);
		}
	}
	*/
	AddControllerPitchInput(Value / movement_speed);
	
}

void AStayCalmCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	
}

void AStayCalmCharacter::LookUpAtRate(float Rate)
{

	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	
	
}

void AStayCalmCharacter::Pause_Game()
{
	UE_LOG(LogTemp, Warning, TEXT("Paused Game"));
	if (PauseMenu != nullptr)
	{

		UE_LOG(LogTemp, Warning, TEXT("Paused Game - Pause Menu Valid"));
		if (!PauseMenu->get_is_paused())
		{
			UE_LOG(LogTemp, Warning, TEXT("Paused Game - Pause Menu is not Paused. Showing Pause Screen"));
			// Disable the character movement but do not disable mouse movement
			//movement_speed = 0.0f;
			APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			playerController->SetInputMode(FInputModeUIOnly());

			// Show Pause Menu Screen
			PauseMenu->AddToViewport();
			PauseMenu->set_is_paused(true);
			playerController->SetShowMouseCursor(true);

		}
		else
		{
			// Enable the character
			//startPanic(panicLevel);
			APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			playerController->SetInputMode(FInputModeGameOnly());
			playerController->SetShowMouseCursor(false);

			// Remove Pause Menu Screen
			PauseMenu->RemoveFromParent();
			PauseMenu->set_is_paused(false);
		}
	}
}

//Sets the delay in ms for the characters input to the parameter. 
void AStayCalmCharacter::setMovementTimeDelay(float time_delay) 
{
	movement_time_delay = time_delay;
}

void AStayCalmCharacter::playPanicHeartBeat(float level)
{
	if (HeartBeatAudioCue != nullptr) 
	{
		HeartBeatAudioCue->SetVolumeMultiplier(level + 0.0);
		HeartBeatAudioCue->Play();
		
	}
	
}

void AStayCalmCharacter::stopPlayingPanicHeartBeat()
{
	if (HeartBeatAudioCue != nullptr)
	{
		HeartBeatAudioCue->Stop();
	}

}


void AStayCalmCharacter::startPanic(int level)
{
	//Stops any panic symptoms before starting the next level
	stopPanic();
	panicLevel = level;

	switch(level) {

	case 1 :
		updatePanicBlur(1);
		playPanicHeartBeat(.5);
		UE_LOG(LogTemp, Warning, TEXT("Panic Level 1"));
		break;
	case 2 :
		updatePanicBlur(1);
		playPanicHeartBeat(.5);
		updateDepthPerception(1);
		setMovementTimeDelay(level_one_movement_time_delay);
		movement_speed = level_one_movement_speed;
		UE_LOG(LogTemp, Warning, TEXT("Panic Level 2"));
		break;
	case 3:
		updatePanicBlur(2);
		playPanicHeartBeat(1.5);
		updateDepthPerception(2);
		setMovementTimeDelay(level_two_movement_time_delay);
		movement_speed = level_two_movement_speed;
		UE_LOG(LogTemp, Warning, TEXT("Panic Level 3"));
		break;
	case 4:
		updatePanicBlur(3);
		playPanicHeartBeat(2.0);
		updateDepthPerception(2);
		setMovementTimeDelay(level_two_movement_time_delay);
		movement_speed = level_two_movement_speed;
		UE_LOG(LogTemp, Warning, TEXT("Panic Level 4"));
		break;

	case 5:
		updatePanicBlur(3);
		playPanicHeartBeat(3.0);
		updateDepthPerception(3);
		setMovementTimeDelay(level_three_movement_time_delay);
		movement_speed = level_three_movement_speed;
		UE_LOG(LogTemp, Warning, TEXT("Panic Level 5"));
		break;
	default:
		stopPanic();

	}
}

/*
* Resets all panic symtoms
*/
void AStayCalmCharacter::stopPanic()
{
		
	UE_LOG(LogTemp, Warning, TEXT("Stop Panic"));
	updatePanicBlur(0);
	stopPlayingPanicHeartBeat();
	updateDepthPerception(0);
	movement_speed = 1.0;
	setMovementTimeDelay(0.0);

}

void AStayCalmCharacter::addAllPanicTriggers()
{
	
	if (GetWorld() != nullptr) 
	{
		
		TArray<AActor*> found_actors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APanicTrigger::StaticClass(), found_actors);
		
		for (int actor = 0; actor < found_actors.Num(); actor++)
		{
			found_triggers.Push(Cast<APanicTrigger>(found_actors[actor]));
			//UE_LOG(LogTemp, Warning, TEXT("Found %d triggers"), found_triggers[actor]->get_panic_level());
		}
		
		found_triggers.Sort([](const APanicTrigger& a, const APanicTrigger& b) { return a.panic_level < b.panic_level; });
		
	}
	
}

void AStayCalmCharacter::panicLineTrace()
{
	UWorld *world = GetWorld();
	
	if (world)
	{
		//Main sight
		FHitResult hit_result;
		FVector start = GetActorLocation();
		FVector end = start + (UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetActorForwardVector() * 500);
		FCollisionObjectQueryParams parameters;
		parameters.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel3);
		parameters.AddObjectTypesToQuery(ECollisionChannel::ECC_Visibility);

		//Peripherial

		FHitResult peripherial_hit_result;
		
		//Left Peripherial check
		FVector end_left = start + (UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetActorForwardVector().RotateAngleAxis(35, FVector(0, 0, 1)) * 1000);

		//Right Peripherial check
		FVector end_right = start + (UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetActorForwardVector().RotateAngleAxis(-35, FVector(0, 0, 1)) * 1000);
		
		FCollisionObjectQueryParams peripherial_parameters;
		peripherial_parameters.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel2);
		peripherial_parameters.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);

		FCollisionQueryParams query_params;
		query_params.AddIgnoredActor(this);

		DrawDebugLine(world, start, end, FColor::Red);
		DrawDebugLine(world, start, end_left, FColor::Emerald);
		DrawDebugLine(world, start, end_right, FColor::Emerald);

		//Check Left Peripherial
		if (world->LineTraceSingleByObjectType(peripherial_hit_result, start, end_left, peripherial_parameters, query_params) ||
			world->LineTraceSingleByObjectType(peripherial_hit_result, start, end_right, peripherial_parameters, query_params))
		{

			APanicTrigger* trigger = Cast<APanicTrigger>(peripherial_hit_result.GetActor());
			UE_LOG(LogTemp, Warning, TEXT("Found Peripherial Trigger"));

			if (trigger != nullptr && trigger->get_is_visible())
			{

				UE_LOG(LogTemp, Warning, TEXT("Peripherial Trigger is visible"));
				if (trigger->get_panic_trigger_active())
				{
					UE_LOG(LogTemp, Warning, TEXT("Peripherial Trigger is active"));
					startPanic(trigger->get_panic_level());
					trigger->trigger_event();

					//Activates the next trigger and removes it from the found triggers array.
					if (found_triggers.Num() >= 1)
					{
						UE_LOG(LogTemp, Warning, TEXT("Activated next trigger. Triggers left %d"), found_triggers.Num());
						found_triggers[0]->set_is_visible(true);
						found_triggers[0]->set_panic_trigger_active(true);
						found_triggers.RemoveAt(0);
					}
					
				}

			}

		}
		
		if (world->LineTraceSingleByObjectType(hit_result, start, end, parameters, query_params))
		{
			
			APanicTrigger* trigger = Cast<APanicTrigger>(hit_result.GetActor());
			UE_LOG(LogTemp, Warning, TEXT("Found Trigger"));

			if (trigger != nullptr && trigger->get_is_visible())
			{
				 
				UE_LOG(LogTemp, Warning, TEXT("Trigger is visible"));
				if (trigger->get_panic_trigger_active())
				{
					UE_LOG(LogTemp, Warning, TEXT("Trigger is active"));
					startPanic(trigger->get_panic_level());
					trigger->trigger_event();

					if (found_triggers.Num() >= 1)
					{
						//Activates the next trigger and removes it from the found triggers array.
						UE_LOG(LogTemp, Warning, TEXT("Activated next trigger. Triggers left %d"), found_triggers.Num());
						found_triggers[0]->set_is_visible(true);
						found_triggers[0]->set_panic_trigger_active(true);
						found_triggers.RemoveAt(0);
					}
					
					
					
					
				}
				
			}
			
		}
		
		
	}
}
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

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void AStayCalmCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	
	//Stops the heartbeat cue from playing
	stopPlayingPanicHeartBeat();

	//Retrieves a list of all of the panic triggers
	addAllPanicTriggers();

	//Activates the first Panic Trigger
	UE_LOG(LogTemp, Warning, TEXT("Found All Triggers %d"), found_triggers.Num());
	if (found_triggers.Num() >= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Activated First Trigger"));
		found_triggers.Pop()->set_panic_trigger_active(true);
	}
	
	
	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
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

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AStayCalmCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AStayCalmCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AStayCalmCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &AStayCalmCharacter::LookRight);
	PlayerInputComponent->BindAxis("LookUp", this, &AStayCalmCharacter::LookUp);
}

void AStayCalmCharacter::OnFire()
{

	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<AStayCalmProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<AStayCalmProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		} 
	}
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
		
		found_triggers.Sort([](const APanicTrigger& a, const APanicTrigger& b) { return a.panic_level > b.panic_level; });
		
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
						found_triggers.Pop()->set_panic_trigger_active(true);
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
						found_triggers.Pop()->set_panic_trigger_active(true);
					}
					
					
					
					
				}
				
			}
			
		}
		
		
	}
}
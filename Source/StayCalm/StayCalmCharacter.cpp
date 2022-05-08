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
#include "PanicProcessVolume.h"
#include "Components/PostProcessComponent.h"


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
	updatePanicBlur(0);
	//updateCameraBlur(4);
	//PanicPostProcessVolume->updateCameraBlur(4);
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

	setCameraBlurLevel(4);
}

void AStayCalmCharacter::setCameraBlurLevel(int level)
{
	UE_LOG(LogTemp, Warning, TEXT("Blur"));
	//updateCameraBlur(level);
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

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


}

void AStayCalmCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

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

//Sets the delay in ms for the characters input to the parameter. 
void AStayCalmCharacter::setMovementTimeDelay(float time_delay) 
{
	movement_time_delay = time_delay;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "PanicTrigger.h"
#include "Components/SceneComponent.h"

// Sets default values
APanicTrigger::APanicTrigger()
{
	trigger_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trigger Mesh"));
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APanicTrigger::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APanicTrigger::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


bool APanicTrigger::get_is_visible() 
{
	return is_visible;
}

void APanicTrigger::set_is_visible(bool visible) 
{
	is_visible = visible;
	// Hides visible components
	SetActorHiddenInGame(!visible);

	// Disables collision components
	SetActorEnableCollision(visible);

	// Stops the Actor from ticking
	SetActorTickEnabled(visible);
}


bool APanicTrigger::get_panic_trigger_active()
{
	return panic_trigger_active;
}


void APanicTrigger::set_panic_trigger_active(bool active) {
	panic_trigger_active = active;
}

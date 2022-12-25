// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/CapsuleComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PanicTrigger.generated.h"

UCLASS()
class STAYCALM_API APanicTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APanicTrigger();

	//Static Mesh for Trigger
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = mesh)
		class UStaticMeshComponent* trigger_mesh;

	//Material to display when the trigger is active
	UPROPERTY(EditAnywhere)
		class UMaterial* on_material;

	//Material to display when the trigger is inactive
	UPROPERTY(EditAnywhere)
		class UMaterial* off_material;

	//The panic level that should be caused by this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Panic);
	int panic_level = 0;

	inline int get_panic_level(){ return panic_level; };

	UFUNCTION(BlueprintCallable)
	bool get_is_visible();

	UFUNCTION(BlueprintCallable)
	void set_is_visible(bool visible);

	UFUNCTION(BlueprintCallable)
	bool get_panic_trigger_active();

	UFUNCTION(BlueprintCallable)
	void set_panic_trigger_active(bool active);

	//Implement what should happen for each panic trigger within the blueprint. Should Return Panic Level for trigger
	UFUNCTION(BlueprintImplementableEvent, Category = Panic)
		void trigger_event();
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Used to determinem if the trigger is visible in game
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Panic)
	bool is_visible = false;

	//Used to determine if the trigger is active and can be triggered
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = Panic)
	bool panic_trigger_active = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

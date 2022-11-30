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

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = mesh)
		class UStaticMeshComponent* trigger_mesh;

	UPROPERTY(EditAnywhere)
		class UMaterial* on_material;

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
	
	//Sets the comparison between panic triggers
	friend bool operator< (const APanicTrigger& l, const APanicTrigger& r)
	{
		return l.panic_level
			< r.panic_level; // keep the same order
	}

	friend bool operator> (const APanicTrigger& l, const APanicTrigger& r)
	{
		return l.panic_level
			> r.panic_level; // keep the same order
	}

	friend bool operator<= (const APanicTrigger& l, const APanicTrigger& r)
	{
		return l.panic_level
			<= r.panic_level; // keep the same order
	}

	friend bool operator>=(const APanicTrigger& l, const APanicTrigger& r)
	{
		return l.panic_level
			>= r.panic_level; // keep the same order
	}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = Panic)
	bool is_visible = true;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = Panic)
	bool panic_trigger_active = true;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

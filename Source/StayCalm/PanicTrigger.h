// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class UStaticMeshComponent* trigger_mesh;

	//The panic level that should be caused by this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Panic);
	int panic_level = 0;
	inline int get_panic_level(){ return panic_level; };

	bool get_is_visible();
	void set_is_visible(bool visible);

	bool get_panic_trigger_active();
	void set_panic_trigger_active(bool active);

	
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

	bool is_visible = false;
	bool panic_trigger_active = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

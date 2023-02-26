// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class STAYCALM_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(BlueprintReadOnly)
	bool is_paused = false;

public:
	/**
		Sets the widget to be in a paused or not paused state. If the game is paused the widget it visible, otherwise it is removed from the parent
		@param new_is_paused - boolean value of the new state of the widget.
	**/
	void set_is_paused(bool new_is_paused);

	/**
	* Returns boolean value if the widget is in the Paused State.
	**/
	bool get_is_paused();

	/*
	* Shows the widget and disables user game input
	*/
	UFUNCTION(BlueprintCallable)
	void show();

	/*
	* Resumes the game by removing the widget from the parent and enables the user game input
	*/
	UFUNCTION(BlueprintCallable)
	void hide();

	/*
	* Redirects the user to the main menu
	*/
	void return_to_main_menu();

	/*
	* Ends the game
	*/
	void close_game();
};

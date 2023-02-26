// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"


void UPauseMenuWidget::set_is_paused(bool new_is_paused)
{
	is_paused = new_is_paused;
}

bool UPauseMenuWidget::get_is_paused()
{
	return is_paused;
}


void UPauseMenuWidget::show()
{
	//Show Pause Widget
	AddToViewport();
	set_is_paused(true);
	
	//Disables character game input
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->SetInputMode(FInputModeUIOnly());
	playerController->SetShowMouseCursor(true);
}

void UPauseMenuWidget::hide()
{
	//Enables Character Game input
	APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	playerController->SetInputMode(FInputModeGameOnly());
	playerController->SetShowMouseCursor(false);

	// Remove Pause Menu Screen
	RemoveFromParent();
	set_is_paused(false);


}

void UPauseMenuWidget::return_to_main_menu()
{

}

void UPauseMenuWidget::close_game()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), UGameplayStatics::GetPlayerController(GetWorld(), 0),EQuitPreference::Quit,false);
}

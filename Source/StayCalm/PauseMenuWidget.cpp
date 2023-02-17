// Fill out your copyright notice in the Description page of Project Settings.


#include "PauseMenuWidget.h"

void UPauseMenuWidget::set_is_paused(bool new_is_paused)
{
	is_paused = new_is_paused;
}

bool UPauseMenuWidget::get_is_paused()
{
	return is_paused;
}
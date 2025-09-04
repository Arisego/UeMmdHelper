// Fill out your copyright notice in the Description page of Project Settings.


#include "Vmd/CineCamera/VmdCineCameraComponent.h"

void UVmdCineCameraComponent::UpdateCameraLens(float DeltaTime, FMinimalViewInfo& DesiredView)
{
    Super::UpdateCameraLens(DeltaTime, DesiredView);

    CurrentDepthOfFieldSensorWidth = DesiredView.PostProcessSettings.DepthOfFieldSensorWidth;
    bOverrideSensorWidth = DesiredView.PostProcessSettings.bOverride_DepthOfFieldFocalDistance;

    if (bUseCustomSensorWidth && bOverrideSensorWidth)
    {
        DesiredView.PostProcessSettings.DepthOfFieldSensorWidth = CustomSensorWidth;
    }
}

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraComponent.h"
#include "VmdCineCameraComponent.generated.h"

/**
 * Helps for having some custom logic on cine camera
 * Custom logic will take no effect if you do not enable it
 */
UCLASS()
class UVmdCineCameraComponent : public UCineCameraComponent
{
    GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, Category="VmdCine")
    bool bOverrideSensorWidth = false;

    UPROPERTY(VisibleAnywhere, Category = "VmdCine")
    float CurrentDepthOfFieldSensorWidth = 0.0f;

    UPROPERTY(VisibleAnywhere, Category="VmdCine", meta = (InlineEditConditionToggle))
    bool bUseCustomSensorWidth = false;

    UPROPERTY(EditAnywhere, Category="VmdCine", meta = (EditCondition = bUseCustomSensorWidth))
    float CustomSensorWidth = 100.0f;
    
protected:
    virtual void UpdateCameraLens(float DeltaTime, FMinimalViewInfo& DesiredView) override;

};

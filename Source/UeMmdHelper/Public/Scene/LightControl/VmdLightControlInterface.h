// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VmdLightControlInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UVmdLightControlInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * 
 */
class UEMMDHELPER_API IVmdLightControlInterface
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    UFUNCTION(BlueprintNativeEvent, CallInEditor)
    void UpdateLightParams(const struct FVmdLightParams& InLightParams);
};

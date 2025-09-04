// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Camera/CameraTypes.h"
#include "MmdSequencerHelper.generated.h"

/**
 * 
 */
UCLASS()
class UMmdSequencerHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    /** these functions are copied from FLevelSequenceHelper::BindActorToLevelSequence, they are private and can not be useed directly */
    static FGuid BindActorToLevelSequence(class AActor* InActor, class ULevelSequence* InLevelSequence);
    static FGuid BindComponentToLevelSequence(class UActorComponent* InComponent, class ULevelSequence* InLevelSequence);

    /** Convert track camera transforms from frame data */
    static FTransform GetConvertedCameraTrans(const FTransform& InBaseTrans, const FVector& InOffset, const FRotator& InRot, const float InDistance);

    /** Convert projection mode from raw data */
    static ECameraProjectionMode::Type ConvertFromVmdCameraPerspective(const uint8 InVal);
};

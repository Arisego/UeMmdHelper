// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VmdControlRigDefines.generated.h"


USTRUCT(BlueprintType)
struct FControlRigMorphConfig
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FName MorphName;

    /**
     * Multiplier of morph value
     * Use -1.0 if the morph is inversed on two mesh
     */
    UPROPERTY(EditAnywhere)
    float MorphScale = 1.0f;
};

USTRUCT(BlueprintType)
struct FVmdControlRigMorphPair
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FName ControlName;

    UPROPERTY(EditAnywhere)
    TArray<FControlRigMorphConfig> TargetMorphs;
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct FVmdControlRigMorphConfig
{
    GENERATED_BODY()

public:
    /**
     * Control rig morph config
     * You can generate it from `UMotionDataAsset`
     */
    UPROPERTY(EditAnywhere)
    TArray<FVmdControlRigMorphPair> MorphMapConfigs;
    
};

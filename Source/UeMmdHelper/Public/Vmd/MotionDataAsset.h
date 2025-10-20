// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MotionDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FVmdCameraFrameData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    uint32	Frame;

    UPROPERTY(EditAnywhere)
    float Length;

    UPROPERTY(EditAnywhere)
    FVector	Location;

    UPROPERTY(EditAnywhere)
    FVector Rotate;

    UPROPERTY(EditAnywhere)
    uint32 ViewingAngle;

    UPROPERTY(EditAnywhere)
    uint8 Perspective;
};

USTRUCT(BlueprintType)
struct FVmdMorphFrameData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    uint32	Frame;

    UPROPERTY(EditAnywhere)
    float	Factor;
};

USTRUCT(BlueprintType)
struct FVmdMorphTrackData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TArray<FVmdMorphFrameData> Frames;
};

USTRUCT(BlueprintType)
struct FMorphMappingConfig
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    FString MorphName;

    /** Inverse the value, use the negative value */
    UPROPERTY(EditAnywhere)
    bool bUseNegative = false;
};

/**
 * Motion data asset in unreal
 * Stored necessary data and provide some helper functions
 */
UCLASS()
class UEMMDHELPER_API UMotionDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UFUNCTION(CallInEditor, Category = "Default")
    void LoadFromVmdFile();
     
    UFUNCTION(CallInEditor, Category = "MorphAnim")
    void PushMorphToAnimation();

protected:
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;

public:
    float GetMorphAnimConvFrameRate() const {return MorphAnimConvFrameRate; }

protected:
    UPROPERTY(EditAnywhere, Category="Default")
    FFilePath MotionPath;

    UPROPERTY(VisibleAnywhere, Category="Default")
    FString TargetModelName;

    /** Frame rate used in morph target animation pushing */
    UPROPERTY(EditAnywhere, Category="MorphAnim")
    float MorphAnimConvFrameRate = 30.0f;

    /** Anim sequence to write morph target animation */
    UPROPERTY(EditAnywhere, Category="MorphAnim")
    TObjectPtr<class UAnimSequence> TargetAnim;

    /** Mapping to target mesh morph name before setting values */
    UPROPERTY(EditAnywhere, Category="MorphAnim|Mapping")
    bool bUseMorphMapping = false;

    /** If enabled, will not set morph if it's not in mapping config */
    UPROPERTY(EditAnywhere, Category="MorphAnim|Mapping", meta=(EditCondition= bUseMorphMapping))
    bool bUseRestrictMapping = true;

    /**
     * Mapping of morph names
     * MMD original <--> Target mesh
     * (Visible-only for back comparability, will auto be transformed to `MorphMapConfigs` while saving asset if it's empty)
     */
    UPROPERTY(VisibleAnywhere, Category = "MorphAnim|Mapping", meta = (EditCondition = bUseMorphMapping))
    TMap<FString, FString> MorphNameMapping;

    /**
     * Mapping of morph names and detailed config
     * MMD original <--> Target mesh
     */
    UPROPERTY(EditAnywhere, Category="MorphAnim|Mapping", meta = (EditCondition = bUseMorphMapping))
    TMap<FString, FMorphMappingConfig> MorphMapConfigs;


public:
    /** Camera frame data */
    UPROPERTY(VisibleAnywhere)
    TArray<FVmdCameraFrameData> CameraFrames;

    /** Morph target track data */
    UPROPERTY(VisibleAnywhere)
    TMap<FString, FVmdMorphTrackData> MorphTracks;
    
};

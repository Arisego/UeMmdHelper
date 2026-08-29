// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Vmd/VmdControlRigDefines.h"
#include "MotionDataAsset.generated.h"



USTRUCT(BlueprintType)
struct FVmdBezier
{
    GENERATED_BODY()

public:
    FVmdBezier();
    FVmdBezier(uint8,uint8,uint8,uint8);
    FVmdBezier(const uint8[2][2]);

    float Evaluate(float InTime) const;
private:
    float EvalX(const float InTime) const;
    float EvalY(const float InTime) const;
    float FindBezierX(const float InTime) const;

public:
    UPROPERTY(EditAnywhere)
    FVector2D Point1;

    UPROPERTY(EditAnywhere)
    FVector2D Point2;
};


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

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierLocation_X;

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierLocation_Y;

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierLocation_Z;

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierRotation;

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierDistance;

    UPROPERTY(EditAnywhere)
    FVmdBezier BezierFOV;
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
struct FVmdBoneFrameData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    uint32	Frame;

    UPROPERTY(EditAnywhere)
    FVector	BoneLocation;

    UPROPERTY(EditAnywhere)
    FQuat BoneRotation;

    //uint8 Bezier[2][2][4];
};

USTRUCT(BlueprintType)
struct FVmdBoneTrackData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TArray<FVmdBoneFrameData> Frames;
};

USTRUCT(BlueprintType)
struct FMorphMappingTarget
{
    GENERATED_BODY()

public:
    /** The morph name on target mesh */
    UPROPERTY(EditAnywhere)
    FName MorphName;

    /** 
     * Multiplier of morph value
     * Use -1.0 if the morph is inversed on two mesh.
     * Keep 1.0(default) will work at most of the time.
     */
    UPROPERTY(EditAnywhere)
    float MorphScale = 1.0f;
};

USTRUCT(BlueprintType)
struct FMorphMappingConfig
{
    GENERATED_BODY()

public:
    /** The Target morph arrays */
    UPROPERTY(EditAnywhere)
    TArray<FMorphMappingTarget> MorphTargets;

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

    /** Init the mapping config with current animation info */
    UFUNCTION(CallInEditor, Category = "MorphAnim")
    void InitMorphMapping();

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

    /** Bone track data */
    UPROPERTY(VisibleAnywhere)
    TMap<FString, FVmdBoneTrackData> BoneTracks;

public:
    //////////////////////////////////////////////////////////////////////////
    // Currently control rig supports a subset of blueprint, not all the functions are supported.
    // So we have to do some additional work to generate config manually.

    /** Generate the control rig config */
    UFUNCTION(CallInEditor, meta = (Category = "ControlRig"))
    void GenerateControlRigConfig();

public:
    /** Copy this to control rig helper function */
    UPROPERTY(VisibleAnywhere, meta=(Category="ControlRig"))
    FVmdControlRigMorphConfig VmdControlRigMorphConfig;

    UPROPERTY(VisibleAnywhere, meta = (Category = "ControlRig"))
    TMap<FName, FString> ControlNameToRawName;

    /** 
     * Morph key that does not need mapping.
     * The raw string will be passed directly to control rig.
     * If the key is already in MorphMapConfigs, it will be ignored.
     * 
     * @note: Unreal will have a internal Sanitized on control rig morph names.
     */
    UPROPERTY(EditAnywhere, meta = (Category = "ControlRig"))
    TSet<FString> DirectCopyMorphs;

public:
    UFUNCTION(CallInEditor, meta = (Category = "ControlRig"))
    void PushMorphDataToLevelSequencer();
    
};
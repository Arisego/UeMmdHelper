// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "VmdCineCamera.generated.h"


UENUM(BlueprintType)
enum class EVmdCineCameraFrameType : uint8
{
    EVCCFT_None UMETA(Hidden),
    /** Key frame only, interpolation with default unreal CubicKey behavior */
    EVCCFT_KeyOnly UMETA(DisplayName = "Key Only"),
    /** Try to interpolate by center/rotation/distance on every frame */
    EVCCFT_Interp UMETA(DisplayName = "Vmd Interp"),

};

/**
 * Derived cine camera with MMD motion data helper
 */
UCLASS()
class AVmdCineCamera : public ACineCameraActor
{
    GENERATED_BODY()

public:
    AVmdCineCamera(const FObjectInitializer& ObjectInitializer);

    float GetDistanceScaleBias() const { return DistanceScaleBias; }
    const FTransform& GetCenterTrans() const { return CenterTrans; }
    class UMotionDataAsset* GetMotionData() const { return ToRawPtr(MotionData); }
    float GetViewAngelBias() const { return ViewAngelBias; };

protected:
    UFUNCTION(CallInEditor, Category="Sequencer")
    void SyncCameraMotion();

    /**
     * Copy camera actor trans to config.
     * You must do it manually as camera actor may be moved by animation or some other logic.
     */
    UFUNCTION(CallInEditor, Category = "Sequencer")
    void CopyCameraTrans();

private:
    void SyncCameraMotion_KeyOnly();
    void SyncCameraMotion_Interped();

protected:
    UPROPERTY(EditAnywhere, Category="Sequencer")
    TObjectPtr<class UMotionDataAsset> MotionData;

    /** 
     * Switch to ensure config is ready
     * Enable this after CenterTrans is configured
     */
    UPROPERTY(EditAnywhere, Category="Sequencer")
    bool bConfigReady = false;

    /**
     * How frame is calculated
     * Using key only mode will make the interpolation work on final camera position, but you will get more control on camera.
     */
    UPROPERTY(EditAnywhere, Category = "Sequencer", meta = (EditCondition = "!bConfigReady"))
    EVmdCineCameraFrameType InterpType = EVmdCineCameraFrameType::EVCCFT_KeyOnly;

    /** 
     * The center transform of camera
     * In most cases, should be the same position of target character and facing the character
     */
    UPROPERTY(EditAnywhere, Category = "Sequencer", meta = (EditCondition = "!bConfigReady"))
    FTransform CenterTrans;

    /** 
     * The distance scale while calculate camera trans
     * Keep it as default should work
     */
    UPROPERTY(EditAnywhere, Category="Sequencer")
    float DistanceScaleBias = 10.0f;

    /** 
     * The multiply bias between camera's FieldOfView and `View Angel` in camera motion data
     * 
     * @note: CineCamera handles FiledOfView internal, you can check `CurrentFocalLength` to ensure its changing
     * @see: UCineCameraComponent::SetFieldOfView
     */
    UPROPERTY(EditAnywhere, Category="Sequencer")
    float ViewAngelBias = 1.666f;

};

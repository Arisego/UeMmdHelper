// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "VmdCineCamera.generated.h"

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

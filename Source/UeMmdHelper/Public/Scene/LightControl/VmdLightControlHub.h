// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/ScriptInterface.h"
#include "VmdLightControlHub.generated.h"

struct FVmdLightParams;






/**
 * Light control hub
 * Calculate and dispatch
 * This actor dose not hold any light component
 */
UCLASS()
class UEMMDHELPER_API AVmdLightControlHub : public AActor
{
    GENERATED_BODY()
    
public:	
    AVmdLightControlHub();

protected:
#if WITH_EDITORONLY_DATA
    /** Billboard used to see the actor in the editor */
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<class UBillboardComponent> SpriteComponent;
#endif

public:
    /**
     * Sequencer time
     * Should be add to sequencer channel and be curved to true sequencer time
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic", interp)
    float SequencerTime;

public:
    /** '
     * SequencerTime change event
     * Will be called every time sequencer changed SequencerTime
     */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Cinematic")
    void SetSequencerTime(const float InSequencerTime);

    /** Collect all actors implements `UVmdLightControlInterface` */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Light")
    void CollectAllLight();

    UFUNCTION(BlueprintNativeEvent, CallInEditor, Category = "Light")
    void CalculateLightParams(const FVmdLightControlTableRow& InLightControl, FVmdLightParams& OutLightParam);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Light", meta=(MustImplement = "/Script/UeMmdHelper.VmdLightControlInterface"))
    TSet<TObjectPtr<class AActor>> LinkedLights;

    /** FVmdLightControlTableRow */
    UPROPERTY(EditAnywhere, meta = (Category = "Light"))
    TObjectPtr<class UDataTable> ControlRawData;

    /** 
     * Centroid map range minimal
     * All the value below it will become 0 in calculation
     */
    UPROPERTY(EditAnywhere, meta = (Category = "Light"))
    float CentroidMin = 0.4f;

    /**
     * Centroid map range maximal
     * All the value above it will become 1 in calculation
     */
    UPROPERTY(EditAnywhere, meta = (Category = "Light"))
    float CentroidMax = 0.6f;

    /** 
     * Mapped centroid to hue range lower bound
     * CentroidMin -> HueMin
     */
    UPROPERTY(EditAnywhere, meta = (Category = "Light"))
    float HueMin = 0.05f;

    /**
    * Mapped centroid to hue range upper bound
    * CentroidMax -> HueMax
    */
    UPROPERTY(EditAnywhere, meta = (Category = "Light"))
    float HueMax = 0.68f;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VmdLightControlTableRow.generated.h"

USTRUCT(BlueprintType)
struct FVmdLightControlTableRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    float time = .0f;

    UPROPERTY(EditAnywhere)
    float rms_energy = .0f;
    UPROPERTY(EditAnywhere)
    float rms_norm = .0f;


    UPROPERTY(EditAnywhere)
    float bass_energy = .0f;
    UPROPERTY(EditAnywhere)
    float low_mid_energy = .0f;
    UPROPERTY(EditAnywhere)
    float mid_energy = .0f;
    UPROPERTY(EditAnywhere)
    float high_mid_energy = .0f;
    UPROPERTY(EditAnywhere)
    float treble_energy = .0f;


    UPROPERTY(EditAnywhere)
    float bass_norm = .0f;
    UPROPERTY(EditAnywhere)
    float low_mid_norm = .0f;
    UPROPERTY(EditAnywhere)
    float mid_norm = .0f;
    UPROPERTY(EditAnywhere)
    float high_mid_norm = .0f;
    UPROPERTY(EditAnywhere)
    float treble_norm = .0f;



    UPROPERTY(EditAnywhere)
    float bass_ratio = .0f;
    UPROPERTY(EditAnywhere)
    float low_mid_ratio = .0f;
    UPROPERTY(EditAnywhere)
    float mid_ratio = .0f;
    UPROPERTY(EditAnywhere)
    float high_mid_ratio = .0f;
    UPROPERTY(EditAnywhere)
    float treble_ratio = .0f;



    UPROPERTY(EditAnywhere)
    float spectral_centroid = .0f;
    UPROPERTY(EditAnywhere)
    float spectral_centroid_norm = .0f;


    UPROPERTY(EditAnywhere)
    float spectral_bandwidth = .0f;
    UPROPERTY(EditAnywhere)
    float spectral_bandwidth_norm = .0f;

    UPROPERTY(EditAnywhere)
    float spectral_rolloff = .0f;
    UPROPERTY(EditAnywhere)
    float spectral_rolloff_norm = .0f;



    UPROPERTY(EditAnywhere)
    float onset = .0f;
    UPROPERTY(EditAnywhere)
    float onset_norm = .0f;


    UPROPERTY(EditAnywhere)
    float spectral_flux = .0f;
    UPROPERTY(EditAnywhere)
    float spectral_flux_norm = .0f;



    UPROPERTY(EditAnywhere)
    float beat = .0f;

    UPROPERTY(EditAnywhere)
    float bpm = .0f;
};
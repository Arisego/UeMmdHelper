// Fill out your copyright notice in the Description page of Project Settings.


#include "Scene/LightControl/VmdLightControlHub.h"

#include "Components/BillboardComponent.h"
#include "EngineUtils.h"

// Sets default values
AVmdLightControlHub::AVmdLightControlHub()
{
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(USceneComponent::GetDefaultSceneRootVariableName());
    Root->Mobility = EComponentMobility::Static;

    SetRootComponent(Root);


#if WITH_EDITORONLY_DATA
    SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
    if (SpriteComponent)
    {
        // Structure to hold one-time initialization
        struct FConstructorStatics
        {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> TriggerTextureObject;
            FName IdTriggers;
            FText NameTriggers;
            FConstructorStatics()
                : TriggerTextureObject(TEXT("/Engine/EditorResources/S_TargetPoint"))
                , IdTriggers(TEXT("TargetPoint"))
                , NameTriggers(NSLOCTEXT("SpriteCategory", "TargetPoint", "Target Points"))
            {}
        };
        static FConstructorStatics ConstructorStatics;

        SpriteComponent->Sprite = ConstructorStatics.TriggerTextureObject.Get();
        SpriteComponent->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
        SpriteComponent->bHiddenInGame = true;
        SpriteComponent->SpriteInfo.Category = ConstructorStatics.IdTriggers;
        SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NameTriggers;
        SpriteComponent->bIsScreenSizeScaled = true;

        SpriteComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        SpriteComponent->Mobility = EComponentMobility::Static;
    }
#endif
}


void AVmdLightControlHub::SetSequencerTime(const float InSequencerTime)
{
    UE_LOG(LogMmdHelper, VeryVerbose, TEXT("AVmdLightControlHub::SetSequencerTime: Time=%f"), InSequencerTime);
    SequencerTime = InSequencerTime;

    if (!ControlRawData)
    {
        return;
    }

    TArray<FVmdLightControlTableRow*> RawControlDataArray;
    ControlRawData->GetAllRows(TEXT("AVmdLightControlHub::SetSequencerTime"), RawControlDataArray);

    FVmdLightControlTableRow FinalControl;
    for (int32 IterIndex = 0; IterIndex < RawControlDataArray.Num(); ++IterIndex)
    {
        /** If its the last data, just use it */
        const int32 TdNextIndex = IterIndex + 1;
        if (!RawControlDataArray.IsValidIndex(TdNextIndex))
        {
            FinalControl = *RawControlDataArray[IterIndex];
            break;
        }

        /** Step until the time of next data is bigger than query time */
        const FVmdLightControlTableRow& TrNextControl = *RawControlDataArray[TdNextIndex];
        if (InSequencerTime > TrNextControl.time)
        {
            continue;
        }

        const FVmdLightControlTableRow& TrCurrentControl = *RawControlDataArray[IterIndex];

        const float TfTimeSpan = (TrNextControl.time - TrNextControl.time);
        FinalControl.time = InSequencerTime;

        const float TfInterpDelta = InSequencerTime - TrCurrentControl.time;
        const float TfInterpDeltaScaled = TfInterpDelta / TfTimeSpan;

        FinalControl.rms_energy = FMath::FInterpTo(TrCurrentControl.rms_energy, TrNextControl.rms_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.rms_norm = FMath::FInterpTo(TrCurrentControl.rms_norm, TrNextControl.rms_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.bass_energy = FMath::FInterpTo(TrCurrentControl.bass_energy, TrNextControl.bass_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.low_mid_energy = FMath::FInterpTo(TrCurrentControl.low_mid_energy, TrNextControl.low_mid_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.mid_energy = FMath::FInterpTo(TrCurrentControl.mid_energy, TrNextControl.mid_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.high_mid_energy = FMath::FInterpTo(TrCurrentControl.high_mid_energy, TrNextControl.high_mid_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.treble_energy = FMath::FInterpTo(TrCurrentControl.treble_energy, TrNextControl.treble_energy, TfInterpDeltaScaled, 1.0f);
        FinalControl.bass_norm = FMath::FInterpTo(TrCurrentControl.bass_norm, TrNextControl.bass_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.low_mid_norm = FMath::FInterpTo(TrCurrentControl.low_mid_norm, TrNextControl.low_mid_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.mid_norm = FMath::FInterpTo(TrCurrentControl.mid_norm, TrNextControl.mid_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.high_mid_norm = FMath::FInterpTo(TrCurrentControl.high_mid_norm, TrNextControl.high_mid_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.treble_norm = FMath::FInterpTo(TrCurrentControl.treble_norm, TrNextControl.treble_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.bass_ratio = FMath::FInterpTo(TrCurrentControl.bass_ratio, TrNextControl.bass_ratio, TfInterpDeltaScaled, 1.0f);
        FinalControl.low_mid_ratio = FMath::FInterpTo(TrCurrentControl.low_mid_ratio, TrNextControl.low_mid_ratio, TfInterpDeltaScaled, 1.0f);
        FinalControl.mid_ratio = FMath::FInterpTo(TrCurrentControl.mid_ratio, TrNextControl.mid_ratio, TfInterpDeltaScaled, 1.0f);
        FinalControl.high_mid_ratio = FMath::FInterpTo(TrCurrentControl.high_mid_ratio, TrNextControl.high_mid_ratio, TfInterpDeltaScaled, 1.0f);
        FinalControl.treble_ratio = FMath::FInterpTo(TrCurrentControl.treble_ratio, TrNextControl.treble_ratio, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_centroid = FMath::FInterpTo(TrCurrentControl.spectral_centroid, TrNextControl.spectral_centroid, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_centroid_norm = FMath::FInterpTo(TrCurrentControl.spectral_centroid_norm, TrNextControl.spectral_centroid_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_bandwidth = FMath::FInterpTo(TrCurrentControl.spectral_bandwidth, TrNextControl.spectral_bandwidth, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_bandwidth_norm = FMath::FInterpTo(TrCurrentControl.spectral_bandwidth_norm, TrNextControl.spectral_bandwidth_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_rolloff = FMath::FInterpTo(TrCurrentControl.spectral_rolloff, TrNextControl.spectral_rolloff, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_rolloff_norm = FMath::FInterpTo(TrCurrentControl.spectral_rolloff_norm, TrNextControl.spectral_rolloff_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.onset = FMath::FInterpTo(TrCurrentControl.onset, TrNextControl.onset, TfInterpDeltaScaled, 1.0f);
        FinalControl.onset_norm = FMath::FInterpTo(TrCurrentControl.onset_norm, TrNextControl.onset_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_flux = FMath::FInterpTo(TrCurrentControl.spectral_flux, TrNextControl.spectral_flux, TfInterpDeltaScaled, 1.0f);
        FinalControl.spectral_flux_norm = FMath::FInterpTo(TrCurrentControl.spectral_flux_norm, TrNextControl.spectral_flux_norm, TfInterpDeltaScaled, 1.0f);
        FinalControl.beat = FMath::FInterpTo(TrCurrentControl.beat, TrNextControl.beat, TfInterpDeltaScaled, 1.0f);
        FinalControl.bpm = FMath::FInterpTo(TrCurrentControl.bpm, TrNextControl.bpm, TfInterpDeltaScaled, 1.0f);

        break;
    }

    /** Calculate light */
    FVmdLightParams TsLightParams;
    CalculateLightParams(FinalControl, TsLightParams);

    /** Update light params to light that we hold */
    for (TObjectPtr<AActor> IterLight : LinkedLights)
    {
        if (!IterLight)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("AVmdLightControlHub::SetSequencerTime: Bad IterLight"));
            continue;
        }

        if (!IterLight->Implements<UVmdLightControlInterface>())
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("AVmdLightControlHub::SetSequencerTime: Bad UVmdLightControlInterface check, actor=(%p)%s"),
                IterLight.Get(),
                *GetFullNameSafe(IterLight.Get())
            );
            continue;
        }

        IVmdLightControlInterface::Execute_UpdateLightParams(IterLight, TsLightParams);
    }
}

void AVmdLightControlHub::CalculateLightParams_Implementation(const FVmdLightControlTableRow& InLightControl, FVmdLightParams& OutLightParam)
{

    //////////////////////////////////////////////////////////////////////////
    // Calculate Intensity
    //////////////////////////////////////////////////////////////////////////

    /** High Frequency Energy */
    const float TfEdgeEnergy = InLightControl.high_mid_norm * 0.40f + InLightControl.treble_norm * 0.60f;

    const float TfOverallLoudness = FMath::Lerp(0.35f, 1.0f, InLightControl.rms_norm);
    const float TfHightEnegyLoudnessBias = TfEdgeEnergy * TfOverallLoudness;

    /** OnSet flash */
    const float TfFlash = FMath::Pow(InLightControl.onset_norm, 1.5f);
    const float TfFlashMultiplier = 1.0f + TfFlash * 1.25f;

    /** Beat pulse */
    const float TfBeatPulse = InLightControl.beat > 0.5f ? 1.0f : 0.0f;
    float TfPulseMultiplier =1.0f + TfBeatPulse * 0.35f;

    /** Put energy together and clamp it */
    const float TfFinalEnergy =TfHightEnegyLoudnessBias * TfPulseMultiplier * TfFlashMultiplier;
    const float TfClampedFinalEnergy = FMath::Clamp(TfFinalEnergy,0.0f,1.0f);

    OutLightParam.LightIntensity = TfClampedFinalEnergy;


    //////////////////////////////////////////////////////////////////////////
    // Light color
    //////////////////////////////////////////////////////////////////////////

    /** Centroid range differs from musics, we need to map it so we can have more variety in color */
    const float TfCentroid = FMath::Clamp(InLightControl.spectral_centroid_norm, CentroidMin, CentroidMax);
    const float TfCentroidMapped = (TfCentroid - CentroidMin) / (CentroidMax - CentroidMin);

    /** Centroid will move as hue in color */
    const float TfHue = FMath::Lerp(HueMin, HueMax, TfCentroidMapped);


    /** Add a hue offset by the ratio of low and high frequency */
    const float Warm = InLightControl.bass_ratio + InLightControl.low_mid_ratio;
    const float Cool = InLightControl.high_mid_ratio + InLightControl.treble_ratio;
    const float Balance = Cool / (Warm + Cool + KINDA_SMALL_NUMBER);

    const float HueOffset = FMath::Lerp(-0.06f, 0.06f, Balance);
    const float TfOffsetedHue = FMath::Clamp(TfHue + HueOffset, 0.0f, 0.85f);


    /** Use root mean square to decide saturation(HSV) */
    const float Saturation = FMath::Lerp(0.65f, 1.0f, InLightControl.rms_norm);

    /** Use normalized high mid frequency to decide color value(HSV) */
    const float Value = FMath::Lerp(0.7f, 1.0f, InLightControl.high_mid_norm);

    /**
     * Convert HSV color to final color
     * In this color, R = H, G = S, B = V  ; H in [0,360] , S&V in [0,1]
     */
    const FLinearColor HSV(TfOffsetedHue * 360.0f, Saturation, Value, 1.0f);
    OutLightParam.LightColor = HSV.HSVToLinearRGB();
    UE_LOG(LogMmdHelper, Verbose, TEXT("AVmdLightControlHub::CalculateLightParams_Implementation: Centroid=(%f)%f HSV=%s"),
        InLightControl.spectral_centroid_norm,
        TfCentroid,
        *HSV.ToString()
    );
}

void AVmdLightControlHub::CollectAllLight()
{
    UWorld* TpWorld = GetWorld();
    if (!TpWorld)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdLightControlHub::CollectAllLight: Bad world"));
        return;
    }

    LinkedLights.Empty();
    for (TActorIterator<AActor> It(TpWorld); It; ++It)
    {
        AActor* TpIterActor = *It;
        if (!IsValid(TpIterActor))
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("AVmdLightControlHub: Bad actor iteration"));
            continue;
        }

        if (!TpIterActor->Implements<UVmdLightControlInterface>())
        {
            continue;
        }

        LinkedLights.Add(TpIterActor);
    }

    Modify();
}


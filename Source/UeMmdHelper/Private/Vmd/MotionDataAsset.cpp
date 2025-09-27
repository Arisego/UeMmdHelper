// Fill out your copyright notice in the Description page of Project Settings.


#include "Vmd/MotionDataAsset.h"
#include "Vmd/VmdDataHelper.h"
#include "UeMmdHelper.h"



#define LOCTEXT_NAMESPACE "VmdDataAsset"


void UMotionDataAsset::LoadFromVmdFile()
{
#if WITH_EDITOR
    FScopedSlowTask SlowTask(3.0f, LOCTEXT("LoadFromVmdFile", "Load motion data from file"));
    SlowTask.MakeDialog(false/*bShowCancelButton*/, true/*bAllowInPIE*/);

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("LoadData", "Serialize file data"));
    FVmdData TsVmdData;
    FVmdDataHelper::LoadVmdDataFromFile(MotionPath.FilePath, TsVmdData);
    TsVmdData.PrintOutData();

    TargetModelName = FVmdDataHelper::ConvertFromMmdName(TsVmdData.GetVmdHeader().TargetModelName);

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("CameraFrame", "Converting camera frame"));
    const FVmdBoneTracks& TsVmdTracks = TsVmdData.GetTrackData();

    CameraFrames.Empty(TsVmdTracks.CameraFrames.Num());
    for (const FVmdCameraFrame& IterRawFrame : TsVmdTracks.CameraFrames)
    {
        FVmdCameraFrameData& TrAdded = CameraFrames.AddZeroed_GetRef();
        TrAdded.Frame = IterRawFrame.Frame;
        TrAdded.Length = IterRawFrame.Length;
        TrAdded.Location = FVector(IterRawFrame.Location[0], IterRawFrame.Location[1], IterRawFrame.Location[2]);
        TrAdded.Rotate = FVector(
            -FMath::RadiansToDegrees(IterRawFrame.Rotate[0]),
            FMath::RadiansToDegrees(IterRawFrame.Rotate[1]),
            FMath::RadiansToDegrees(IterRawFrame.Rotate[2])
        );

        TrAdded.ViewingAngle = IterRawFrame.ViewingAngle;
        TrAdded.Perspective = IterRawFrame.Perspective;
    }

    Algo::Sort(CameraFrames, [&](const FVmdCameraFrameData& A, const FVmdCameraFrameData& B)
        {
            return A.Frame < B.Frame;
        });

    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("MorphFrame", "Converting morph frame"));

    MorphTracks.Empty(0);
    {
        TMap<FString, FVmdMorphTrackData> TmapTracks;

        /** Read raw data into mapped data */
        for (const FVmdFaceFrame& IterRawFrame : TsVmdTracks.FaceFrames)
        {
            const FString TstrName = FVmdDataHelper::ConvertFromMmdName(IterRawFrame.Name);

            FVmdMorphTrackData& TrTrack = TmapTracks.FindOrAdd(TstrName);
            FVmdMorphFrameData& TrAdded = TrTrack.Frames.AddZeroed_GetRef();

            TrAdded.Frame = IterRawFrame.Frame;
            TrAdded.Factor = IterRawFrame.Factor;
        }

        /** Filter frame data */
        for (TPair<FString, FVmdMorphTrackData>& IterMorphTrack : TmapTracks)
        {
            const FString& TrName = IterMorphTrack.Key;
            FVmdMorphTrackData& TrTrack = IterMorphTrack.Value;

            /** Ignore empty data */
            if (TrTrack.Frames.Num() == 0)
            {
                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::LoadFromVmdFile:(Filter) remov empty, name=%s"),
                    *TrName
                );
                continue;
            }

            /** Ignore data with only one frame */
            if (TrTrack.Frames.Num() == 1)
            {
                if (TrTrack.Frames[0].Factor != 0.0f)
                {
                    UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::LoadFromVmdFile:(Filter) Ignored factor not zero"));
                }

                if (TrTrack.Frames[0].Frame != 0)
                {
                    UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::LoadFromVmdFile:(Filter) Ignored frame not zero"));
                }

                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::LoadFromVmdFile:(Filter) remov single frame, name=%s"),
                    *TrName
                );
                continue;
            }

            /** Insert data */
            MorphTracks.Add(IterMorphTrack);
        }
    }

    /** Sort the frame data */
    for (TPair<FString, FVmdMorphTrackData>& IterMorphTrack : MorphTracks)
    {
        const FString& TrName = IterMorphTrack.Key;
        FVmdMorphTrackData& TrTrack = IterMorphTrack.Value;
        Algo::Sort(TrTrack.Frames, [&](const FVmdMorphFrameData& A, const FVmdMorphFrameData& B)
            {
                return A.Frame < B.Frame;
            });

        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::LoadFromVmdFile: Morph track, name=%s num=%d"),
            *TrName,
            TrTrack.Frames.Num()
        );
    }

    Modify();
#endif
}


void UMotionDataAsset::PushMorphToAnimation()
{
#if WITH_EDITOR
    /** Check target animation */
    if (!IsValid(TargetAnim))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad UAnimSequence"));
        return;
    }

    UAnimSequence* TpAnimSeq = TargetAnim;
    USkeleton* TpSkeleton = TpAnimSeq->GetSkeleton();
    if (!IsValid(TpSkeleton))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad GetSkeleton"));
        return;
    }

    USkeletalMesh* TpSkelMesh = TpSkeleton->GetPreviewMesh();
    if (!IsValid(TpSkelMesh))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad GetPreviewMesh"));
        return;
    }

    IAnimationDataController& TpAnimDataController = TpAnimSeq->GetController();
    TpAnimDataController.OpenBracket(LOCTEXT("VmdMorphImport", "Importing morph from vmd"));

    const float TfAnimLen = TpAnimSeq->GetPlayLength();
    const float TfAnimRate = GetMorphAnimConvFrameRate();

    for (const TPair<FString, FVmdMorphTrackData>& IterMorphTrack : MorphTracks)
    {
        const FString& TrName = IterMorphTrack.Key;
        const FVmdMorphTrackData& TrTrack = IterMorphTrack.Value;

        FName TrMorphName = NAME_None;
        if (bUseMorphMapping)
        {
            const FString* TsMappedName = MorphNameMapping.Find(TrName);
            if (TsMappedName)
            {
                const FString& TrMappedName = *TsMappedName;
                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Mapped, raw=%s name=%s"),
                    *TrName,
                    *TrMappedName
                );
                TrMorphName = *TrMappedName;
            }
            else
            {
                if (bUseRestrictMapping)
                {
                    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Not mapped morph ignored, name=%s"), *TrName);
                    continue;
                }

                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Not mapped morph use raw, name=%s"), *TrName);
            }
        }
        else 
        {
            TrMorphName = *TrName;
        }


        /** Check morph target */
        UMorphTarget* TpMorphTarget = TpSkelMesh->FindMorphTarget(TrMorphName);
        if (!TpMorphTarget)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: No morph, name=%s"),
                *TrMorphName.ToString()
            );
            continue;
        }

        /** Try create meta data */
        FCurveMetaData* TpCurveMeta = TpSkeleton->GetCurveMetaData(TrMorphName);
        if (!TpCurveMeta || !TpCurveMeta->Type.bMorphtarget)
        {
            TpSkeleton->AddCurveMetaData(TrMorphName);
            UAnimCurveMetaData* TpCuremeataData = TpSkeleton->GetAssetUserData<UAnimCurveMetaData>();
            if (!IsValid(TpCuremeataData))
            {
                UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad curver meta data, name=%s"),
                    *TrMorphName.ToString()
                );
                continue;
            }

            TpCurveMeta = TpCuremeataData->GetCurveMetaData(TrMorphName);
            if (!TpCurveMeta)
            {
                UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad curver meta data create, name=%s"),
                    *TrMorphName.ToString()
                );
                continue;
            }

            if (!TpCurveMeta->Type.bMorphtarget)
            {
                TpCuremeataData->SetCurveMetaDataMorphTarget(TrMorphName, true);
                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Auto created meta data, name=%s meta=(%p)%s"),
                    *TrMorphName.ToString(),
                    TpCuremeataData,
                    *GetNameSafe(TpCuremeataData)
                );
            }

            TpSkeleton->Modify();
        }

        /** Add curve */
        const FAnimationCurveIdentifier MetadataCurveId(TrMorphName, ERawCurveTrackTypes::RCT_Float);
        TpAnimDataController.AddCurve(MetadataCurveId, AACF_Metadata);

        const FFloatCurve* TpNewCurve = TpAnimSeq->GetDataModel()->FindFloatCurve(MetadataCurveId);

        /**
         * This check ensures curve creation
         * It's here to catch some unknown error, maybe remove it if every thing goes well
         */
        check(TpNewCurve && "Bad logic");

        /** Add data to curve */
        FRichCurve TsMorphRichCurve;
        for (const FVmdMorphFrameData& IterFrame : TrTrack.Frames)
        {
            /** Convert frame time */
            float TfTimeInCurve = IterFrame.Frame / TfAnimRate;
            if (TfTimeInCurve > TfAnimLen)
            {
                /**
                 * Ignore if morph animation is longer than target animation
                 * We do not automatically modify animation length
                 */
                UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad time couverted, track=%s frame=%f>%f, frame=%d"),
                    *TrMorphName.ToString(),
                    TfTimeInCurve,
                    TfAnimLen,
                    IterFrame.Frame
                );
                continue;
            }

            const float TfCurveValue = IterFrame.Factor;
            const float TfTimeValue = TfTimeInCurve;

            FKeyHandle TsKeyHandle = TsMorphRichCurve.AddKey(TfTimeValue, TfCurveValue, false);
            TsMorphRichCurve.SetKeyInterpMode(TsKeyHandle, ERichCurveInterpMode::RCIM_Linear);
            TsMorphRichCurve.SetKeyTangentMode(TsKeyHandle, ERichCurveTangentMode::RCTM_Auto);
            TsMorphRichCurve.SetKeyTangentWeightMode(TsKeyHandle, ERichCurveTangentWeightMode::RCTWM_WeightedNone);
        }

        TpAnimDataController.SetCurveKeys(MetadataCurveId, TsMorphRichCurve.GetConstRefOfKeys());
    }

    TpAnimDataController.NotifyPopulated();
    TpAnimDataController.CloseBracket();
    return;
#endif
}

#undef LOCTEXT_NAMESPACE
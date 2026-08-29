// Fill out your copyright notice in the Description page of Project Settings.


#include "Vmd/MotionDataAsset.h"
#include "Vmd/VmdDataHelper.h"
#include "UeMmdHelper.h"
#include "UObject/ObjectSaveContext.h"
#include "MovieSceneTrack.h"
#include "MovieSceneSection.h"
#include "SequencerCurveEditorObject.h"
#include "Sequencer/MovieSceneControlRigParameterTrack.h"



#define LOCTEXT_NAMESPACE "VmdDataAsset"


FVmdBezier::FVmdBezier()
{
    Point1 = FVector2D::ZeroVector;
    Point2 = FVector2D::ZeroVector;
}

FVmdBezier::FVmdBezier(uint8 x0, uint8 y0, uint8 x1, uint8 y1)
{
    Point1 = FVector2D((float)x0 / 127.0f, (float)y0 / 127.0f);
    Point2 = FVector2D((float)x1 / 127.0f, (float)y1 / 127.0f);
}


FVmdBezier::FVmdBezier(const uint8 RawArray[2][2])
    :FVmdBezier(RawArray[0][0], RawArray[0][1], RawArray[1][0], RawArray[1][1])
{

}

float FVmdBezier::Evaluate(float InTime) const
{
    const float TfTime = FindBezierX(InTime);
    return EvalY(TfTime);
}

float FVmdBezier::EvalX(const float InTime) const
{
    const float oneMinusT = 1.0f - InTime;

    const float controlPointX0 = 0.0f;
    const float controlPointX1 = Point1.X;
    const float controlPointX2 = Point2.X;
    const float controlPointX3 = 1.0f;

    const float bezierWeight0 = oneMinusT * oneMinusT * oneMinusT;
    const float bezierWeight1 = 3.0f * oneMinusT * oneMinusT * InTime;
    const float bezierWeight2 = 3.0f * oneMinusT * InTime * InTime;
    const float bezierWeight3 = InTime * InTime * InTime;

    return
        bezierWeight0 * controlPointX0 +
        bezierWeight1 * controlPointX1 +
        bezierWeight2 * controlPointX2 +
        bezierWeight3 * controlPointX3;
}

float FVmdBezier::EvalY(const float InTime) const
{
    const float oneMinusT = 1.0f - InTime;

    const float controlPointY0 = 0.0f;
    const float controlPointY1 = Point1.Y;
    const float controlPointY2 = Point2.Y;
    const float controlPointY3 = 1.0f;

    const float bezierWeight0 = oneMinusT * oneMinusT * oneMinusT;
    const float bezierWeight1 = 3.0f * oneMinusT * oneMinusT * InTime;
    const float bezierWeight2 = 3.0f * oneMinusT * InTime * InTime;
    const float bezierWeight3 = InTime * InTime * InTime;

    return
        bezierWeight0 * controlPointY0 +
        bezierWeight1 * controlPointY1 +
        bezierWeight2 * controlPointY2 +
        bezierWeight3 * controlPointY3;
}

float FVmdBezier::FindBezierX(const float InTime) const
{
    const float tolerance = 0.00001f;

    float start = 0.0f;
    float stop = 1.0f;

    float time = FMath::Clamp(InTime, 0.0f, 1.0f);

    for (int iteration = 0; iteration < 32; ++iteration)
    {
        const float t = (start + stop) * 0.5f;
        const float x = EvalX(t);

        if (std::abs(time - x) <= tolerance)
        {
            return t;
        }

        if (time < x)
        {
            stop = t;
        }
        else
        {
            start = t;
        }
    }

    return (start + stop) * 0.5f;
}

void UMotionDataAsset::LoadFromVmdFile()
{
#if WITH_EDITOR
    FScopedSlowTask SlowTask(4.0f, LOCTEXT("LoadFromVmdFile", "Load motion data from file"));
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

        TrAdded.BezierLocation_X = FVmdBezier(IterRawFrame.Interpolation[0]);
        TrAdded.BezierLocation_Y = FVmdBezier(IterRawFrame.Interpolation[1]);
        TrAdded.BezierLocation_Z = FVmdBezier(IterRawFrame.Interpolation[2]);
        TrAdded.BezierRotation = FVmdBezier(IterRawFrame.Interpolation[3]);
        TrAdded.BezierDistance = FVmdBezier(IterRawFrame.Interpolation[4]);
        TrAdded.BezierFOV = FVmdBezier(IterRawFrame.Interpolation[5]);
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
    
    SlowTask.EnterProgressFrame(1.0f, LOCTEXT("BoneTracks", "Converting bone tracks"));
    BoneTracks.Empty(0);
    {
        for (const FVmdBoneFrame& IterRawFrame : TsVmdTracks.BoneFrames)
        {
            const FString TstrName = FVmdDataHelper::ConvertFromMmdName(IterRawFrame.Name);

            FVmdBoneTrackData& TrTrack = BoneTracks.FindOrAdd(TstrName);
            FVmdBoneFrameData& TrAdded = TrTrack.Frames.AddZeroed_GetRef();

            TrAdded.Frame = IterRawFrame.Frame;
            TrAdded.BoneLocation = FVector(IterRawFrame.Position[0], IterRawFrame.Position[1], IterRawFrame.Position[2]);
            TrAdded.BoneRotation = FQuat(IterRawFrame.Quaternion[0], IterRawFrame.Quaternion[1], IterRawFrame.Quaternion[2], IterRawFrame.Quaternion[3]);
        }
    }

    Modify();
#endif
}


struct FPushMorphToAnimationRecord
{
    bool bSuccess = false;
    uint32 CntMapped = 0;
    uint32 FrameCount = 0;
};

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

    TMap<FString, FPushMorphToAnimationRecord> TsRecords;
    uint32 CntPassed = 0;
    for (const TPair<FString, FVmdMorphTrackData>& IterMorphTrack : MorphTracks)
    {
        const FString& TrName = IterMorphTrack.Key;
        const FVmdMorphTrackData& TrTrack = IterMorphTrack.Value;

        TMap<FName, float> TsMorphModifyInfs;
        FPushMorphToAnimationRecord& TrRecord = TsRecords.FindOrAdd(TrName);

        if (bUseMorphMapping)
        {
            const FMorphMappingConfig* TpMorphMapConfig = MorphMapConfigs.Find(TrName);
            if (TpMorphMapConfig)
            {
                for(const FMorphMappingTarget& IterMorphTarget : TpMorphMapConfig->MorphTargets)
                {
                    const FName& TsTargetName = IterMorphTarget.MorphName;
                    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Mapped, raw=%s name=%s scale=%f"),
                        *TrName,
                        *TsTargetName.ToString(),
                        IterMorphTarget.MorphScale
                    );

                    TsMorphModifyInfs.FindOrAdd(TsTargetName) = IterMorphTarget.MorphScale;
                    TrRecord.CntMapped++;
                }

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
            TsMorphModifyInfs.FindOrAdd(*TrName) = 1.0f;
        }

        for (const TPair<FName, float>& IterMorphInf : TsMorphModifyInfs)
        {
            const FName& TsMorphName = IterMorphInf.Key;
            const float& TfMorphScale = IterMorphInf.Value;

            /** Check morph target */
            UMorphTarget* TpMorphTarget = TpSkelMesh->FindMorphTarget(TsMorphName);
            if (!TpMorphTarget)
            {
                UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: No morph, name=%s"),
                    *TsMorphName.ToString()
                );
                continue;
            }

            /** Try create meta data */
            FCurveMetaData* TpCurveMeta = TpSkeleton->GetCurveMetaData(TsMorphName);
            if (!TpCurveMeta || !TpCurveMeta->Type.bMorphtarget)
            {
                TpSkeleton->AddCurveMetaData(TsMorphName);
                UAnimCurveMetaData* TpCuremeataData = TpSkeleton->GetAssetUserData<UAnimCurveMetaData>();
                if (!IsValid(TpCuremeataData))
                {
                    UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad curver meta data, name=%s"),
                        *TsMorphName.ToString()
                    );
                    continue;
                }

                TpCurveMeta = TpCuremeataData->GetCurveMetaData(TsMorphName);
                if (!TpCurveMeta)
                {
                    UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphToAnimation: Bad curver meta data create, name=%s"),
                        *TsMorphName.ToString()
                    );
                    continue;
                }

                if (!TpCurveMeta->Type.bMorphtarget)
                {
                    TpCuremeataData->SetCurveMetaDataMorphTarget(TsMorphName, true);
                    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Auto created meta data, name=%s meta=(%p)%s"),
                        *TsMorphName.ToString(),
                        TpCuremeataData,
                        *GetNameSafe(TpCuremeataData)
                    );
                }

                TpSkeleton->Modify();
            }

            /** Add curve */
            const FAnimationCurveIdentifier MetadataCurveId(TsMorphName, ERawCurveTrackTypes::RCT_Float);
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
                        *TsMorphName.ToString(),
                        TfTimeInCurve,
                        TfAnimLen,
                        IterFrame.Frame
                    );
                    continue;
                }

                const float TfCurveValue = TfMorphScale * IterFrame.Factor;
                const float TfTimeValue = TfTimeInCurve;

                FKeyHandle TsKeyHandle = TsMorphRichCurve.AddKey(TfTimeValue, TfCurveValue, false);
                TsMorphRichCurve.SetKeyInterpMode(TsKeyHandle, ERichCurveInterpMode::RCIM_Linear);
                TsMorphRichCurve.SetKeyTangentMode(TsKeyHandle, ERichCurveTangentMode::RCTM_Auto);
                TsMorphRichCurve.SetKeyTangentWeightMode(TsKeyHandle, ERichCurveTangentWeightMode::RCTWM_WeightedNone);
            }

            TpAnimDataController.SetCurveKeys(MetadataCurveId, TsMorphRichCurve.GetConstRefOfKeys());
        }


        TrRecord.bSuccess = true;
        TrRecord.FrameCount = TrTrack.Frames.Num();
        ++CntPassed;
    }



    TpAnimDataController.NotifyPopulated();
    TpAnimDataController.CloseBracket();

    ///Print records in log
    {
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphToAnimation: Morph import summary, count=%u/%d"),
            CntPassed,
            TsRecords.Num()
        );
        for (const TPair<FString, FPushMorphToAnimationRecord>& Iter : TsRecords)
        {
            const FString& Name = Iter.Key;
            const FPushMorphToAnimationRecord& R = Iter.Value;

            if (R.bSuccess)
            {
                UE_LOG(LogMmdHelper, Log, TEXT("Passed, name=(%u)%s mapped=%u"), 
                    R.FrameCount,
                    *Name,
                    R.CntMapped
                );
            }
            else
            {
                UE_LOG(LogMmdHelper, Warning, TEXT("Ignored, name=%s"), *Name);
            }
        }
    }

    return;
#endif
}

void UMotionDataAsset::InitMorphMapping()
{
    for (const TPair<FString, FVmdMorphTrackData>& IterMorphTrack : MorphTracks)
    {
        MorphMapConfigs.FindOrAdd(IterMorphTrack.Key);
    }

    Modify();
}


FName GetSafeControlRigName(const FString& ControlKeyRaw, const TSet<FName>& ControlNames)
{
    const FName& SanitizedName = URigHierarchy::GetSanitizedName(FName(*ControlKeyRaw));
    FName TsSafeName = SanitizedName;

    int32 TdNameTryCnt = 0;
    while (ControlNames.Contains(TsSafeName))
    {
        TsSafeName = *FString::Printf(TEXT("%s_%d"), *SanitizedName.ToString(), TdNameTryCnt);
        ++TdNameTryCnt;
    };

    return TsSafeName;
}

void UMotionDataAsset::GenerateControlRigConfig()
{
    VmdControlRigMorphConfig.MorphMapConfigs.Empty(MorphMapConfigs.Num());
    ControlNameToRawName.Empty(MorphMapConfigs.Num());

    /** Cache for mapped key, so we can filter them while decide unhandled ones */
    TSet<FString> TsMappedMorphKey;

    TSet<FName> ControlNames;
    for (const TPair<FString, FMorphMappingConfig>& IterMorphMap: MorphMapConfigs)
    {
        FVmdControlRigMorphPair& TrNewMorphPair = VmdControlRigMorphConfig.MorphMapConfigs.AddDefaulted_GetRef();

        /** Control name conversion */
        const FString& ControlKeyRaw = IterMorphMap.Key;
        const FName& TsSafeName = GetSafeControlRigName(ControlKeyRaw, ControlNames);

        ControlNames.Add(TsSafeName);
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::GenerateControlRigConfig: Rig name, %s => %s"),
            *ControlKeyRaw,
            *TsSafeName.ToString()
        );

        TrNewMorphPair.ControlName = TsSafeName;
        ControlNameToRawName.FindOrAdd(TsSafeName) = ControlKeyRaw;

        const FMorphMappingConfig& IterMorphMapConfig = IterMorphMap.Value;
        for (const FMorphMappingTarget& IterMorph : IterMorphMapConfig.MorphTargets)
        {
            FControlRigMorphConfig& TrNewMorphConfig = TrNewMorphPair.TargetMorphs.AddDefaulted_GetRef();
            TrNewMorphConfig.MorphName = IterMorph.MorphName;
            TrNewMorphConfig.MorphScale = IterMorph.MorphScale;
        }
    }

    /** Handle cirect copy morphs */
    for (const FString& IterMorphKey : DirectCopyMorphs)
    {
        /** Ignore if already mapped */
        if (TsMappedMorphKey.Contains(IterMorphKey))
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::GenerateControlRigConfig: Key [%s] already in MorphMapConfigs"), *IterMorphKey);
            continue;
        }

        /** Control name conversion */
        const FString& ControlKeyRaw = IterMorphKey;
        const FName& TsSafeName = GetSafeControlRigName(ControlKeyRaw, ControlNames);

        ControlNames.Add(TsSafeName);
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::GenerateControlRigConfig: Direct name, %s => %s"),
            *ControlKeyRaw,
            *TsSafeName.ToString()
        );

        FVmdControlRigMorphPair& TrNewMorphPair = VmdControlRigMorphConfig.MorphMapConfigs.AddDefaulted_GetRef();
        TrNewMorphPair.ControlName = TsSafeName;
        ControlNameToRawName.FindOrAdd(TsSafeName) = ControlKeyRaw;

        /** Mapped to the same name */
        {
            FControlRigMorphConfig& TrNewMorphConfig = TrNewMorphPair.TargetMorphs.AddDefaulted_GetRef();
            TrNewMorphConfig.MorphName = TsSafeName;
            TrNewMorphConfig.MorphScale = 1.0f;
        }
    }
    Modify();
}

void UMotionDataAsset::PushMorphDataToLevelSequencer()
{
#if WITH_EDITOR
    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: --- Work start ---"));

    /** Ignore if no data */
    const int32 TdConfigNum = VmdControlRigMorphConfig.MorphMapConfigs.Num();
    if (TdConfigNum <= 0)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: VmdControlRigMorphConfig empty, ignored"));
        return;
    }

    ULevelSequence* TpLevelSeq = ULevelSequenceEditorBlueprintLibrary::GetFocusedLevelSequence();
    if (!IsValid(TpLevelSeq))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Bad level sequencer"));
        return;
    }

    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Sequencer=%s"), *GetNameSafe(TpLevelSeq));

    /** Print out debug info */
    {
        const TArray<FMovieSceneBindingProxy>& TarrSelBinding = ULevelSequenceEditorBlueprintLibrary::GetSelectedBindings();
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: GetSelectedBindings num=%d"), TarrSelBinding.Num());
        for (const FMovieSceneBindingProxy& IterSel : TarrSelBinding)
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > %s"), *IterSel.BindingID.ToString());
        }

        const TArray<UMovieSceneTrack*>& TarrSelectedTracks = ULevelSequenceEditorBlueprintLibrary::GetSelectedTracks();
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: GetSelectedTracks num=%d"), TarrSelectedTracks.Num());
        for (const UMovieSceneTrack* IterSel : TarrSelectedTracks)
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > %s"), *GetNameSafe(IterSel));
        }

        const TArray<FSequencerChannelProxy>& TarrSelectedChannels = ULevelSequenceEditorBlueprintLibrary::GetSelectedChannels();
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: GetSelectedChannels num=%d"), TarrSelectedChannels.Num());
        for (const FSequencerChannelProxy& IterSel : TarrSelectedChannels)
        {
            UMovieSceneSection* Section = IterSel.Section;
            if (Section)
            {
                UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > (%s)%s"),
                    *IterSel.ChannelName.ToString(),
                    *GetNameSafe(Section)
                );
            }
            else
            {
                UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > [bad]%s"), *IterSel.ChannelName.ToString());
            }
        }

        /** Gets the currently selected channels. */
        const TArray<UMovieSceneSection*>& TarrSelectedSections = ULevelSequenceEditorBlueprintLibrary::GetSelectedSections();
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: GetSelectedSections num=%d"), TarrSelectedSections.Num());
        for (const UMovieSceneSection* IterSel : TarrSelectedSections)
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > %s"), *GetNameSafe(IterSel));
        }
    }

    UMovieScene* TpMovieScene = TpLevelSeq->GetMovieScene();
    if (!IsValid(TpMovieScene))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Bad GetMovieScene"));
        return;
    }

    const TArray<UMovieSceneTrack*>& TarrSelectedTracks = ULevelSequenceEditorBlueprintLibrary::GetSelectedTracks();
    if (TarrSelectedTracks.Num() != 1)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: check failed, should select the control rig track, current cnt=%d"),
            TarrSelectedTracks.Num()
        );
        return;
    }

    UMovieSceneTrack* TpCurSceneTrack = TarrSelectedTracks[0];
    if (!IsValid(TpCurSceneTrack))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Bad current selected track ptr."));
        return;
    }

    const UMovieSceneControlRigParameterTrack* TpControlRigTrack = Cast<UMovieSceneControlRigParameterTrack>(TpCurSceneTrack);
    if (!IsValid(TpControlRigTrack))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Select track type failed, ptr=(%p(%s) class=%s"),
            TpCurSceneTrack,
            *GetFullNameSafe(TpCurSceneTrack),
            *GetFullNameSafe(TpCurSceneTrack->GetClass())
        );
        return;
    }

    /** Check binded control rig */
    const UControlRig* TpControlRig = TpControlRigTrack->GetControlRig();
    if (!IsValid(TpControlRig))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Bad control rig from track, ptr=(%p(%s) class=%s"),
            TpCurSceneTrack,
            *GetFullNameSafe(TpCurSceneTrack),
            *GetFullNameSafe(TpCurSceneTrack->GetClass())
        );
        return;
    }

    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Control rig=(%p)%s"), TpControlRig, *GetFullNameSafe(TpControlRig));

    const TArray<UMovieSceneSection*>& TrCrSections = TpControlRigTrack->GetAllSections();
    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: Section num=%d"), TrCrSections.Num());

    for (const UMovieSceneSection* Section : TrCrSections)
    {
        const UMovieSceneControlRigParameterSection* CRSection = CastChecked<UMovieSceneControlRigParameterSection>(Section);
        if (!IsValid(CRSection))
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > Bad type, ptr=(%p)%s"), Section, *GetNameSafe(Section));
            continue;
        }

        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > (%p)%s"), CRSection, *GetNameSafe(CRSection));
        for (const TPair<FName, FChannelMapInfo> IterChanelMap : CRSection->ControlChannelMap)
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: >>> %s %d"),
                *IterChanelMap.Key.ToString(),
                IterChanelMap.Value.TotalChannelIndex
            );
        }
    }

    FFrameRate TickResolution = TpMovieScene->GetTickResolution();
    FFrameRate DisplayRate = TpMovieScene->GetDisplayRate();

    FScopedSlowTask SlowTask(TdConfigNum, LOCTEXT("Op_Seuqencer_ControlRig", "Sending morph data to control rig in sequencer"));
    SlowTask.MakeDialog(false/*bShowCancelButton*/, true/*bAllowInPIE*/);

    /** Get and cache the section ptr */
    UMovieSceneSection* TpSection = TpControlRigTrack->GetSectionToKey();
    UMovieSceneControlRigParameterSection* CRSection = CastChecked<UMovieSceneControlRigParameterSection>(TpSection);
    if (!IsValid(CRSection))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Bad section, ptr=(%p)%s"), TpSection, *GetNameSafe(TpSection));
        return;
    }

    /**
     * Get channel proxy
     * this will call ReconstructChannelProxy which is needed before accessing the ControlChannelMap.
     * @note: The behavior of channel proxy is complicated, if the order changed during operation, we will not cache it.
     */
    FMovieSceneChannelProxy& TrChanelProxy = CRSection->GetChannelProxy();
    TArrayView<FMovieSceneFloatChannel*> FloatChannels = TrChanelProxy.GetChannels<FMovieSceneFloatChannel>();

    /** Cache handled raw data key */
    TSet<FString> RawDataHandledKeys;

    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) === Total count:%d ==="), VmdControlRigMorphConfig.MorphMapConfigs.Num());
    for (const FVmdControlRigMorphPair& IterPair: VmdControlRigMorphConfig.MorphMapConfigs)
    {
        SlowTask.EnterProgressFrame(1.0f, FText::FromName(IterPair.ControlName));

        /** Find the raw morph animation name */
        const FString* TpRawName = ControlNameToRawName.Find(IterPair.ControlName);
        if (!TpRawName)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Bad raw name, name=%s"),
                *IterPair.ControlName.ToString()
            );
            continue;
        }
        const FString& TstrRawName = *TpRawName;

        /** 
         * Check data exist
         * This will not be a warning, as the morph may not be animated
         */
        FVmdMorphTrackData* TpMorphTrackDataRaw = MorphTracks.Find(TstrRawName);
        if (!TpMorphTrackDataRaw)
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Ignore no data, name=%s rawname=%s"),
                *IterPair.ControlName.ToString(),
                *TstrRawName
            );
            continue;
        }

        /** Find channel info */
        FChannelMapInfo* pChannelIndex = CRSection->ControlChannelMap.Find(IterPair.ControlName);
        if (pChannelIndex == nullptr)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Bad control, name=%s"), *IterPair.ControlName.ToString());
            continue;
        }

        /** Check if control exist on rig  */
        int32 ChannelIndex = pChannelIndex->ChannelIndex;
        FRigControlElement* ControlElement = TpControlRig->FindControl(IterPair.ControlName);
        if (!ControlElement)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Control not on rig, name=%s"), *IterPair.ControlName.ToString());
            continue;
        }

        /** Check if rig type matches */
        if (ControlElement->Settings.ControlType != ERigControlType::Float)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Control type fail, name=%s val=%d"),
                *IterPair.ControlName.ToString(),
                (int32)ControlElement->Settings.ControlType
            );
            continue;
        }

        /** Get the float channel */
        FMovieSceneFloatChannel* TpFloatChannel = FloatChannels.IsValidIndex(ChannelIndex) ? FloatChannels[ChannelIndex] : nullptr;
        if(!TpFloatChannel)
        {
            UE_LOG(LogMmdHelper, Warning, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Bad channel, name=%s index=%d/%d"),
                *IterPair.ControlName.ToString(),
                ChannelIndex,
                FloatChannels.Num()
            );
            continue;
        }

        /** Start frame process */
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) Start for %s, ptr=(%p)%s channel[%d]=%p frame=%d"),
            *IterPair.ControlName.ToString(),
            TpSection, *GetNameSafe(TpSection),
            ChannelIndex,
            TpFloatChannel,
            TpMorphTrackDataRaw->Frames.Num()
        );
        for (const FVmdMorphFrameData& IterRawFrame : TpMorphTrackDataRaw->Frames)
        {
            const FFrameNumber TsCurFrame = FFrameRate::TransformTime(FFrameNumber((int32)IterRawFrame.Frame), DisplayRate, TickResolution).GetFrame();
            TpFloatChannel->AddCubicKey(TsCurFrame, IterRawFrame.Factor);
        }

        RawDataHandledKeys.Add(TstrRawName);
    }

    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer:(Push) === End pushed:%d/%d ==="),
        RawDataHandledKeys.Num(),
        MorphTracks.Num()
    );
    for (const TPair<FString, FVmdMorphTrackData>& IterMorphTrack : MorphTracks)
    {
        const FString& IterRawKey = IterMorphTrack.Key;
        if (RawDataHandledKeys.Contains(IterRawKey))
        {
            continue;
        }

        if (!MorphMapConfigs.Contains(IterRawKey))
        {
            UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > %s, not in MorphMapConfigs"), *IterRawKey);
            continue;
        }

        /** Ignored by logic above, its hard to put them together */
        UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: > %s, other error. Try search above log for detail"), *IterRawKey);
    }
    UE_LOG(LogMmdHelper, Log, TEXT("UMotionDataAsset::PushMorphDataToLevelSequencer: --- All work done ---"));


    /** 
     * Mark modify and notify changed, so other logic can have a chance to know we changed things
     * Sometimes sequencer editor may not update morph if it does not know we changed it, a re-save may help but it's confusing.
     */
    CRSection->Modify();
    CRSection->BroadcastChanged();
    TpMovieScene->Modify();
    TpMovieScene->BroadcastChanged();

#endif
}

void UMotionDataAsset::PreSave(FObjectPreSaveContext SaveContext)
{
    Super::PreSave(SaveContext);
}

#undef LOCTEXT_NAMESPACE
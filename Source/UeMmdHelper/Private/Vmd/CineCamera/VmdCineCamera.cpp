// Fill out your copyright notice in the Description page of Project Settings.


#include "Vmd/CineCamera/VmdCineCamera.h"

#include "UeMmdHelper.h"
#include "Vmd/CineCamera/VmdCineCameraComponent.h"
#include "Vmd/MotionDataAsset.h"
#include "Helper/MmdSequencerHelper.h"


#if WITH_EDITOR
#include "LevelSequenceEditorBlueprintLibrary.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Tracks/MovieSceneByteTrack.h"
#include "Sections/MovieSceneByteSection.h"
#include "Channels/MovieSceneByteChannel.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "Sections/MovieSceneFloatSection.h"
#endif



#define LOCTEXT_NAMESPACE "AVmdCineCamera"

static const FName MmdSpringArmLengthName = TEXT("VmdCameraTransLen");
static const FName CameraFovName = TEXT("FieldOfView");
static const FName ProjectionModeName = TEXT("ProjectionMode");




AVmdCineCamera::AVmdCineCamera(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer
        .SetDefaultSubobjectClass<UVmdCineCameraComponent>(TEXT("CameraComponent"))
    )
{
}

void AVmdCineCamera::SyncCameraMotion()
{
    switch (InterpType)
    {
    case EVmdCineCameraFrameType::EVCCFT_KeyOnly:
    {
        SyncCameraMotion_KeyOnly();
        break;
    }
    case EVmdCineCameraFrameType::EVCCFT_Interp:
    {
        SyncCameraMotion_Interped();
        break;
    }
    default:
    {
        UE_LOG(LogMmdHelper, Error, TEXT("AVmdCineCamera::SyncCameraMotion: Bad InterpType"));
        break;
    }
    }
}

void AVmdCineCamera::SyncCameraMotion_KeyOnly()
{
#if WITH_EDITOR
    /** Get focused sequence */
    ULevelSequence* TpLevelSeq = ULevelSequenceEditorBlueprintLibrary::GetFocusedLevelSequence();
    if (!IsValid(TpLevelSeq))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: Bad level sequencer"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No Level Sequence", "Could not find active level sequence")
        );
        return;
    }

    UMovieScene* TpMovieScene = TpLevelSeq->GetMovieScene();
    if (!IsValid(TpMovieScene))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: Bad GetMovieScene"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No Movie Sequence", "Could not find movie sequence in level sequence")
        );
        return;
    }

    /** Get motion data */
    UMotionDataAsset* TpMotionData = GetMotionData();
    if (!TpMotionData)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: Bad GetMotionData"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No MotionData", "MotionData not found, please check the `MotionData` config")
        );
        return;
    }

    /** Get config */
    const float TfDistScaleBias = GetDistanceScaleBias();
    const FTransform TsTargetTrans = GetCenterTrans();

    UE_LOG(LogMmdHelper, Log, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: Camera=%p)%s"), this, *GetNameSafe(this));
    const FScopedTransaction Transaction(LOCTEXT("SyncCameraMotion_KeyOnly", "Apply VMD Camera motions"));
    FScopedSlowTask SlowTask(3.0f, FText::Format(LOCTEXT("BeginSyncMotionDatas", "Sync motion data {0}"), FText::FromName(TpMotionData->GetFName())));
    SlowTask.MakeDialog(false/*bShowCancelButton*/, true/*bAllowInPIE*/);

    /** Prepare data for time convert */
    FFrameRate TickResolution = TpMovieScene->GetTickResolution();
    FFrameRate DisplayRate = TpMovieScene->GetDisplayRate();

    //////////////////////////////////////////////////////////////////////////
    /** Processing camera transform track */
    do
    {
        const FGuid PossessableGuid = UMmdSequencerHelper::BindActorToLevelSequence(this, TpLevelSeq);

        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera trans", "Camera trans"));
        UMovieScene3DTransformTrack* TransformTrack = TpMovieScene->FindTrack<UMovieScene3DTransformTrack>(PossessableGuid);
        if (!TransformTrack)
        {
            TransformTrack = TpMovieScene->AddTrack<UMovieScene3DTransformTrack>(PossessableGuid);
        }
        else
        {
            TransformTrack->RemoveAllAnimationData();
        }

        UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
        TransformSection->SetMask(FMovieSceneTransformMask(EMovieSceneTransformChannel::Translation | EMovieSceneTransformChannel::Rotation));

        TransformTrack->AddSection(*TransformSection);
        TransformSection->SetRange(TRange<FFrameNumber>::All());

        FMovieSceneChannelProxy& TrChanelProxy = TransformSection->GetChannelProxy();
        for (const FVmdCameraFrameData& IterCameraFrame : TpMotionData->CameraFrames)
        {
            const FFrameNumber TsCurFrame = FFrameRate::TransformTime(FFrameNumber((int32)IterCameraFrame.Frame), DisplayRate, TickResolution).GetFrame();

            const FVector TfvCenterOffset = FVector(IterCameraFrame.Location.Z, IterCameraFrame.Location.X, IterCameraFrame.Location.Y) * TfDistScaleBias;
            const FRotator TfrRot = FRotator(-IterCameraFrame.Rotate.X, -IterCameraFrame.Rotate.Y, IterCameraFrame.Rotate.Z);
            const float TfCameraLen = IterCameraFrame.Length * TfDistScaleBias;

            const FTransform TsFinalTrans = UMmdSequencerHelper::GetConvertedCameraTrans(TsTargetTrans, TfvCenterOffset, TfrRot, TfCameraLen);
            const FRotator& TfrFinalRot = TsFinalTrans.GetRotation().Rotator();

            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(0)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().X);
            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(1)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Y);
            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(2)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Z);

            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(3)->AddCubicKey(TsCurFrame, TfrFinalRot.Roll);
            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(4)->AddCubicKey(TsCurFrame, TfrFinalRot.Pitch);
            TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(5)->AddCubicKey(TsCurFrame, TfrFinalRot.Yaw);

        }
    } while (false);


    //////////////////////////////////////////////////////////////////////////
    /** Processing camera FOV track */
    UCineCameraComponent* TpCamera = GetCineCameraComponent();
    const FGuid CameraGuid = UMmdSequencerHelper::BindComponentToLevelSequence(TpCamera, TpLevelSeq);

    do
    {
        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera fov", "Camera fov"));
        UMovieSceneFloatTrack* FovTrack = TpMovieScene->FindTrack<UMovieSceneFloatTrack>(CameraGuid, CameraFovName);
        if (!FovTrack)
        {
            FovTrack = TpMovieScene->AddTrack<UMovieSceneFloatTrack>(CameraGuid);
        }
        else
        {
            FovTrack->RemoveAllAnimationData();
        }

        FovTrack->SetPropertyNameAndPath(CameraFovName, CameraFovName.ToString());
        UMovieSceneFloatSection* FovSection = Cast<UMovieSceneFloatSection>(FovTrack->CreateNewSection());

        FovTrack->AddSection(*FovSection);
        FovSection->SetRange(TRange<FFrameNumber>::All());

        const float TfFovScale = GetViewAngelBias();

        for (const FVmdCameraFrameData& IterCameraFrame : TpMotionData->CameraFrames)
        {
            FovSection->GetChannel().AddCubicKey(
                FFrameRate::TransformTime(FFrameNumber((int32)IterCameraFrame.Frame), DisplayRate, TickResolution).GetFrame(),
                IterCameraFrame.ViewingAngle * TfFovScale
            );
        }
    } while (false);

    do
    {
        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera ProjectionMode", "Camera ProjectionMode"));
        UMovieSceneByteTrack* ProjectionModeTrack = TpMovieScene->FindTrack<UMovieSceneByteTrack>(CameraGuid, ProjectionModeName);
        if (!ProjectionModeTrack)
        {
            ProjectionModeTrack = TpMovieScene->AddTrack<UMovieSceneByteTrack>(CameraGuid);
        }
        else
        {
            ProjectionModeTrack->RemoveAllAnimationData();
        }

        UEnum* TpProjectionEnum = FindObject<UEnum>(nullptr, TEXT("/Script/Engine.ECameraProjectionMode"), EFindObjectFlags::ExactClass);
        UE_LOG(LogMmdHelper, Log, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: ProjectionMode, enum=%s"), *GetNameSafe(TpProjectionEnum));
        ProjectionModeTrack->SetEnum(TpProjectionEnum);

        ProjectionModeTrack->SetPropertyNameAndPath(ProjectionModeName, ProjectionModeName.ToString());
        UMovieSceneByteSection* ProjectionModeSection = Cast<UMovieSceneByteSection>(ProjectionModeTrack->CreateNewSection());

        ProjectionModeTrack->AddSection(*ProjectionModeSection);
        ProjectionModeSection->SetRange(TRange<FFrameNumber>::All());

        FMovieSceneByteChannel* Channel = ProjectionModeSection->GetChannelProxy().GetChannel<FMovieSceneByteChannel>(0);

        for (const FVmdCameraFrameData& IterCameraFrame : TpMotionData->CameraFrames)
        {
            Channel->GetData().AddKey(
                FFrameRate::TransformTime(FFrameNumber((int32)IterCameraFrame.Frame), DisplayRate, TickResolution).GetFrame(),
                UMmdSequencerHelper::ConvertFromVmdCameraPerspective(IterCameraFrame.Perspective)
            );
        }

    } while (false);
#endif
}

void AVmdCineCamera::CopyCameraTrans()
{
    /** If config is ready, CopyCameraTrans is not supposed to be called */
    if (bConfigReady)
    {
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("Config not ready", "bConfigReady is true, could not modify")
        );
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::CopyCameraTrans: bConfigReady is true, no modify"));
        return;
    }

    const FTransform& TsActorTrans = GetActorTransform();
    UE_LOG(LogMmdHelper, Log, TEXT("AVmdCineCamera::CopyCameraTrans: CenterTrans modified, current=%s new=%s"),
        *CenterTrans.ToString(),
        *TsActorTrans.ToString()
    );
    CenterTrans = TsActorTrans;
}

void AVmdCineCamera::SyncCameraMotion_Interped()
{
    /** Get focused sequence */
    ULevelSequence* TpLevelSeq = ULevelSequenceEditorBlueprintLibrary::GetFocusedLevelSequence();
    if (!IsValid(TpLevelSeq))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_Interped: Bad level sequencer"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No Level Sequence", "Could not find active level sequence")
        );
        return;
    }

    UMovieScene* TpMovieScene = TpLevelSeq->GetMovieScene();
    if (!IsValid(TpMovieScene))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_Interped: Bad GetMovieScene"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No Movie Sequence", "Could not find movie sequence in level sequence")
        );
        return;
    }

    /** Get motion data */
    UMotionDataAsset* TpMotionData = GetMotionData();
    if (!TpMotionData)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("AVmdCineCamera::SyncCameraMotion_Interped: Bad GetMotionData"));
        FMessageDialog::Open(EAppMsgCategory::Error, EAppMsgType::Type::Ok,
            LOCTEXT("No MotionData", "MotionData not found, please check the `MotionData` config")
        );
        return;
    }

    /** Prepare data for time convert */
    FFrameRate TickResolution = TpMovieScene->GetTickResolution();
    FFrameRate DisplayRate = TpMovieScene->GetDisplayRate();

    const FScopedTransaction Transaction(LOCTEXT("SyncCameraMotion_KeyOnly", "Apply VMD Camera motions"));
    FScopedSlowTask SlowTask(3.0f, FText::Format(LOCTEXT("BeginSyncMotionDatas", "Sync motion data {0}"), FText::FromName(TpMotionData->GetFName())));
    SlowTask.MakeDialog(false/*bShowCancelButton*/, true/*bAllowInPIE*/);

    {
        /** Get config */
        const float TfDistScaleBias = GetDistanceScaleBias();
        const FTransform TsTargetTrans = GetCenterTrans();

        const FGuid PossessableGuid = UMmdSequencerHelper::BindActorToLevelSequence(this, TpLevelSeq);

        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera trans", "Camera trans"));

        /** Find or create track */
        UMovieScene3DTransformTrack* TransformTrack = TpMovieScene->FindTrack<UMovieScene3DTransformTrack>(PossessableGuid);
        if (!TransformTrack)
        {
            TransformTrack = TpMovieScene->AddTrack<UMovieScene3DTransformTrack>(PossessableGuid);
        }
        else
        {
            TransformTrack->RemoveAllAnimationData();
        }



        /** Find channel proxy */
        UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(TransformTrack->CreateNewSection());
        TransformSection->SetMask(FMovieSceneTransformMask(EMovieSceneTransformChannel::Translation | EMovieSceneTransformChannel::Rotation));

        TransformTrack->AddSection(*TransformSection);
        TransformSection->SetRange(TRange<FFrameNumber>::All());

        FMovieSceneChannelProxy& TrChanelProxy = TransformSection->GetChannelProxy();

        /** Start data process */
        const TArray<FVmdCameraFrameData>& TarrCameraFrameRaw = TpMotionData->CameraFrames;
        for (int32 IterFrameIdx = 0; IterFrameIdx < TarrCameraFrameRaw.Num(); ++IterFrameIdx)
        {
            const FVmdCameraFrameData& TrCurrentRawFrame = TarrCameraFrameRaw[IterFrameIdx];
            const uint32 TdCurRawFrameCnt = TrCurrentRawFrame.Frame;

            /** Push current frame */
            {
                const FVmdCameraFrameData& IterCameraFrame = TrCurrentRawFrame;

                const FFrameNumber TsCurFrame = FFrameRate::TransformTime(FFrameNumber((int32)IterCameraFrame.Frame), DisplayRate, TickResolution).GetFrame();

                const FVector TfvCenterOffset = FVector(IterCameraFrame.Location.Z, IterCameraFrame.Location.X, IterCameraFrame.Location.Y) * TfDistScaleBias;
                const FRotator TfrRot = FRotator(-IterCameraFrame.Rotate.X, -IterCameraFrame.Rotate.Y, IterCameraFrame.Rotate.Z);
                const float TfCameraLen = IterCameraFrame.Length * TfDistScaleBias;
                UE_LOG(LogMmdHelper, Log, TEXT("Interp camera(Key), frame=%u loc=%s rot=%s lan=%f"), TdCurRawFrameCnt, *TfvCenterOffset.ToString(), *TfrRot.ToString(), TfCameraLen);

                const FTransform TsFinalTrans = UMmdSequencerHelper::GetConvertedCameraTrans(TsTargetTrans, TfvCenterOffset, TfrRot, TfCameraLen);
                const FRotator& TfrFinalRot = TsFinalTrans.GetRotation().Rotator();

                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(0)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().X);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(1)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Y);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(2)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Z);

                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(3)->AddCubicKey(TsCurFrame, TfrFinalRot.Roll);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(4)->AddCubicKey(TsCurFrame, TfrFinalRot.Pitch);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(5)->AddCubicKey(TsCurFrame, TfrFinalRot.Yaw);
            }

            /** Check next data existence */
            const int32 TdNextIdx = IterFrameIdx + 1;
            if (!TarrCameraFrameRaw.IsValidIndex(TdNextIdx))
            {
                continue;
            }

            const FVmdCameraFrameData& TrNextRawFrame = TarrCameraFrameRaw[TdNextIdx];
            const uint32 TdNextRawFrameCnt = TrNextRawFrame.Frame;

            /** Interp frame */
            const float TfInterpSpan = TdNextRawFrameCnt - TdCurRawFrameCnt;
            for (uint32 IterFrameCreateIdx = TdCurRawFrameCnt + 1; IterFrameCreateIdx < TdNextRawFrameCnt; ++IterFrameCreateIdx)
            {
                /** interpolation */
                const float TfInterpTime = (IterFrameCreateIdx - TdCurRawFrameCnt) / TfInterpSpan;

                const float TfLocationInperpX = FMath::FInterpTo(TrCurrentRawFrame.Location.X, TrNextRawFrame.Location.X, TrCurrentRawFrame.BezierLocation_X.Evaluate(TfInterpTime), 1.0f);
                const float TfLocationInperpY = FMath::FInterpTo(TrCurrentRawFrame.Location.Y, TrNextRawFrame.Location.Y, TrCurrentRawFrame.BezierLocation_Y.Evaluate(TfInterpTime), 1.0f);
                const float TfLocationInperpZ = FMath::FInterpTo(TrCurrentRawFrame.Location.Z, TrNextRawFrame.Location.Z, TrCurrentRawFrame.BezierLocation_Z.Evaluate(TfInterpTime), 1.0f);

                const float TfDistanceInperp = FMath::FInterpTo(TrCurrentRawFrame.Length, TrNextRawFrame.Length, TrCurrentRawFrame.BezierDistance.Evaluate(TfInterpTime), 1.0f);

                const FVector TfvRotationInperp = FMath::VInterpTo(TrCurrentRawFrame.Rotate, TrNextRawFrame.Rotate, TrCurrentRawFrame.BezierRotation.Evaluate(TfInterpTime), 1.0f);

                /** calculate data */
                const FFrameNumber TsCurFrame = FFrameRate::TransformTime(FFrameNumber((int32)IterFrameCreateIdx), DisplayRate, TickResolution).GetFrame();

                const FVector TfvCenterOffset = FVector(TfLocationInperpZ, TfLocationInperpX, TfLocationInperpY) * TfDistScaleBias;
                const FRotator TfrRot = FRotator(-TfvRotationInperp.X, -TfvRotationInperp.Y, TfvRotationInperp.Z);
                const float TfCameraLen = TfDistanceInperp * TfDistScaleBias;

                UE_LOG(LogMmdHelper, Log, TEXT("Interp camera, frame=%u loc=%s rot=%s lan=%f"), IterFrameCreateIdx, *TfvRotationInperp.ToString(), *TfrRot.ToString(), TfCameraLen);
                const FTransform TsFinalTrans = UMmdSequencerHelper::GetConvertedCameraTrans(TsTargetTrans, TfvCenterOffset, TfrRot, TfCameraLen);
                const FRotator& TfrFinalRot = TsFinalTrans.GetRotation().Rotator();

                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(0)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().X);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(1)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Y);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(2)->AddCubicKey(TsCurFrame, TsFinalTrans.GetLocation().Z);

                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(3)->AddCubicKey(TsCurFrame, TfrFinalRot.Roll);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(4)->AddCubicKey(TsCurFrame, TfrFinalRot.Pitch);
                TrChanelProxy.GetChannel<FMovieSceneDoubleChannel>(5)->AddCubicKey(TsCurFrame, TfrFinalRot.Yaw);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    /** Processing camera FOV track */
    UCineCameraComponent* TpCamera = GetCineCameraComponent();
    const FGuid CameraGuid = UMmdSequencerHelper::BindComponentToLevelSequence(TpCamera, TpLevelSeq);

    do
    {
        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera fov", "Camera fov"));
        UMovieSceneFloatTrack* FovTrack = TpMovieScene->FindTrack<UMovieSceneFloatTrack>(CameraGuid, CameraFovName);
        if (!FovTrack)
        {
            FovTrack = TpMovieScene->AddTrack<UMovieSceneFloatTrack>(CameraGuid);
        }
        else
        {
            FovTrack->RemoveAllAnimationData();
        }

        FovTrack->SetPropertyNameAndPath(CameraFovName, CameraFovName.ToString());
        UMovieSceneFloatSection* FovSection = Cast<UMovieSceneFloatSection>(FovTrack->CreateNewSection());

        FovTrack->AddSection(*FovSection);
        FovSection->SetRange(TRange<FFrameNumber>::All());

        const float TfFovScale = GetViewAngelBias();

        const TArray<FVmdCameraFrameData>& TarrCameraFrameRaw = TpMotionData->CameraFrames;
        for (int32 IterFrameIdx = 0; IterFrameIdx < TarrCameraFrameRaw.Num(); ++IterFrameIdx)
        {
            const FVmdCameraFrameData& TrCurrentRawFrame = TarrCameraFrameRaw[IterFrameIdx];
            const uint32 TdCurRawFrameCnt = TrCurrentRawFrame.Frame;

            {
                UE_LOG(LogMmdHelper, Log, TEXT("Interp pov(Key), frame=%u pov=%u"), TdCurRawFrameCnt, TrCurrentRawFrame.ViewingAngle);
                FovSection->GetChannel().AddCubicKey(
                    FFrameRate::TransformTime(FFrameNumber((int32)TrCurrentRawFrame.Frame), DisplayRate, TickResolution).GetFrame(),
                    TrCurrentRawFrame.ViewingAngle* TfFovScale
                );
            }

            /** Check next data existence */
            const int32 TdNextIdx = IterFrameIdx + 1;
            if (!TarrCameraFrameRaw.IsValidIndex(TdNextIdx))
            {
                continue;
            }

            const FVmdCameraFrameData& TrNextRawFrame = TarrCameraFrameRaw[TdNextIdx];
            const uint32 TdNextRawFrameCnt = TrNextRawFrame.Frame;

            /** Interp frame */
            const float TfInterpSpan = TdNextRawFrameCnt - TdCurRawFrameCnt;
            for (uint32 IterFrameCreateIdx = TdCurRawFrameCnt + 1; IterFrameCreateIdx < TdNextRawFrameCnt; ++IterFrameCreateIdx)
            {
                /** interpolation */
                const float TfInterpTime = (IterFrameCreateIdx - TdCurRawFrameCnt) / TfInterpSpan;
                const float TfInperpPov = FMath::FInterpTo((float)TrCurrentRawFrame.ViewingAngle, (float)TrNextRawFrame.ViewingAngle, TrCurrentRawFrame.BezierFOV.Evaluate(TfInterpTime), 1.0f);

                UE_LOG(LogMmdHelper, Log, TEXT("Interp pov, frame=%u pov=%f(%u,%u,%f)"), IterFrameCreateIdx, TfInperpPov,
                    TrCurrentRawFrame.ViewingAngle,
                    TrNextRawFrame.ViewingAngle,
                    TfInterpTime
                );

                const FFrameNumber TsCurFrame = FFrameRate::TransformTime(FFrameNumber((int32)TdCurRawFrameCnt), DisplayRate, TickResolution).GetFrame();

                FovSection->GetChannel().AddCubicKey(
                    FFrameRate::TransformTime(TsCurFrame, DisplayRate, TickResolution).GetFrame(),
                    TfInperpPov * TfFovScale
                );
            }
        }

    } while (false);

    /** Processing camera ProjectionMode track */
    do
    {
        SlowTask.EnterProgressFrame(1.0f, LOCTEXT("Camera ProjectionMode", "Camera ProjectionMode"));
        UMovieSceneByteTrack* ProjectionModeTrack = TpMovieScene->FindTrack<UMovieSceneByteTrack>(CameraGuid, ProjectionModeName);
        if (!ProjectionModeTrack)
        {
            ProjectionModeTrack = TpMovieScene->AddTrack<UMovieSceneByteTrack>(CameraGuid);
        }
        else
        {
            ProjectionModeTrack->RemoveAllAnimationData();
        }

        UEnum* TpProjectionEnum = FindObject<UEnum>(nullptr, TEXT("/Script/Engine.ECameraProjectionMode"), EFindObjectFlags::ExactClass);
        UE_LOG(LogMmdHelper, Log, TEXT("AVmdCineCamera::SyncCameraMotion_KeyOnly: ProjectionMode, enum=%s"), *GetNameSafe(TpProjectionEnum));
        ProjectionModeTrack->SetEnum(TpProjectionEnum);

        ProjectionModeTrack->SetPropertyNameAndPath(ProjectionModeName, ProjectionModeName.ToString());
        UMovieSceneByteSection* ProjectionModeSection = Cast<UMovieSceneByteSection>(ProjectionModeTrack->CreateNewSection());

        ProjectionModeTrack->AddSection(*ProjectionModeSection);
        ProjectionModeSection->SetRange(TRange<FFrameNumber>::All());

        FMovieSceneByteChannel* Channel = ProjectionModeSection->GetChannelProxy().GetChannel<FMovieSceneByteChannel>(0);

        for (const FVmdCameraFrameData& IterCameraFrame : TpMotionData->CameraFrames)
        {
            Channel->GetData().AddKey(
                FFrameRate::TransformTime(FFrameNumber((int32)IterCameraFrame.Frame), DisplayRate, TickResolution).GetFrame(),
                UMmdSequencerHelper::ConvertFromVmdCameraPerspective(IterCameraFrame.Perspective)
            );
        }

    } while (false);
   
}

#undef LOCTEXT_NAMESPACE

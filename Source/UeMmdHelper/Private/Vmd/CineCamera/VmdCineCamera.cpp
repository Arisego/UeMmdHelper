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
#if WITH_EDITOR
    /** Get focused sequence */
    ULevelSequence* TpLevelSeq = ULevelSequenceEditorBlueprintLibrary::GetFocusedLevelSequence();
    if (!IsValid(TpLevelSeq))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("FCameraTrackHelper::ApplyCineCameraMotions: Bad level sequencer"));
        return;
    }

    UMovieScene* TpMovieScene = TpLevelSeq->GetMovieScene();
    if (!IsValid(TpMovieScene))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("FCameraTrackHelper::ApplyCineCameraMotions: Bad GetMovieScene"));
        return;
    }

    /** Get motion data */
    UMotionDataAsset* TpMotionData = GetMotionData();
    if (!TpMotionData)
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("FCameraTrackHelper::ApplyCineCameraMotions: Bad GetMotionData"));
        return;
    }

    /** Get config */
    const float TfDistScaleBias = GetDistanceScaleBias();
    const FTransform TsTargetTrans = GetCenterTrans();

    UE_LOG(LogMmdHelper, Log, TEXT("AMmdCamera::ApplyCineCameraMotions: Camera=%p)%s"), this, *GetNameSafe(this));
    const FScopedTransaction Transaction(LOCTEXT("ApplyCineCameraMotions", "Apply VMD Camera motions"));
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

        UEnum* TpProjectionEnum = FindObject<UEnum>(((UPackage*)-1), TEXT("ECameraProjectionMode"), true);
        UE_LOG(LogMmdHelper, Log, TEXT("FCameraTrackHelper::ApplyCineCameraMotions: ProjectionMode, enum=%s"), *GetNameSafe(TpProjectionEnum));
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


#undef LOCTEXT_NAMESPACE

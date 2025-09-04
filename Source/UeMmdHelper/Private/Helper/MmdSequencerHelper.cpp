// Fill out your copyright notice in the Description page of Project Settings.

#include "MmdSequencerHelper.h"

#include "UeMmdHelper.h"
#include "LevelSequence.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieScene.h"


FGuid UMmdSequencerHelper::BindActorToLevelSequence(AActor* InActor, class ULevelSequence* InLevelSequence)
{
    FGuid TsActorBinding;

#if WITH_EDITOR
    if (!IsValid(InActor))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad actor"));
        return TsActorBinding;
    }

    if (!IsValid(InLevelSequence))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad LevelSequence"));
        return TsActorBinding;
    }

    UMovieScene* TpMovieScene = InLevelSequence->GetMovieScene();
    if (!IsValid(InLevelSequence))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad GetMovieScene"));
        return TsActorBinding;
    }

    TSharedRef<const UE::MovieScene::FSharedPlaybackState> SharedPlaybackState = MovieSceneHelpers::CreateTransientSharedPlaybackState(InActor, InLevelSequence);
    TsActorBinding = InLevelSequence->FindBindingFromObject(InActor, SharedPlaybackState);
    if (!TsActorBinding.IsValid())
    {
        TsActorBinding = TpMovieScene->AddPossessable(InActor->GetActorLabel(), InActor->GetClass());
        InLevelSequence->BindPossessableObject(TsActorBinding, *InActor, InActor->GetWorld());
    }
#endif
    return TsActorBinding;
}

FGuid UMmdSequencerHelper::BindComponentToLevelSequence(class UActorComponent* Component, class ULevelSequence* InLevelSequence)
{
    FGuid ComponentBinding;
    if (!IsValid(Component))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad InComponent"));
        return ComponentBinding;
    }

    if (!IsValid(InLevelSequence))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad LevelSequence"));
        return ComponentBinding;
    }

    UMovieScene* TpMovieScene = InLevelSequence->GetMovieScene();
    if (!IsValid(InLevelSequence))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad GetMovieScene"));
        return ComponentBinding;
    }

    AActor* TpOwnerActor = Component->GetOwner();
    if (!IsValid(TpOwnerActor))
    {
        UE_LOG(LogMmdHelper, Warning, TEXT("UMmdSequencerHelper::BindActorToLevelSequence: Bad GetOwner"));
        return ComponentBinding;
    }

    TSharedRef<const UE::MovieScene::FSharedPlaybackState> SharedPlaybackState = MovieSceneHelpers::CreateTransientSharedPlaybackState(Component, InLevelSequence);
    FGuid ActorBinding = BindActorToLevelSequence(TpOwnerActor, InLevelSequence);

    ComponentBinding = InLevelSequence->FindBindingFromObject(Component, SharedPlaybackState);
    if (!ComponentBinding.IsValid())
    {
        ComponentBinding = TpMovieScene->AddPossessable(Component->GetReadableName(), Component->GetClass());

        if (ActorBinding.IsValid() && ComponentBinding.IsValid())
        {
            if (FMovieScenePossessable* ComponentPossessable = TpMovieScene->FindPossessable(ComponentBinding))
            {
                ComponentPossessable->SetParent(ActorBinding, TpMovieScene);
            }
        }

        // Bind component
        InLevelSequence->BindPossessableObject(ComponentBinding, *Component, TpOwnerActor);
    }

    return ComponentBinding;
}

FTransform UMmdSequencerHelper::GetConvertedCameraTrans(const FTransform& InBaseTrans, const FVector& InOffset, const FRotator& InRot, const float InDistance)
{
    /** Add relative transform on basic */
    const FVector TsTransformedLoc = InBaseTrans.TransformPosition(InOffset);
    const FQuat TsTansformedRot = InBaseTrans.TransformRotation(InRot.Quaternion());

    /** Move transform on direction */
    const FVector TfrDirection = TsTansformedRot.Vector();
    const FVector FinalPosition = TsTransformedLoc + TfrDirection * InDistance;

    return FTransform(TsTansformedRot, FinalPosition);
}

ECameraProjectionMode::Type UMmdSequencerHelper::ConvertFromVmdCameraPerspective(const uint8 InVal)
{
    switch (InVal)
    {
    case 0:
        return ECameraProjectionMode::Perspective;
    case 1:
        return ECameraProjectionMode::Orthographic;
    default:
        /** Undefined camera type */
        checkNoEntry();
        break;
    }

    return ECameraProjectionMode::Perspective;
}

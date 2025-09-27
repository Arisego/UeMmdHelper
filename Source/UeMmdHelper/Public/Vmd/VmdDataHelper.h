// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


struct FVmdData;
namespace FVmdDataHelper
{
    /** 
     * Load motion data from vmd file
     * 
     * @param InFilePath Absolute path of vmd file
     * @param OutData Data to fill
     */
    void LoadVmdDataFromFile(const FString& InFilePath, FVmdData& OutData);

    FString ConvertFromMmdName(const char* InName);
}

//////////////////////////////////////////////////////////////////////////
/** Defines of raw vmd data */

struct FVmdHeader
{
    char MagicHeader[30];	
    char TargetModelName[20];	

public:
    friend FArchive& operator<<(FArchive& Ar, FVmdHeader& P)
    {
        Ar.Serialize(P.MagicHeader, sizeof(P.MagicHeader));
        Ar.Serialize(P.TargetModelName, sizeof(P.TargetModelName));
        return Ar;
    }
};

struct FVmdBoneFrame
{
    char Name[15];
    uint32 Frame;
    float Position[3];
    float Quaternion[4];
    uint8 Bezier[2][2][4];

public:
    friend FArchive& operator<<(FArchive& Ar, FVmdBoneFrame& P)
    {
        Ar.Serialize(P.Name, sizeof(P.Name));
        Ar << P.Frame;

        Ar.Serialize(P.Position, sizeof(P.Position));
        Ar.Serialize(P.Quaternion, sizeof(P.Quaternion));
        Ar.Serialize(P.Bezier, sizeof(P.Bezier));

        /** Dummy */
        Ar.Seek(Ar.Tell() + (48 * sizeof(uint8)));

        return Ar;
    }
};

struct FVmdFaceFrame
{
    char	Name[15];
    uint32	Frame;
    float	Factor;

public:
    friend FArchive& operator<<(FArchive& Ar, FVmdFaceFrame& P)
    {
        Ar.Serialize(P.Name, sizeof(P.Name));
        Ar << P.Frame;
        Ar << P.Factor;
        return Ar;
    }
};

struct FVmdCameraFrame
{
    uint32	Frame;
    float	Length;
    float	Location[3];
    float	Rotate[3];
    uint8	Interpolation[6][2][2];
    uint32	ViewingAngle;
    uint8	Perspective;

public:
    friend FArchive& operator<<(FArchive& Ar, FVmdCameraFrame& P)
    {
        Ar << P.Frame;
        Ar << P.Length;

        Ar.Serialize(P.Location, sizeof(P.Location));
        Ar.Serialize(P.Rotate, sizeof(P.Rotate));
        Ar.Serialize(P.Interpolation, sizeof(P.Interpolation));

        Ar << P.ViewingAngle;
        Ar << P.Perspective;

        return Ar;
    }

};

struct FVmdBoneTracks
{
    TArray<FVmdBoneFrame> BoneFrames;
    TArray<FVmdFaceFrame> FaceFrames;
    TArray<FVmdCameraFrame> CameraFrames;

public:
    friend FArchive& operator<<(FArchive& Ar, FVmdBoneTracks& P)
    {
        int32 CountReadIn;

        Ar << CountReadIn;
        P.BoneFrames.SetNumZeroed(CountReadIn);
        for (FVmdBoneFrame& IterFrame : P.BoneFrames)
        {
            Ar << IterFrame;
        }

        Ar << CountReadIn;
        P.FaceFrames.SetNumZeroed(CountReadIn);
        for (FVmdFaceFrame& IterFrame : P.FaceFrames)
        {
            Ar << IterFrame;
        }

        Ar << CountReadIn;
        P.CameraFrames.SetNumZeroed(CountReadIn);
        for (FVmdCameraFrame& IterFrame : P.CameraFrames)
        {
            Ar << IterFrame;
        }

        return Ar;
    }
};

/**
 * The top level struct of vmd data
 * Can read data from FArchive
 */
struct FVmdData
{
public:
    void PrintOutData();
    const FVmdBoneTracks& GetTrackData() const {return TrackData;}
    const FVmdHeader& GetVmdHeader() const {return VmdHeader;}

    friend FArchive& operator<<(FArchive& Ar, FVmdData& P)
    {
        Ar << P.VmdHeader;
        if (strcmp(P.VmdHeader.MagicHeader, "Vocaloid Motion Data 0002"))
        {
            UE_LOG(LogTemp, Warning, TEXT("FVmdData::ReadFromFile: Header check failed"));
            Ar.SetError();
            return Ar;
        }

        Ar << P.TrackData;
        return Ar;
    }

private:
    FVmdHeader VmdHeader;
    FVmdBoneTracks TrackData;
};

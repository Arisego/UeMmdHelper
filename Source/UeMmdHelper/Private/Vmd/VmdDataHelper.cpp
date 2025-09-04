// Fill out your copyright notice in the Description page of Project Settings.

#include "Vmd/VmdDataHelper.h"

#include "Miscs/SjisToUnicode.h"
#include "UeMmdHelper.h"
#include "HAL/FileManagerGeneric.h"



void FVmdDataHelper::LoadVmdDataFromFile(const FString& InFilePath, FVmdData& OutData)
{
    /** get the file handler */
    IPlatformFile& FileSystem = IPlatformFile::GetPlatformPhysical();
    IFileHandle* FileHandle(FileSystem.OpenRead(*InFilePath));

    if (!ensure(FileHandle))
    {
        UE_LOG(LogMmdHelper, Error, TEXT("FVmdDataHelper::LoadVmdDataFromFile: Failed create handle %s"), *InFilePath);
        return;
    }

    UE_LOG(LogMmdHelper, Log, TEXT("FVmdDataHelper::LoadVmdDataFromFile: Start, file=%s size=%lld"), *InFilePath, FileHandle->Size());

    /** init file reader */
    FArchiveFileReaderGeneric FileReader(FileHandle, *InFilePath, FileHandle->Size());
    FileReader << OutData;
    FileReader.Close();

    UE_LOG(LogMmdHelper, Log, TEXT("FVmdDataHelper::LoadVmdDataFromFile: Ends"));
}

FString FVmdDataHelper::ConvertFromMmdName(const char* InName)
{
    return FString((wchar_t*)saba::ConvertSjisToU16String(InName).c_str());
}

void FVmdData::PrintOutData()
{
    UE_LOG(LogMmdHelper, Log, TEXT("------ FVmdData::PrintOutData: Starte ------"));

    UE_LOG(LogMmdHelper, Log, TEXT("Magic:%s"), ANSI_TO_TCHAR(VmdHeader.MagicHeader));
    UE_LOG(LogMmdHelper, Log, TEXT("Model:%s"), ANSI_TO_TCHAR(VmdHeader.TargetModelName));

    UE_LOG(LogMmdHelper, Log, TEXT("BoneFrames:%d"), TrackData.BoneFrames.Num());
    UE_LOG(LogMmdHelper, Log, TEXT("FaceFrames:%d"), TrackData.FaceFrames.Num());
    UE_LOG(LogMmdHelper, Log, TEXT("CameraFrames:%d"), TrackData.CameraFrames.Num());

    UE_LOG(LogMmdHelper, Log, TEXT("== BoneFrame info =="));
    for (const FVmdBoneFrame& IterFrame : TrackData.BoneFrames)
    {
        ;
        UE_LOG(LogMmdHelper, Log, TEXT("Name:%s Frame:%d"), *FVmdDataHelper::ConvertFromMmdName(IterFrame.Name), IterFrame.Frame);
    }

    UE_LOG(LogMmdHelper, Log, TEXT("== FaceFrames info =="));
    for (const FVmdFaceFrame& IterFrame : TrackData.FaceFrames)
    {
        UE_LOG(LogMmdHelper, Log, TEXT("Name:%s Frame:%d"), *FVmdDataHelper::ConvertFromMmdName(IterFrame.Name), IterFrame.Frame);
    }

    UE_LOG(LogMmdHelper, Log, TEXT("== CameraFrames info =="));
    for (const FVmdCameraFrame& IterFrame : TrackData.CameraFrames)
    {
        UE_LOG(LogMmdHelper, Log, TEXT("Frame:%d Location:%f,%f,%f Rotation:%f,%f,%f"),
            IterFrame.Frame,
            IterFrame.Location[0], IterFrame.Location[1], IterFrame.Location[2],
            IterFrame.Rotate[0], IterFrame.Rotate[1], IterFrame.Rotate[2]
        );
    }

    UE_LOG(LogMmdHelper, Log, TEXT("------ FVmdData::PrintOutData: End ------"));
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "UeMmdHelper.h"

#define LOCTEXT_NAMESPACE "FUeMmdHelperModule"
DEFINE_LOG_CATEGORY(LogMmdHelper)


void FUeMmdHelperModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUeMmdHelperModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FUeMmdHelperModule, UeMmdHelper)
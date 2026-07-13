#pragma once

#include <cstdint>

#include "engine/core/engine_types.h"
#include "engine/core/fast_string.h"
struct CSystemFileName {
    static int IsExtensionGbx(const CFastStringInt &filename);
    static int IsExtension(const CFastStringInt &filename,
                           const char *extension);
    static int IsNormalized(const CFastStringInt &filename);
    static int IsValidShortFileName(const CFastStringInt &filename);
    static int SplitFirstDirectory(
            const CFastStringInt &filename,
            CFastStringInt &outDirectory,
            unsigned long &inOutOffset);
    static void SplitFirstDirectory(
            CFastStringInt &filename,
            CFastStringInt &outDirectory);
    static void ConcatDirectory(
            CFastStringInt &filename,
            const CFastStringInt &directory);
    static void StripTrailingSlash(CFastStringInt &filename);
    static int IsDirectoryName(const CFastStringInt &filename);
    static void SplitPath(
            const CFastStringInt &filename,
            CFastStringInt *outDrive,
            CFastStringInt *outDirectory,
            CFastStringInt *outBaseName,
            CFastStringInt *outExtensionLast,
            CFastStringInt *outExtensionComplete);
    static void ExtractExtLast(
            const CFastStringInt &filename,
            CFastStringInt &outExtension);
    static void StripLastExtension(
            const CFastStringInt &filename,
            CFastStringInt &outName);
    static void FixFileName(CFastStringInt &filename, unsigned long flags);
    static void FilterFileName(
            CFastStringInt &outFilename,
            const CFastStringInt &source,
            int allowBackslash,
            int allowDrivePrefix);
    static void Normalize(
            CFastStringInt &filename,
            int forceTrailingSlash);
    static int GetRelativeName(
            const CFastStringInt &filename,
            const CFastStringInt &baseName,
            CFastStringInt &outRelativeName);
    static void SplitLastDirectory(
            CFastStringInt &filename,
            CFastStringInt &outDirectory);
    static void ExtractShortBaseName(
            const CFastStringInt &filename,
            CFastStringInt &outBaseName);
    static void ExtractShortName(
            const CFastStringInt &filename,
            CFastStringInt &outShortName);
    static void ExtractFullPathName(
            const CFastStringInt &filename,
            CFastStringInt &outFullPathName);
    static void ExtractExtComplete(
            const CFastStringInt &filename,
            CFastStringInt &outExtension);
    static void ExtractDriveDirAndBaseName(
            const CFastStringInt &filename,
            CFastStringInt &outName);
    static void StripExtension(
            const CFastStringInt &filename,
            CFastStringInt &outName);
};

#pragma once

#include "engine/core/engine_types.h"
#include <memory>

#include "engine/resources/asset_audience.h"
#include "engine/core/cmw_nod.h"
#include "engine/core/fast_string.h"
#include "engine/resources/system_fid_parameters.h"
struct CSystemFidParameters::SParam {
    SParam(const SParam &other) = default;
    virtual ~SParam(void);

    int IsOptional(void) const;
    int MatchesAudience(const AssetAudience &candidateAudience) const;
    int MatchesFid(CSystemFid &fid) const;
    int MatchesAnyFids(const CFastBuffer<CSystemFid *> &fids) const;
    virtual SParam *Duplicate(void) const = 0;
    std::unique_ptr<SParam> Clone(void) const;
    virtual void CopyFrom(const SParam &source);
    virtual void Compare(
            const SParam &candidate,
            int &sameValue,
            int &differentKey) const = 0;
    virtual int IsDefault(void) const = 0;

protected:
    SParam(
            AssetAudience audience,
            int optional);
    static void StoreCompareResult(
            int sameValue,
            int differentKey,
            int &sameValueOut,
            int &differentKeyOut);

    AssetAudience audience;
    bool optional;
};

struct CSystemFidParameters::SParam_Id : SParam {
    CMwId id;
    unsigned long value;

private:
    SParam_Id(void);

public:
    SParam_Id(const CMwId &id,
              unsigned long value,
              AssetAudience audience,
              int optional);
    ~SParam_Id(void) override = default;

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(
            const SParam &candidate,
            int &sameValue,
            int &differentKey) const override;
    int IsDefault(void) const override;
};

struct CSystemFidParameters::SParam_Set : SParam {
    CSystemFid *fid;
    CFastString paramName;
    CFastStringInt value;

private:
    SParam_Set(void);

public:
    SParam_Set(CSystemFid *fid,
               const CFastString &paramName,
               const CFastStringInt &value,
               AssetAudience audience);
    ~SParam_Set(void) override;

    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(
            const SParam &candidate,
            int &sameValue,
            int &differentKey) const override;
    int IsDefault(void) const override;
};

struct CSystemFidParameters::SParam_Fid_Common : SParam {
    CMwNodRef<CSystemPackDesc> packDesc;
    CFastString prefix;
    unsigned long packElemIndex;

    ~SParam_Fid_Common(void) override;

    CSystemPackDesc *PackDesc(void) const;
    const CFastString &RemapPrefix(void) const;
    unsigned long PackElemIndexValue(void) const;
    int HasPackDesc(void) const;
    int HasSpecialPackDesc(void) const;
    void StorePackTarget(const CSystemPackDesc *&outPackDesc,
                         unsigned long &outPackElemIndex) const;
    void StoreMatchedRemap(const CSystemPackDesc *&outPackDesc,
                           CFastString &outPackElemName,
                           unsigned long &outPackElemIndex) const;
    void CopyFrom(const SParam &source) override;
    int IsDefault(void) const override;

protected:
    SParam_Fid_Common(CSystemPackDesc *packDesc,
                      const CFastString &prefix,
                      unsigned long packElemIndex);
    void CompareFidCommon(
            const SParam_Fid_Common &candidate,
            int targetMatches,
            int &sameValue,
            int &differentKey) const;
};

struct CSystemFidParameters::SParam_Fid : SParam_Fid_Common {
    CSystemFid *fid;

private:
    SParam_Fid(void);

public:
    explicit SParam_Fid(CSystemFid *fid);
    SParam_Fid(CSystemFid *fid,
               CSystemPackDesc *packDesc,
               const CFastString &prefix,
               unsigned long packElemIndex);
    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(
            const SParam &candidate,
            int &sameValue,
            int &differentKey) const override;
    int TargetsFid(const CSystemFid *loadableFid) const;
};

struct CSystemFidParameters::SParam_Fids : SParam_Fid_Common {
    CSystemFids *fids;

private:
    SParam_Fids(void);

public:
    explicit SParam_Fids(CSystemFids *fids);
    SParam_Fids(CSystemFids *fids,
                CSystemPackDesc *packDesc,
                const CFastString &prefix,
                unsigned long packElemIndex);
    SParam *Duplicate(void) const override;
    void CopyFrom(const SParam &source) override;
    void Compare(
            const SParam &candidate,
            int &sameValue,
            int &differentKey) const override;
    int BuildFolderRemapName(CSystemFid *loadableFid,
                             CFastString *outPackElemName) const;
};

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "engine/resources/asset_audience.h"
#include "engine/resources/asset_type.h"
#include "engine/core/classic_buffer.h"
#include "engine/core/cfast_buffer.h"
#include "engine/core/cmw_nod.h"
#include "engine/core/engine_types.h"
#include "engine/resources/system_fid_parameters.h"
struct CSystemFids;

struct CSystemFid : CMwNod {
    enum EFidType : u32 {
        FidType_File = 0u,
        FidType_Memory = 1u,
        FidType_ResourceFile = 2u,
    };

    struct SHeaderUserData;
    struct SHeaderUserDataDeleter {
        void operator()(SHeaderUserData *data) const;
    };
    using HeaderUserDataPtr =
            std::unique_ptr<SHeaderUserData, SHeaderUserDataDeleter>;

    struct CLoader {
        virtual CClassicBuffer *OpenBuffer(
                const CSystemFid *fid,
                CClassicBuffer::EMode mode,
                int forRefs) const = 0;
        virtual void CloseBuffer(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const = 0;
        virtual void OnLoaderOverriden(
                const CSystemFid *fid,
                CLoader *newLoader);
        virtual void UpdateFidProps(CSystemFid *fid) const = 0;
        virtual CSystemFid *GetRealFileOnDisk(CSystemFid *fid) const = 0;

    protected:
        ~CLoader(void) = default;
    };

    struct CLoaderFile : CLoader {
        CClassicBuffer *OpenBuffer(
                const CSystemFid *fid,
                CClassicBuffer::EMode mode,
                int forRefs) const override;
        void CloseBuffer(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const override;
        void UpdateFidProps(CSystemFid *fid) const override;
        CSystemFid *GetRealFileOnDisk(CSystemFid *fid) const override;
    };

    struct CLoaderParametrized : CLoader {
        CClassicBuffer *OpenBuffer(
                const CSystemFid *fid,
                CClassicBuffer::EMode mode,
                int forRefs) const override;
        void CloseBuffer(
                const CSystemFid *fid,
                CClassicBuffer *buffer) const override;
        void UpdateFidProps(CSystemFid *fid) const override;
        CSystemFid *GetRealFileOnDisk(CSystemFid *fid) const override;
    };

    static CLoaderFile s_DefaultLoaderFile;
    static CLoaderParametrized s_DefaultLoaderParametrized;

    CSystemFid(void);
    ~CSystemFid(void) override;
    virtual int IsEqualFid(CSystemFid *other);
    virtual void CopyFromFid(CSystemFid *other);
    virtual void UpdateNameDown(void);
    virtual int RequestWrite(void);
    void DetachNod(CMwNod *nod);
    void SetNod(CMwNod *nod);
    AssetType GetAssetType(void);
    void SetAssetType(AssetType type);
    void ClearAssetType(void);
    int LoadNode(CMwNod *&outNode);
    void LoadHeaderUserData(CClassicBuffer *buffer);
    void SetNoHeaderUserDatas(void);
    void ResetHeaderUserDatas(void);
    void HeaderUserData_DumpToCache(
            const unsigned char *&outData,
            unsigned long &outSize) const;
    void HeaderUserData_RestoreFromCache(
            const unsigned char *source,
            unsigned long size);
    void SetVirtualLoader(CLoader *loader, unsigned long loaderArg);
    CClassicBuffer *BufferOpen(CClassicBuffer::EMode mode) const;
    CClassicBuffer *BufferOpen(
            CClassicBuffer::EMode mode,
            bool forReferences) const;
    void BufferClose(CClassicBuffer *buffer) const;
    void UpdateFidProps(void);
    CSystemFid *ParametrizedGetLoadableFid(void) const;
    CSystemFid *ParametrizedGetFid(
            const CSystemFidParameters &params) const;
    void ParametrizedAddFid(CSystemFid *fid);
    const CFastBuffer<CSystemFid *> &ParametrizedGetAllSubFids(void) const;
    CMwNod *GetNod(const CSystemFidParameters &params) const;
    bool MatchesAudience(const AssetAudience &audience);
    CSystemFids *Location(void) const;
    CMwNod *LoadedNode(void) const;
    CSystemFidParameters &Parameters(void);
    const CSystemFidParameters &Parameters(void) const;
    bool UsesDefaultFileLoader(void) const;

protected:
    bool metadataNeedsRefresh = false;

private:
    friend struct CSystemEngine;
    friend struct CSystemFids;
    friend struct CSystemManagerFile;

    CSystemFids *location = nullptr;
    CMwNod *nod = nullptr;
    CSystemFid *loadableFid = nullptr;
    CSystemFidParameters parameters;
    CLoader *loader = nullptr;
    std::vector<std::unique_ptr<CSystemFid>> parametrizedFids;
    mutable CFastBuffer<CSystemFid *> parametrizedFidView;
    bool headerUserDataLoaded = false;
    HeaderUserDataPtr headerUserData;
    std::optional<AssetType> assetType;
    void DetachFromLoadableParent(void);
    std::unique_ptr<CSystemFid> ReleaseParametrizedFid(CSystemFid *fid);
};

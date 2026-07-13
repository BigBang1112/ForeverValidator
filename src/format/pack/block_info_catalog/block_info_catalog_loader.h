#pragma once

class BlockInfoCatalog;
class BlockInfoAssetStore;
struct CPlugFilePack;

bool LoadBlockInfoCatalog(const CPlugFilePack &pack,
                          BlockInfoAssetStore &assets,
                          BlockInfoCatalog &catalog);

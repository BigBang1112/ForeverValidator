#pragma once

#include <array>
#include <optional>
#include <variant>

#include "engine/core/cmw_nod.h"
#include "engine/scene/scene_mobil.h"
class CGameCtnPylonColumn : public CMwNod {
public:
    struct Span {
        unsigned long firstY;
        unsigned long lastY;
    };

    CGameCtnPylonColumn(void);
    ~CGameCtnPylonColumn(void) override;
    void RemovePylon(unsigned long y);
    void AddPylon(unsigned long firstY, unsigned long lastY);
    void SetMobil(CSceneMobil *mobil);
    void SetMobils(CSceneMobil *lowerMobil,
                   CSceneMobil *middleMobil,
                   CSceneMobil *upperMobil);
    void DeleteMobils(void);
    static CMwNod *MwNewCGameCtnPylonColumn(void);

    const std::optional<Span> &PylonSpan(void) const;
    bool HasPylon(void) const;
    CSceneMobil *SingleMobil(void) const;
    CSceneMobil *LowerMobil(void) const;
    CSceneMobil *MiddleMobil(void) const;
    CSceneMobil *UpperMobil(void) const;

private:
    using SingleMobilRef = CMwNodRef<CSceneMobil>;
    using TripleMobils = std::array<CMwNodRef<CSceneMobil>, 3>;

    std::optional<Span> span;
    std::variant<std::monostate, SingleMobilRef, TripleMobils> mobils;
};

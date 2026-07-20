#pragma once

class CGameCtnBlock;
class CGameCtnChallenge;
class CSceneMobil;
class ReplaySceneBlockPlacements;
struct SPylonMobil;

class CGameCtnApp {
public:
    CGameCtnApp(ReplaySceneBlockPlacements &placements,
                float squareSize,
                float squareHeight);
    CGameCtnApp(ReplaySceneBlockPlacements &placements,
                CGameCtnChallenge &challenge,
                float squareSize,
                float squareHeight);

    virtual void UpdateBlockMobils(void);
    void AddClipsToScene(void);
    void AddBlockMobilToScene(CGameCtnBlock *block);
    void AddPylonMobilToScene(SPylonMobil *pylonMobil);
    void RemoveMobilFromScene(CSceneMobil *mobil);

private:
    void AddPylonPart(CSceneMobil &mobil, const struct GmIso4 &worldIso);

    ReplaySceneBlockPlacements *placements_ = nullptr;
    CGameCtnChallenge *challenge_ = nullptr;
    float squareSize_ = 32.0f;
    float squareHeight_ = 8.0f;
};

#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

#include "engine/core/cmw_nod.h"
class CMwCmdBuffer;

class CMwCmd : public CMwNod {
public:
    CMwCmd(void);
    ~CMwCmd(void) override;

    virtual void Run(void);
    virtual void Install(void);
    virtual void Uninstall(void);
    void SetCmdBuffer(CMwCmdBuffer *cmdBuffer);
    void SetSchemeLocation(unsigned long schemeLocation);

private:
    enum class InstallState {
        Uninstalled,
        Installed,
        PendingUninstall,
    };

    CMwCmdBuffer *cmdBuffer_ = nullptr;
    InstallState installState_ = InstallState::Uninstalled;

    friend class CMwCmdBuffer;
};

class CMwCmdBuffer : public CMwNod {
public:
    CMwCmdBuffer(void);
    ~CMwCmdBuffer(void) override;

    void SetCatCount(unsigned long categoryCount);
    void Run(unsigned long categoryIndex);
    void UnistallCmd(CMwCmd *command, int forceImmediate);

    void Clear(void);
    bool Empty(void) const;

protected:
    void AddCmd(CMwCmd *command);

private:
    using CommandRef = CMwNodRef<CMwCmd>;
    using CommandCategory = std::deque<CommandRef>;

    struct CommandLocation {
        std::size_t categoryIndex;
        std::size_t commandIndex;
    };

    std::optional<CommandLocation> FindCommand(CMwCmd *command) const;
    CommandRef RemoveCommand(CommandLocation location);
    void RemovePendingCommands(void);
    void ReleaseCommands(void);
    std::size_t CommandCount(void) const;

    std::vector<CommandCategory> categories_;
    std::optional<std::size_t> activeCategory_;
    std::ptrdiff_t activeIndex_ = -1;
    std::ptrdiff_t activeRunLimit_ = 0;
    bool hasPendingUninstallRequests_ = false;
    bool destroying_ = false;

    friend class CMwCmd;
};

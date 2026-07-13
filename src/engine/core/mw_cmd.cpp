#include "engine/core/mw_cmd.h"
#include <utility>

#include "engine/core/mw_cmd_buffer_core.h"
CMwCmd::CMwCmd(void) = default;

CMwCmd::~CMwCmd(void) = default;

void CMwCmd::Run(void) {}

void CMwCmd::Install(void) {
    if (installState_ == InstallState::Installed) {
        return;
    }
    if (installState_ == InstallState::PendingUninstall) {
        installState_ = InstallState::Installed;
        return;
    }
    if (cmdBuffer_ == nullptr) {
        return;
    }

    cmdBuffer_->AddCmd(this);
    installState_ = InstallState::Installed;
}

void CMwCmd::Uninstall(void) {
    if (installState_ != InstallState::Installed) {
        return;
    }

    installState_ = InstallState::PendingUninstall;
    if (cmdBuffer_ != nullptr) {
        cmdBuffer_->UnistallCmd(this, 0);
    }
}

void CMwCmd::SetCmdBuffer(CMwCmdBuffer *cmdBuffer) {
    if (installState_ == InstallState::Uninstalled) {
        cmdBuffer_ = cmdBuffer;
    }
}

void CMwCmd::SetSchemeLocation(unsigned long schemeLocation) {
    CMwCmdBufferCore *core = CMwCmdBufferCore::Current();
    SetCmdBuffer(core != nullptr
            ? &core->CommandBufferForScheme(schemeLocation)
            : nullptr);
}

CMwCmdBuffer::CMwCmdBuffer(void) : categories_(1u) {}

CMwCmdBuffer::~CMwCmdBuffer(void) {
    destroying_ = true;
    ReleaseCommands();
}

void CMwCmdBuffer::ReleaseCommands(void) {
    activeIndex_ = static_cast<std::ptrdiff_t>(CommandCount());
    activeCategory_.reset();

    for (CommandCategory &category : categories_) {
        for (CommandRef &commandRef : category) {
            CMwCmd *command = commandRef.Get();
            if (command != nullptr) {
                command->cmdBuffer_ = nullptr;
                command->installState_ = CMwCmd::InstallState::Uninstalled;
                commandRef.MwSetNod(nullptr);
            }
        }
    }
    categories_.clear();
    hasPendingUninstallRequests_ = false;
    activeIndex_ = -1;
    activeRunLimit_ = 0;
}

void CMwCmdBuffer::Clear(void) {
    destroying_ = true;
    ReleaseCommands();
    categories_.resize(1u);
    destroying_ = false;
}

bool CMwCmdBuffer::Empty(void) const {
    return CommandCount() == 0u;
}

std::size_t CMwCmdBuffer::CommandCount(void) const {
    std::size_t commandCount = 0u;
    for (const CommandCategory &category : categories_) {
        commandCount += category.size();
    }
    return commandCount;
}

void CMwCmdBuffer::SetCatCount(unsigned long categoryCount) {
    if (CommandCount() != 0u) {
        return;
    }
    categories_.clear();
    categories_.resize(static_cast<std::size_t>(categoryCount));
}

void CMwCmdBuffer::AddCmd(CMwCmd *command) {
    if (command == nullptr || categories_.empty() ||
        command->installState_ == CMwCmd::InstallState::PendingUninstall) {
        return;
    }

    std::size_t targetIndex = 0u;
    for (std::size_t categoryIndex = 1u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        if (categories_[targetIndex].size() >
            categories_[categoryIndex].size()) {
            targetIndex = categoryIndex;
        }
    }

    categories_[targetIndex].emplace_back(command);
    for (std::size_t categoryIndex = targetIndex + 1u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        CommandCategory &category = categories_[categoryIndex];
        if (category.empty()) {
            continue;
        }
        CommandRef first(std::move(category.front()));
        category.pop_front();
        category.push_back(std::move(first));
    }
    ++activeRunLimit_;
}

std::optional<CMwCmdBuffer::CommandLocation> CMwCmdBuffer::FindCommand(
        CMwCmd *command) const {
    for (std::size_t categoryIndex = 0u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        const CommandCategory &category = categories_[categoryIndex];
        for (std::size_t commandIndex = 0u;
             commandIndex < category.size();
             ++commandIndex) {
            if (category[commandIndex].Get() == command) {
                return CommandLocation{categoryIndex, commandIndex};
            }
        }
    }
    return std::nullopt;
}

CMwCmdBuffer::CommandRef CMwCmdBuffer::RemoveCommand(
        CommandLocation location) {
    CommandCategory &category = categories_[location.categoryIndex];
    const std::size_t lastIndex = category.size() - 1u;
    if (location.commandIndex != lastIndex) {
        std::swap(category[location.commandIndex], category[lastIndex]);
    }

    CommandRef removed(std::move(category.back()));
    category.pop_back();

    for (std::size_t categoryIndex = location.categoryIndex + 1u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        CommandCategory &laterCategory = categories_[categoryIndex];
        if (laterCategory.empty()) {
            continue;
        }
        CommandRef last(std::move(laterCategory.back()));
        laterCategory.pop_back();
        laterCategory.push_front(std::move(last));
    }
    return removed;
}

void CMwCmdBuffer::RemovePendingCommands(void) {
    if (!hasPendingUninstallRequests_) {
        return;
    }

    for (std::size_t categoryIndex = 0u;
         categoryIndex < categories_.size();
         ++categoryIndex) {
        std::size_t commandIndex = 0u;
        std::size_t remaining = categories_[categoryIndex].size();
        while (commandIndex < remaining) {
            CMwCmd *command = categories_[categoryIndex][commandIndex].Get();
            if (command == nullptr ||
                command->installState_ !=
                        CMwCmd::InstallState::PendingUninstall) {
                ++commandIndex;
                continue;
            }

            command->installState_ = CMwCmd::InstallState::Uninstalled;
            command->cmdBuffer_ = nullptr;
            CommandRef removed = RemoveCommand(
                    {categoryIndex, commandIndex});
            removed.MwSetNod(nullptr);
            --remaining;
        }
    }
    hasPendingUninstallRequests_ = false;
}

void CMwCmdBuffer::Run(unsigned long categoryIndex) {
    RemovePendingCommands();
    if (categoryIndex >= categories_.size()) {
        return;
    }

    activeCategory_ = static_cast<std::size_t>(categoryIndex);
    activeRunLimit_ = static_cast<std::ptrdiff_t>(
            categories_[*activeCategory_].size());
    activeIndex_ = 0;
    while (activeIndex_ < activeRunLimit_) {
        CMwCmd *command = categories_[*activeCategory_][
                static_cast<std::size_t>(activeIndex_)].Get();
        if (command != nullptr) {
            command->Run();
        }
        ++activeIndex_;
    }

    activeCategory_.reset();
    activeIndex_ = -1;
    activeRunLimit_ = 0;
    RemovePendingCommands();
}

void CMwCmdBuffer::UnistallCmd(
        CMwCmd *command,
        int forceImmediate) {
    if (command == nullptr || destroying_) {
        return;
    }

    const std::optional<CommandLocation> location = FindCommand(command);
    if (!location.has_value()) {
        return;
    }

    const bool isLaterActiveCommand =
            activeCategory_.has_value() &&
            location->categoryIndex == *activeCategory_ &&
            static_cast<std::ptrdiff_t>(location->commandIndex) > activeIndex_;
    if (isLaterActiveCommand) {
        --activeRunLimit_;
    } else if (forceImmediate != 0) {
        if (activeCategory_.has_value()) {
            --activeRunLimit_;
            --activeIndex_;
        }
    } else {
        hasPendingUninstallRequests_ = true;
        return;
    }

    command->installState_ = CMwCmd::InstallState::Uninstalled;
    command->cmdBuffer_ = nullptr;
    CommandRef removed = RemoveCommand(*location);
    removed.MwSetNod(nullptr);
}

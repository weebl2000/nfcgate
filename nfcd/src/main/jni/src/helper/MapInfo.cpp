#include <nfcd/helper/MapInfo.h>
#include <nfcd/nfcd.h>

#include <fstream>
#include <regex>

#include <link.h>

bool MapInfo::create() {
    int rv = dl_iterate_phdr([] (struct dl_phdr_info *info, size_t, void *user_data) {
        auto *instance = (MapInfo *)user_data;

        for (size_t i = 0; i < info->dlpi_phnum; i++) {
            instance->mRanges.push_back(RangeData {
                info->dlpi_addr + info->dlpi_phdr[i].p_vaddr,
                info->dlpi_addr + info->dlpi_phdr[i].p_vaddr + info->dlpi_phdr[i].p_memsz,
                static_cast<uint8_t>(info->dlpi_phdr[i].p_flags),
                info->dlpi_name
            });
        }

        return 0;
    }, this);
    LOG_ASSERT_S(rv == 0, return false, "Error iterating dl program header");

    return true;
}

std::set<std::string> MapInfo::loadedLibraries() const {
    std::set<std::string> result;

    for (auto &range : mRanges) {
        result.emplace(range.label);
    }

    return result;
}

void *MapInfo::getBaseAddress(const std::string &library) const {
    for (auto &range : mRanges) {
        // skip range that does not match the library
        if (!StringUtil::strEndsWith(range.label, library))
            continue;

        // skip range without read permission or without enough space for ELF header
        if ((range.perms & 4) != 4 || range.end - range.start <= 4)
            continue;

        // check ELF magic bytes to confirm this region as the base
        if (memcmp((void *)range.start, "\x7f" "ELF", 4) != 0)
            continue;

        return (void *)range.start;
    }

    return nullptr;
}

const MapInfo::RangeData *MapInfo::rangeFromAddress(uintptr_t addr, uint64_t size) const {
    for (auto &range : mRanges)
        if (addr >= range.start && (addr + size) <= range.end)
            return &range;

    return nullptr;
}

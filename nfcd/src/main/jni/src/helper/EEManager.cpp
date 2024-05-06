#include <nfcd/helper/EEManager.h>
#include <nfcd/nfcd.h>

#include <cinttypes>
#include <vector>

std::set<uint16_t> EEManager::findActiveEEs() const {
    size_t eeCount = countEEs();
    // detect struct size only if needed
    if (!mStructSize)
        findStructSize(eeCount);

    std::set<uint16_t> result;
    if (eeCount > 0) {
        std::vector<uint8_t> buffer(mStructSize * eeCount);

        uint8_t num_ee = eeCount;
        globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, reinterpret_cast<void*>(buffer.data()));

        for (size_t i = 0; i < num_ee; i++) {
            uint8_t *eeInfo = buffer.data() + mStructSize * i;

            // uint16_t ee_handle is always at the start of each struct
            uint16_t ee_handle = *reinterpret_cast<uint16_t *>(eeInfo);
            // uint8_t ee_status is always at offset 2 in each struct
            uint8_t ee_status = *reinterpret_cast<uint16_t *>(eeInfo + sizeof(uint16_t));

            // only non-deactivated EEs
            if (ee_status != NFA_EE_STATUS_INACTIVE)
                result.insert(ee_handle);
        }
    }

    return result;
}

void EEManager::markDeactivated(uint16_t handle) {
    mDeactivated.insert(handle);
}

void EEManager::markActivated(uint16_t handle) {
    mDeactivated.erase(handle);
}

size_t EEManager::countEEs() const {
    std::vector<uint8_t> temp(255 * getApproxStructSize());

    uint8_t num_ee = 255;
    globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, reinterpret_cast<void*>(temp.data()));
    return num_ee;
}

size_t EEManager::findStructSize(size_t eeCount) const {
    StructSizeProber prober([] (int numEE, uint8_t *dest, size_t size) {
        uint8_t num_ee = numEE;
        globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, dest);
        return num_ee == numEE;
    });

    if (eeCount <= 1)
        return getApproxStructSize();

    return prober.detectStructSize(getApproxStructSize());
}

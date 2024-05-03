#include <nfcd/nfcd.h>
#include <jni.h>

static void beginCollectingEvents() {
    globals.eventQueue.beginCollecting();
}

static void waitForEvent(uint8_t event, bool checkStatus = true) {
    uint8_t status;
    if (globals.eventQueue.waitFor(event, status, 500)) {
        if (checkStatus && status != 0)
            LOGW("[event] Unexpected status for %s: expected 0, got %d",
                 System::nfaEventName(event).c_str(), status);
    }
    else
        LOGW("[event] Waiting for %s failed: timeout reached", System::nfaEventName(event).c_str());
}

std::set<tNCI_DISCOVERY_TYPE> discoveryTypesFromConfig(const Config &config) {
    std::set<tNCI_DISCOVERY_TYPE> result;

    for (const auto &option : config.options()) {
        if (option.name().find("LA") == 0) {
            result.emplace(NCI_DISCOVERY_TYPE_LISTEN_A);
            result.emplace(NCI_DISCOVERY_TYPE_LISTEN_A_ACTIVE);
        }
        else if (option.name().find("LB") == 0) {
            result.emplace(NCI_DISCOVERY_TYPE_LISTEN_B);
        }
        else if (option.name().find("LF") == 0) {
            result.emplace(NCI_DISCOVERY_TYPE_LISTEN_F);
            result.emplace(NCI_DISCOVERY_TYPE_LISTEN_F_ACTIVE);
        }
    }

    return result;
}

void nfaEnableDiscovery() {
    beginCollectingEvents();
    globals.hNFA_StartRfDiscovery->call<def_NFA_StartRfDiscovery>();
    LOGD("[nfcd] Starting RF discovery");
    waitForEvent(NFA_RF_DISCOVERY_STARTED_EVT);
}

void nfaDisableDiscovery() {
    beginCollectingEvents();
    globals.hNFA_StopRfDiscovery->call<def_NFA_StopRfDiscovery>();
    LOGD("[nfcd] Stopping RF discovery");
    waitForEvent(NFA_RF_DISCOVERY_STOPPED_EVT, false);
}

void nfaEnablePolling() {
    /*
     * Note: only enable known technologies, since enabling all (0xFF) also enabled exotic
     * proprietary ones, which may fail to start without special configuration
     */
    beginCollectingEvents();
    globals.hNFA_EnablePolling->call<def_NFA_EnablePolling>(SAFE_TECH_MASK);
    LOGD("[nfcd] Enabling polling");
    waitForEvent(NFA_POLL_ENABLED_EVT);
}

void nfaDisablePolling() {
    beginCollectingEvents();
    globals.hNFA_DisablePolling->call<def_NFA_DisablePolling>();
    LOGD("[nfcd] Disabling polling");
    waitForEvent(NFA_POLL_DISABLED_EVT);
}

size_t getEEHandleCount(size_t estMaxElementSize) {
    uint8_t temp[estMaxElementSize];

    uint8_t num_ee = 255;
    globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, reinterpret_cast<void*>(&temp));
    return num_ee;
}

size_t getEEStructSize(size_t eeCount, size_t estMaxElementSize) {
    StructSizeProber prober([] (int numEE, uint8_t *dest, size_t size) {
        uint8_t num_ee = numEE;
        globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, dest);
        return num_ee == numEE;
    });

    if (eeCount <= 1)
        return estMaxElementSize;

    return prober.detectStructSize(estMaxElementSize);
}

std::vector<uint16_t> getEEHandles() {
    std::vector<uint16_t> result;

    // 10KB to be safe
    size_t estMaxElementSize = 10000;
    // count EE entries
    size_t eeCount = getEEHandleCount(estMaxElementSize);
    // detect struct size
    size_t eeStructSize = getEEStructSize(eeCount, estMaxElementSize);

    if (eeCount > 0) {
        uint8_t num_ee = 255;
        uint8_t data[(eeStructSize + 1000) * eeCount];
        globals.hNFA_EeGetInfo->call<def_NFA_EeGetInfo>(&num_ee, reinterpret_cast<void*>(&data));

        for (size_t i = 0; i < num_ee; i++)
            // uint16_t ee_handle is always at the start of each struct
            result.push_back(*reinterpret_cast<uint16_t*>(data + eeStructSize * i));
    }

    return result;
}

void disableEEs() {
    for (uint16_t eeHandle : getEEHandles()) {
        LOGD("Disabling EE: %x", eeHandle);

        globals.hNFA_EeModeSet->call<def_NFA_EeModeSet>(eeHandle, 0 /* disable */);
        usleep(25000);
    }
}

void limitDiscoveryTypes(std::set<tNCI_DISCOVERY_TYPE> discoveryTypes) {
    globals.discoveryTypes = discoveryTypes;
    for (tNCI_DISCOVERY_TYPE discoveryType : discoveryTypes)
        LOGD("[nfcd] Limiting listen to type %d", discoveryType);
}

void applyConfig(Config &config) {
    config_ref bin_stream;
    config.build(bin_stream);

    globals.guardEnabled = false;
    globals.hNFC_SetConfig->callHook<def_NFC_SetConfig>(config.total(), bin_stream.get());
    globals.guardEnabled = true;

    // wait for config to set before returning
    usleep(35000);
}

extern "C" {
    JNIEXPORT jboolean JNICALL Java_de_tu_1darmstadt_seemoo_nfcgate_xposed_Native_isHookEnabled(JNIEnv *, jobject) {
        return globals.hookStaticEnabled && globals.hookDynamicEnabled;
    }

    JNIEXPORT jboolean JNICALL Java_de_tu_1darmstadt_seemoo_nfcgate_xposed_Native_isPatchEnabled(JNIEnv *, jobject) {
        return globals.patchEnabled;
    }

    JNIEXPORT void JNICALL Java_de_tu_1darmstadt_seemoo_nfcgate_xposed_Native_setConfig(JNIEnv *env, jobject, jbyteArray config) {
        // parse config value stream
        jsize config_len = env->GetArrayLength(config);
        jbyte *config_data = env->GetByteArrayElements(config, nullptr);
        globals.hookValues.parse(config_len, (uint8_t *) config_data);
        env->ReleaseByteArrayElements(config, config_data, 0);

        // begin re-routing AIDs and limiting discovery types
        globals.patchEnabled = true;

        // disable discovery before changing anything
        nfaDisableDiscovery();
        {
            // disable EEs that may be interfering with our config
            disableEEs();
            // limit discovery types only for the selected technologies
            auto discoveryTypes = discoveryTypesFromConfig(globals.hookValues);
            if (!discoveryTypes.empty())
                limitDiscoveryTypes(discoveryTypes);
            // apply the config stream
            applyConfig(globals.hookValues);
            // disable polling
            nfaDisablePolling();
        }
        // re-enable discovery after changes were made
        nfaEnableDiscovery();
    }

    JNIEXPORT void JNICALL Java_de_tu_1darmstadt_seemoo_nfcgate_xposed_Native_resetConfig(JNIEnv *, jobject) {
        if (globals.patchEnabled) {
            // stop re-routing AIDs and limiting discovery types
            globals.patchEnabled = false;
            globals.discoveryTypes.clear();

            // disable discovery before changing anything
            nfaDisableDiscovery();
            // re-enable polling
            nfaEnablePolling();
            // re-enable discovery after changes were made
            nfaEnableDiscovery();
        }
    }
}

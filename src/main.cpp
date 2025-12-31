#include <Geode/modify/MoreOptionsLayer.hpp>

bool operator==(const FMOD_GUID& left, const FMOD_GUID& right) {
    return std::memcmp(&left, &right, sizeof(FMOD_GUID)) == 0;
}

void getAudioDeviceName(char* buffer) {
    auto system = FMODAudioEngine::get()->m_system;
    int driver = 0;
    system->getDriver(&driver);
    system->getDriverInfo(driver, buffer, 256, nullptr, nullptr, nullptr, nullptr);
}

void saveOffsetForAudioDevice() {
    char name[256]; getAudioDeviceName(name);
    auto mod = geode::Mod::get();

    int offset = FMODAudioEngine::get()->m_musicOffset;

    if (!mod->hasSavedValue(name) || mod->getSavedValue<int64_t>(name, 0) != offset) {
        mod->setSavedValue<int64_t>(name, offset);
        geode::Notification::create(fmt::format("Audio offset for\n{}\nsaved as {}ms.", name, offset), geode::NotificationIcon::Info)->show();
    }
}

void loadOffsetForAudioDevice() {
    char name[256]; getAudioDeviceName(name);
    auto mod = geode::Mod::get();

    int offset = mod->getSavedValue<int64_t>(name, 0);

    if (FMODAudioEngine::get()->m_musicOffset != offset) {
        geode::Notification::create(fmt::format("Audio offset set to {}ms.", offset), geode::NotificationIcon::Info)->show();

        FMODAudioEngine::get()->m_musicOffset = offset;
    }

    if (auto popup = cocos2d::CCScene::get()->getChildByType<MoreOptionsLayer>(0)) {
        popup->m_offsetInput->setString(std::to_string(offset));
    }
}

class $modify(MoreOptionsLayer) {
    void onClose(cocos2d::CCObject* sender) {
        MoreOptionsLayer::onClose(sender);
        saveOffsetForAudioDevice();
    }
};

class AudioOutputPoller : public cocos2d::CCObject {
public:
    FMOD::System* m_system;
    FMOD_GUID m_lastGUID;

    AudioOutputPoller()
        : m_system(FMODAudioEngine::get()->m_system)
        , m_lastGUID({}) {}

    void poll(float dt) {
        int driver = 0;
        m_system->getDriver(&driver); // likely always zero for windows at least

        FMOD_GUID guid;
        m_system->getDriverInfo(driver, nullptr, 0, &guid, nullptr, nullptr, nullptr);

        if (guid != m_lastGUID) {
            loadOffsetForAudioDevice();
            m_lastGUID = guid;
        }
    }
};

$on_game(Loaded) {
    cocos2d::CCScheduler::get()->scheduleSelector(schedule_selector(AudioOutputPoller::poll), new AudioOutputPoller, .5f, false);

    if (geode::Mod::get()->getSavedValue<bool>("is-first-startup", true)) {
        geode::Mod::get()->setSavedValue<bool>("is-first-startup", true);
        saveOffsetForAudioDevice();
    } else {
        loadOffsetForAudioDevice();
    }
}

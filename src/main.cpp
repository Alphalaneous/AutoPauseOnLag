#include <Geode/Geode.hpp>

using namespace geode::prelude;

// based on prevter's auto pause on lag for osu!

struct LagChecker : public CCObject {

	static LagChecker* create() {
		auto ret = new LagChecker();
		ret->init();
		ret->autorelease();
		return ret;
	}

	bool init() {
		m_enabled = Mod::get()->getSettingValue<bool>("enabled");
		m_threshold = Mod::get()->getSettingValue<float>("threshold");
		listenForSettingChanges("enabled", [&](bool setting) {
			m_enabled = setting;
		});
		listenForSettingChanges("threshold", [&](float setting) {
			m_threshold = setting;
		});
		return true;
	}

	void update(float dt) {
		if (!m_enabled) return;

		if (auto playLayer = PlayLayer::get()) {

			constexpr float GRACE_PERIOD = 2000.f;

			const float ms = dt * 1000.f;
			const float time = CCDirector::get()->m_fAccumDt * 1000.f;
			const bool isGrace = time < (m_lastPauseTime + GRACE_PERIOD);

			if (ms >= m_threshold && !isGrace && playLayer->m_timePlayed != 0 && !playLayer->m_isPaused) {
				m_lastPauseTime = time;
				for (auto [k, v] : FMODAudioEngine::get()->m_channelIDToChannel) {
					unsigned int pos = 0;
					v->getPosition(&pos, FMOD_TIMEUNIT_MS);
					v->setPosition(pos - ms, FMOD_TIMEUNIT_MS);
				}
				playLayer->pauseGame(true);
				geode::Notification::create(fmt::format("Lag spike of {}ms detected.", ms))->show();
			}
		}
	}

	bool m_enabled;
	float m_threshold;
	float m_lastPauseTime = 0.f;
};

$execute {
	CCScheduler::get()->scheduleUpdateForTarget(LagChecker::create(), INT_MIN, false);
}
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "menu.hpp"
#include "replay_system.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
	ReplaySystem::get().loadSettings();
}

static bool isThePlayLayer(GJBaseGameLayer* layer) {
	auto pl = PlayLayer::get();
	return pl && static_cast<GJBaseGameLayer*>(pl) == layer;
}

class $modify(RBBaseGameLayer, GJBaseGameLayer) {
	void processCommands(float dt, bool isHalfTick, bool isLastTick) {
		auto pl = PlayLayer::get();
		if (!isThePlayLayer(this)) {
			GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
			return;
		}
		auto& rs = ReplaySystem::get();
		rs.beforeStep(pl);
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
		rs.afterStep();
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		if (button == 1 && isThePlayLayer(this)) {
			auto& rs = ReplaySystem::get();
			if (!rs.injecting) {
				if (rs.isPlaying()) return;
				rs.onButton(PlayLayer::get(), down, isPlayer1);
			}
		}
		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}
};

class $modify(RBPlayLayer, PlayLayer) {
	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();
		ReplaySystem::get().onLevelEnter(this);
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		ReplaySystem::get().onReset(this);
	}

	void createCheckpoint() {
		PlayLayer::createCheckpoint();
		ReplaySystem::get().onCheckpoint(this);
	}

	void levelComplete() {
		ReplaySystem::get().onLevelExit();
		PlayLayer::levelComplete();
	}

	void onQuit() {
		ReplaySystem::get().onLevelExit();
		PlayLayer::onQuit();
	}
};

class $modify(RBPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		const auto win = CCDirector::sharedDirector()->getWinSize();
		auto menu = CCMenu::create();
		menu->setPosition({0.f, 0.f});

		auto btn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("ReplayBot", 80, true, "goldFont.fnt", "GJ_button_01.png", 26.f, 0.6f),
			this, menu_selector(RBPauseLayer::onReplayBot));
		btn->setPosition({60.f, win.height - 30.f});
		menu->addChild(btn);
		this->addChild(menu);
	}

	void onReplayBot(CCObject*) {
		RBMenu::open();
	}

	void onResume(CCObject* sender) {
		if (ReplaySystem::get().consumeRestartOnResume()) {
			PauseLayer::onRestart(nullptr);
			return;
		}
		PauseLayer::onResume(sender);
	}
};

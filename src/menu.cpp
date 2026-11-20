#include "menu.hpp"
#include "replay_system.hpp"

#include <cstdlib>

using namespace geode::prelude;

RBMenu* RBMenu::create() {
	auto ret = new RBMenu();
	if (ret && ret->init()) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

void RBMenu::open() {
	if (auto menu = RBMenu::create()) menu->show();
}

bool RBMenu::init() {
	if (!this->initWithColor({0, 0, 0, 105})) return false;

	auto& rs = ReplaySystem::get();
	const auto win = CCDirector::sharedDirector()->getWinSize();
	const CCPoint c = win / 2.f;

	m_mainLayer = CCLayer::create();
	this->addChild(m_mainLayer);

	auto bg = extension::CCScale9Sprite::create("GJ_square01.png", {0.f, 0.f, 80.f, 80.f});
	bg->setContentSize({380.f, 250.f});
	bg->setPosition(c);
	m_mainLayer->addChild(bg, -1);

	m_buttonMenu = CCMenu::create();
	m_buttonMenu->setPosition({0.f, 0.f});
	m_mainLayer->addChild(m_buttonMenu);

	this->registerWithTouchDispatcher();
	CCTouchDispatcher::get()->registerForcePrio(this, 2);

	auto label = [&](const char* text, CCPoint pos, float scale, CCPoint anchor = {0.5f, 0.5f}, const char* font = "bigFont.fnt") {
		auto l = CCLabelBMFont::create(text, font);
		l->setScale(scale);
		l->setPosition(pos);
		l->setAnchorPoint(anchor);
		m_mainLayer->addChild(l);
		return l;
	};

	const float left = c.x - 170.f;
	const float top = c.y + 125.f;

	label("ReplayBot", {c.x, top - 18.f}, 0.8f);

	auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
	closeSpr->setScale(0.7f);
	auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(RBMenu::onClose));
	closeBtn->setPosition({c.x - 190.f + 4.f, top - 4.f});
	m_buttonMenu->addChild(closeBtn);

	m_recordSprite = ButtonSprite::create("Record");
	auto recBtn = CCMenuItemSpriteExtra::create(m_recordSprite, this, menu_selector(RBMenu::onRecord));
	recBtn->setPosition({c.x - 70.f, top - 55.f});
	m_buttonMenu->addChild(recBtn);

	m_playSprite = ButtonSprite::create("Play");
	auto playBtn = CCMenuItemSpriteExtra::create(m_playSprite, this, menu_selector(RBMenu::onPlay));
	playBtn->setPosition({c.x + 70.f, top - 55.f});
	m_buttonMenu->addChild(playBtn);

	m_info = label("", {left, top - 90.f}, 0.5f, {0.f, 1.f}, "chatFont.fnt");

	m_nameInput = TextInput::create(170.f, "replay name", "bigFont.fnt");
	m_nameInput->setFilter("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-");
	m_nameInput->setMaxCharCount(24);
	m_nameInput->setPosition({left + 95.f, top - 150.f});
	m_mainLayer->addChild(m_nameInput);

	auto saveBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Save", 50, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f),
		this, menu_selector(RBMenu::onSave));
	saveBtn->setPosition({left + 235.f, top - 150.f});
	m_buttonMenu->addChild(saveBtn);

	auto loadBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Load", 50, true, "goldFont.fnt", "GJ_button_02.png", 30.f, 0.7f),
		this, menu_selector(RBMenu::onLoad));
	loadBtn->setPosition({left + 295.f, top - 150.f});
	m_buttonMenu->addChild(loadBtn);

	auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
	infoSpr->setScale(0.6f);
	auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(RBMenu::onPathInfo));
	infoBtn->setPosition({left + 340.f, top - 150.f});
	m_buttonMenu->addChild(infoBtn);

	m_saved = label("", {left, top - 172.f}, 0.4f, {0.f, 1.f}, "chatFont.fnt");

	auto frameToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onFrameMode), 0.55f);
	frameToggle->setPosition({left + 12.f, top - 215.f});
	frameToggle->toggle(rs.defaultType == ReplayType::Frame);
	m_buttonMenu->addChild(frameToggle);
	label("Frame mode", {left + 30.f, top - 215.f}, 0.4f, {0.f, 0.5f});

	auto statusToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onStatusText), 0.55f);
	statusToggle->setPosition({left + 150.f, top - 215.f});
	statusToggle->toggle(rs.showStatus);
	m_buttonMenu->addChild(statusToggle);
	label("Status text", {left + 168.f, top - 215.f}, 0.4f, {0.f, 0.5f});

	label("Speed", {left + 262.f, top - 215.f}, 0.4f, {0.f, 0.5f});
	m_speedInput = TextInput::create(50.f, "1", "bigFont.fnt");
	m_speedInput->setFilter("0123456789.");
	m_speedInput->setMaxCharCount(5);
	m_speedInput->setString(fmt::format("{}", rs.speed));
	m_speedInput->setPosition({left + 332.f, top - 215.f});
	m_speedInput->setCallback([](std::string const& text) {
		char* end = nullptr;
		const float v = std::strtof(text.c_str(), &end);
		if (end != text.c_str() && v >= 0.05f && v <= 20.f) {
			ReplaySystem::get().setSpeed(v);
			ReplaySystem::get().saveSettings();
		}
	});
	m_mainLayer->addChild(m_speedInput);

	this->setKeypadEnabled(true);
	this->setTouchEnabled(true);

	this->refresh();
	return true;
}

void RBMenu::refresh() {
	auto& rs = ReplaySystem::get();

	m_recordSprite->setString(rs.isRecording() ? "Stop" : "Record");
	m_playSprite->setString(rs.isPlaying() ? "Stop" : "Play");

	m_info->setString(fmt::format(
		"Current replay\nActions: {}\nMode: {}\nTPS: {}",
		rs.replay.actions().size(),
		rs.replay.type() == ReplayType::Frame ? "Frame" : "X Pos",
		rs.replay.tps()).c_str());

	std::string names;
	int shown = 0;
	for (const auto& n : rs.listReplays()) {
		if (shown++ == 8) { names += " ..."; break; }
		if (!names.empty()) names += ", ";
		names += n;
	}
	m_saved->setString(names.empty() ? "Saved: (none yet)" : ("Saved: " + names).c_str());
}

void RBMenu::keyBackClicked() {
	ReplaySystem::get().saveSettings();
	FLAlertLayer::keyBackClicked();
}

void RBMenu::onClose(CCObject*) {
	this->keyBackClicked();
}

void RBMenu::onRecord(CCObject*) {
	auto& rs = ReplaySystem::get();
	if (rs.isRecording()) {
		rs.stop();
		this->refresh();
		return;
	}
	if (rs.replay.actions().empty()) {
		rs.startRecording();
		this->refresh();
		return;
	}
	createQuickPopup(
		"Overwrite?",
		"Recording will <cr>overwrite</c> the replay that is currently loaded.",
		"Cancel", "Record",
		[this](FLAlertLayer*, bool btn2) {
			if (!btn2) return;
			ReplaySystem::get().startRecording();
			this->refresh();
		});
}

void RBMenu::onPlay(CCObject*) {
	auto& rs = ReplaySystem::get();
	if (rs.isPlaying()) {
		rs.stop();
	} else if (!rs.startPlaying()) {
		FLAlertLayer::create("ReplayBot", "No replay loaded. Record or load one first.", "OK")->show();
	}
	this->refresh();
}

void RBMenu::onSave(CCObject*) {
	const auto err = ReplaySystem::get().saveReplay(m_nameInput->getString());
	FLAlertLayer::create("ReplayBot", err.empty() ? std::string("Replay saved.") : err, "OK")->show();
	this->refresh();
}

void RBMenu::onLoad(CCObject*) {
	const auto name = m_nameInput->getString();
	auto doLoad = [this, name]() {
		const auto msg = ReplaySystem::get().loadReplay(name);
		FLAlertLayer::create("ReplayBot", msg.empty() ? std::string("Replay loaded.") : msg, "OK")->show();
		this->refresh();
	};
	if (ReplaySystem::get().replay.actions().empty()) {
		doLoad();
		return;
	}
	createQuickPopup(
		"Overwrite?",
		"Loading will <cr>overwrite</c> the replay that is currently loaded.",
		"Cancel", "Load",
		[doLoad](FLAlertLayer*, bool btn2) {
			if (btn2) doLoad();
		});
}

void RBMenu::onFrameMode(CCObject* sender) {
	auto on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
	ReplaySystem::get().defaultType = on ? ReplayType::Frame : ReplayType::XPos;
	ReplaySystem::get().saveSettings();
}

void RBMenu::onStatusText(CCObject* sender) {
	auto on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
	ReplaySystem::get().showStatus = on;
	ReplaySystem::get().saveSettings();
	ReplaySystem::get().refreshLabel();
}

void RBMenu::onPathInfo(CCObject*) {
	FLAlertLayer::create(
		"Replay folder",
		"Replays are saved as <cy>name.replay</c> in:\n" + ReplaySystem::get().replayDir().string() +
			"\n\nOld ReplayBot .replay files can be copied in there.",
		"OK")->show();
}

#include "menu.hpp"
#include "replay_system.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace geode::prelude;

namespace {
	template <class Dispatcher, class Target>
	void forcePrio(Dispatcher* dispatcher, Target* target) {
		if constexpr (requires { dispatcher->registerForcePrio(target, 2); }) {
			dispatcher->registerForcePrio(target, 2);
		} else {
			dispatcher->registerForcePrio(target, 2, 0);
		}
	}
}

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
	bg->setContentSize({380.f, 305.f});
	bg->setPosition({c.x, c.y - 27.5f});
	m_mainLayer->addChild(bg, -1);

	m_buttonMenu = CCMenu::create();
	m_buttonMenu->setPosition({0.f, 0.f});
	m_mainLayer->addChild(m_buttonMenu);

	this->registerWithTouchDispatcher();
	forcePrio(CCTouchDispatcher::get(), this);

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

	auto clearBtn = CCMenuItemSpriteExtra::create(ButtonSprite::create("Clear", 50, true, "goldFont.fnt", "GJ_button_06.png", 30.f, 0.7f),
		this, menu_selector(RBMenu::onClear));
	clearBtn->setPosition({left + 300.f, top - 100.f});
	m_buttonMenu->addChild(clearBtn);

	auto noclipToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onNoclip), 0.55f);
	noclipToggle->setPosition({left + 250.f, top - 215.f});
	noclipToggle->toggle(rs.noclip);
	m_buttonMenu->addChild(noclipToggle);
	label("Noclip", {left + 268.f, top - 215.f}, 0.4f, {0.f, 0.5f});

	auto statusToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onStatusText), 0.55f);
	statusToggle->setPosition({left + 130.f, top - 215.f});
	statusToggle->toggle(rs.showStatus);
	m_buttonMenu->addChild(statusToggle);
	label("Status text", {left + 148.f, top - 215.f}, 0.4f, {0.f, 0.5f});

	auto speedToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onSpeedhack), 0.55f);
	speedToggle->setPosition({left + 12.f, top - 245.f});
	speedToggle->toggle(rs.speedhack);
	m_buttonMenu->addChild(speedToggle);
	label("Speedhack", {left + 30.f, top - 245.f}, 0.4f, {0.f, 0.5f});

	auto inputFixToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RBMenu::onInputFixes), 0.55f);
	inputFixToggle->setPosition({left + 12.f, top - 275.f});
	inputFixToggle->toggle(rs.inputFixesEnabled);
	m_buttonMenu->addChild(inputFixToggle);
	label("Input fixes", {left + 30.f, top - 275.f}, 0.4f, {0.f, 0.5f});

	label("Speed", {left + 150.f, top - 245.f}, 0.4f, {0.f, 0.5f});
	m_speedInput = TextInput::create(50.f, "1", "bigFont.fnt");
	m_speedInput->setFilter("0123456789.");
	m_speedInput->setMaxCharCount(5);
	m_speedInput->setString(fmt::format("{}", rs.speed));
	m_speedInput->setPosition({left + 225.f, top - 245.f});
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

void RBMenu::loadByName(std::string const& name) {
	auto doLoad = [name]() {
		const auto msg = ReplaySystem::get().loadReplay(name);
		if (msg.empty()) Notification::create("Replay loaded", NotificationIcon::Success)->show();
		else FLAlertLayer::create("ReplayBot", msg, "OK")->show();
	};
	if (ReplaySystem::get().replay.actions().empty()) {
		doLoad();
		this->refresh();
		return;
	}
	Ref<RBMenu> self = this;
	createQuickPopup(
		"Overwrite?",
		"Loading will <cr>overwrite</c> the replay that is currently loaded.",
		"Cancel", "Load",
		[self, doLoad](FLAlertLayer*, bool btn2) {
			if (!btn2) return;
			doLoad();
			self->refresh();
		});
}

void RBMenu::onLoad(CCObject*) {
	auto names = ReplaySystem::get().listReplays();
	std::string typed = m_nameInput->getString();
	std::transform(typed.begin(), typed.end(), typed.begin(), [](unsigned char ch) { return std::tolower(ch); });
	if (!typed.empty()) {
		std::erase_if(names, [&](std::string const& n) {
			std::string low = n;
			std::transform(low.begin(), low.end(), low.begin(), [](unsigned char ch) { return std::tolower(ch); });
			return low.find(typed) == std::string::npos;
		});
	}
	if (names.empty()) {
		FLAlertLayer::create("ReplayBot", "No .replay macros found.", "OK")->show();
		return;
	}
	Ref<RBMenu> self = this;
	if (auto list = RBLoadMenu::create(std::move(names), [self](std::string const& name) {
		self->m_nameInput->setString(name);
		self->loadByName(name);
	})) list->show();
}

void RBMenu::onClear(CCObject*) {
	ReplaySystem::get().clearReplay();
	this->refresh();
}

void RBMenu::onInputFixes(CCObject* sender) {
	auto on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
	ReplaySystem::get().inputFixesEnabled = on;
	ReplaySystem::get().saveSettings();
}

void RBMenu::onNoclip(CCObject* sender) {
	auto on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
	ReplaySystem::get().noclip = on;
	ReplaySystem::get().saveSettings();
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

void RBMenu::onSpeedhack(CCObject* sender) {
	auto on = !static_cast<CCMenuItemToggler*>(sender)->isToggled();
	ReplaySystem::get().setSpeedhack(on);
	ReplaySystem::get().saveSettings();
}

void RBMenu::onPathInfo(CCObject*) {
	FLAlertLayer::create(
		"Replay folder",
		"Replays are saved as <cy>name.replay</c> in:\n" + ReplaySystem::get().replayDir().string() +
			"\n\nOld ReplayBot .replay files can be copied in there.",
		"OK")->show();
}

RBLoadMenu* RBLoadMenu::create(std::vector<std::string> names, std::function<void(std::string const&)> onPick) {
	auto ret = new RBLoadMenu();
	if (ret && ret->init(std::move(names), std::move(onPick))) {
		ret->autorelease();
		return ret;
	}
	CC_SAFE_DELETE(ret);
	return nullptr;
}

bool RBLoadMenu::init(std::vector<std::string> names, std::function<void(std::string const&)> onPick) {
	if (!this->initWithColor({0, 0, 0, 105})) return false;

	m_names = std::move(names);
	m_onPick = std::move(onPick);

	const auto win = CCDirector::sharedDirector()->getWinSize();
	const CCPoint c = win / 2.f;

	m_mainLayer = CCLayer::create();
	this->addChild(m_mainLayer);

	auto bg = extension::CCScale9Sprite::create("GJ_square01.png", {0.f, 0.f, 80.f, 80.f});
	bg->setContentSize({300.f, 290.f});
	bg->setPosition(c);
	m_mainLayer->addChild(bg, -1);

	m_buttonMenu = CCMenu::create();
	m_buttonMenu->setPosition({0.f, 0.f});
	m_mainLayer->addChild(m_buttonMenu);

	this->registerWithTouchDispatcher();
	forcePrio(CCTouchDispatcher::get(), this);

	auto title = CCLabelBMFont::create("Load Replay", "bigFont.fnt");
	title->setScale(0.7f);
	title->setPosition({c.x, c.y + 125.f});
	m_mainLayer->addChild(title);

	auto closeSpr = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
	closeSpr->setScale(0.7f);
	auto closeBtn = CCMenuItemSpriteExtra::create(closeSpr, this, menu_selector(RBLoadMenu::onClose));
	closeBtn->setPosition({c.x - 150.f + 4.f, c.y + 145.f - 4.f});
	m_buttonMenu->addChild(closeBtn);

	m_listMenu = CCMenu::create();
	m_listMenu->setPosition({0.f, 0.f});
	m_mainLayer->addChild(m_listMenu);

	auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	prevSpr->setScale(0.6f);
	auto prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(RBLoadMenu::onPrev));
	prevBtn->setPosition({c.x - 90.f, c.y - 125.f});
	m_buttonMenu->addChild(prevBtn);

	auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
	nextSpr->setScale(0.6f);
	nextSpr->setFlipX(true);
	auto nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(RBLoadMenu::onNext));
	nextBtn->setPosition({c.x + 90.f, c.y - 125.f});
	m_buttonMenu->addChild(nextBtn);

	m_pageLabel = CCLabelBMFont::create("", "chatFont.fnt");
	m_pageLabel->setScale(0.6f);
	m_pageLabel->setPosition({c.x, c.y - 125.f});
	m_mainLayer->addChild(m_pageLabel);

	this->setKeypadEnabled(true);
	this->setTouchEnabled(true);

	this->rebuild();
	return true;
}

void RBLoadMenu::rebuild() {
	constexpr size_t perPage = 6;
	const auto win = CCDirector::sharedDirector()->getWinSize();
	const CCPoint c = win / 2.f;

	m_listMenu->removeAllChildren();

	const size_t pages = std::max<size_t>(1, (m_names.size() + perPage - 1) / perPage);
	if (m_page >= pages) m_page = pages - 1;

	const size_t first = m_page * perPage;
	for (size_t i = 0; i < perPage && first + i < m_names.size(); ++i) {
		const size_t index = first + i;
		auto spr = ButtonSprite::create(m_names[index].c_str(), 220, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f);
		auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RBLoadMenu::onPick));
		btn->setTag(static_cast<int>(index));
		btn->setPosition({c.x, c.y + 80.f - 38.f * static_cast<float>(i)});
		m_listMenu->addChild(btn);
	}

	m_pageLabel->setString(fmt::format("{}/{}", m_page + 1, pages).c_str());
}

void RBLoadMenu::keyBackClicked() {
	FLAlertLayer::keyBackClicked();
}

void RBLoadMenu::onClose(CCObject*) {
	this->keyBackClicked();
}

void RBLoadMenu::onPick(CCObject* sender) {
	const size_t index = static_cast<size_t>(static_cast<CCNode*>(sender)->getTag());
	if (index >= m_names.size()) return;
	const std::string name = m_names[index];
	auto cb = m_onPick;
	this->keyBackClicked();
	if (cb) cb(name);
}

void RBLoadMenu::onPrev(CCObject*) {
	if (m_page > 0) --m_page;
	this->rebuild();
}

void RBLoadMenu::onNext(CCObject*) {
	++m_page;
	this->rebuild();
}

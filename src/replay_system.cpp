#include "replay_system.hpp"

#include <algorithm>
#include <type_traits>

using namespace geode::prelude;

constexpr const char* kStatusLabelID = "replaybot-status";

namespace {
	template <class P>
	std::filesystem::path toStdPath(const P& p) {
		if constexpr (std::is_convertible_v<const P&, std::filesystem::path>) {
			return p;
		} else if constexpr (requires { p.string(); }) {
			return std::filesystem::path(std::string(p.string()));
		} else if constexpr (requires { p.str(); }) {
			return std::filesystem::path(std::string(p.str()));
		} else if constexpr (requires { p.toString(); }) {
			return std::filesystem::path(std::string(p.toString()));
		} else if constexpr (requires { p.c_str(); }) {
			return std::filesystem::path(std::string(p.c_str()));
		} else if constexpr (requires { fmt::format("{}", p); }) {
			return std::filesystem::path(fmt::format("{}", p));
		} else {
			static_assert(sizeof(P) == 0, "cannot convert the save dir to std::filesystem::path");
		}
	}
}


void ReplaySystem::applyTimeScale() {
	auto* scheduler = CCDirector::sharedDirector()->getScheduler();
	const bool wantScale = speedhack && m_inLevel;
	if (wantScale) {
		if (!m_scaleApplied) {
			m_prevTimeScale = scheduler->getTimeScale();
			m_scaleApplied = true;
		}
		scheduler->setTimeScale(speed);
	} else if (m_scaleApplied) {
		scheduler->setTimeScale(m_prevTimeScale);
		m_scaleApplied = false;
	}
}

void ReplaySystem::setState(RBState state) {
	m_state = state;
	applyTimeScale();
	refreshLabel();
}

bool ReplaySystem::startRecording() {
	auto pl = PlayLayer::get();
	if (!pl) return false;
	replay = Replay(kTps, defaultType);
	m_actionIndex = 0;
	m_restartOnResume = m_frame > 0;
	setState(RBState::Recording);
	return true;
}

bool ReplaySystem::startPlaying() {
	auto pl = PlayLayer::get();
	if (!pl || replay.actions().empty()) return false;
	m_actionIndex = 0;
	m_restartOnResume = m_frame > 0;
	setState(RBState::Playing);
	return true;
}

void ReplaySystem::stop() {
	m_restartOnResume = false;
	setState(RBState::Idle);
}

void ReplaySystem::setSpeed(float value) {
	speed = value;
	applyTimeScale();
}

void ReplaySystem::setSpeedhack(bool on) {
	speedhack = on;
	applyTimeScale();
}

bool ReplaySystem::consumeRestartOnResume() {
	const bool v = m_restartOnResume;
	m_restartOnResume = false;
	return v;
}


void ReplaySystem::onLevelEnter(PlayLayer* pl) {
	m_frame = 0;
	m_actionIndex = 0;
	m_checkpointFrames.clear();
	m_haveLastCheckpoint = false;
	m_checkpointCount = 0;
	m_lastCheckpointFrame = 0;
	m_inLevel = true;
	applyTimeScale();
	refreshLabel();
}

void ReplaySystem::onLevelExit() {
	m_frame = 0;
	m_checkpointFrames.clear();
	m_haveLastCheckpoint = false;
	m_checkpointCount = 0;
	m_lastCheckpointFrame = 0;
	m_inLevel = false;
	stop();
}

void ReplaySystem::onCheckpoint(PlayLayer* pl) {
	if (pl->m_checkpointArray && pl->m_checkpointArray->count() > 0) {
		auto* obj = pl->m_checkpointArray->lastObject();
		m_checkpointFrames[obj] = {m_frame, replay.actions().size()};
		m_lastCheckpointFrame = m_frame;
		m_lastCheckpointActions = replay.actions().size();
		m_haveLastCheckpoint = true;
	}
}

void ReplaySystem::onCheckpointRemoved(PlayLayer* pl) {
	if (!isRecording()) return;
	auto& acts = replay.actions();
	size_t keep = 0;
	if (pl->m_checkpointArray && pl->m_checkpointArray->count() > 0) {
		auto it = m_checkpointFrames.find(pl->m_checkpointArray->lastObject());
		if (it != m_checkpointFrames.end()) keep = it->second.actions;
		else keep = m_haveLastCheckpoint ? m_lastCheckpointActions : acts.size();
	}
	if (keep < acts.size()) acts.resize(keep);
	refreshLabel();
}

void ReplaySystem::onAllCheckpointsRemoved() {
	m_checkpointFrames.clear();
	m_haveLastCheckpoint = false;
	m_checkpointCount = 0;
	m_lastCheckpointFrame = 0;
	m_lastCheckpointActions = 0;
}

void ReplaySystem::trackCheckpoint(PlayLayer* pl) {
	const size_t count = pl->m_checkpointArray ? pl->m_checkpointArray->count() : 0;
	if (count < m_checkpointCount) {
		m_checkpointCount = count;
		onCheckpointRemoved(pl);
		return;
	}
	m_checkpointCount = count;
	if (count == 0) return;
	auto* obj = pl->m_checkpointArray->lastObject();
	if (m_checkpointFrames.try_emplace(obj, CheckpointSnap{m_frame, replay.actions().size()}).second) {
		m_lastCheckpointFrame = m_frame;
		m_lastCheckpointActions = replay.actions().size();
		m_haveLastCheckpoint = true;
	}
}

void ReplaySystem::onReset(PlayLayer* pl) {
	m_inLevel = true;
	applyTimeScale();

	uint32_t checkpointFrame = 0;
	if (pl->m_checkpointArray && pl->m_checkpointArray->count() > 0) {
		auto it = m_checkpointFrames.find(pl->m_checkpointArray->lastObject());
		if (it != m_checkpointFrames.end()) checkpointFrame = it->second.frame;
		else if (m_haveLastCheckpoint) checkpointFrame = m_lastCheckpointFrame;
	}
	m_frame = checkpointFrame;
	const float px = pl->m_player1->getPositionX();

	if (isPlaying()) {
		inject(pl, false, true);
		inject(pl, false, false);

		auto& acts = replay.actions();
		size_t i = 0;
		if (replay.type() == ReplayType::Frame) {
			while (i < acts.size() && acts[i].frame < m_frame) ++i;
		} else {
			while (i < acts.size() && acts[i].x < px) ++i;
		}
		m_actionIndex = i;

		for (bool p2 : {false, true}) {
			for (size_t j = i; j-- > 0;) {
				if (acts[j].player2 == p2) {
					if (acts[j].hold) inject(pl, true, !p2);
					break;
				}
			}
		}
	} else if (isRecording()) {
		if (replay.type() == ReplayType::Frame) replay.removeAfterFrame(m_frame);
		else replay.removeAfterX(px);

		for (bool p2 : {false, true}) {
			if (lastHeld(p2)) addAction(pl, false, p2);
		}
		refreshLabel();
	}
}

void ReplaySystem::beforeStep(PlayLayer* pl) {
	trackCheckpoint(pl);
	if (!isPlaying() || !pl->m_player1) return;

	auto& acts = replay.actions();
	const float px = pl->m_player1->getPositionX();
	while (m_actionIndex < acts.size()) {
		const Action& a = acts[m_actionIndex];
		const bool due = replay.type() == ReplayType::Frame ? m_frame >= a.frame : px >= a.x;
		if (!due) break;
		inject(pl, a.hold, !a.player2);
		++m_actionIndex;
	}
	if (m_actionIndex >= acts.size()) stop();
}

void ReplaySystem::onButton(PlayLayer* pl, bool down, bool isPlayer1) {
	if (!isRecording()) return;
	const bool p2 = !isPlayer1;
	if (lastHeld(p2) == down) return;
	addAction(pl, down, p2);
	refreshLabel();
}

void ReplaySystem::inject(PlayLayer* pl, bool down, bool isPlayer1) {
	injecting = true;
	pl->handleButton(down, 1, isPlayer1);
	injecting = false;
}


bool ReplaySystem::lastHeld(bool player2) const {
	const auto& acts = replay.actions();
	for (size_t i = acts.size(); i-- > 0;) {
		if (acts[i].player2 == player2) return acts[i].hold;
	}
	return false;
}

void ReplaySystem::addAction(PlayLayer* pl, bool hold, bool player2) {
	Action a;
	a.hold = hold;
	a.player2 = player2;
	if (replay.type() == ReplayType::Frame) a.frame = m_frame;
	else a.x = pl->m_player1->getPositionX();
	replay.add(a);
}

void ReplaySystem::refreshLabel() {
	auto pl = PlayLayer::get();
	if (!pl || !pl->m_uiLayer) return;

	auto* label = static_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID(kStatusLabelID));
	if (!label) {
		label = CCLabelBMFont::create("", "chatFont.fnt");
		label->setID(kStatusLabelID);
		label->setAnchorPoint({0.f, 0.f});
		label->setPosition({5.f, 5.f});
		pl->m_uiLayer->addChild(label, 999);
	}
	if (!showStatus) label->setString("");
	else if (isRecording()) {
		int presses = 0;
		int releases = 0;
		for (const auto& a : replay.actions()) {
			if (a.hold) ++presses;
			else ++releases;
		}
		label->setString(fmt::format("Recording {}/{}", presses, releases).c_str());
	}
	else if (isPlaying()) label->setString("Playing");
	else label->setString("");
}


void ReplaySystem::loadSettings() {
	auto* mod = Mod::get();
	defaultType = mod->getSavedValue<bool>("frame-mode", true) ? ReplayType::Frame : ReplayType::XPos;
	showStatus = mod->getSavedValue<bool>("show-status", true);
	speed = mod->getSavedValue<float>("speed", 1.f);
	speedhack = mod->getSavedValue<bool>("speedhack", false);
}

void ReplaySystem::saveSettings() {
	auto* mod = Mod::get();
	mod->setSavedValue<bool>("frame-mode", defaultType == ReplayType::Frame);
	mod->setSavedValue<bool>("show-status", showStatus);
	mod->setSavedValue<float>("speed", speed);
	mod->setSavedValue<bool>("speedhack", speedhack);
}


std::filesystem::path ReplaySystem::replayDir() {
	std::filesystem::path dir = toStdPath(Mod::get()->getSaveDir());
	dir /= "replays";
	std::error_code ec;
	std::filesystem::create_directories(dir, ec);
	return dir;
}

std::vector<std::string> ReplaySystem::listReplays() {
	std::vector<std::string> names;
	std::error_code ec;
	for (const auto& entry : std::filesystem::directory_iterator(replayDir(), ec)) {
		if (entry.is_regular_file() && entry.path().extension() == ".replay")
			names.push_back(entry.path().stem().string());
	}
	std::sort(names.begin(), names.end());
	return names;
}

std::string ReplaySystem::saveReplay(const std::string& name) {
	if (name.empty()) return "Type a replay name first.";
	if (replay.actions().empty()) return "There is nothing to save yet.";
	if (!replay.save(replayDir() / (name + ".replay"))) return "Could not write the file.";
	return "";
}

std::string ReplaySystem::loadReplay(const std::string& name) {
	if (name.empty()) return "Type a replay name first.";
	auto loaded = Replay::load(replayDir() / (name + ".replay"));
	if (!loaded) return "Could not read that replay (missing or corrupt).";
	stop();
	replay = std::move(*loaded);
	if (replay.tps() != kTps) {
		return fmt::format(
			"Loaded, but it was recorded at {} fps. 2.2 runs physics at 240 steps/s, so it will probably desync.",
			replay.tps());
	}
	return "";
}

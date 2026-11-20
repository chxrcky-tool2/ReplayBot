#pragma once
#include "replay.hpp"

#include <Geode/Geode.hpp>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

enum class RBState { Idle, Recording, Playing };

class ReplaySystem {
	RBState m_state = RBState::Idle;
	uint32_t m_frame = 0;
	size_t m_actionIndex = 0;
	bool m_restartOnResume = false;
	bool m_scaleApplied = false;
	bool m_inLevel = false;
	uint32_t m_lastCheckpointFrame = 0;
	bool m_haveLastCheckpoint = false;
	float m_prevTimeScale = 1.f;
	struct CheckpointSnap { uint32_t frame = 0; size_t actions = 0; };
	std::unordered_map<cocos2d::CCObject*, CheckpointSnap> m_checkpointFrames;
	size_t m_lastCheckpointActions = 0;
	size_t m_checkpointCount = 0;

	ReplaySystem() = default;

	void setState(RBState state);
	void applyTimeScale();
	void trackCheckpoint(PlayLayer* pl);
	bool lastHeld(bool player2) const;
	void addAction(PlayLayer* pl, bool hold, bool player2);

public:
	static constexpr float kTps = 240.f;

	static ReplaySystem& get() {
		static ReplaySystem instance;
		return instance;
	}
	ReplaySystem(const ReplaySystem&) = delete;

	Replay replay;
	ReplayType defaultType = ReplayType::Frame;
	bool showStatus = true;
	float speed = 1.f;
	bool speedhack = false;
	bool injecting = false;

	bool isRecording() const { return m_state == RBState::Recording; }
	bool isPlaying() const { return m_state == RBState::Playing; }
	uint32_t frame() const { return m_frame; }

	void loadSettings();
	void saveSettings();

	bool startRecording();
	bool startPlaying();
	void stop();
	void setSpeed(float value);
	void setSpeedhack(bool on);

	void onLevelEnter(PlayLayer* pl);
	void onLevelExit();
	void onReset(PlayLayer* pl);
	void onCheckpoint(PlayLayer* pl);
	void onCheckpointRemoved(PlayLayer* pl);
	void onAllCheckpointsRemoved();
	void beforeStep(PlayLayer* pl);
	void afterStep() { ++m_frame; }
	void onButton(PlayLayer* pl, bool down, bool isPlayer1);
	bool consumeRestartOnResume();

	void inject(PlayLayer* pl, bool down, bool isPlayer1);
	void refreshLabel();

	std::filesystem::path replayDir();
	std::vector<std::string> listReplays();
	std::string saveReplay(const std::string& name);
	std::string loadReplay(const std::string& name);
};

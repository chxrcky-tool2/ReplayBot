#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

enum class ReplayType : uint8_t { XPos = 0, Frame = 1 };

struct Action {
	union {
		float x;
		uint32_t frame;
	};
	bool hold = false;
	bool player2 = false;

	Action() : frame(0) {}
};

class Replay {
	std::vector<Action> m_actions;
	float m_tps;
	ReplayType m_type;

public:
	explicit Replay(float tps = 240.f, ReplayType type = ReplayType::Frame) : m_tps(tps), m_type(type) {}

	float tps() const { return m_tps; }
	ReplayType type() const { return m_type; }
	std::vector<Action>& actions() { return m_actions; }
	const std::vector<Action>& actions() const { return m_actions; }

	void add(const Action& action) { m_actions.push_back(action); }
	void removeAfterX(float x);
	void removeAfterFrame(uint32_t frame);

	bool save(const std::filesystem::path& path) const;
	static std::optional<Replay> load(const std::filesystem::path& path);
};

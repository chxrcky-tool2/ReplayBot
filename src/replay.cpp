#include "replay.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>

namespace {
	constexpr uint8_t kFormatVersion = 2;
	constexpr char kMagic[4] = {'R', 'P', 'L', 'Y'};

	template <class T>
	void writeBin(std::ofstream& f, T v) {
		f.write(reinterpret_cast<const char*>(&v), sizeof(T));
	}

	template <class T>
	bool readBin(std::ifstream& f, T& v) {
		f.read(reinterpret_cast<char*>(&v), sizeof(T));
		return static_cast<bool>(f);
	}
}

void Replay::removeAfterX(float x) {
	std::erase_if(m_actions, [&](const Action& a) { return a.x >= x; });
}

void Replay::removeAfterFrame(uint32_t frame) {
	std::erase_if(m_actions, [&](const Action& a) { return a.frame >= frame; });
}

bool Replay::save(const std::filesystem::path& path) const {
	std::ofstream f(path, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!f) return false;

	f.write(kMagic, 4);
	writeBin<uint8_t>(f, kFormatVersion);
	writeBin<uint8_t>(f, static_cast<uint8_t>(m_type));
	writeBin<float>(f, m_tps);
	for (const auto& a : m_actions) {
		if (m_type == ReplayType::XPos) writeBin<float>(f, a.x);
		else writeBin<uint32_t>(f, a.frame);
		writeBin<uint8_t>(f, static_cast<uint8_t>((a.hold ? 1 : 0) | (a.player2 ? 2 : 0)));
	}
	return static_cast<bool>(f);
}

std::optional<Replay> Replay::load(const std::filesystem::path& path) {
	std::ifstream f(path, std::ios::binary | std::ios::in);
	if (!f) return std::nullopt;

	char magic[4];
	f.read(magic, 4);
	if (!f) return std::nullopt;

	Replay r(0.f, ReplayType::XPos);

	if (std::memcmp(magic, kMagic, 4) == 0) {
		uint8_t ver = 0;
		if (!readBin(f, ver) || (ver != 1 && ver != 2)) return std::nullopt;
		if (ver == 2) {
			uint8_t t = 0;
			if (!readBin(f, t) || t > 1) return std::nullopt;
			r.m_type = static_cast<ReplayType>(t);
		}
		if (!readBin(f, r.m_tps)) return std::nullopt;

		while (true) {
			Action a;
			if (r.m_type == ReplayType::XPos) {
				if (!readBin(f, a.x)) break;
			} else {
				if (!readBin(f, a.frame)) break;
			}
			uint8_t state = 0;
			if (!readBin(f, state)) break;
			a.hold = state & 1;
			a.player2 = state & 2;
			r.add(a);
		}
	} else {
		std::memcpy(&r.m_tps, magic, 4);
		while (true) {
			Action a;
			uint8_t hold = 0, p2 = 0;
			if (!readBin(f, a.x) || !readBin(f, hold) || !readBin(f, p2)) break;
			a.hold = hold != 0;
			a.player2 = p2 != 0;
			r.add(a);
		}
	}

	if (!std::isfinite(r.m_tps) || r.m_tps <= 0.f) return std::nullopt;
	return r;
}

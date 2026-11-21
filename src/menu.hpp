#pragma once
#include <Geode/Geode.hpp>
#include <functional>
#include <string>
#include <vector>

class RBMenu : public FLAlertLayer {
protected:
	geode::TextInput* m_nameInput = nullptr;
	geode::TextInput* m_speedInput = nullptr;
	cocos2d::CCLabelBMFont* m_info = nullptr;
	cocos2d::CCLabelBMFont* m_saved = nullptr;
	ButtonSprite* m_recordSprite = nullptr;
	ButtonSprite* m_playSprite = nullptr;

	bool init() override;
	void refresh();
	void keyBackClicked() override;

	void onClose(cocos2d::CCObject*);
	void onRecord(cocos2d::CCObject*);
	void onPlay(cocos2d::CCObject*);
	void onSave(cocos2d::CCObject*);
	void onLoad(cocos2d::CCObject*);
	void onFrameMode(cocos2d::CCObject*);
	void onStatusText(cocos2d::CCObject*);
	void onSpeedhack(cocos2d::CCObject*);
	void onPathInfo(cocos2d::CCObject*);
	void onClear(cocos2d::CCObject*);
	void onNoclip(cocos2d::CCObject*);
	void onInputFixes(cocos2d::CCObject*);
	void loadByName(std::string const& name);

public:
	static RBMenu* create();
	static void open();
};

class RBLoadMenu : public FLAlertLayer {
protected:
	std::vector<std::string> m_names;
	std::function<void(std::string const&)> m_onPick;
	size_t m_page = 0;
	cocos2d::CCMenu* m_listMenu = nullptr;
	cocos2d::CCLabelBMFont* m_pageLabel = nullptr;

	bool init(std::vector<std::string> names, std::function<void(std::string const&)> onPick);
	void rebuild();
	void keyBackClicked() override;

	void onClose(cocos2d::CCObject*);
	void onPick(cocos2d::CCObject*);
	void onPrev(cocos2d::CCObject*);
	void onNext(cocos2d::CCObject*);

public:
	static RBLoadMenu* create(std::vector<std::string> names, std::function<void(std::string const&)> onPick);
};

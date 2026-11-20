#pragma once
#include <Geode/Geode.hpp>

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
	void onPathInfo(cocos2d::CCObject*);

public:
	static RBMenu* create();
	static void open();
};

#include <Geode/Geode.hpp>

using namespace geode::prelude;


#include <Geode/modify/FLAlertLayer.hpp>
#include <Geode/modify/HSVLiveOverlay.hpp>
#include <Geode/modify/LevelLeaderboard.hpp>

#include <Geode/modify/CCTouchDispatcher.hpp>


#define BOUNCE_ID "bouncing"_spr
#define NO_BOUNCE_ID "no-bounce"_spr
#define BG_REPLACER_ID "bg-replacer"_spr
#define BOUNCE_PRESET_ID "bounce-preset"_spr


#define setting(type, name) Mod::get()->getSettingValue<type>(name)
#define area(ccSize) (ccSize.width * ccSize.height)


#define STANDARD_BOUNCE(classname) class $modify(classname) {			\
	void show() {														\
		classname::show();												\
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();	\
	}																	\
};																		\


struct BouncePreset : CCObject {
	short m_xVec, m_yVec;
	CCPoint m_pos;
	BouncePreset(short xVec, short yVec, CCPoint pos): m_xVec(xVec), m_yVec(yVec), m_pos(pos) {
		this->autorelease();
	}
}; 


void findPage(std::string id, int zOrd, short xVec, short yVec, CCPoint pos);


class $modify(MyFLAlertLayer, FLAlertLayer) {

	struct Fields {
		short xVector;
		short yVector;
		CCRect bouncingRect;
		float speed;
		bool actionCreated = false;
		bool fixDropDownAnimation = false;
	};


	inline void scheduleBounceStart(float wait = 0) {
		scheduleOnce(schedule_selector(MyFLAlertLayer::createBounceAction), wait);
	}


	void createBounceAction(float) {
		auto f = m_fields.self();
		// if (getParent() != CCScene::get()) return;
		if (!setting(bool, "enable")) return;
		if (f->actionCreated) return;
		if (!getParent()) return;
		if (getUserObject(NO_BOUNCE_ID)) return;

		for (CCNode* node = this; node != nullptr; node = node->getParent()) {
			if (node->getUserObject(BOUNCE_ID)) return;
		}

		f->actionCreated = true;

		if (setting(bool, "no-triggers")) {
			if (typeinfo_cast<SetupTriggerPopup*>(this)) return;
			if (typeinfo_cast<GJColorSetupLayer*>(this)) return;
			if (typeinfo_cast<HSVWidgetPopup*>(this)) return;
			if (typeinfo_cast<LevelSettingsLayer*>(this)) return;
			if (typeinfo_cast<ColorSelectLiveOverlay*>(this)) return;
			if (typeinfo_cast<HSVLiveOverlay*>(this)) return;
			if (getID() == "SetGroupIDLayer") return;
			if (getID() == "CustomizeObjectLayer") return;
		}

		if (!m_mainLayer) return;
		CCNode* bg = m_mainLayer->getChildByType<CCScale9Sprite>(0);
		if (!bg) bg = m_mainLayer->getChildByType<NineSlice>(0);
		if (!bg) bg = m_mainLayer->getChildByID(BG_REPLACER_ID);
		if (!bg) bg = m_mainLayer->getChildByType<CCSprite>(0);
		if (!bg) return;

		// exceptions
		if (bg->getID() == "eclipse.eclipse-menu/bg-behind") return;

		// popups with presets
		bool usePresetValues = false;
		if (auto obj = static_cast<BouncePreset*>(getUserObject(BOUNCE_PRESET_ID))) {
			f->xVector = obj->m_xVec;
			f->yVector = obj->m_yVec;
			m_mainLayer->setPosition(obj->m_pos);
			usePresetValues = true;
		}

		// fix popups with dropdown animations
		if (typeinfo_cast<CommunityCreditsPage*>(this)) {
			f->fixDropDownAnimation = true;
		}

		const auto winSz = CCDirector::get()->getWinSize();
		const auto bgBox = bg->boundingBox();

		if (bgBox.size.width >= winSz.width || bgBox.size.height >= winSz.height) return;
		if (setting(bool, "only-small") && area(winSz) < area(bgBox.size) * 3.5) return;

		const CCPoint shift = ccp(bgBox.getMidX(), bgBox.getMidY()) - (m_mainLayer->getContentSize() / 2);

		f->bouncingRect = CCRect(
			bgBox.size.width / 2 - shift.x,
			bgBox.size.height / 2 - shift.y,
			winSz.width - bgBox.size.width,
			winSz.height - bgBox.size.height
		);

		if (!usePresetValues) {
			int r = rand();
			f->xVector = (r & 4) ? 1 : -1;
			f->yVector = (r & 2) ? 1 : -1;
		}
		
		if (setting(bool, "heavy-popups")) {
			f->speed = (66 - 0.0003 * area(bgBox.size)) * (setting(float, "anim-speed"));
		} else {
			f->speed = setting(float, "anim-speed") * 40;
		}

		for (auto* child : CCArrayExt<CCNode*>(getChildren())) {
			if (typeinfo_cast<FLAlertLayer*>(child)) {
				continue; // don't touch stacked popups
			}
			child->setUserObject(BOUNCE_ID, CCBool::create(true));
		}
		
		if (usePresetValues) {
			bounce();
		} else {
			runAction(CCSequence::createWithTwoActions(
				CCDelayTime::create(setting(float, "delay")),
				CCCallFunc::create(this, callfunc_selector(MyFLAlertLayer::bounce))
			));
		}
	}


	void bounce() {
		const auto f = m_fields.self();

		CCPoint pos;
		if (f->fixDropDownAnimation) {
			auto oldPoint = m_mainLayer->getPosition();
			m_mainLayer->setPosition({0,0});
			pos = m_mainLayer->convertToWorldSpace(m_mainLayer->getContentSize() / 2);
			m_mainLayer->setPosition(oldPoint);
			f->fixDropDownAnimation = false;
		} else {
			pos = m_mainLayer->convertToWorldSpace(m_mainLayer->getContentSize() / 2);
		}

		// find the next direction vector
		bool xSwitched = false;
		if (pos.x > f->bouncingRect.getMaxX() - 0.5 || pos.x < f->bouncingRect.getMinX() + 0.5) {
			f->xVector *= -1;
			xSwitched = true;
		}
		if (pos.y > f->bouncingRect.getMaxY() - 0.5 || pos.y < f->bouncingRect.getMinY() + 0.5) {
			f->yVector *= -1;
			if (xSwitched) {
				log::info("OMG IT CORNERED!!!  OMG IT CORNERED!!!  OMG IT CORNERED!!!");
			}
		}

		// find the next bounce pos, delta pos and duration
		const float xTarget = (f->xVector == 1) ? f->bouncingRect.getMaxX() : f->bouncingRect.getMinX();
		const float yTarget = (f->yVector == 1) ? f->bouncingRect.getMaxY() : f->bouncingRect.getMinY();
		const float xDist = std::abs(pos.x - xTarget);
		const float yDist = std::abs(pos.y - yTarget);
		const float dist = std::min(xDist, yDist);
		const float delay = dist / f->speed;

		// run action
		bool flag = true;
		for (auto* child : CCArrayExt<CCNode*>(getChildren())) {
			if (!child->getUserObject(BOUNCE_ID)) continue;
			if (flag) {
				child->runAction(CCSequence::createWithTwoActions(
					CCMoveBy::create(delay, ccp(f->xVector, f->yVector) * dist),
					CCCallFunc::create(this, callfunc_selector(MyFLAlertLayer::bounce))
				));
				flag = false;
			} else {
				child->runAction(CCMoveBy::create(delay, ccp(f->xVector, f->yVector) * dist));
			}
		}
	}


	void onBtn1(CCObject* sender) {
		// fix betterinfo pages popups
		bool trackPages = false;
		if (auto menu = m_mainLayer->getChildByID("main-menu")) {
			if (menu->getChildByID("cvolton.betterinfo/next-button")) {
				trackPages = true;
			}
		}
		if (!m_mainLayer || !trackPages) return FLAlertLayer::onBtn1(sender);
		const auto id = getID();
		const auto zOrd = getZOrder();
		const auto xVec = m_fields->xVector;
		const auto yVec = m_fields->yVector;
		const auto pos = m_mainLayer->getPosition();

		FLAlertLayer::onBtn1(sender);

		queueInMainThread([id, zOrd, xVec, yVec, pos](){
			findPage(id, zOrd, xVec, yVec, pos);
		});
	}
};

class $modify(LevelLeaderboard) {
	// fix leaderboard pages
	void reloadLeaderboard(LevelLeaderboardType type, LevelLeaderboardMode mode) {
		// don't do f = m_fields here, it'll crash
		short xVec = reinterpret_cast<MyFLAlertLayer*>(this)->m_fields->xVector;
		short yVec = reinterpret_cast<MyFLAlertLayer*>(this)->m_fields->yVector;
		auto pos = m_mainLayer->getPosition();
		LevelLeaderboard::reloadLeaderboard(type, mode);
		if (auto newPopup = CCScene::get()->getChildByType<LevelLeaderboard>(0)) {
			newPopup->setUserObject(BOUNCE_PRESET_ID, new BouncePreset(xVec, yVec, pos));
			newPopup->m_mainLayer->setPosition(pos);
		}
	}
};


void findPage(std::string id, int zOrd, short xVec, short yVec, CCPoint pos) {
	if (auto popup = CCScene::get()->getChildByID(id); popup && popup->getZOrder() == zOrd) {
		if (typeinfo_cast<FLAlertLayer*>(popup)) {
			popup->setUserObject(BOUNCE_PRESET_ID, new BouncePreset(xVec, yVec, pos));
			reinterpret_cast<MyFLAlertLayer*>(popup)->createBounceAction(0);
		}
	}
}


class $modify(HSVLiveOverlay) {
	void show() {
		HSVLiveOverlay::show();
		if (typeinfo_cast<HSVLiveOverlay*>(this)) { // ! bindings address issue
			if (m_widget && m_mainLayer) { // fix non-standard popup structure
				if (auto realBg = m_widget->getChildByType<CCScale9Sprite>(0)) {
					auto fakeBg = CCSprite::create();
					fakeBg->setPosition(m_widget->getPosition());
					fakeBg->setContentSize(realBg->getScaledContentSize() * m_widget->getScale());
					m_mainLayer->addChild(fakeBg);
					fakeBg->setID(BG_REPLACER_ID);
				}
			}
		}
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};


class $modify(CCTouchDispatcher) {
	// new way of finding popups
	void registerForcePrio(CCObject* p0, int p1) {
		CCTouchDispatcher::registerForcePrio(p0, p1);
		if (auto popup = typeinfo_cast<FLAlertLayer*>(p0)) {
			reinterpret_cast<MyFLAlertLayer*>(popup)->scheduleBounceStart();
		}
	}
};

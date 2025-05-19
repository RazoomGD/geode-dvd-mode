#include <Geode/Geode.hpp>

using namespace geode::prelude;

#include <Geode/modify/FLAlertLayer.hpp>
#include <Geode/modify/SetIDPopup.hpp>
#include <Geode/modify/SetTextPopup.hpp>
#include <Geode/modify/SetupTriggerPopup.hpp>
#include <Geode/modify/ShardsPage.hpp>
#include <Geode/modify/CharacterColorPage.hpp>
#include <Geode/modify/CommunityCreditsPage.hpp>
#include <Geode/modify/ColorSelectPopup.hpp>
#include <Geode/modify/CustomSongLayer.hpp>
#include <Geode/modify/SetupPulsePopup.hpp>
#include <Geode/modify/DailyLevelPage.hpp>
#include <Geode/modify/GJPathsLayer.hpp>
#include <Geode/modify/GJPathPage.hpp>
#include <Geode/modify/ChallengesPage.hpp>
#include <Geode/modify/GJPromoPopup.hpp>
#include <Geode/modify/LevelLeaderboard.hpp>


#define setting(type, name) Mod::get()->getSettingValue<type>(name)
#define area(ccSize) (ccSize.width * ccSize.height)

class $modify(MyFLAlertLayer, FLAlertLayer) {

	struct Fields {
		short xVector;
		short yVector;
		CCRect bouncingRect;
		float speed;
	};

	inline void scheduleBounceStart(float wait = 0) {
		scheduleOnce(schedule_selector(MyFLAlertLayer::startBouncing), wait);
	}

	void startBouncing(float) {
		// if (getParent() != CCScene::get()) return;
		if (!setting(bool, "enable")) return;
		for (CCNode* node = this; node != nullptr; node = node->getParent()) {
			if (node->getUserObject("bouncing"_spr)) return;
		}
		if (getUserObject("no-bounce"_spr)) return;
		if (setting(bool, "no-triggers")) {
			if (typeinfo_cast<SetupTriggerPopup*>(this)) return;
			if (typeinfo_cast<GJColorSetupLayer*>(this)) return;
			if (typeinfo_cast<HSVWidgetPopup*>(this)) return;
			if (typeinfo_cast<LevelSettingsLayer*>(this)) return;
			if (getID() == "SetGroupIDLayer") return;
			if (getID() == "CustomizeObjectLayer") return;
		}

		if (!m_mainLayer) return;
		CCNode* bg = m_mainLayer->getChildByType<CCScale9Sprite>(0);
		if (!bg) bg = m_mainLayer->getChildByType<CCSprite>(0);
		if (!bg) return;

		const auto winSz = CCDirector::get()->getWinSize();
		const auto bgBox = bg->boundingBox();

		if (bgBox.size.width >= winSz.width || bgBox.size.height >= winSz.height) return;
		if (setting(bool, "only-small") && area(winSz) < area(bgBox.size) * 3.5) return;

		const CCPoint shift = ccp(bgBox.getMidX(), bgBox.getMidY()) - (m_mainLayer->getContentSize() / 2);

		m_fields->bouncingRect = CCRect(
			bgBox.size.width / 2 - shift.x,
			bgBox.size.height / 2 - shift.y,
			winSz.width - bgBox.size.width,
			winSz.height - bgBox.size.height
		);

		int r = rand();
		m_fields->xVector = (r & 4) ? 1 : -1;
		m_fields->yVector = (r & 2) ? 1 : -1;
		
		if (setting(bool, "heavy-popups")) {
			m_fields->speed = (66 - 0.0003 * area(bgBox.size)) * (setting(float, "anim-speed"));
		} else {
			m_fields->speed = setting(float, "anim-speed") * 40;
		}

		runAction(CCSequence::createWithTwoActions(
			CCDelayTime::create(setting(float, "delay")),
			CCCallFunc::create(this, callfunc_selector(MyFLAlertLayer::bounce))
		));

		setUserObject("bouncing"_spr, CCBool::create(true));
	}

	void bounce() {
		const auto pos = m_mainLayer->convertToWorldSpace(m_mainLayer->getContentSize() / 2);
		const auto f = m_fields.self();

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

	void show() {
		FLAlertLayer::show();
		if (getID() == "ProfilePage") return; //  - stacked popups issue 
		scheduleBounceStart();
	}
};


// special popups

class $modify(SetIDPopup) {
	void show() {
		SetIDPopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(SetTextPopup) {
	void show() {
		SetTextPopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(SetupTriggerPopup) {
	void show() {
		SetupTriggerPopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(ShardsPage) {
	void show() {
		ShardsPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(CharacterColorPage) {
	void show() {
		CharacterColorPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

// class $modify(RewardsPage) { - stacked popups issue 
// 	void show() {
// 		RewardsPage::show();
// 		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
// 	}
// };

// class $modify(InfoLayer) { - stacked popups issue
// 	void show() {
// 		InfoLayer::show();
// 		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
// 	}
// };

class $modify(CommunityCreditsPage) {
	void show() {
		CommunityCreditsPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(ColorSelectPopup) {
	void show() {
		ColorSelectPopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(CustomSongLayer) {
	void show() {
		CustomSongLayer::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(SetupPulsePopup) {
	void show() {
		SetupPulsePopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(DailyLevelPage) {
	void show() {
		DailyLevelPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(GJPathsLayer) {
	void show() {
		GJPathsLayer::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(GJPathPage) {
	void show() {
		GJPathPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(ChallengesPage) {
	void show() {
		ChallengesPage::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(GJPromoPopup) {
	void show() {
		GJPromoPopup::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};

class $modify(LevelLeaderboard) {
	void show() {
		LevelLeaderboard::show();
		reinterpret_cast<MyFLAlertLayer*>(this)->scheduleBounceStart();
	}
};


#include "resultscreen.h"
#include "gamescreen.h"

class ResultScreen
{
public:
	int isActive = 0;

	int bgEntity;
	int packageText;
	int timeText;
	int resultText;
	int rankText;
	int pressStartId;

	double offset;

	std::string resultStrings[5] = {
		"The emperor is defeated! You are THE mailest MAN of them all!", 
		"The emperor is looking at your file saying \"Omoshiroi\". You almost have him!",
		"Colonel is starting to approve of you! If this was a dating simulator he would be at two hearts now!!", 
		"Remember, being a mailman in 2023 is not a walk in the park like being a green beret! Stay strong!",
		"Don't give up mailman, bad luck, dogs and poor collision detection must never stop a real mailman!"};

	std::string rankStrings[5] = {
	"S",
	"A",
	"B",
	"C",
	"F" };

	ResultScreen() {

		offset = 240;
		bgEntity = addBlitzEntity(Vector3D(0, 240 - offset, 80));
		addBlitzMugenAnimationComponent(bgEntity, getGameSprites(), getGameAnimations(), 500);
		setBlitzMugenAnimationIndependentOfCamera(bgEntity);
		setBlitzMugenAnimationVisibility(bgEntity, 0);

		pressStartId = addBlitzEntity(Vector3D(235, 205 - offset, 81));
		addBlitzMugenAnimationComponent(pressStartId, getGameSprites(), getGameAnimations(), 501);
		setBlitzMugenAnimationIndependentOfCamera(pressStartId);
		setBlitzMugenAnimationVisibility(pressStartId, 0);

		packageText = addMugenTextMugenStyle("12", Vector3D(70, 90 - offset, 81), Vector3DI(1, 0, 1));
		setMugenTextVisibility(packageText, 0);

		timeText = addMugenTextMugenStyle("1:00:00", Vector3D(40, 130 - offset, 81), Vector3DI(1, 0, 1));
		setMugenTextVisibility(timeText, 0);

		resultText = addMugenTextMugenStyle(resultStrings[3].c_str(), Vector3D(5, 195 - offset, 81), Vector3DI(1, 0, 1));
		setMugenTextTextBoxWidth(resultText, 160);
		setMugenTextVisibility(resultText, 0);

		rankText = addMugenTextMugenStyle("F", Vector3D(60, 155 - offset, 81), Vector3DI(1, 0, 1));
		setMugenTextVisibility(rankText, 0);
	}

	void startFadeIn()
	{
		std::stringstream ss;
		ss << getPackagesCollectedCount();
		changeMugenText(packageText, ss.str().c_str());

		std::stringstream ss2;
		ss2 << (getGameTicksSpent() / 60.0) << " SECONDS";
		changeMugenText(timeText, ss2.str().c_str());

		auto packages = getPackagesCollectedCount();
		int rank = 0;
		if (packages >= 1) rank = 1;
		if (packages >= 3) rank = 2;
		if (packages >= 6) rank = 3;
		if (packages >= 10) rank = 4;
		rank = 4 - rank;

		changeMugenText(resultText, resultStrings[rank].c_str());
		changeMugenText(rankText, rankStrings[rank].c_str());

		setBlitzMugenAnimationVisibility(bgEntity, 1);
		setBlitzMugenAnimationVisibility(pressStartId, 1);
		setMugenTextVisibility(packageText, 1);
		setMugenTextVisibility(timeText, 1);
		setMugenTextVisibility(resultText, 1);
		setMugenTextVisibility(rankText, 1);
		isActive = 1;

		streamMusicFileOnce("music/result.ogg");
	}

	void reset()
	{
		setBlitzMugenAnimationVisibility(bgEntity, 0);
		setBlitzMugenAnimationVisibility(pressStartId, 0);
		setMugenTextVisibility(packageText, 0);
		setMugenTextVisibility(timeText, 0);
		setMugenTextVisibility(resultText, 0);
		setMugenTextVisibility(rankText, 0);
		isActive = 1;
	}

	void update() {
		if (!isActive) return;
		updateInput();
		updateFadeIn();
	}

	void updateFadeIn()
	{
	
		auto pos = getBlitzEntityPositionReference(bgEntity);
		if (pos->y < 240)
		{
			auto speed = 5;
			pos->y += speed;
			addBlitzEntityPositionY(pressStartId, speed);
			addMugenTextPosition(packageText, Position(0, speed, 0));
			addMugenTextPosition(timeText, Position(0, speed, 0));
			addMugenTextPosition(resultText, Position(0, speed, 0));
			addMugenTextPosition(rankText, Position(0, speed, 0));
		}
	}

	void updateInput()
	{
		if (hasPressedStartFlank())
		{
			resetGameState();
			setNewScreen(getGameScreen());
		}
	}

};


EXPORT_ACTOR_CLASS(ResultScreen);

int isResultScreenActive()
{
	return gResultScreen->isActive;
}

void startResultScreen()
{
	gResultScreen->startFadeIn();
}
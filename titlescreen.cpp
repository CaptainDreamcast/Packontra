#include "titlescreen.h"

#include <prism/blitz.h>

#include "gamescreen.h"

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

	int bgEntity;
	int pressStartEntity;
	int titleEntity;

	int hasFlashed;
	int flashTicks;
} gTitleScreenData;

static void loadTitleScreen() {
	gTitleScreenData.mSprites = loadMugenSpriteFileWithoutPalette("game/TITLE.sff");
	gTitleScreenData.mAnimations = loadMugenAnimationFile("game/TITLE.air");
	
	
	gTitleScreenData.bgEntity = addBlitzEntity(Vector3D(0, 0, 1));
	addBlitzMugenAnimationComponent(gTitleScreenData.bgEntity, &gTitleScreenData.mSprites, &gTitleScreenData.mAnimations, 2);

	gTitleScreenData.titleEntity = addBlitzEntity(Vector3D(160, 50 - 320, 2));
	addBlitzMugenAnimationComponent(gTitleScreenData.titleEntity, &gTitleScreenData.mSprites, &gTitleScreenData.mAnimations, 3);

	gTitleScreenData.pressStartEntity = addBlitzEntity(Vector3D(160, 220, 2));
	addBlitzMugenAnimationComponent(gTitleScreenData.pressStartEntity, &gTitleScreenData.mSprites, &gTitleScreenData.mAnimations, 1);
	setBlitzMugenAnimationVisibility(gTitleScreenData.pressStartEntity, 0);

	streamMusicFileOnce("music/title.ogg");

	gTitleScreenData.hasFlashed = false;
	gTitleScreenData.flashTicks = 20;

	setWrapperTitleScreen(getTitleScreen());
}


static void gotoGameScreen(void* tCaller) {
	(void)tCaller;
	resetGameState();
	setNewScreen(getGameScreen());
}

static void updateTitleScreenFadeIn()
{
	auto* pos = getBlitzEntityPositionReference(gTitleScreenData.bgEntity);
	if (pos->x <= 320)
	{
		pos->x += 1.7;
		auto* pos2 = getBlitzEntityPositionReference(gTitleScreenData.titleEntity);
		pos2->y += 1.7;
	}
	else
	{
		if (!gTitleScreenData.hasFlashed)
		{
			setScreenColor(COLOR_WHITE);
			gTitleScreenData.hasFlashed = 1;
		}
		else if (gTitleScreenData.hasFlashed && gTitleScreenData.flashTicks)
		{
			gTitleScreenData.flashTicks--;
			if (!gTitleScreenData.flashTicks)
			{
				setScreenColor(COLOR_BLACK);
				enableDrawing();
				setBlitzMugenAnimationVisibility(gTitleScreenData.pressStartEntity, 1);
			}
		}
	}
}

static void updateTitleScreen() {

	updateTitleScreenFadeIn();
	if (hasPressedAFlank() || hasPressedStartFlank()) {
		addFadeOut(30, gotoGameScreen, NULL);
	}
}

static Screen gTitleScreen;

Screen* getTitleScreen() {
	gTitleScreen = makeScreen(loadTitleScreen, updateTitleScreen);
	return &gTitleScreen;
};

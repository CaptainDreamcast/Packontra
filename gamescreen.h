#pragma once

#include <prism/blitz.h>

Screen* getGameScreen();

MugenSpriteFile* getGameSprites();
MugenAnimations* getGameAnimations();
MugenSounds* getGameSounds();

Vector2D getPlayerStartPosition();
void shootPackage(int id, Vector2D pos, double angle);

int getPackageMaxTime(int id);
void removePackage(int id);
Vector2D getPackageTargetPosition(int id);
int isValidPackage(int id);
int canPackageBeSelected(int id);

GeoRectangle2D getPlayerConstraintRectangle();
GeoRectangle2D getShotConstraintRectangle();
GeoRectangle2D getGameCameraRange();

void resetGameState();
int getGameTicksSpent();

int getPackagesCollectedCount();
#pragma once

#include <prism/blitz.h>

typedef struct {
	CollisionListData* mList;
	int mNumber;
	int mNumber2;
} CollisionData;

void loadGameCollisions();
CollisionListData* getPlayerCollisionList();
CollisionListData* getPlayerCollectionCollisionList();
CollisionListData* getBackgroundCollisionList();
CollisionListData* getHurtingCollisionList();
CollisionListData* getCollectableCollisionList();
CollisionListData* getShotCollisionList();
CollisionListData* getHouseCollisionList();

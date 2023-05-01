#include "collision.h"

#include <prism/blitz.h>

static struct {
	CollisionListData* mPlayerList;
	CollisionListData* mPlayerCollectionList;
	CollisionListData* mBackgroundList;
	CollisionListData* mHurtingList;
	CollisionListData* mCollectableList;
	CollisionListData* mShotList;
	CollisionListData* mHouseList;
} gCollision;

void loadGameCollisions()
{
	gCollision.mPlayerList = addCollisionListToHandler();
	gCollision.mPlayerCollectionList = addCollisionListToHandler();
	gCollision.mBackgroundList = addCollisionListToHandler();
	gCollision.mHurtingList = addCollisionListToHandler();
	gCollision.mCollectableList = addCollisionListToHandler();
	gCollision.mShotList = addCollisionListToHandler();
	gCollision.mHouseList = addCollisionListToHandler();
	addCollisionHandlerCheck(gCollision.mPlayerList, gCollision.mBackgroundList);
	addCollisionHandlerCheck(gCollision.mPlayerCollectionList, gCollision.mCollectableList);
	addCollisionHandlerCheck(gCollision.mPlayerList, gCollision.mHurtingList);
	addCollisionHandlerCheck(gCollision.mHurtingList, gCollision.mBackgroundList);
	addCollisionHandlerCheck(gCollision.mCollectableList, gCollision.mBackgroundList);
	addCollisionHandlerCheck(gCollision.mCollectableList, gCollision.mHurtingList);
	addCollisionHandlerCheck(gCollision.mShotList, gCollision.mHurtingList);
	addCollisionHandlerCheck(gCollision.mCollectableList, gCollision.mHouseList);
}

CollisionListData* getPlayerCollisionList()
{
	return gCollision.mPlayerList;
}

CollisionListData* getPlayerCollectionCollisionList()
{
	return gCollision.mPlayerCollectionList;
}

CollisionListData* getBackgroundCollisionList()
{
	return gCollision.mBackgroundList;
}

CollisionListData* getHurtingCollisionList()
{
	return gCollision.mHurtingList;
}

CollisionListData* getCollectableCollisionList()
{
	return gCollision.mCollectableList;
}

CollisionListData* getShotCollisionList()
{
	return gCollision.mShotList;
}

CollisionListData* getHouseCollisionList()
{
	return gCollision.mHouseList;
}

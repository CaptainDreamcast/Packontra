#include "shoothandler.h"

#include "gamescreen.h"
#include "collision.h"
#include "resultscreen.h"

static void shotHitCB(void*, void*);

static struct
{
	GeoRectangle2D shotExistRectangle;

} gShootHandlerData;

struct StampShot
{
	int entityId;
	CollisionData collisionData;
	int shouldBeDeleted = 0;

	int update()
	{
		if (shouldBeDeleted) return 1;
		auto pos = getBlitzEntityPositionReference(entityId);
		if (!checkPointInRectangle(gShootHandlerData.shotExistRectangle, pos->xy()))
		{
			removeBlitzEntity(entityId);
			shouldBeDeleted = 1;
		}
		return 0;
	}
};

class ShootHandler
{
	std::map<int, StampShot> mShots;

public:
	ShootHandler() {
	}
	void update() { 
		if (isResultScreenActive()) return;
		gShootHandlerData.shotExistRectangle = getShotConstraintRectangle();
		updateStampShots(); 
	}

	void updateStampShots()
	{
		std::vector<int> removeList;

		for (auto& shot : mShots)
		{
			if (shot.second.update())
			{
				removeList.push_back(shot.second.entityId);
			}
		}

		for (auto& entry : removeList)
		{
			unloadShot(entry);
		}
	}

	void unloadShot(int id)
	{
		mShots.erase(id);
	}

	void addStampShot(Vector2D basePosition, double angle)
	{
		auto entityId = addBlitzEntity(basePosition.xyz(40));
		StampShot& shot = mShots[entityId];
		shot.entityId = entityId;
		addBlitzMugenAnimationComponent(shot.entityId, getGameSprites(), getGameAnimations(), 40);
		addBlitzPhysicsComponent(shot.entityId);
		Velocity v = vecRotateZ(Vector3D(10, 0, 0), angle);
		addBlitzPhysicsVelocity(shot.entityId, v);

		shot.collisionData.mList = getShotCollisionList();
		shot.collisionData.mNumber = shot.entityId;
		addBlitzCollisionComponent(shot.entityId);
		auto collisionId = addBlitzCollisionRect(shot.entityId, getShotCollisionList(), makeCollisionRect(Vector2D(-2, -2), Vector2D(2, 2)));
		setBlitzCollisionCollisionData(shot.entityId, collisionId, &shot.collisionData);
		addBlitzCollisionCB(shot.entityId, collisionId, shotHitCB, &shot.collisionData);
	}

	void shotHitFunc(void* tCaller, void*)
	{
		CollisionData* myData = (CollisionData*)tCaller;
		assert(mShots.find(myData->mNumber) != mShots.end());
		auto& shot = mShots[myData->mNumber];
		if (shot.shouldBeDeleted) return;
		removeBlitzEntity(myData->mNumber);
		shot.shouldBeDeleted = 1;
	}
};

EXPORT_ACTOR_CLASS(ShootHandler);

void shootStamp(Vector2D basePosition, double angle) {
	gShootHandler->addStampShot(basePosition, angle);
};

static void shotHitCB(void* tCaller, void* tCollisionData)
{
	gShootHandler->shotHitFunc(tCaller, tCollisionData);
}
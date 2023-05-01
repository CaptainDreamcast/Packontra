#include "gamescreen.h"
#include "collision.h"
#include "shoothandler.h"
#include "player.h"
#include "gameoverscreen.h"
#include "resultscreen.h"

static void packageHitCB(void*, void*);
static void enemyHitCB(void*, void*);

#define PACKAGE_Z	40
#define PACKAGE_TEXT_Z	70

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;
	MugenSounds mSounds;

	MugenSounds mDeliverySounds;

	int mWidth;
	int mHeight;

	int tileSizeX;
	int tileSizeY;

	int mHasPlayerStartTile;
	Vector2DI mPlayerStartTile;

	int ticksSpent;
	int packagesDelivered;

} gGameScreenData;

static Vector2D getRealPositionFromTilePosition(const Vector2DI& tilePos)
{
	return Vector2D(0, 240 - gGameScreenData.mHeight) + tilePos * 16.f;
}

static Vector2D getRealPositionFromTilePosition(int x, int y)
{
	return Vector2D(0, 240 - gGameScreenData.mHeight) + Vector2DI(x, y) * 16.f;
}

class GameScreen {
public:

	std::vector<std::vector<int>> mLevelEntityIDs;

	CollisionData mBackgroundCollisionData;

	int overallTimerTicksLeft;
	int overallTimerTextId;

	int bgEntity;

	struct House
	{
		Vector2D position;
		int entityId = -1;

		CollisionData collisionData;
	};
	std::vector<House> mHouses;

	struct Package
	{
		int entityId;
		int shouldBeDeleted = 0;
		CollisionData collisionData;
		int target;

		int hasTimeAssociated;
		int timeMax;
		int timeTextId;

		int cannotBeCollectedTicks = 0;

		Package()
		{
			
		}

		int update()
		{
			if (shouldBeDeleted) {
				shouldBeDeleted--;
				return !shouldBeDeleted;
			};

			if (hasTimeAssociated)
			{
				setMugenTextPosition(timeTextId, ((getBlitzEntityPosition(entityId) - getBlitzCameraHandlerPosition()).xy() + Vector2D(-20, -10)).xyz(PACKAGE_TEXT_Z));
			}

			if (cannotBeCollectedTicks) cannotBeCollectedTicks--;
				
			return 0;
		}

		void unload()
		{
			if (hasTimeAssociated)
			{
				removeMugenText(timeTextId);
			}
			removeBlitzEntity(entityId);
		}
	};
	std::map<int, Package> mPackages;

	struct Enemy
	{
		int entityId;
		CollisionData collisionData;
		int isMovingRight;
		int tileY;
		int health = 0;
		int isDisabled = 0;
		int disableTicks = 0;

		Enemy() {

		}

	};
	std::map<int, Enemy> mEnemies;


	GameScreen() {
		loadGameCollisions();
		loadCollisionData();

		loadFiles();
		loadBG();
		loadTiles();
		generateHouses();
		loadPackageHandling();
		loadTimeLimit();
		loadTutorial();

		instantiateActor(getShootHandler());
		instantiateActor(getPlayer());
		instantiateActor(getResultScreen());

		setBlitzCameraHandlerRange(GeoRectangle2D(0, -gGameScreenData.mHeight + 240, gGameScreenData.mWidth, gGameScreenData.mHeight));

		streamMusicFile("music/game.ogg");
	}

	void loadTutorial()
	{
		generatePackageGeneral(getRealPositionFromTilePosition(5, gGameScreenData.tileSizeY - 1), mHouses.size() - 1);
	}

	void loadTimeLimit()
	{
		overallTimerTicksLeft = 60 * 45;
		overallTimerTextId = addMugenTextMugenStyle("TIME LEFT: 120.00 SECONDS", Vector3D(80, 10, 70), Vector3DI(1, 0, 1));
	}

	void loadPackageHandling()
	{
	}

	void loadCollisionData()
	{
		mBackgroundCollisionData.mList = getBackgroundCollisionList();
	}

	void loadFiles()
	{
		gGameScreenData.mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
		gGameScreenData.mAnimations = loadMugenAnimationFile("game/GAME.air");
		gGameScreenData.mSounds = loadMugenSoundFile("game/GAME.snd");
		gGameScreenData.mDeliverySounds = loadMugenSoundFile("game/DELIVERY.snd");
	}

	void loadBG()
	{
		bgEntity = addBlitzEntity(Vector3D(0, 0, 1));
		addBlitzMugenAnimationComponent(bgEntity, &gGameScreenData.mSprites, &gGameScreenData.mAnimations, 70);
		setBlitzMugenAnimationIndependentOfCamera(bgEntity);
	}

	void loadTiles()
	{
		char path[1024];
		sprintf(path, "game/level.txt");
		Buffer b = fileToBuffer(path);
		BufferPointer p = getBufferPointer(b);

		gGameScreenData.mWidth = readIntegerFromTextStreamBufferPointer(&p) * 16;
		gGameScreenData.mHeight = readIntegerFromTextStreamBufferPointer(&p) * 16;

		auto startPosition = Vector2D(0, 240 - gGameScreenData.mHeight);
		gGameScreenData.tileSizeX = gGameScreenData.mWidth / 16;
		gGameScreenData.tileSizeY = gGameScreenData.mHeight / 16;
		initLevelEntityIds(gGameScreenData.tileSizeX, gGameScreenData.tileSizeY);

		int y, x;
		for (y = 0; y < gGameScreenData.tileSizeY; y++) {
			for (x = 0; x < gGameScreenData.tileSizeX; x++) {
				int index = readIntegerFromTextStreamBufferPointer(&p);
				if (!index) continue;
				if (index == -1) {
					gGameScreenData.mHasPlayerStartTile = 1;
					gGameScreenData.mPlayerStartTile = Vector2DI(x, y);
					continue;
				}
				if (index == -2) {
					addHouse(startPosition + Vector2D(x * 16 + 8, y * 16 + 16));
					continue;
				}

				if (index == -3) {
					addEnemy(startPosition + Vector2D(x * 16 + 8, y * 16 + 8), y);
					continue;
				}

				int animation, horizontalMirror, verticalMirror;
				loadAnimationFromLevelTile(index, &animation, &horizontalMirror, &verticalMirror);
				auto pos = (startPosition + Vector2D((x + horizontalMirror) * 16, (y + verticalMirror) * 16)).xyz(10);
				mLevelEntityIDs[y][x] = addBlitzEntity(pos);
				addBlitzMugenAnimationComponent(mLevelEntityIDs[y][x], &gGameScreenData.mSprites, &gGameScreenData.mAnimations, animation);
				setMugenAnimationFaceDirection(getBlitzMugenAnimationElement(mLevelEntityIDs[y][x]), !horizontalMirror);
				setMugenAnimationVerticalFaceDirection(getBlitzMugenAnimationElement(mLevelEntityIDs[y][x]), !verticalMirror);
				setCollisionFromIndex(mLevelEntityIDs[y][x], pos, animation, horizontalMirror, verticalMirror, x, y, index);
			}
		}

		freeBuffer(b);

		if (!gGameScreenData.mHasPlayerStartTile) {
			logWarning("Unable to find player start tile.");
		}
	}

	void addEnemy(Vector2D pos, int tileY)
	{
		auto listId = mEnemies.size();
		auto id = mEnemies.size();
		auto& enemy = mEnemies[id];
		enemy.entityId = addBlitzEntity(pos.xyz(40));
		addBlitzMugenAnimationComponent(enemy.entityId, getGameSprites(), getGameAnimations(), 30);
		addBlitzPhysicsComponent(enemy.entityId);
		setBlitzPhysicsGravity(enemy.entityId, Vector3D(0, 0.2, 0));

		enemy.collisionData.mList = getHurtingCollisionList();
		enemy.collisionData.mNumber = listId;
		addBlitzCollisionComponent(enemy.entityId);
		auto collisionId = addBlitzCollisionRect(enemy.entityId, getHurtingCollisionList(), makeCollisionRect(Vector2D(-8, -45), Vector2D(8, 0)));
		setBlitzCollisionCollisionData(enemy.entityId, collisionId, &enemy.collisionData);
		addBlitzCollisionCB(enemy.entityId, collisionId, enemyHitCB, &enemy.collisionData);
		setBlitzCollisionSolid(enemy.entityId, collisionId, 1);

		enemy.tileY = tileY;
		enemy.isMovingRight = 1;
	}

	void setCollisionFromIndex(int tEntityID, Position tPos, int tIndex, int tHorizontalMirror, int tVerticalMirror, int x, int y, int tTrueIndex) {
		if (tIndex >= 204) return;

		addBlitzCollisionComponent(tEntityID);

		CollisionListData* collisionList;
		CollisionData* collisionData;
		int collisionID;
		if (tIndex < 20) {
			CollisionRect rect;
			if (tHorizontalMirror && tVerticalMirror) rect = makeCollisionRect(Position2D(-16, -16), Position2D(0, 0));
			if (!tHorizontalMirror && tVerticalMirror) rect = makeCollisionRect(Position2D(0, -16), Position2D(16, 0));
			if (tHorizontalMirror && !tVerticalMirror) rect = makeCollisionRect(Position2D(-16, 0), Position2D(0, 16));
			if (!tHorizontalMirror && !tVerticalMirror) rect = makeCollisionRect(Position2D(0, 0), Position2D(16, 16));
			collisionList = getBackgroundCollisionList();
			collisionData = &mBackgroundCollisionData;
			collisionID = addBlitzCollisionRect(tEntityID, collisionList, rect);
			setBlitzCollisionSolid(tEntityID, collisionID, 0);
		}
		else {
			logErrorFormat("Unimplemented index: %d.", tIndex);
			abortSystem();
			collisionID = -1;
			collisionData = &mBackgroundCollisionData;
		}

		setBlitzCollisionCollisionData(tEntityID, collisionID, collisionData);
	}

	void loadAnimationFromLevelTile(int i, int* oAnimation, int* oHorizontalMirror, int* oVerticalMirror) {
		if (i == 11 || i == 10) {
			*oAnimation = 1;
			*oHorizontalMirror = 0;
			*oVerticalMirror = 0;
		}
	}

	void initLevelEntityIds(int sizeX, int sizeY)
	{
		mLevelEntityIDs = std::vector<std::vector<int>>(sizeY);
		for (int y = 0; y < sizeY; y++)
		{
			mLevelEntityIDs[y] = std::vector<int>(sizeX, -1);
		}
	}

	void addHouse(Vector2D position)
	{
		House house;
		house.position = position;
		house.collisionData.mList = getHouseCollisionList();
		house.collisionData.mNumber = mHouses.size();
		mHouses.push_back(house);
	}

	void generateHouses()
	{
		for (int i = 0; i < int(mHouses.size()); i++)
		{
			auto& house = mHouses[i];
			house.entityId = addBlitzEntity(house.position.xyz(5));
			addBlitzMugenAnimationComponent(house.entityId, &gGameScreenData.mSprites, &gGameScreenData.mAnimations, 20);
			auto rect = makeCollisionRect(Position2D(-16, -60), Position2D(16, 0));
			auto collisionList = getHouseCollisionList();
			auto collisionID = addBlitzCollisionRect(house.entityId, collisionList, rect);
			auto collisionData = &house.collisionData;
			setBlitzCollisionCollisionData(house.entityId, collisionID, collisionData);
		}
	}

	void update() {
		if (isResultScreenActive()) return;
		updateBG();
		updatePackageHandling();
		updateTimeLimit();
		updateEnemies();
		updateGameTicks();
	}

	void updateGameTicks()
	{
		gGameScreenData.ticksSpent++;
	}

	void updateBG()
	{
		auto camPos = getBlitzCameraHandlerPosition();
		auto preRange = getGameCameraRange();
		const auto sz = getScreenSize();
		auto camRange = preRange;
		camRange.mBottomRight = camRange.mBottomRight - Vector2D(sz.x, sz.y);

		auto tx = (camPos.x - camRange.mTopLeft.x) / (camRange.mBottomRight.x - camRange.mTopLeft.x);
		auto ty = (camPos.y - camRange.mTopLeft.y) / (camRange.mBottomRight.y - camRange.mTopLeft.y);

		Vector2D bgDelta = Vector2D(400 - 320, 300 - 240);
		setBlitzEntityPositionX(bgEntity, -bgDelta.x * tx);
		setBlitzEntityPositionY(bgEntity, -bgDelta.y * ty);
	}

	void updateEnemies()
	{
		for (auto& enemyPair : mEnemies)
		{
			auto& enemy = enemyPair.second;
			if (enemy.isDisabled)
			{
				enemy.disableTicks--;
				if (enemy.disableTicks <= 0)
				{
					enemy.isDisabled = 0;
					changeBlitzMugenAnimation(enemy.entityId, 30);
				}
				continue;
			}

			auto pos = getBlitzEntityPosition(enemy.entityId);
			auto enemyTileX = int(pos.x / 16);
			auto tileBelow = mLevelEntityIDs[enemy.tileY + 1][enemyTileX];
			if (isOnLeftTile(enemy.tileY + 1, enemyTileX) && !enemy.isMovingRight && (int(pos.x) % 16 < 8))
			{
				enemy.isMovingRight = 1;
				setBlitzMugenAnimationFaceDirection(enemy.entityId, 0);
			}
			else if (isOnRightTile(enemy.tileY + 1, enemyTileX) && enemy.isMovingRight && (int(pos.x) % 16 > 8))
			{
				enemy.isMovingRight = 0;
				setBlitzMugenAnimationFaceDirection(enemy.entityId, 1);
			}

			if (enemy.isMovingRight)
			{
				addBlitzEntityPositionX(enemy.entityId, 1);
			}
			else
			{
				addBlitzEntityPositionX(enemy.entityId, -1);
			}
		}
	}

	int isOnLeftTile(int y, int x)
	{
		return mLevelEntityIDs[y][x - 1] == -1;
	}

	int isOnRightTile(int y, int x)
	{
		return mLevelEntityIDs[y][x + 1] == -1;
	}


	void updateTimeLimit()
	{
		overallTimerTicksLeft--;
		std::stringstream ss;
		ss << "TIME LEFT: " << (double(overallTimerTicksLeft) / 60.0);
		changeMugenText(overallTimerTextId, ss.str().c_str());

		if (overallTimerTicksLeft == 0)
		{
			showResultScreen();
		}
	}

	void updatePackageHandling()
	{
		updatePackageGeneration();
		updateExistingPackages();
	}

	int packageGenerataionTicks = 0;
	void updatePackageGeneration()
	{
		packageGenerataionTicks--;
		if (packageGenerataionTicks <= 0)
		{
			packageGenerataionTicks += 60;
			if (mPackages.size() <= 0)
			{
				generatePackage();
			}
		}
	}

	void generatePackage()
	{
		Vector2D pos;
		while (true)
		{
			int x = randfromInteger(0, gGameScreenData.tileSizeX - 1);
			int y = randfromInteger(0, gGameScreenData.tileSizeY - 2);
			auto& tile = mLevelEntityIDs[y][x];
			auto& tileBelow = mLevelEntityIDs[y + 1][x];
			if (tileBelow != -1 && tile == -1)
			{
				pos = getRealPositionFromTilePosition(x, y);
				break;
			}
		}

		int target = randfromInteger(0, mHouses.size() - 1);
		generatePackageGeneral(pos, target);
	}

	void generatePackageGeneral(Vector2D pos, int target)
	{
		double targetDistance = vecLength(getBlitzEntityPosition(mHouses[target].entityId).xy() - pos);
		auto entityId = addBlitzEntity(pos.xyz(PACKAGE_Z));
		auto& package = mPackages[entityId];

		package.entityId = entityId;
		addBlitzMugenAnimationComponent(package.entityId, getGameSprites(), getGameAnimations(), 50);
		addBlitzPhysicsComponent(package.entityId);

		package.collisionData.mList = getCollectableCollisionList();
		package.collisionData.mNumber = package.entityId;
		addBlitzCollisionComponent(package.entityId);
		auto collisionId = addBlitzCollisionRect(package.entityId, getCollectableCollisionList(), makeCollisionRect(Vector2D(-5, -10), Vector2D(5, 0)));
		setBlitzCollisionCollisionData(package.entityId, collisionId, &package.collisionData);
		addBlitzCollisionCB(package.entityId, collisionId, packageHitCB, &package.collisionData);
		setBlitzCollisionSolid(package.entityId, collisionId, 1);
		setBlitzPhysicsGravity(package.entityId, Vector3D(0, 0.2, 0));
		setBlitzPhysicsDragFactorOnCollision(package.entityId, Vector3D(0.4, 0, 0));

		package.target = target;
		package.hasTimeAssociated = 1;
		package.timeMax = targetDistanceToTime(targetDistance);
		std::stringstream ss;
		ss << package.timeMax << " SECONDS";
		package.timeTextId = addMugenTextMugenStyle(ss.str().c_str(), (pos + Vector2D(-20, -10)).xyz(PACKAGE_TEXT_Z), Vector3DI(-1, 7, 1));
	}

	int targetDistanceToTime(double targetDistance)
	{
		return targetDistance / 30;
	}

	void updateExistingPackages()
	{
		std::vector<int> removeList;
		for (auto& package : mPackages)
		{
			if (package.second.update())
			{
				removeList.push_back(package.second.entityId);
			}
		}

		for (auto& id : removeList)
		{
			mPackages.erase(id);
		}
	}

	void packageHitFunc(void* tCaller, void* tCollisionData)
	{
		auto myData = (CollisionData*)tCaller;
		auto otherData = (CollisionData*)tCollisionData;
	
		auto& package = mPackages[myData->mNumber];
		if (package.shouldBeDeleted) return;

		if (otherData->mList == getPlayerCollectionCollisionList() && !package.cannotBeCollectedTicks && getBlitzMugenAnimationVisibility(myData->mNumber))
		{
			if (package.hasTimeAssociated)
			{
				removeMugenText(package.timeTextId);
				overallTimerTicksLeft += package.timeMax;
				package.hasTimeAssociated = 0;
			}
			setBlitzMugenAnimationVisibility(myData->mNumber, 0);
		}
		else if (otherData->mList == getHouseCollisionList() && otherData->mNumber == package.target)
		{
			overallTimerTicksLeft += getPlayerTimeLeft() * 60;
			removePackageFromPlayer();
			tryPlayMugenSound(&gGameScreenData.mDeliverySounds, 1, std::min(package.target, 8));
			gGameScreenData.packagesDelivered++;
			if (gGameScreenData.packagesDelivered == 10)
			{
				showResultScreen();
			}
			changeBlitzMugenAnimation(mHouses[package.target].entityId, 21);
			package.shouldBeDeleted = 2;
			package.unload();
		}
	}

	void showResultScreen()
	{
		startResultScreen();
	}

	void enemyHitFunc(void* tCaller, void* tCollisionData)
	{
		auto myData = (CollisionData*)tCaller;
		auto otherData = (CollisionData*)tCollisionData;

		assert(mEnemies.find(myData->mNumber) != mEnemies.end());
		auto& enemy = mEnemies[myData->mNumber];
		if (otherData->mList == getCollectableCollisionList())
		{
			tryPlayMugenSound(getGameSounds(), 2, 2);
			enemy.health = 0;
			setEnemyDisabled(enemy);
		}
		else if (otherData->mList == getShotCollisionList())
		{
			tryPlayMugenSound(getGameSounds(), 2, 2);
			enemy.health--;
			if (enemy.health <= 0)
			{
				setEnemyDisabled(enemy);
			}
		}
	}

	void setEnemyDisabled(Enemy& enemy)
	{
		tryPlayMugenSound(getGameSounds(), 2, 3);
		enemy.isDisabled = 1;
		enemy.disableTicks = 300;
		changeBlitzMugenAnimation(enemy.entityId, 31);
	}

	void shootPackage(int id, Vector2D pos, double angle)
	{
		setBlitzEntityPosition(id, pos.xyz(PACKAGE_Z));
		Acceleration acc = vecRotateZ(Vector3D(3, 0, 0), angle);
		setBlitzMugenAnimationVisibility(id, 1);
		addBlitzPhysicsImpulse(id, acc);
		mPackages[id].cannotBeCollectedTicks = 60;
	}

	int getPackageMaxTime(int id)
	{
		return mPackages[id].timeMax;
	}

	void removePackage(int id)
	{
		mPackages[id].shouldBeDeleted = 2;
		mPackages[id].unload();

	}

	Vector2D getPackageTargetPosition(int id)
	{
		auto& package = mPackages[id];
		return getBlitzEntityPosition(mHouses[package.target].entityId).xy();
	}

	int isValidPackage(int id)
	{
		assert(mPackages.find(id) != mPackages.end());
		return !mPackages[id].shouldBeDeleted;
	}

	int canPackageBeSelected(int id)
	{
		assert(mPackages.find(id) != mPackages.end());
		return !mPackages[id].cannotBeCollectedTicks;
	}
};

EXPORT_SCREEN_CLASS(GameScreen);

MugenSpriteFile* getGameSprites()
{
	return &gGameScreenData.mSprites;
}
MugenAnimations* getGameAnimations()
{
	return &gGameScreenData.mAnimations;
}
MugenSounds* getGameSounds()
{
	return &gGameScreenData.mSounds;
}

Vector2D getPlayerStartPosition()
{
	return getRealPositionFromTilePosition(gGameScreenData.mPlayerStartTile);
}

static void packageHitCB(void* tCaller, void* tCollisionData)
{
	gGameScreen->packageHitFunc(tCaller, tCollisionData);
}

static void enemyHitCB(void* tCaller, void* tCollisionData)
{
	gGameScreen->enemyHitFunc(tCaller, tCollisionData);
}

void shootPackage(int id, Vector2D pos, double angle)
{
	gGameScreen->shootPackage(id, pos, angle);
}

int getPackageMaxTime(int id)
{
	return gGameScreen->getPackageMaxTime(id);
}

void removePackage(int id)
{
	gGameScreen->removePackage(id);
}

Vector2D getPackageTargetPosition(int id)
{
	return gGameScreen->getPackageTargetPosition(id);
}

int isValidPackage(int id)
{
	return gGameScreen->isValidPackage(id);
}

int canPackageBeSelected(int id)
{
	return gGameScreen->canPackageBeSelected(id);
}

GeoRectangle2D getPlayerConstraintRectangle()
{
	return GeoRectangle2D(8, -INF, gGameScreenData.mWidth - 16, INF);
}

GeoRectangle2D getShotConstraintRectangle()
{
	return GeoRectangle2D(0, 240 - gGameScreenData.mHeight, gGameScreenData.mWidth - 16, gGameScreenData.mHeight);
}

GeoRectangle2D getGameCameraRange()
{
	return GeoRectangle2D(0, -gGameScreenData.mHeight + 240, gGameScreenData.mWidth, gGameScreenData.mHeight);
}

void resetGameState()
{
	gGameScreenData.ticksSpent = 0;
	gGameScreenData.packagesDelivered = 0;
}

int getGameTicksSpent()
{
	return gGameScreenData.ticksSpent;
}

int getPackagesCollectedCount()
{
	return gGameScreenData.packagesDelivered;
}
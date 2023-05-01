#include "player.h"

#include "gamescreen.h"
#include "collision.h"
#include "shoothandler.h"
#include "resultscreen.h"

static void playerHitCB(void* tCaller, void* tCollisionData);
static void playerCollectedCB(void* tCaller, void* tCollisionData);

class Player
{
public:
	int entityId;

	CollisionData collisionData;
	int collisionId;

	CollisionData collectCollisionData;
	int collectCollisionId;

	int hasPackage = 0;
	int packageId = -1;
	int hasPackageSelected = 0;

	double shotAngle = 0;

	int hasTimer = 0;
	int timerTextId;
	int timeLeftPackageTicks = 0;
	int arrowEntityId;

	int packageHolderId;

	Player() {
		entityId = addBlitzEntity(getPlayerStartPosition().xyz(40));
		addBlitzMugenAnimationComponent(entityId, getGameSprites(), getGameAnimations(), 10);
		addBlitzPhysicsComponent(entityId);
		setBlitzPhysicsGravity(entityId, Vector3D(0, 0.2, 0));
		//setBlitzPhysicsDragFactorOnCollision(entityId, Vector3D(0.2, 0, 0));


		collisionData.mList = getPlayerCollisionList();
		addBlitzCollisionComponent(entityId);
		collisionId = addBlitzCollisionRect(entityId, getPlayerCollisionList(), makeCollisionRect(Vector2D(-8, -16), Vector2D(8, 0)));
		setBlitzCollisionSolid(entityId, collisionId, 1);
		setBlitzCollisionCollisionData(entityId, collisionId, &collisionData);
		addBlitzCollisionCB(entityId, collisionId, playerHitCB, NULL);

		collectCollisionData.mList = getPlayerCollectionCollisionList();
		collectCollisionId = addBlitzCollisionRect(entityId, getPlayerCollectionCollisionList(), makeCollisionRect(Vector2D(-12, -55), Vector2D(12, 0)));
		setBlitzCollisionCollisionData(entityId, collectCollisionId, &collectCollisionData);
		addBlitzCollisionCB(entityId, collectCollisionId, playerCollectedCB, NULL);

		timerTextId = addMugenTextMugenStyle("30 seconds", Vector3D(160, 10, 70), Vector3DI(-1, 7, 1));
		setMugenTextVisibility(timerTextId, 0);

		arrowEntityId = addBlitzEntity(getPlayerStartPosition().xyz(40));
		addBlitzMugenAnimationComponent(arrowEntityId, getGameSprites(), getGameAnimations(), 60);
		setBlitzMugenAnimationVisibility(arrowEntityId, false);

		packageHolderId = addBlitzEntity(Vector2D(315, 235).xyz(70));
		addBlitzMugenAnimationComponent(packageHolderId, getGameSprites(), getGameAnimations(), 200);
		setBlitzMugenAnimationIndependentOfCamera(packageHolderId);

		hasTimer = 0;
	}
	void update() {
		if (isResultScreenActive()) return;
		updateShootDirection();
		updatePlayerMovement(4, 4);
		updateLanding();
		updatePlayerGravity();
		updateCollisionSolidity();
		updateConstraint();
		updatePackageHolder();

		updateTimer();
		updateCamera();
	}

	void updatePackageHolder()
	{
		changeBlitzMugenAnimation(packageHolderId, 200 + hasPackage);
	}

	void updateCamera()
	{
		setBlitzCameraPositionBasedOnCenterPoint(getBlitzEntityPosition(entityId));
	}

	void updateTimer()
	{
		if (!hasTimer)
		{
			return;
		}

		timeLeftPackageTicks--;
		if (timeLeftPackageTicks < 0)
		{
			hasPackage = 0;
			hasPackageSelected = 0;
			removePackage(packageId);
			setMugenTextVisibility(timerTextId, 0);
			setBlitzMugenAnimationVisibility(arrowEntityId, false);
			hasTimer = 0;

			return;
		}
		else
		{
			std::stringstream ss;
			ss << "TIME LEFT: " << (timeLeftPackageTicks / 60.0);
			changeMugenText(timerTextId, ss.str().c_str());

			auto pos = getBlitzEntityPosition(entityId);
			setBlitzEntityPosition(arrowEntityId, pos + Vector2D(0, -10));
			pos =  pos - getBlitzCameraHandlerPosition();
			setMugenTextPosition(timerTextId, pos + Vector2D(-20, -27));

			auto targetPos = getPackageTargetPosition(packageId);
			auto angle = getAngleFromDirection((targetPos - getBlitzEntityPosition(entityId).xy()).xyz(0));
			setBlitzMugenAnimationAngle(arrowEntityId, angle + M_PI);
		}
	}

	int getCurrentAnimationBase()
	{
		return 10;
	}

	int shootDirection = 0;
	void updateShootDirection()
	{
		if (hasPressedLeft() && hasPressedUp()) { shotAngle = degreesToRadians(225); shootDirection = 1; }
		else if (hasPressedLeft() && hasPressedDown()) {
			shotAngle = degreesToRadians(135); shootDirection = 2;
		}
		else if (hasPressedRight() && hasPressedUp()) {
			shotAngle = degreesToRadians(315); shootDirection = 1;
		}
		else if (hasPressedRight() && hasPressedDown()) {
			shotAngle = degreesToRadians(45); shootDirection = 2;
		}
		else if (hasPressedRight()) {
			shotAngle = degreesToRadians(0); shootDirection = 0;
		}
		else if (hasPressedLeft()) {
			shotAngle = degreesToRadians(180); shootDirection = 0;
		}
		else if (hasPressedDown()) {
			shotAngle = degreesToRadians(90); shootDirection = 3;
		}
		else if (hasPressedUp()) {
			shotAngle = degreesToRadians(270); shootDirection = 4;
		}
	}

	void updatePlayerMovement(double tSpeed, double tJumpForce) {

		if (hasPressedLeft()) {
			if (getBlitzMugenAnimationAnimationNumber(entityId) == getCurrentAnimationBase() + getAnimationOffsetFromShotDirection()) changeBlitzMugenAnimation(entityId, getCurrentAnimationBase() + 1);
			setBlitzMugenAnimationFaceDirection(entityId, 0);
			setBlitzPhysicsVelocityX(entityId, -tSpeed);
		}
		if (hasPressedRight()) {
			if (getBlitzMugenAnimationAnimationNumber(entityId) == getCurrentAnimationBase() + getAnimationOffsetFromShotDirection()) changeBlitzMugenAnimation(entityId, getCurrentAnimationBase() + 1);
			setBlitzMugenAnimationFaceDirection(entityId, 1);
			setBlitzPhysicsVelocityX(entityId, tSpeed);
		}

		if (!hasPressedLeft() && !hasPressedRight()) {
			if (getBlitzMugenAnimationAnimationNumber(entityId) == getCurrentAnimationBase() + 1) changeBlitzMugenAnimation(entityId, getCurrentAnimationBase() + getAnimationOffsetFromShotDirection());
			setBlitzPhysicsVelocityX(entityId, 0);
		}

		if (getBlitzMugenAnimationAnimationNumber(entityId) != getCurrentAnimationBase() + 1 && getBlitzMugenAnimationAnimationNumber(entityId) != getCurrentAnimationBase() + 2)
		{
			changeBlitzMugenAnimationIfDifferent(entityId, getCurrentAnimationBase() + getAnimationOffsetFromShotDirection());
		}

		if (hasPressedAFlank() && getBlitzMugenAnimationAnimationNumber(entityId) != getCurrentAnimationBase() + 2 && tJumpForce) {
			//tryPlayMugenSound(getGameSounds(), 1, 1);
			changeBlitzMugenAnimation(entityId, getCurrentAnimationBase() + 2);
			setBlitzPhysicsVelocityY(entityId, -tJumpForce);
		}

		if (hasPressedBFlank())
		{
			playerShootStamp();
		}
		else if (hasPressedYFlank())
		{
			playerShootPackage();
		}
		else if (hasPressedXFlank())
		{
			honk();
		}
	}

	void honk()
	{
		int id = randfromInteger(0, 3);
		tryPlayMugenSound(getGameSounds(), 3, id);
	}

	void updateLanding() {
		if (getBlitzMugenAnimationAnimationNumber(entityId) == getCurrentAnimationBase() + 2 && hasBlitzCollidedBottom(entityId) && getBlitzPhysicsVelocityY(entityId) >= 0) {
			//tryPlayMugenSound(&gPlayer.mSounds, 1, 1);
			changeBlitzMugenAnimation(entityId, getCurrentAnimationBase() + getAnimationOffsetFromShotDirection());
			tryPlayMugenSound(getGameSounds(), 2, 1);
		}
	}

	void updatePlayerGravity() {
		Position vel = getBlitzPhysicsVelocity(entityId);
		if (vel.y > 0) setBlitzPhysicsGravity(entityId, Vector3D(0, 0.2, 0));
		else setBlitzPhysicsGravity(entityId, Vector3D(0, 0.1, 0));
	}

	void updateCollisionSolidity() {
		Position vel = getBlitzPhysicsVelocity(entityId);
		if (vel.y > 0) 	setBlitzCollisionSolid(entityId, collisionId, 1);
		else setBlitzCollisionUnsolid(entityId, collisionId);
	}

	void updateConstraint() {
		auto rect = getPlayerConstraintRectangle();
		auto pos = getBlitzEntityPositionReference(entityId);
		pos->x = clamp(pos->x, rect.mTopLeft.x, rect.mBottomRight.x);
	}

	void switchShotIfPossible()
	{
		if (hasPackage)
		{
			hasPackageSelected ^= 1;
		}
	}

	void playerShootPackage()
	{
		auto shotOffset = getShotOffset();
		if (hasPackage)
		{
			hasPackage = 0;
			hasPackageSelected = 0;
			shootPackage(packageId, getBlitzEntityPosition(entityId).xy() + shotOffset, shotAngle);
		}
	}

	Vector2D getShotOffset()
	{
		Vector2D baseVector;
		if (shootDirection == 0)
		{
			baseVector = Vector2D(13, -17);
		}
		else if (shootDirection == 1)
		{
			baseVector = Vector2D(10, -29);
		}
		else if (shootDirection == 2)
		{
			baseVector = Vector2D(10, -11);
		}
		else if (shootDirection == 3)
		{
			baseVector = Vector2D(3, -9);
		}
		else if (shootDirection == 4)
		{
			baseVector = Vector2D(3, -34);
		}
		if (!getBlitzMugenAnimationIsFacingRight(entityId))
		{
			baseVector.x *= -1;
		}
		return baseVector;
	}

	int getAnimationOffsetFromShotDirection()
	{
		if (shootDirection == 1) return 101;
		if (shootDirection == 2) return 102;
		if (shootDirection == 4) return 103;
		if (shootDirection == 3) return 104;
		return 0;
	}
	void playerShootStamp()
	{
		tryPlayMugenSoundAdvanced(getGameSounds(), 2, 0, 0.05);
		auto shotOffset = getShotOffset();
		shootStamp(getBlitzEntityPosition(entityId).xy() + shotOffset, shotAngle);
	}


	void playerHitFunc(void*, void* tCollisionData) {
		CollisionData* collisionData = (CollisionData*)tCollisionData;
		auto listID = collisionData->mList;
		if (listID == getHurtingCollisionList()) {
			//die();
		}
	}

	void playerCollectedFunc(void*, void* tCollisionData) {
		//tryPlayMugenSound(&gPlayer.mSounds, 1, 0);

		CollisionData* collisionData = (CollisionData*)tCollisionData;
		auto listID = collisionData->mList;
		if (listID == getCollectableCollisionList() && isValidPackage(collisionData->mNumber) && canPackageBeSelected(collisionData->mNumber) && !hasPackage) {
			collectPackage(collisionData->mNumber);
		}
	}

	void collectPackage(int id)
	{
		packageId = id;
		hasPackage = 1;
		if (!hasTimer)
		{
			hasTimer = 1;
			setMugenTextVisibility(timerTextId, 1);
			setBlitzMugenAnimationVisibility(arrowEntityId, true);
			timeLeftPackageTicks = getPackageMaxTime(id) * 60;
		}
	}

	void removePackageFromPlayer()
	{
		hasTimer = 0;
		setMugenTextVisibility(timerTextId, 0);
		setBlitzMugenAnimationVisibility(arrowEntityId, 0);
	}

	double getPlayerTimeLeft()
	{
		return timeLeftPackageTicks / 60.0;
	}
};

EXPORT_ACTOR_CLASS(Player);

static void playerHitCB(void* tCaller, void* tCollisionData)
{
	gPlayer->playerHitFunc(tCaller, tCollisionData);
}

static void playerCollectedCB(void* tCaller, void* tCollisionData)
{
	gPlayer->playerCollectedFunc(tCaller, tCollisionData);
}

void removePackageFromPlayer()
{
	gPlayer->removePackageFromPlayer();
}

double getPlayerTimeLeft()
{
	return gPlayer->getPlayerTimeLeft();
}
/////////////////////////////////////////
//
//             OpenLieroX
//
// code under LGPL, based on JasonBs work,
// enhanced by Dark Charlie and Albert Zeyer
//
//
/////////////////////////////////////////


// Projectile Class
// Created 11/2/02
// Jason Boettcher


#include "LieroX.h"
#include "CGameScript.h" // for all PRJ_* and PJ_* constants only
#include "GfxPrimitives.h"
#include "CProjectile.h"
#include "Protocol.h"
#include "CWorm.h"
#include "Entity.h"
#include "MathLib.h"
#include "CClient.h"
#include "ProfileSystem.h"


void CProjectile::setUnused() {
	bUsed = false;

	onInvalidation.occurred( EventData(this) );
}

///////////////////
// Spawn the projectile
// this function is called by CClient::SpawnProjectile()
void CProjectile::Spawn(proj_t *_proj, CVec _pos, CVec _vel, int _rot, int _owner, int _random, float time, float ignoreWormCollBeforeTime)
{
	// Safety (it is used for indexing later)
	if (_owner >= MAX_WORMS || _owner < 0)  {
		printf("WARNING: bad owner ID in CProjectile::Spawn");
		return;
	}

	tProjInfo = _proj;
	fLife = 0;
	fExtra = 0;
	vOldPos = _pos;
	vPosition = _pos;
	vVelocity = _vel;
	fRotation = (float)_rot;
	iOwner = _owner;
	bUsed = true;
	fLastTrailProj = -99999;
	iRandom = _random;
    iFrameX = 0;
	fIgnoreWormCollBeforeTime = ignoreWormCollBeforeTime;

    fTimeVarRandom = GetFixedRandomNum(iRandom);
	fLastSimulationTime = time;
	fSpawnTime = time;

	fSpeed = _vel.GetLength();
	CalculateCheckSteps();

	fFrame = 0;
	bFrameDelta = true;

	firstbounce = true;

	// Choose a colour
	if(tProjInfo->Type == PRJ_PIXEL) {
		int c = GetRandomInt(tProjInfo->NumColours-1);
		if(c == 0)
			iColour = MakeColour(tProjInfo->Colour1[0], tProjInfo->Colour1[1], tProjInfo->Colour1[2]);
		else if(c==1)
			iColour = MakeColour(tProjInfo->Colour2[0], tProjInfo->Colour2[1], tProjInfo->Colour2[2]);

		iColSize = 1;
	} else
		iColSize = 2;

	fGravity = 100.0f; // Default
	if (tProjInfo->UseCustomGravity)
		fGravity = (float)tProjInfo->Gravity;

	fWallshootTime = 0.01f + GetRandomNum() / 1000; // Support wallshooting - ignore collisions before this time

	bChangesSpeed = ((int)fGravity == 0) && ((int)tProjInfo->Dampening == 1)
		&& (tProjInfo->Type != PJ_BOUNCE || (int)tProjInfo->Hit_BounceCoeff == 1);  // Changes speed on bounce

    // Events
    bExplode = false;
    bTouched = false;
}


///////////////////
// Gets a random float from a special list
// TODO: how does this belong to projectiles? move it perhaps out here
float CProjectile::getRandomFloat(void)
{
	float r = GetFixedRandomNum(iRandom++);

	iRandom %= 255;

	return r;
}


int FinalWormCollisionCheck(CProjectile* proj, const CVec& vFrameOldPos, const CVec& vFrameOldVel, CWorm* worms, float dt, float* enddt, int curResult) {
	// do we get any worm?
	if(proj->GetProjInfo()->PlyHit_Type != PJ_NOTHING) {
		CVec dif = proj->GetPosition() - vFrameOldPos;
		float len = NormalizeVector( &dif );

		// the worm has a size of 4*4 in ProjWormColl, so it's save to check every second pixel here
		for (float p = 0.0f; p <= len; p += 2.0f) {
			CVec curpos = vFrameOldPos + dif * p;

			int ret = proj->ProjWormColl(curpos, worms);
			if (ret >= 0)  {
				if (enddt) {
					if (len != 0)
						*enddt = dt * p / len;
					else
						*enddt = dt;
				}
				proj->setNewPosition( curpos ); // save the new position at the first collision
				proj->setNewVel( vFrameOldVel ); // don't get faster
				return ret;
			}
		}
	}

	return curResult;
}

//////////////////////
// Pre-calculates the check steps for collisions
void CProjectile::CalculateCheckSteps()
{
	MIN_CHECKSTEP = 4;
	MAX_CHECKSTEP = 6;
	AVG_CHECKSTEP = 4;

	iCheckSpeedLen = (int)vVelocity.GetLength2();
	if (iCheckSpeedLen < 14000)  {
		MIN_CHECKSTEP = 0;
		MAX_CHECKSTEP = 3;
		AVG_CHECKSTEP = 2;
	} else if (iCheckSpeedLen < 75000)  {
		MIN_CHECKSTEP = 0;
		MAX_CHECKSTEP = 4;
		AVG_CHECKSTEP = 2;
	} else if (iCheckSpeedLen < 250000)  {
		MIN_CHECKSTEP = 1;
		MAX_CHECKSTEP = 5;
		AVG_CHECKSTEP = 2;

	// HINT: add or substract some small random number for high speeds, it behaves more like original LX
	} else {
		int rnd = GetRandomInt(2)*SIGN(GetRandomNum());
		MIN_CHECKSTEP = 6;
		if (tProjInfo->Hit_Type == PJ_BOUNCE)  { // HINT: this avoids fast bouncing projectiles to stay in a wall too often (for example zimm)
			MAX_CHECKSTEP = 2;
			AVG_CHECKSTEP = 2;
		} else {
			MAX_CHECKSTEP = 9 + rnd;
			AVG_CHECKSTEP = 6 + rnd;
		}
	}

	MAX_CHECKSTEP2 = MAX_CHECKSTEP * MAX_CHECKSTEP;
	MIN_CHECKSTEP2 = MIN_CHECKSTEP * MIN_CHECKSTEP;
}


///////////////////////
// Checks for collision with the level border
bool CProjectile::MapBoundsCollision(int px, int py, CMap *map)
{
	CollisionSide = 0;

	if (px - iColSize < 0)
		CollisionSide |= COL_LEFT;

	if (px + iColSize >= (int)map->GetWidth())
		CollisionSide |= COL_RIGHT;

	if (py - iColSize < 0)
		CollisionSide |= COL_TOP;

	if (py + iColSize >= (int)map->GetHeight())
		CollisionSide |= COL_BOTTOM;

	return CollisionSide != 0;
}

////////////////////////////
// Checks for collision with the terrain
CProjectile::ColInfo CProjectile::TerrainCollision(int px, int py, CMap *map)
{
	int xend = px + iColSize;
	int yend = py + iColSize;

	ColInfo res = { 0, 0, 0, 0, false, false };

	// If the current cell is empty, don't check for the collision
	{
	const int gf1 = (py - iColSize) / map->getGridHeight() * map->getGridCols() + (px - iColSize) / map->getGridWidth();
	const int gf2 = (py - iColSize) / map->getGridHeight() * map->getGridCols() + (xend) / map->getGridWidth();
	const int gf3 = (yend) / map->getGridHeight() * map->getGridCols() + (px - iColSize) / map->getGridWidth();
	const int gf4 = (yend) / map->getGridHeight() * map->getGridCols() + (xend) / map->getGridWidth();
	const uchar *pf = map->getAbsoluteGridFlags();
	if ((pf[gf1] | pf[gf2] | pf[gf3] | pf[gf4]) == PX_EMPTY)
		return res;
	}

	// Check for the collision
	for(int y = py - iColSize; y <= yend; ++y) {

		uchar *pf = map->GetPixelFlags() + y * map->GetWidth() + px - iColSize;

		for(int x = px - iColSize; x <= xend; ++x) {

			// Solid pixel
			if(*pf & (PX_DIRT|PX_ROCK)) {
				if (y < py)
					++res.top;
				else if (y > py)
					++res.bottom;
				if (x < px)
					++res.left;
				else if (x > px)
					++res.right;

				res.onlyDirt = (res.onlyDirt && (*pf & PX_DIRT));
				res.collided = true;
			}

			pf++;
		}
	}

	return res;
}

////////////////////////
// Handle the terrain collsion (helper function)
void CProjectile::HandleCollision(const CProjectile::ColInfo &c, const CVec& oldpos, const CVec& oldvel, float dt)
{

	if(tProjInfo->Hit_Type == PJ_EXPLODE && c.onlyDirt) {
		// HINT: don't reset vPosition here, because we want
		//		the explosion near (inside) the object
		//		this behavior is the same as in original LX
		return;
	}

	bool bounce = false;

	// Bit of a hack
	switch (tProjInfo->Hit_Type)  {
	case PJ_BOUNCE:
		// HINT: don't reset vPosition here; it will be reset,
		//		depending on the collisionside
		bounce = true;
		break;
	case PJ_NOTHING:  // PJ_NOTHING projectiles go through walls (but a bit slower)
		vPosition -= (vVelocity * dt) * 0.5f;				// Note: the speed in walls could be moddable
		vOldPos = vPosition;
		break;
	default:
		vPosition = vOldPos;
		vVelocity = oldvel;
		return;
	}

	int vx = (int)vVelocity.x;
	int vy = (int)vVelocity.y;

	// Find the collision side
	if ((c.left > c.right || c.left > 2) && c.left > 1 && vx < 0) {
		if(bounce)
			vPosition.x = oldpos.x;
		CollisionSide |= COL_LEFT;
	}

	if ((c.right > c.left || c.right > 2) && c.right > 1 && vx > 0) {
		if(bounce)
			vPosition.x = oldpos.x;
		CollisionSide |= COL_RIGHT;
	}

	if (c.top > 1 && vy < 0) {
		if(bounce)
			vPosition.y = oldpos.y;
		CollisionSide |= COL_TOP;
	}

	if (c.bottom > 1 && vy > 0) {
		if(bounce)
			vPosition.y = oldpos.y;
		CollisionSide |= COL_BOTTOM;
	}

	// If the velocity is too low, just stop me
	if (abs(vx) < 2)
		vVelocity.x = 0;
	if (abs(vy) < 2)
		vVelocity.y = 0;
}

///////////////////
// Check for a collision, updates velocity and position
// Returns:
// -1 if some collision
// -1000 if none
// >=0 is collision with worm (the return-value is the ID)
// TODO: we need one single CheckCollision which is used everywhere in the code
// atm we have 2 CProj::CC, Map:CC and ProjWormColl and fastTraceLine
// we should complete the function in CMap.cpp in a general way by using fastTraceLine
// also dt shouldn't be a parameter, you should specify a start- and an endpoint
// (for example CWorm_AI also uses this to check some possible cases)
int CProjectile::CheckCollision(float dt, CMap *map, CWorm* worms, float* enddt)
{
	static const int NONE_COL_RET = -1000;
	static const int SOME_COL_RET = -1;

	// Check if we need to recalculate the checksteps (projectile changed its velocity too much)
	if (bChangesSpeed)  {
		int len = (int)vVelocity.GetLength2();
		if (abs(len - iCheckSpeedLen) > 50000)
			CalculateCheckSteps();
	}

	CVec vOldVel = vVelocity;
	CVec newvel = vVelocity;

	// Gravity
	newvel.y += fGravity * dt;

	// Dampening
	// HINT: as this function is always called with fixed dt, we can do it this way
	newvel *= tProjInfo->Dampening;

	float checkstep = newvel.GetLength2(); // |v|^2
	if ((int)(checkstep * dt * dt) > MAX_CHECKSTEP2) { // |dp|^2=|v*dt|^2
		// calc new dt, so that we have |v*dt|=AVG_CHECKSTEP
		// checkstep is new dt
		checkstep = (float)AVG_CHECKSTEP / sqrt(checkstep);
		checkstep = MAX(checkstep, 0.001f);

		// In some bad cases (float accurance problems mainly),
		// it's possible that checkstep >= dt .
		// If we would not check this case, we get in an infinie
		// recursive loop.
		// Therefore if this is the case, we don't do multiple checksteps.
		if(checkstep < dt) {
			for(float time = 0; time < dt; time += checkstep) {
				int ret = CheckCollision((time + checkstep > dt) ? dt - time : checkstep, map,worms,enddt);
				if(ret >= -1) {
					if(enddt) *enddt += time;
					return ret;
				}
			}

			if(enddt) *enddt = dt;
			return NONE_COL_RET;
		}
	}

	vVelocity = newvel;
	if(enddt) *enddt = dt;
	CVec vFrameOldPos = vPosition;
	vPosition += vVelocity * dt;

	// if distance is to short to last check, just return here without a check
	if ((int)(vOldPos - vPosition).GetLength2() < MIN_CHECKSTEP2) {
/*		printf("pos dif = %f , ", (vOldPos - vPosition).GetLength());
		printf("len = %f , ", sqrt(len));
		printf("vel = %f , ", vVelocity.GetLength());
		printf("mincheckstep = %i\n", MIN_CHECKSTEP);	*/
		return FinalWormCollisionCheck(this, vFrameOldPos, vOldVel, worms, dt, enddt, NONE_COL_RET);
	}

	int px = (int)(vPosition.x);
	int py = (int)(vPosition.y);

	// Hit edges
	if (MapBoundsCollision(px, py, map))  {
		vPosition = vOldPos;
		vVelocity = vOldVel;

		return FinalWormCollisionCheck(this, vFrameOldPos, vOldVel, worms, dt, enddt, SOME_COL_RET);
	}

	// Make wallshooting possible
	// NOTE: wallshooting is a bug in old LX physics that many players got used to
	if (tLX->fCurTime - fSpawnTime <= fWallshootTime)
		return FinalWormCollisionCheck(this, vFrameOldPos, vOldVel, worms, dt, enddt, NONE_COL_RET);

	// Check collision with the terrain
	ColInfo c = TerrainCollision(px, py, map);

	// Check for a collision
	if(c.collided) {
		HandleCollision(c, vFrameOldPos, vOldVel, dt);

		return FinalWormCollisionCheck(this, vFrameOldPos, vOldVel, worms, dt, enddt, SOME_COL_RET);
	}

	// the move was safe, save the position
	vOldPos = vPosition;

	return FinalWormCollisionCheck(this, vFrameOldPos, vOldVel, worms, dt, enddt, NONE_COL_RET);
}

///////////////////
// Check for a collision (static version; doesnt do anything else then checking)
// Returns true if there was a collision, otherwise false is returned
int CProjectile::CheckCollision(proj_t* tProjInfo, float dt, CMap *map, CVec pos, CVec vel)
{
	// Check if it hit the terrain
	int mw = map->GetWidth();
	int mh = map->GetHeight();
	int w,h;

	if(tProjInfo->Type == PRJ_PIXEL)
		w=h=1;
	else
		w=h=2;

	float maxspeed2 = (float)(4*w*w+4*w+1); // (2w+1)^2
	if( (vel*dt).GetLength2() > maxspeed2) {
		dt *= 0.5f;

		if(CheckCollision(tProjInfo,dt,map,pos,vel))
			return true;

		pos += vel*dt;

		return CheckCollision(tProjInfo,dt,map,pos,vel);
	}

	pos += vel*dt;

	int px = (int)pos.x;
	int py = (int)pos.y;

	// Hit edges
	if(px-w<0 || py-h<0 || px+w>=mw || py+h>=mh)
		return true;

	const uchar* gridflags = map->getAbsoluteGridFlags();
	int grid_w = map->getGridWidth();
	int grid_h = map->getGridHeight();
	int grid_cols = map->getGridCols();
	if(grid_w < 2*w+1 || grid_h < 2*h+1 // this ensures, that this check is safe
	|| gridflags[((py-h)/grid_h)*grid_cols + (px-w)/grid_w] & (PX_ROCK|PX_DIRT)
	|| gridflags[((py+h)/grid_h)*grid_cols + (px-w)/grid_w] & (PX_ROCK|PX_DIRT)
	|| gridflags[((py-h)/grid_h)*grid_cols + (px+w)/grid_w] & (PX_ROCK|PX_DIRT)
	|| gridflags[((py+h)/grid_h)*grid_cols + (px+w)/grid_w] & (PX_ROCK|PX_DIRT))
	for(int y=py-h;y<=py+h;y++) {

		uchar *pf = map->GetPixelFlags() + y*mw + px-w;

		for(int x=px-w;x<=px+w;x++) {


			if(!(*pf & PX_EMPTY))
				return true;

			pf++;
		}
	}


	// No collision
	return false;
}

///////////////////
// Draw the projectile
void CProjectile::Draw(SDL_Surface * bmpDest, CViewport *view)
{
	int wx = view->GetWorldX();
	int wy = view->GetWorldY();
	int l = view->GetLeft();
	int t = view->GetTop();
	float framestep;

	int x=((int)vPosition.x-wx)*2+l;
	int y=((int)vPosition.y-wy)*2+t;

	// Clipping on the viewport
	if((x<l || x>l+view->GetVirtW()))
		return;
	if((y<t || y>t+view->GetVirtH()))
		return;

    switch (tProjInfo->Type) {
	case PRJ_PIXEL:
		DrawRectFill2x2(bmpDest, x - 1, y - 1,iColour);
        return;

	case PRJ_IMAGE:  {

		if(tProjInfo->bmpImage == NULL)
			return;

		// Spinning projectile only when moving
		if(tProjInfo->Rotating && (vVelocity.x != 0 || vVelocity.y != 0))
			framestep = fRotation / (float)tProjInfo->RotIncrement;
		else
			framestep = 0;

		// Directed in the direction the projectile is travelling
		if(tProjInfo->UseAngle) {
			CVec dir = vVelocity;
			float angle = (float)( -atan2(dir.x,dir.y) * (180.0f/PI) );
			float offset = 360.0f / (float)tProjInfo->AngleImages;

			FMOD(angle, 360.0f);

			framestep = angle / offset;
		}

		// Special angle
		// Basically another way of organising the angles in images
		// Straight up is in the middle, rotating left goes left, rotating right goes right in terms
		// of image index's from the centre
		if(tProjInfo->UseSpecAngle) {
			CVec dir = vVelocity;
			float angle = (float)( -atan2(dir.x,dir.y) * (180.0f/PI) );
			int direct = 0;

			if(angle > 0)
				angle=180-angle;
			if(angle < 0) {
				angle=180+angle;
				direct = 1;
			}
			if(angle == 0)
				direct = 0;


			int num = (tProjInfo->AngleImages - 1) / 2;
			if(direct == 0)
				// Left side
				framestep = (float)(151-angle) / 151.0f * (float)num;
			else {
				// Right side
				framestep = (float)angle / 151.0f * (float)num;
				framestep += num+1;
			}
		}

		if(tProjInfo->Animating)
			framestep = fFrame;

		int size = tProjInfo->bmpImage->h;
		int half = size/2;
        iFrameX = (int)framestep*size;
		MOD(iFrameX, tProjInfo->bmpImage->w);

		DrawImageAdv(bmpDest, tProjInfo->bmpImage, iFrameX, 0, x-half, y-half, size,size);
	}
	return;
	}
}


///////////////////
// Draw the projectiles shadow
void CProjectile::DrawShadow(SDL_Surface * bmpDest, CViewport *view, CMap *map)
{
	if (tLX->fDeltaTime >= 0.1f) // Don't draw projectile shadows with FPS <= 10 to get a little better performance
		return;

	int wx = view->GetWorldX();
	int wy = view->GetWorldY();
	int l = view->GetLeft();
	int t = view->GetTop();

	int x=((int)vPosition.x-wx)*2+l;
	int y=((int)vPosition.y-wy)*2+t;

	// Clipping on the viewport
	if((x<l || x>l+view->GetVirtW()))
		return;
	if((y<t || y>t+view->GetVirtH()))
		return;

	switch (tProjInfo->Type)  {

	// Pixel
	case PRJ_PIXEL:
		map->DrawPixelShadow(bmpDest, view, (int)vPosition.x, (int)vPosition.y);
		break;

	// Image
	case PRJ_IMAGE:  {

			if(tProjInfo->bmpImage == NULL)
				return;
			/*if (tProjInfo->bmpImage.get()->w <= 2 && tProjInfo->bmpImage.get()->h <= 2)  {
				map->DrawPixelShadow(bmpDest, view, (int)vPosition.x, (int)vPosition.y);
				return;
			}*/

			int size = tProjInfo->bmpImage->h;
			int half = size / 2;
			map->DrawObjectShadow(bmpDest, tProjInfo->bmpImage, iFrameX, 0, size,size, view, (int)vPosition.x-(half>>1), (int)vPosition.y-(half>>1));
		}
		break;
    }
}


///////////////////
// Bounce
// HINT: this is not exactly the way the original LX did it,
//		but this way is way more correct and it seems to work OK
//		(original LX resets the bounce-direction on each checked side)
void CProjectile::Bounce(float fCoeff)
{
	float x,y;
	x=y=1;

	// This code is right, it should be done like that
	// However, we want to keep compatibility with .56 and when on each client would be another simulation,
	// we couldn't call that compatibility at all

	// For now we just keep the old, wrong code, so noone will call OLX players as cheaters
/*	if(CollisionSide & (COL_TOP|COL_BOTTOM)) {
		y=-y;
	}
	if(CollisionSide & (COL_LEFT|COL_RIGHT)) {
		x=-x;
	}

	if(CollisionSide & COL_TOP) {
		y*=fCoeff;
	}
	if(CollisionSide & COL_BOTTOM) {
		y*=fCoeff;
	}
	if(CollisionSide & COL_LEFT) {
		x*=fCoeff;
	}
	if(CollisionSide & COL_RIGHT) {
		x*=fCoeff;
	}*/


	// WARNING: this code should not be used; it is simply wrong
	//	(this was the way the original LX did it)

	if (CollisionSide & COL_TOP)  {
		x = fCoeff;
		y = -fCoeff;
	}
	if (CollisionSide & COL_BOTTOM)  {
		x = fCoeff;
		y = -fCoeff;
	}
	if (CollisionSide & COL_LEFT)  {
		x = -fCoeff;
		y = fCoeff;
	}
	if (CollisionSide & COL_RIGHT)  {
		x = -fCoeff;
		y = fCoeff;
	}


	vVelocity.x *= x;
	vVelocity.y *= y;
}


///////////////////
// Check for collisions with worms
// HINT: this function is not used at the moment
//		(ProjWormColl is used directly from within CheckCollision)
int CProjectile::CheckWormCollision(CWorm *worms)
{
	static const float divisions = 5;
	CVec dir = vPosition - vOldPos;
	float length = NormalizeVector(&dir);

	// Length must be at least 'divisions' in size so we do at least 1 check
	// So stationary projectiles also get checked (mines)
	length = MAX(length, divisions);

	// Go through at fixed positions
	CVec pos = vOldPos;
	int wrm;
	for(float p=0; p<length; p+=divisions, pos += dir*divisions) {
		wrm = ProjWormColl(pos, worms);
		if( wrm >= 0)
			return wrm;
	}

	// AI hack (i know it's dirty, but it's fast)
	// Checks, whether this projectile is heading to any AI worm
	// If so, sets the worm's property Heading to ourself
	CVec mutual_speed;
	CWorm *w = worms;
	for(short i=0;i<MAX_WORMS;i++,w++) {

		// Only AI worms need this
		if (!w->isUsed() || !w->getAlive() || w->getType() != PRF_COMPUTER)
			continue;

		mutual_speed = vVelocity - (*w->getVelocity());

		// This projectile is heading to the worm
		if (SIGN(mutual_speed.x) == SIGN(w->getPos().x - vPosition.x) &&
			SIGN(mutual_speed.y) == SIGN(w->getPos().y - vPosition.y))  {

			int len = (int)mutual_speed.GetLength();
			float dist = 60.0f;

			// Get the dangerous distance for various speeds
			if (len < 30)
				dist = 20.0f;
			else if (len < 60)
				dist = 30.0f;
			else if (len < 90)
				dist = 50.0f;

		}
	}


	// No worms hit
	return -1;
}


///////////////////
// Lower level projectile-worm collision test
// TODO: move to physics?
int CProjectile::ProjWormColl(CVec pos, CWorm *worms)
{
	int px = (int)pos.x;
	int py = (int)pos.y;
	int wx,wy;

	const static int wsize = 4;

	CWorm* ownerWorm = NULL;
	if(this->iOwner >= 0 && this->iOwner < MAX_WORMS) {
		ownerWorm = &worms[this->iOwner];
		if(!ownerWorm->isUsed() || ownerWorm->getFlag())
			ownerWorm = NULL;
	}
	
	CWorm *w = worms;
	for(short i=0;i<MAX_WORMS;i++,w++) {
		if(!w->isUsed() || !w->getAlive() || w->getFlag())
			continue;
		
		if(ownerWorm && cClient->isTeamGame() && !cClient->getGameLobby()->features[FT_TEAMHIT] && w != ownerWorm && w->getTeam() == ownerWorm->getTeam())
		   continue;
		
		if(ownerWorm && !cClient->getGameLobby()->features[FT_SELFHIT] && w == ownerWorm)
			continue;
		
		wx = (int)w->getPos().x;
		wy = (int)w->getPos().y;

		// AABB - Point test
		if( abs(wx-px) < wsize && abs(wy-py) < wsize) {

			CollisionSide = 0;

			// Calculate the side of the collision (obsolete??)
			if(px < wx-2)
				CollisionSide |= COL_LEFT;
			else if(px > wx+2)
				CollisionSide |= COL_RIGHT;
			if(py < wy-2)
				CollisionSide |= COL_TOP;
			else if(py > wy+2)
				CollisionSide |= COL_BOTTOM;

			return i;
		}
	}

	// No worm was hit
	return -1;
}

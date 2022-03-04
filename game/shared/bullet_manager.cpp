// (More) Reallistic simulated bullets
//
// This code is originally based from article on Valve Developer Community
// The original code you can find by this link:
// http://developer.valvesoftware.com/wiki/Simulated_Bullets

// NOTENOTE: Tested only on localhost. There maybe a latency errors and others
// NOTENOTE: The simulation might be strange.

#include "cbase.h"
#include "util_shared.h"
#include "bullet_manager.h"
#include "effect_dispatch_data.h"
#include "tier0/vprof.h"
#include "decals.h"
#include "func_break.h"
CBulletManager *g_pBulletManager;
CUtlLinkedList<CSimulatedBullet*> g_Bullets;

//#ifdef CLIENT_DLL//-------------------------------------------------
#include "engine/ivdebugoverlay.h"
//#include "c_te_effect_dispatch.h"
//ConVar g_debug_client_bullets("g_debug_client_bullets", "0", FCVAR_CHEAT);
//extern void FX_PlayerTracer(Vector& start, Vector& end);
//#else//-------------------------------------------------------------
#include "te_effect_dispatch.h"
#include "soundent.h"
#include "player_pickup.h"
#include "ilagcompensationmanager.h"
ConVar g_debug_bullets("g_debug_bullets", "0", FCVAR_CHEAT, "Debug of bullet simulation\nThe white line shows the bullet trail.\nThe red line shows not passed penetration test.\nThe green line shows passed penetration test. Turn developer mode for more information.");
//#endif//------------------------------------------------------------

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_RICO_DOT_ANGLE 0.5f	//Maximum dot allowed for any ricochet
#define MIN_RICO_SPEED_PERC 0.1f	//Minimum speed percent allowed for any ricochet
//#define NUM_PENETRATIONS	1		//Dark Energy Bullet penetration

static void BulletSpeedModifierCallback(ConVar *var, const char *pOldString)
{
	if (var->GetFloat() == 0.0f) //To avoid math exception
		var->Revert();
}
ConVar sv_bullet_speed_modifier("sv_bullet_speed_modifier", "700.000000");

static void UnrealRicochetCallback(ConVar *var, const char *pOldString)
{
	if (gpGlobals->maxClients > 1)
	{
		var->Revert();
		Msg("Cannot use unreal ricochet in multiplayer game\n");
	}

	if (var->GetBool()) //To avoid math exception
		Warning("\nWarning! Enabling unreal ricochet may cause the game crash.\n\n");
}
ConVar sv_bullet_unrealricochet("sv_bullet_unrealricochet", "0");



static void BulletStopSpeedCallback(ConVar *var, const char *pOldString)
{
	int val = var->GetInt();
	if (val<ONE_HIT_MODE)
		var->Revert();
	else if (BulletManager())
		BulletManager()->UpdateBulletStopSpeed();
}
ConVar sv_bullet_stop_speed("sv_bullet_stop_speed", "200");
//ConVar sv_bullet_speed_forced("sv_bullet_speed_forced", "0");


LINK_ENTITY_TO_CLASS(bullet_manager, CBulletManager);
#if 0
LINK_ENTITY_TO_CLASS(dark_energy_bullet, CDarkEnergyBullet);

//=========================================================
//=========================================================

BEGIN_DATADESC(CDarkEnergyBullet)

DEFINE_FIELD(m_AmmoType, FIELD_INTEGER),
DEFINE_FIELD(m_PenetratedAmmoType, FIELD_INTEGER),
DEFINE_FIELD(m_iImpacts, FIELD_INTEGER),
DEFINE_FIELD(m_vecOrigin, FIELD_VECTOR),
DEFINE_FIELD(m_vecDir, FIELD_VECTOR),
DEFINE_FIELD(m_flLastThink, FIELD_TIME),
DEFINE_FIELD(m_flDarkBulletSpeed, FIELD_FLOAT),

DEFINE_FIELD(m_vecStart, FIELD_VECTOR),
DEFINE_FIELD(m_vecEnd, FIELD_VECTOR),

DEFINE_THINKFUNC(BulletThink),

END_DATADESC()

#endif
//-----------------------------------------------------------------------------
//
// Regular Bullet
//
//-----------------------------------------------------------------------------

//==================================================
// Purpose:	Constructor
//==================================================
CSimulatedBullet::CSimulatedBullet()
{
	m_vecOrigin.Init();
	m_vecDirShooting.Init();
	m_flInitialBulletSpeed = m_flBulletSpeed = 0;
	m_flInitialBulletMass = m_flBulletMass = 0.8;	// Default 9mm weight
	m_flEntryDensity = 0.0f;
	bStuckInWall = false;

	m_iDamageType = 2;
}

//==================================================
// Purpose:	Constructor
//==================================================
CSimulatedBullet::CSimulatedBullet(FireBulletsInfo_t info, Vector newdir, CBaseEntity *pInfictor, CBaseEntity *pAdditionalIgnoreEnt,
	bool bTraceHull
//#ifndef CLIENT_DLL
//	, CBaseEntity *pCaller
//#endif
	)
{
	// Input validation
//	Assert(pInfictor);
//#ifndef CLIENT_DLL
//	Assert(pCaller);
//#endif

	bulletinfo = info;			// Setup Fire bullets information here

	p_eInfictor = pInfictor;	// Setup inflictor

	// Create a list of entities with which this bullet does not collide.
	m_pIgnoreList = new CTraceFilterSimpleList(COLLISION_GROUP_NONE);
	Assert(m_pIgnoreList);

	bStuckInWall = false;

	// Don't collide with the entity firing the bullet.
	m_pIgnoreList->AddEntityToIgnore(p_eInfictor);
	//	CBaseEntity *pPlayerAlly;
	//	if (p_eInfictor->IsPlayer)	// Player shoots
	//	{
	//		m_pIgnoreList->AddEntityToIgnore(pPlayerAlly->Classify() = CLASS_PLAYER_ALLY);
	//	}

	// Don't collide with some optionally-specified entity. 
	if (pAdditionalIgnoreEnt != NULL)
		m_pIgnoreList->AddEntityToIgnore(pAdditionalIgnoreEnt);

	m_iDamageType = GetAmmoDef()->DamageType(bulletinfo.m_iAmmoType);


	//m_szTracerName = (char*)p_eInfictor->GetTracerType(); 

	// Basic information about the bullet (pInfictor->IsNPC() && sv_bullet_speed_forced.GetInt() > 0) 
	m_flInitialBulletSpeed = m_flBulletSpeed = GetAmmoDef()->GetAmmoOfIndex(bulletinfo.m_iAmmoType)->bulletSpeed;
	m_flBulletSpeed = m_flInitialBulletSpeed + RandomFloat(-50, 50);
	m_flInitialBulletMass = m_flBulletMass = GetAmmoDef()->GetAmmoOfIndex(bulletinfo.m_iAmmoType)->bulletMass;
	m_flBulletDiameter = GetAmmoDef()->GetAmmoOfIndex(bulletinfo.m_iAmmoType)->bulletDiameter;
	m_vecDirShooting = newdir;
	m_vecOrigin = bulletinfo.m_vecSrc;
	m_bTraceHull = bTraceHull;
//#ifndef CLIENT_DLL
//	m_hCaller = pCaller;
//#endif
	m_flEntryDensity = 0.0f;
	m_vecTraceRay = m_vecOrigin + m_vecDirShooting * m_flBulletSpeed;
	m_flRayLength = m_flInitialBulletSpeed;
}



//==================================================
// Purpose:	Deconstructor
//==================================================
CSimulatedBullet::~CSimulatedBullet()
{
	delete m_pIgnoreList;
}
//==================================================
// Purpose:	Simulates a bullet through a ray of its bullet speed
//==================================================
bool CSimulatedBullet::SimulateBullet(void)
{
	VPROF("C_SimulatedBullet::SimulateBullet");

	if (!p_eInfictor)
	{
//		p_eInfictor = bulletinfo.m_pAttacker;

		if (!p_eInfictor)
			return false;
	}
/*#ifndef CLIENT_DLL
	if (!m_hCaller)
	{
		return false;
	}
#endif
*/
	if (!IsFinite(m_flBulletSpeed))
		return false;		 //prevent a weird crash

	trace_t trace;
	Vector vecTraceStart(m_vecOrigin);
	if (m_flBulletSpeed <= 0 || m_flBulletMass <= 0) //Avoid errors;
		return false;

	m_flRayLength = m_flBulletSpeed;

	if (GetAmmoDef()->Flags(GetAmmoTypeIndex()) & AMMO_DARK_ENERGY)
	{
		m_vecTraceRay = m_vecOrigin + m_vecDirShooting * m_flBulletSpeed;
	}
	else
	{
		m_flBulletSpeed += m_flBulletMass * m_vecDirShooting.z; // 0.8f instead of mass
		m_vecTraceRay = m_vecOrigin + m_vecDirShooting * m_flBulletSpeed;
		m_vecDirShooting.z -= 0.1 / m_flBulletSpeed;
	}
#ifdef GAME_DLL
	if (bulletinfo.m_flLatency != 0)
	{
		m_vecTraceRay *= bulletinfo.m_flLatency * 100;
	}
#endif

	if (!IsInWorld())
	{
		return false;
	}

	if (bStuckInWall)
		return false;

	if (!m_bWasInWater)
	{
		WaterHit(m_vecOrigin, m_vecTraceRay);
	}

	m_bWasInWater = (UTIL_PointContents(m_vecOrigin)& MASK_WATER) ? true : false;

	if (m_bWasInWater && GetAmmoDef()->Flags(GetAmmoTypeIndex()) != AMMO_DARK_ENERGY)
	{
		m_flBulletSpeed -= m_flBulletSpeed * 0.6;
#ifdef GAME_DLL
		//This is a server stuff
		UTIL_BubbleTrail(m_vecOrigin, m_vecTraceRay, 5);
#endif
	}

//#ifndef CLIENT_DLL
	if (m_bWasInWater)
	{
		CEffectData tracerData;
		tracerData.m_vStart = m_vecOrigin;
		tracerData.m_vOrigin = m_vecTraceRay;

		tracerData.m_fFlags = TRACER_TYPE_WATERBULLET;

		DispatchEffect("TracerSound", tracerData);
	}
//#endif

	static int	tracerCount;
	bool bulletSpeedCheck;
	bulletSpeedCheck = false;
	//	int iAttachment = p_eInfictor->GetTracerAttachment();
	trace_t tr;

	if (m_bTraceHull)
		UTIL_TraceHull(m_vecOrigin, m_vecTraceRay, Vector(-m_flBulletDiameter, -m_flBulletDiameter, -m_flBulletDiameter),
		Vector(m_flBulletDiameter, m_flBulletDiameter, m_flBulletDiameter), MASK_SHOT, m_pIgnoreList, &trace);
	else
		UTIL_TraceLine(m_vecOrigin, m_vecTraceRay, MASK_SHOT, m_pIgnoreList, &trace);

	//	if (!m_bWasInWater)
	//	{
	//		UTIL_Tracer(m_vecOrigin, trace.endpos, p_eInfictor->entindex(), TRACER_DONT_USE_ATTACHMENT, m_flBulletSpeed, true, p_eInfictor->GetTracerType());
	//	}

/*#ifdef 0 //CLIENT_DLL
	if (g_debug_client_bullets.GetBool())
	{
		debugoverlay->AddLineOverlay(trace.startpos, trace.endpos, 255, 0, 0, true, 10.0f);
	}
	//	else if (GetAmmoDef()->Flags(GetAmmoTypeIndex()) & AMMO_DARK_ENERGY)
	//	{
	//		FX_PlayerTracer(vecTraceStart, m_vecOrigin);
	//	}
#else*/
	if (g_debug_bullets.GetBool())
	{
		NDebugOverlay::Line(trace.startpos, trace.endpos, 255, 255, 255, true, 10.0f);
	}
	//	else if (GetAmmoDef()->Flags(GetAmmoTypeIndex()) != AMMO_DARK_ENERGY)
	//	{
	//		NDebugOverlay::Line(trace.startpos, trace.endpos, 240, 200, 133, true, 0.02f);
	//	}
	/*
	const char *pszTracerName = p_eInfictor->GetTracerType();
	int iAttachment = p_eInfictor->GetTracerAttachment();
	UTIL_Tracer(trace.startpos, trace.endpos, 0, iAttachment, m_flBulletSpeed, true, pszTracerName);*/
	// Clip the ray to the first collided solid returned from traceline
	CTakeDamageInfo triggerInfo(p_eInfictor, bulletinfo.m_pAttacker, bulletinfo.m_flDamage, GetDamageType());
	CalculateBulletDamageForce(&triggerInfo, bulletinfo.m_iAmmoType, m_vecDirShooting, trace.endpos);
	triggerInfo.ScaleDamageForce(bulletinfo.m_flDamageForceScale);
	triggerInfo.SetAmmoType(bulletinfo.m_iAmmoType);
	BulletManager()->SendTraceAttackToTriggers(triggerInfo, trace.startpos, trace.endpos, m_vecDirShooting);
//#endif

	if (trace.fraction == 1.0f)
	{
		m_vecOrigin += m_vecDirShooting * m_flBulletSpeed; //Do the WAY

		CEffectData data;
		data.m_vStart = trace.startpos;
		data.m_vOrigin = trace.endpos;
		data.m_nDamageType = GetDamageType();

		DispatchEffect("RagdollImpact", data);

		return true;
	}
	else
	{
		EntityImpact(trace);

		if (trace.m_pEnt == p_eInfictor) //HACK: Remove bullet if we hit self (for frag grenades)
			return false;

		if (!(trace.surface.flags&SURF_SKY))
		{
			if (trace.allsolid)//in solid
			{
				if (!AllSolid(trace))
					return false;

				m_vecOrigin += m_vecDirShooting * m_flBulletSpeed; //Do the WAY

				bulletSpeedCheck = true;
			}
			else if (trace.fraction != 1.0f)//hit solid
			{
				if (!EndSolid(trace))
					return false;

				bulletSpeedCheck = true;
			}
			else if (trace.startsolid)//exit solid
			{
				if (!StartSolid(trace))
					return false;

				m_vecOrigin += m_vecDirShooting * m_flBulletSpeed; //Do the WAY

				bulletSpeedCheck = true;
			}
			else
			{
				//don't do a bullet speed check for not touching anything
			}
		}
		else
		{
			return false; //Through sky? No.
		}
	}

	if (sv_bullet_unrealricochet.GetBool()) //Fun bullet ricochet fix
	{
		delete m_pIgnoreList; //Prevent causing of memory leak
		m_pIgnoreList = new CTraceFilterSimpleList(COLLISION_GROUP_NONE);
	}

	if (bulletSpeedCheck)
	{
		if (m_flBulletSpeed <= BulletManager()->BulletStopSpeed())
		{
			return false;
		}
	}

	return true;
}


//==================================================
// Purpose:	Simulates when a solid is exited
//==================================================
bool CSimulatedBullet::StartSolid(trace_t &ptr)
{
	switch (BulletManager()->BulletStopSpeed())
	{
	case BUTTER_MODE:
	{
		//Do nothing to bullet speed
		return true;
	}
	case ONE_HIT_MODE:
	{
		return false;
	}
	default:
	{	
		float flPenetrationDistance = VectorLength(AbsEntry - AbsExit);

		m_flBulletSpeed -= flPenetrationDistance * m_flEntryDensity / sv_bullet_speed_modifier.GetFloat();
		return true;
	}
	}
	return true;
}


//==================================================
// Purpose:	Simulates when a solid is being passed through
//==================================================
bool CSimulatedBullet::AllSolid(trace_t &ptr)
{
	switch (BulletManager()->BulletStopSpeed())
	{
	case BUTTER_MODE:
	{
		//Do nothing to bullet speed
		return true;
	}
	case ONE_HIT_MODE:
	{
		return false;
	}
	default:
	{
		m_flBulletSpeed -= m_flBulletSpeed * m_flEntryDensity / sv_bullet_speed_modifier.GetFloat();
		return true;
	}
	}
	return true;
}


//==================================================
// Purpose:	Simulate when a surface is hit
//==================================================
bool CSimulatedBullet::EndSolid(trace_t &ptr)
{
	m_vecEntryPosition = ptr.endpos;

//#ifndef 0 //CLIENT_DLL
	int soundEntChannel = (bulletinfo.m_nFlags&FIRE_BULLETS_TEMPORARY_DANGER_SOUND) ? SOUNDENT_CHANNEL_BULLET_IMPACT : SOUNDENT_CHANNEL_UNSPECIFIED;

	CSoundEnt::InsertSound(SOUND_BULLET_IMPACT, m_vecEntryPosition, 200, 0.5, p_eInfictor, soundEntChannel);
//#endif

	if (FStrEq(ptr.surface.name, "tools/toolsblockbullets"))
	{
		return false;
	}
	m_flEntryDensity = physprops->GetSurfaceData(ptr.surface.surfaceProps)->physics.density;

	trace_t rtr;
	Vector vecEnd = m_vecEntryPosition + m_vecDirShooting * 32; //32 units

	//Doing now test of penetration
	UTIL_TraceLine(m_vecEntryPosition + m_vecDirShooting * 1.1, vecEnd, MASK_SHOT, m_pIgnoreList, &rtr);

	AbsEntry = m_vecEntryPosition;
	AbsExit = rtr.startpos;

	float flPenetrationDistance = VectorLength(AbsEntry - AbsExit);

	DesiredDistance = 0.0f;

	surfacedata_t *p_penetrsurf = physprops->GetSurfaceData(ptr.surface.surfaceProps);
	switch (p_penetrsurf->game.material)
	{
	case CHAR_TEX_WOOD:
		DesiredDistance = 9.0f; // 9 units in hammer
		break;
	case CHAR_TEX_GRATE:
		DesiredDistance = 9.0f; // 9 units in hammer
		break;
	case CHAR_TEX_CONCRETE:
		DesiredDistance = 4.0f; // 4 units in hammer
		break;
	case CHAR_TEX_TILE:
		DesiredDistance = 5.0f; // 5 units in hammer
		break;
	case CHAR_TEX_COMPUTER:
		DesiredDistance = 16.0f; // 5 units in hammer
		break;
	case CHAR_TEX_VENT:
		DesiredDistance = 4.0f; // 4 units in hammer and no more(!)
		break;
	case CHAR_TEX_METAL:
		DesiredDistance = 2.75f; // 1 units in hammer. We cannot penetrate a really 'fat' metal wall. Corners are good.
		break;
	case CHAR_TEX_PLASTIC:
		DesiredDistance = 16.0f; // 16 units in hammer: Plastic can more
		break;
	case CHAR_TEX_ANTLION:
		DesiredDistance = 1.0f; // 1.5 units in hammer
		break;
	case CHAR_TEX_BLOODYFLESH:
		DesiredDistance = 1.0f; // Don't penetrate multiple times.
		break;
	case CHAR_TEX_FLESH:
		DesiredDistance = 1.0f; // Don't penetrate multiple times.
		break;
	case CHAR_TEX_DIRT:
		DesiredDistance = 6.0f; // 6 units in hammer: >4 cm of plaster can be penetrated
		break;
	case CHAR_TEX_GLASS:

		if (ptr.m_pEnt->ClassMatches("func_breakable"))
		{
			// Query the func_breakable for whether it wants to allow for bullet penetration
			if (ptr.m_pEnt->HasSpawnFlags(SF_BREAK_NO_BULLET_PENETRATION) == false)
			{
				DesiredDistance = 4.0f; // maximum 4 units in hammer.
			}
			else
			{
				DesiredDistance = 0.25f; // maximum half a centimeter.
			}
		}

//		DesiredDistance = 0.5f; // maximum half a centimeter.
		break;
	}

	Vector	reflect;
	float fldot = m_vecDirShooting.Dot(ptr.plane.normal);						//Getting angles from lasttrace
	float flRicoDotAngle = -MAX_RICO_DOT_ANGLE;

	// Don't ricochet off creatures unless at an extreme angle
	if (ptr.m_pEnt->IsPlayer() || ptr.m_pEnt->IsNPC())
	{
		flRicoDotAngle *= 0.22;
	}

#if 0
	bool bMustDoRico = (GetAmmoDef()->Flags(GetAmmoTypeIndex()) != AMMO_DARK_ENERGY &&
		fldot > flRicoDotAngle && GetBulletSpeedRatio() > MIN_RICO_SPEED_PERC); // We can't do richochet when bullet has lowest speed

	if (sv_bullet_unrealricochet.GetBool() && p_eInfictor->IsPlayer()) //Cheat is only for player,yet =)
		bMustDoRico = true;

	if (bMustDoRico)
	{
		if (!sv_bullet_unrealricochet.GetBool())
		{
			{
				m_flBulletSpeed -= m_flBulletSpeed * (-fldot) * (0.1 / m_flBulletMass) * random->RandomFloat(1.00, 0.85);	// More mass = better ricochets
				m_flBulletMass -= m_flBulletMass * (-fldot) / 2;
			}
		}
		else
		{
			m_flBulletSpeed *= 0.9;
		}

		reflect = m_vecDirShooting + (ptr.plane.normal * (fldot*-2.0f));	//reflecting

		if (gpGlobals->maxClients == 1 && !sv_bullet_unrealricochet.GetBool()) //Use more simple for multiplayer
		{
			reflect[0] += random->RandomFloat(fldot, -fldot);
			reflect[1] += random->RandomFloat(fldot, -fldot);
			reflect[2] += random->RandomFloat(fldot, -fldot);
		}

		m_flEntryDensity *= 0.2;

		m_vecDirShooting = reflect;

		m_vecOrigin = ptr.endpos + m_vecDirShooting;//(ptr.endpos + m_vecDirShooting*1.1) + m_vecDirShooting * m_flBulletSpeed;
	}
	else
#endif
	{
		if (flPenetrationDistance > DesiredDistance || ptr.IsDispSurface()) // || ptr.IsDispSurface()
		{
			bool bMustDoRico = (GetAmmoDef()->Flags(GetAmmoTypeIndex()) != AMMO_DARK_ENERGY &&
				fldot > flRicoDotAngle && GetBulletSpeedRatio() > MIN_RICO_SPEED_PERC); // We can't do richochet when bullet has lowest speed

			if ( GetAmmoDef()->Flags(GetAmmoTypeIndex()) & AMMO_DARK_ENERGY )
				bMustDoRico = false;

			if (sv_bullet_unrealricochet.GetBool() && p_eInfictor->IsPlayer()) //Cheat is only for player,yet =)
				bMustDoRico = true;

			if (bMustDoRico)
			{
				if (!sv_bullet_unrealricochet.GetBool())
				{
					{
						m_flBulletSpeed *= (1.0f / -fldot) * (m_flBulletMass * random->RandomFloat(0.005, 0.1));	// More mass = better ricochets
						m_flBulletMass *= m_flBulletMass / (-fldot);
//						m_flBulletSpeed *= (1.0f / -fldot) * m_flBulletMass * random->RandomFloat(0.00625, 0.125);
					}
				}
				else
				{
					m_flBulletSpeed *= 0.9;
				}

				reflect = m_vecDirShooting + (ptr.plane.normal * (fldot*-2.0f));	//reflecting

				if (gpGlobals->maxClients == 1 && !sv_bullet_unrealricochet.GetBool()) //Use more simple for multiplayer
				{
					reflect[0] += random->RandomFloat(fldot, -fldot);
					reflect[1] += random->RandomFloat(fldot, -fldot);
					reflect[2] += random->RandomFloat(fldot, -fldot);
				}

				m_flEntryDensity *= 0.2;

				m_vecDirShooting = reflect;

				m_vecOrigin = ptr.endpos + m_vecDirShooting;//(ptr.endpos + m_vecDirShooting*1.1) + m_vecDirShooting * m_flBulletSpeed;
			}
			else
			{
				bStuckInWall = true;

#ifdef GAME_DLL
				if (g_debug_bullets.GetBool())
				{
					NDebugOverlay::Line(AbsEntry, AbsExit, 255, 0, 0, true, 10.0f);

					if (!ptr.IsDispSurface())
						DevMsg("Cannot penetrate surface, The distance(%f) > %f \n", flPenetrationDistance, DesiredDistance);
					else
						DevMsg("Displacement penetration was tempolary disabled\n");
				}
#endif
			}
		}
		else
		{
			trace_t tr;
			UTIL_TraceLine(AbsExit + m_vecDirShooting * 1.1, AbsEntry, MASK_SHOT, m_pIgnoreList, &tr);
			UTIL_ImpactTrace(&tr, GetDamageType()); // On exit

#ifdef GAME_DLL
			if (g_debug_bullets.GetBool())
			{
				NDebugOverlay::Line(AbsEntry, AbsExit, 0, 255, 0, true, 10.0f);
			}
#endif
			m_vecDirShooting[0] += (random->RandomFloat(-flPenetrationDistance, flPenetrationDistance))*0.03;
			m_vecDirShooting[1] += (random->RandomFloat(-flPenetrationDistance, flPenetrationDistance))*0.03;
			m_vecDirShooting[2] += (random->RandomFloat(-flPenetrationDistance, flPenetrationDistance))*0.03;

			VectorNormalize(m_vecDirShooting);

			m_vecOrigin = AbsExit + m_vecDirShooting;
		}
	}

#ifdef GAME_DLL
	//Cancel making dust underwater:
	if (!m_bWasInWater)
	{
		UTIL_ImpactTrace(&ptr, GetDamageType());
	}
#endif

	m_flBulletSpeed -= flPenetrationDistance * m_flEntryDensity / sv_bullet_speed_modifier.GetFloat();

	if ( GetAmmoDef()->Flags(GetAmmoTypeIndex()) & AMMO_DARK_ENERGY ||
		BulletManager()->BulletStopSpeed() == ONE_HIT_MODE )
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// analog of HandleShotImpactingWater
//-----------------------------------------------------------------------------
bool CSimulatedBullet::WaterHit(const Vector &vecStart, const Vector &vecEnd)
{
	trace_t	waterTrace;
	// Trace again with water enabled
	UTIL_TraceLine(vecStart, vecEnd, (MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME), m_pIgnoreList, &waterTrace);

	// See if this is the point we entered
	if ((enginetrace->GetPointContents(waterTrace.endpos - Vector(0, 0, 0.1f)) & (CONTENTS_WATER | CONTENTS_SLIME)) == 0)
		return false;

	int	nMinSplashSize = GetAmmoDef()->MinSplashSize(GetAmmoTypeIndex()) * (1.5 - GetBulletSpeedRatio());
	int	nMaxSplashSize = GetAmmoDef()->MaxSplashSize(GetAmmoTypeIndex()) * (1.5 - GetBulletSpeedRatio()); //High speed - small splash

	CEffectData	data;
	data.m_vOrigin = waterTrace.endpos;
	data.m_vNormal = waterTrace.plane.normal;
	data.m_flScale = random->RandomFloat(nMinSplashSize, nMaxSplashSize);
	if (waterTrace.contents & CONTENTS_SLIME)
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}
	DispatchEffect("gunshotsplash", data);
	return true;
}



//==================================================
// Purpose:	Entity impact procedure
//==================================================
void CSimulatedBullet::EntityImpact(trace_t &ptr)
{
	if (bStuckInWall)
		return;

	if (ptr.m_pEnt != NULL)
	{
		//Hit inflicted once to avoid perfomance errors
		// NPc shoots the player
		if (!p_eInfictor->IsPlayer())
		{
			if (ptr.m_pEnt->IsPlayer())
			{
				if (m_pIgnoreList->ShouldHitEntity(ptr.m_pEnt, MASK_SHOT))
				{
					m_pIgnoreList->AddEntityToIgnore(ptr.m_pEnt);
				}
				else
				{
					return;
				}
			}
		}

		if (ptr.m_pEnt == m_hLastHit)
			return;

		m_hLastHit = ptr.m_pEnt;

		float flMaxDamage = g_pGameRules->GetAmmoDamage(p_eInfictor, ptr.m_pEnt, bulletinfo.m_iAmmoType);
		float flMaxForce = bulletinfo.m_flDamageForceScale;

		float flActualDamage = flMaxDamage;
		float flActualForce = flMaxForce;

		//To make it more reallistic
		float fldot = m_vecDirShooting.Dot(ptr.plane.normal);
		//We affecting damage by angle. If we have lower angle of reflection, doing lower damage.

		flActualDamage *= Square(GetBulletSpeedRatio()) * GetBulletMassRatio() * -fldot; //And also affect damage by speed and weight modifications 
		flActualForce *= GetBulletSpeedRatio() * GetBulletMassRatio() * -fldot;

		if (flActualDamage > flMaxDamage && flActualForce > flMaxForce)
		{
			flActualDamage = flMaxDamage;
			flActualForce = flMaxForce;
		}

		DevMsg("Hit: %s, Damage: %f, Force: %f \n", ptr.m_pEnt->GetClassname(), flActualDamage, flActualForce);

		CTakeDamageInfo dmgInfo(p_eInfictor, bulletinfo.m_pAttacker, flActualDamage, GetDamageType());

		CalculateBulletDamageForce(&dmgInfo, bulletinfo.m_iAmmoType, m_vecDirShooting, ptr.endpos);
		dmgInfo.ScaleDamageForce(flActualForce);
		dmgInfo.SetAmmoType(bulletinfo.m_iAmmoType);

		ptr.m_pEnt->DispatchTraceAttack(dmgInfo, bulletinfo.m_vecDirShooting, &ptr);

#ifdef GAME_DLL
		ApplyMultiDamage(); //It's requried

		if (GetAmmoDef()->Flags(GetAmmoTypeIndex()) & AMMO_FORCE_DROP_IF_CARRIED)
		{
			// Make sure if the player is holding this, he drops it
			Pickup_ForcePlayerToDropThisObject(ptr.m_pEnt);
		}
#endif
	}
}


//==================================================
// Purpose:	Simulates all bullets every centisecond
//==================================================
//#ifndef 0 //CLIENT_DLL
void CBulletManager::Think(void)
//#else
//void CBulletManager::ClientThink(void)
//#endif
{
	unsigned short iNext = 0;
	for (unsigned short i = g_Bullets.Head(); i != g_Bullets.InvalidIndex(); i = iNext)
	{
		iNext = g_Bullets.Next(i);
		if (!g_Bullets[i]->SimulateBullet())
			RemoveBullet(i);
	}
	if (g_Bullets.Head() != g_Bullets.InvalidIndex())
	{
/*#ifdef 0 //CLIENT_DLL
		SetNextClientThink(gpGlobals->curtime + 0.01f);
#else*/
		SetNextThink(gpGlobals->curtime + 0.01f);
//#endif
	}
}


//==================================================
// Purpose:	Called by sv_bullet_stop_speed callback to keep track of resources
//==================================================
void CBulletManager::UpdateBulletStopSpeed(void)
{
	m_iBulletStopSpeed = sv_bullet_stop_speed.GetInt();
}

//#ifndef CLIENT_DLL
void CBulletManager::SendTraceAttackToTriggers(const CTakeDamageInfo &info, const Vector& start, const Vector& end, const Vector& dir)
{
	TraceAttackToTriggers(info, start, end, dir);
}
//#endif


//==================================================
// Purpose:	Add bullet to linked list
//			Handle lag compensation with "prebullet simulation"
// Output:	Bullet's index (-1 for error)
//==================================================
int CBulletManager::AddBullet(CSimulatedBullet *pBullet)
{
	if (pBullet->GetAmmoTypeIndex() == -1)
	{
		Msg("ERROR: Undefined ammo type!\n");
		return -1;
	}
	unsigned short index = g_Bullets.AddToTail(pBullet);
/*#ifdef 0//CLIENT_DLL
	DevMsg("Client Bullet Created (%i)\n", index);
	if (g_Bullets.Count() == 1)
	{
		SetNextClientThink(gpGlobals->curtime + 0.01f);
	}
#else*/
	DevMsg("Bullet Created (%i) LagCompensation %f\n", index, pBullet->bulletinfo.m_flLatency);
	if (pBullet->bulletinfo.m_flLatency != 0.0f)
		pBullet->SimulateBullet(); //Pre-simulation

	if (g_Bullets.Count() == 1)
	{
		SetNextThink(gpGlobals->curtime + 0.01f);
	}
//#endif
	return index;
}

//==================================================
//==================================================
/*int CBulletManager::AddDarkEnergyBullet(CDarkEnergyBullet *pDarkEnergyBullet)
{
if (pDarkEnergyBullet->GetAmmoTypeIndex() == -1)
{
return -1;
}
#ifdef CLIENT_DLL
DevMsg("Client Bullet Created (%i)\n", index);
if (g_Bullets.Count() == 1)
{
SetNextClientThink(gpGlobals->curtime + 0.01f);
}
#else
//	DevMsg("Bullet Created (%i) LagCompensation %f\n", index, pDarkEnergyBullet->bulletinfo.m_flLatency);
//	if (pDarkEnergyBullet->bulletinfo.m_flLatency != 0.0f)
//		pDarkEnergyBullet->BulletThink(); //Pre-simulation

if (g_Bullets.Count() == 1)
{
SetNextThink(gpGlobals->curtime + 0.01f);
}
#endif
return 1;
}
*/

//==================================================
// Purpose:	Remove the bullet at index from the linked list
// Output:	Next index
//==================================================
void CBulletManager::RemoveBullet(int index)
{
	g_Bullets.Next(index);
#ifdef CLIENT_DLL
	DevMsg("Client ");
#endif
	DevMsg("Bullet Removed (%i)\n", index);
	g_Bullets.Remove(index);
	if (g_Bullets.Count() == 0)
	{
#ifdef CLIENT_DLL
		SetNextClientThink(TICK_NEVER_THINK);
#else
		SetNextThink(TICK_NEVER_THINK);
#endif
	}
}

#if 0
//-----------------------------------------------------------------------------
//
// Dark Energy Bullet
//
//-----------------------------------------------------------------------------

//---------------------------------------------------------
//---------------------------------------------------------
void CDarkEnergyBullet::Precache()
{
}


//---------------------------------------------------------
//---------------------------------------------------------
void CDarkEnergyBullet::BulletThink(void)
{
	// Set the bullet up to think again.
	SetNextThink(gpGlobals->curtime + 0.01);

	// Trace this timeslice of the bullet.
	Vector vecStart;
	Vector vecEnd;
	float flInterval;

	flInterval = gpGlobals->curtime - GetLastThink();
	vecStart = GetAbsOrigin();
	vecEnd = vecStart + (m_vecDir * (m_flDarkBulletSpeed * flInterval));
	float flDist = (vecStart - vecEnd).Length();

	//Msg(".");
	//	CAmmoDef*	pAmmoDef = GetAmmoDef();

	trace_t tr;
	UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, p_eInflictor, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction != 1.0)
	{
		// This slice of bullet will hit something.
		GetOwnerEntity()->FireBullets(1, vecStart, m_vecDir, vec3_origin, flDist, m_AmmoType, 0);
		m_iImpacts++;

		if (m_iImpacts == NUM_PENETRATIONS)
		{
			// Bullet stops when it has penetrated enough times.
			Stop();
			return;
		}
		else
		{
#define STEP_SIZE	2
#define NUM_STEPS	6
			// Try to slide a 'cursor' through the object that was hit. 
			Vector vecCursor = tr.endpos;

			for (int i = 0; i < NUM_STEPS; i++)
			{
				//Msg("-");
				vecCursor += m_vecDir * STEP_SIZE;

				if (UTIL_PointContents(vecCursor) != CONTENTS_SOLID)
				{
					// Passed out of a solid! 
					SetAbsOrigin(vecCursor);

					// Fire another tracer.
					UTIL_TraceLine(vecCursor, vecCursor + m_vecDir * 8192, MASK_SHOT, p_eInflictor, COLLISION_GROUP_NONE, &tr);
					UTIL_Tracer(vecCursor, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, m_flDarkBulletSpeed, true, "AR2Tracer");
					return;
				}
			}

			// Bullet also stops when it fails to exit material after penetrating this far.
			//Msg("#\n");
			Stop();
			return;
		}
	}
	else
	{
		SetAbsOrigin(vecEnd);
	}
}


//=========================================================
//=========================================================
CDarkEnergyBullet::CDarkEnergyBullet(FireBulletsInfo_t info, Vector newdir, CBaseEntity *pOwner)
{
	m_flLastThink = gpGlobals->curtime;
	bulletinfo = info;			// Setup Fire bullets information here

	if (m_AmmoType == -1)
	{
		// This is the bullet that is used for all subsequent FireBullets() calls after the first
		// call penetrates a surface and keeps going.
		m_PenetratedAmmoType = GetAmmoDef()->Index("AR2");
	}
	p_eInflictor = pOwner;

	//	UTIL_SetOrigin(p_eInflictor, vecOrigin);

	m_vecDir = newdir;
	VectorNormalize(m_vecDir);

	// Set speed;
	m_flDarkBulletSpeed = GetAmmoDef()->GetAmmoOfIndex(bulletinfo.m_iAmmoType)->bulletSpeed;

	// Start the tracer here, and tell it to end at the end of the last trace
	// the trace comes from the loop above that does penetration.
	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + m_vecDir * 8192, MASK_SOLID_BRUSHONLY, p_eInflictor, COLLISION_GROUP_NONE, &tr);
	//	UTIL_Tracer(vecOrigin, tr.endpos, 0, TRACER_DONT_USE_ATTACHMENT, m_flDarkBulletSpeed, true, "StriderTracer");

	SetThink(&CDarkEnergyBullet::BulletThink);
	SetNextThink(gpGlobals->curtime);
}


//---------------------------------------------------------
//---------------------------------------------------------
void CDarkEnergyBullet::Init(void)
{
	m_vecDir.Init();
	m_AmmoType = -1;
	m_iImpacts = 0;
}


//---------------------------------------------------------
//---------------------------------------------------------
void CDarkEnergyBullet::Stop(void)
{
	// The bullet doesn't retire immediately because it still has a sound
	// in the world that is relying on the bullet's position as a react origin.
	// So stick around for another second or so.
	SetThink(&CBaseEntity::SUB_Remove);
	SetNextThink(gpGlobals->curtime + 1.0);
}
#endif
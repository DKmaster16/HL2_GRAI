// (More) Reallistic simulated bullets
//
// This code is originally based from article on Valve Developer Community
// The original code you can find by this link:
// http://developer.valvesoftware.com/wiki/Simulated_Bullets

#include "ammodef.h"
#include "takedamageinfo.h"

#define BUTTER_MODE -1
#define ONE_HIT_MODE -2

#ifdef CLIENT_DLL//-----------------------
class C_BulletManager;
extern C_BulletManager *g_pBulletManager;
#define CBulletManager C_BulletManager
#define CSimulatedBullet C_SimulatedBullet
#else//-----------------------------------
class CBulletManager;
extern CBulletManager *g_pBulletManager;
#endif//----------------------------------

inline CBulletManager *BulletManager()
{
	return g_pBulletManager;
}

extern ConVar g_debug_bullets;
class CSimulatedBullet
{
public:
	CSimulatedBullet();
	CSimulatedBullet(FireBulletsInfo_t info, Vector newdir, CBaseEntity *pInfictor, CBaseEntity *pAdditionalIgnoreEnt,
		bool bTraceHull
#ifndef CLIENT_DLL
		, CBaseEntity *pCaller
#endif
		);
	~CSimulatedBullet();

	inline float GetBulletSpeedRatio(void) //The percent of bullet speed 
	{
		return m_flBulletSpeed / m_flInitialBulletSpeed;
	}

	inline float GetBulletMassRatio(void) //The percent of bullet mass 
	{
		return m_flBulletMass / m_flInitialBulletMass;
	}

	inline bool IsInWorld(void) const
	{
		if (m_vecOrigin.x >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.y >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.z >= MAX_COORD_INTEGER) return false;
		if (m_vecOrigin.x <= MIN_COORD_INTEGER) return false;
		if (m_vecOrigin.y <= MIN_COORD_INTEGER) return false;
		if (m_vecOrigin.z <= MIN_COORD_INTEGER) return false;
		return true;
	}


	bool StartSolid(trace_t &ptr); //Exits solid
	bool AllSolid(trace_t &ptr); //continues in solid
	bool EndSolid(trace_t &ptr); //Enters solid
	bool WaterHit(const Vector &vecStart, const Vector &vecEnd); //Tests water collision

	bool SimulateBullet(void); //Main function of bullet simulation

	void EntityImpact(trace_t &ptr);	//Impact test
	inline int GetDamageType(void) const
	{
		return m_iDamageType;
	}

	inline int GetAmmoTypeIndex(void) const
	{
		return bulletinfo.m_iAmmoType;
	}
private:
	bool m_bTraceHull;	//Trace hull?
	bool m_bWasInWater;

	CTraceFilterSimpleList *m_pIgnoreList; //already hit
#ifndef CLIENT_DLL
	CUtlVector<CBaseEntity *> m_CompensationConsiderations; //Couldn't resist
#endif

	EHANDLE m_hCaller;
//	EHANDLE	m_hLastHit;		//Last hit (I think it doesn't work)


	float m_flBulletSpeed;  //The changeable bullet speed
	float m_flBulletMass;  //The changeable bullet mass
	float m_flEntryDensity; //Sets when doing penetration test
	float m_flInitialBulletSpeed;
	float m_flInitialBulletMass;
	float m_flRayLength;

	float DesiredDistance; //Sets when doing penetration test

	bool m_bPenetrated;

	int m_iDamageType;

	Vector m_vecDirShooting;   //The bullet direction with applied spread
	Vector m_vecOrigin;		   //Bullet origin

	Vector m_vecEntryPosition; //Sets when doing penetration test

	Vector m_vecTraceRay;

	Vector AbsEntry;
	Vector AbsExit;

	CBaseEntity *p_eInfictor;
public:
	FireBulletsInfo_t bulletinfo;
private:
	bool bStuckInWall; // Indicates bullet 
};

extern CUtlLinkedList<CSimulatedBullet*> g_Bullets; //Bullet list


class CBulletManager : public CBaseEntity
{
	DECLARE_CLASS(CBulletManager, CBaseEntity);
public:
	~CBulletManager()
	{
		g_Bullets.PurgeAndDeleteElements();
	}
	int AddBullet(CSimulatedBullet *pBullet);
//	int AddDarkEnergyBullet(CDarkEnergyBullet *pDarkEnergyBullet);
#ifdef CLIENT_DLL
	void ClientThink(void);
#else
	void Think(void);
	void SendTraceAttackToTriggers(const CTakeDamageInfo &info, const Vector& start, const Vector& end, const Vector& dir);
#endif

	void RemoveBullet(int index);	//Removes bullet.
	void UpdateBulletStopSpeed(void);	//Updates bullet speed

	int BulletStopSpeed(void) //returns speed that the bullet must be removed
	{
		return m_iBulletStopSpeed;
	}

private:
	int m_iBulletStopSpeed;
};

//=========================================================
//=========================================================
#if 0
class CDarkEnergyBullet : public CBaseEntity
{
public:
	DECLARE_CLASS(CDarkEnergyBullet, CBaseEntity);

	CDarkEnergyBullet(void) { Init(); }

	Vector	m_vecDir;

	Vector		m_vecStart;
	Vector		m_vecEnd;

	FireBulletsInfo_t bulletinfo;

	float	m_flLastThink;
	int		m_AmmoType;
	int		m_PenetratedAmmoType;
	float	m_flDarkBulletSpeed;  //The non-changeable bullet speed
	bool	m_bDirectShot;

	inline int GetAmmoTypeIndex(void) const
	{
		return bulletinfo.m_iAmmoType;
	}
	void Precache(void);

	CDarkEnergyBullet(FireBulletsInfo_t info, Vector newdir, CBaseEntity *pOwner);
	void Stop(void);

	void BulletThink(void);

	void Init(void);

	DECLARE_DATADESC();

private:
	// This tracks how many times this single bullet has 
	// struck. This is for penetration, so the bullet can
	// go through things.
	int		m_iImpacts;
	CBaseEntity *p_eInflictor;
};
#endif
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONAR2_H
#define	WEAPONAR2_H

#include "basegrenade_shared.h"
#include "basehlcombatweapon.h"
#include "in_buttons.h"


class CWeaponAR2 : public CHLSelectFireMachineGun
{
public:
	DECLARE_CLASS(CWeaponAR2, CHLSelectFireMachineGun);

	CWeaponAR2();

	DECLARE_SERVERCLASS();

	void	ItemPostFrame( void );
	void	Precache( void );
	
	void	SecondaryAttack( void );
	void	DelayedAttack( void );

	void	SelectFire(void);

	const char *GetTracerType( void ) { return "AR2Tracer"; }

	void	AddViewKick( void );

	void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	void	FireNPCSecondaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	void	Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	int		GetMinBurst( void ) { return 2; }	// was 2
	int		GetMaxBurst( void ) { return 5; }	// was 5
	float	GetMinRestTime( void ) { return 0.4; }
	float	GetMaxRestTime( void ) { return 1.0; }
	float	GetFireRate(void);

	bool	CanHolster( void );
	bool	Reload( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	Activity	GetPrimaryAttackActivity( void );
	
	void	DoImpactEffect( trace_t &tr, int nDamageType );

	virtual const Vector& GetBulletSpread( void )
	{
		// Define "spread" parameters based on the "owner" and what they are doing
		static Vector plrSingleCone = VECTOR_CONE_0125DEGREES;	// Single Fire
		static Vector plrAccurateCone = VECTOR_CONE_05DEGREES;	// Zooming or ducking
		static Vector plrCone = VECTOR_CONE_1DEGREES;	// Standing, moving around, the default
		static Vector plrRunCone = VECTOR_CONE_4DEGREES;	// Player sprint accuracy
		static Vector plrJumpCone = VECTOR_CONE_15DEGREES;	// Player jump/midair accuracy
		static Vector npcCone = VECTOR_CONE_1DEGREES;	// NPC cone when standing still
		static Vector npcMoveCone = VECTOR_CONE_2DEGREES;	// NPC cone when moving
		static Vector npcConeMidAir = VECTOR_CONE_4DEGREES;	// NPC cone when rappeling
		static Vector vitalAllyCone = VECTOR_CONE_05DEGREES;	// Vital ally cone (Barney)


		if (GetOwner() && GetOwner()->IsNPC())
		{
			/*if (GetOwner() && (GetOwner()->Classify() == CLASS_PLAYER_ALLY_VITAL))	
			{
				return vitalAllyCone;
			}
			else */
			if (GetOwner()->IsMoving())
			{
				return npcMoveCone;
			}
			else if (GetOwner()->GetGroundEntity() == nullptr)
			{
				return npcConeMidAir;
			}
			else
			{
				return npcCone;
			}
		}
		//static Vector cone;

		// We must know the player "owns" the weapon before different cones may be used
		CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());
		if (pPlayer->GetGroundEntity() == nullptr)
			return plrJumpCone;
		if (pPlayer->m_nButtons & IN_SPEED)
			return plrRunCone;
		if (m_iFireMode == FIREMODE_SEMI)
			return plrSingleCone;
		if (pPlayer->m_nButtons & IN_DUCK)
			return plrAccurateCone;
		if (pPlayer->m_nButtons & IN_ZOOM)
			return plrAccurateCone;
		else
			return plrCone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

protected:

	float					m_flDelayedFire;
	bool					m_bShotDelayed;
	int						m_nVentPose;

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
};


#endif	//WEAPONAR2_H

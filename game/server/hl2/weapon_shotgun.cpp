//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A shotgun.
//
//			Primary attack: single barrel shot.
//			Secondary attack: double barrel shot.
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon_shared.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"		// For g_pGameRules
#include "in_buttons.h"
#include "soundent.h"
#include "vstdlib/random.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sk_auto_reload_time;
extern ConVar sk_plr_num_shotgun_pellets;

class CWeaponShotgun : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponShotgun, CBaseHLCombatWeapon );

	DECLARE_SERVERCLASS();

private:
	bool	m_bNeedPump;		// When emptied completely or firing secondary
	bool	m_bDelayedFire1;	// Fire primary when finished reloading
	bool	m_bDelayedFire2;	// Fire secondary when finished reloading
//	float	m_flSoonestPrimaryAttack;

public:
	void	Precache( void );

	int CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector AllyCone = VECTOR_CONE_4DEGREES;
		static Vector cone = VECTOR_CONE_6DEGREES;

		if( GetOwner() && (GetOwner()->Classify() == CLASS_PLAYER_ALLY) )	// _VITAL
		{
			// Give Alyx's shotgun blasts more a more directed punch. She needs
			// to be at least as deadly as she would be with her pistol to stay interesting (sjb)
			return AllyCone;
		}

		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	virtual int				GetMinBurst() { return 1; }
	virtual int				GetMaxBurst() { return 3; }

	virtual float			GetMinRestTime();
	virtual float			GetMaxRestTime();

	virtual float			GetFireRate( void );

	virtual void Equip(CBaseCombatCharacter *pOwner);

	bool StartReload( void );
	bool Reload( void );
	void FillClip( void );
	void FinishReload( void );
	void CheckHolsterReload( void );
	void Pump( void );
//	void WeaponIdle( void );
	void ItemHolsterFrame( void );
	void ItemPostFrame( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void DryFire( void );

	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	void Operator_ForceNPCFire( CBaseCombatCharacter  *pOperator, bool bSecondary );
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	DECLARE_ACTTABLE();

	CWeaponShotgun(void);
};

IMPLEMENT_SERVERCLASS_ST(CWeaponShotgun, DT_WeaponShotgun)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_shotgun, CWeaponShotgun );
PRECACHE_WEAPON_REGISTER(weapon_shotgun);

BEGIN_DATADESC( CWeaponShotgun )
//DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_bNeedPump, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDelayedFire1, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDelayedFire2, FIELD_BOOLEAN ),

END_DATADESC()

acttable_t	CWeaponShotgun::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

	{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

	{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
	{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
	{ ACT_RUN,						ACT_RUN_RIFLE,						true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
	{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
};

IMPLEMENT_ACTTABLE(CWeaponShotgun);

void CWeaponShotgun::Precache( void )
{
	CBaseCombatWeapon::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CWeaponShotgun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );
	pOperator->DoMuzzleFlash();
//	We need to know if he's moving or not, then do the drill
//	WeaponSound( SINGLE_NPC );
//	m_iClip1 = m_iClip1 - 1;	

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	// Only combine soldiers can double shoot, metrocops and citizens use semi auto fire instead
	if ( !npc->IsMoving() && FClassnameIs(GetOwner(), "npc_combine_s") || (GetOwner()->Classify() == CLASS_PLAYER_ALLY_VITAL) )
	{
		// Shotgunner stands still while shooting, that means he has to do a pump animation after each shot, so we let him fire double barreled shot.
		pOperator->FireBullets(sk_plr_num_shotgun_pellets.GetInt() * 2, vecShootOrigin, vecShootDir, GetBulletSpread()*1.75f, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
		WeaponSound(WPN_DOUBLE);
		m_iClip1 = m_iClip1 - 2;
		WeaponSound(SPECIAL1);
	}
	else
	{
		// Shotgunner is moving and shooting, do semi auto shooting due to not having to pump.
		pOperator->FireBullets(sk_plr_num_shotgun_pellets.GetInt() - 1, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, true, true);
		pOperator->FireBullets(1, vecShootOrigin, vecShootDir, GetBulletSpread() * 1.75f, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
		WeaponSound(SINGLE_NPC);
		m_iClip1 = m_iClip1 - 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponShotgun::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	FireNPCPrimaryAttack( pOperator, true );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			FireNPCPrimaryAttack( pOperator, false );
		}
		break;

		default:
			CBaseCombatWeapon::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:	When we shipped HL2, the shotgun weapon did not override the
//			BaseCombatWeapon default rest time of 0.3 to 0.6 seconds. When
//			NPC's fight from a stationary position, their animation events
//			govern when they fire so the rate of fire is specified by the
//			animation. When NPC's move-and-shoot, the rate of fire is 
//			specifically controlled by the shot regulator, so it's imporant
//			that GetMinRestTime and GetMaxRestTime are implemented and provide
//			reasonable defaults for the weapon. To address difficulty concerns,
//			we are going to fix the combine's rate of shotgun fire in episodic.
//			This change will not affect Alyx using a shotgun in EP1. (sjb)
//-----------------------------------------------------------------------------
float CWeaponShotgun::GetMinRestTime()
{
	if (GetOwner() && GetOwner()->Classify() == CLASS_COMBINE && FClassnameIs(GetOwner(), "npc_combine_s"))
	{
		if (g_pGameRules->IsSkillLevel(SKILL_DIABOLICAL))	//hl2_episodic.GetBool() &&
		{
			return 0.2f;
		}
		else
		{
			return 0.6f;
		}
	}

	return 1.2f;

	return BaseClass::GetMinRestTime();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CWeaponShotgun::GetMaxRestTime()
{
	if (GetOwner() && GetOwner()->Classify() == CLASS_COMBINE && FClassnameIs(GetOwner(), "npc_combine_s"))
	{
		if (g_pGameRules->IsSkillLevel(SKILL_DIABOLICAL))	//hl2_episodic.GetBool() &&
		{
			return 0.5f;
		}
		else
		{
			return 1.2f;
		}
	}

	return 1.5f;

	return BaseClass::GetMaxRestTime();
}

//-----------------------------------------------------------------------------
// Purpose: Time between successive shots in a burst. Also returned for EP2
//			with an eye to not messing up Alyx in EP1.
//-----------------------------------------------------------------------------
float CWeaponShotgun::GetFireRate()
{
	if (GetOwner() && GetOwner()->Classify() == CLASS_COMBINE && FClassnameIs(GetOwner(), "npc_combine_s"))
	{
		if ( g_pGameRules->IsSkillLevel(SKILL_DIABOLICAL) )	//hl2_episodic.GetBool() &&
		{
			return 0.4f;	// OLD: 0.8f
		}
		else
		{
			return 0.7f;	// OLD: 0.8f
		}
	}

	return 0.8f;	// was 0.7
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponShotgun::StartReload( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	// If shotgun totally emptied then a pump animation is needed
	
	//NOTENOTE: This is kinda lame because the player doesn't get strong feedback on when the reload has finished,
	//			without the pump.  Technically, it's incorrect, but it's good for feedback...

	if (m_iClip1 <= 0)
	{
		m_bNeedPump = true;
	}

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );

	// Make shotgun shell visible
	SetBodygroup(1,0);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	m_bInReload = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponShotgun::Reload( void )
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Shotgun Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim( ACT_VM_RELOAD );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();	// Reload a bit faster
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::FinishReload( void )
{
	// Make shotgun shell invisible
	SetBodygroup(1,1);

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration() - 0.02;	// Finish a bit faster
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration() - 0.02;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::FillClip( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play weapon pump anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponShotgun::Pump( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;
	
	m_bNeedPump = false;
	
	WeaponSound( SPECIAL1 );

	// Finish reload animation
	SendWeaponAnim( ACT_SHOTGUN_PUMP );

	pOwner->m_flNextAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.125f;	// 375 RPM
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.125f;
//	m_flSoonestPrimaryAttack = gpGlobals->curtime + SequenceDuration();	// approx. 0.28f 214 RPM
	m_iClip1 -= 1;

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 1.0 );
	
	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( sk_plr_num_shotgun_pellets.GetInt() - 1, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, true, true );
	pPlayer->FireBullets( 1, vecSrc, vecAiming, GetBulletSpread() * 1.75f, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);
	
	pPlayer->ViewPunch( QAngle( random->RandomFloat( -3, -2 ), random->RandomFloat( -3, 3 ), 0 ) );	// (-2, -1) (-2, 2)

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2, GetOwner() );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	if( m_iClip1 )
	{
		// No pump, primary fire is semi-auto to keep it relevent compared to buffed secondary.
		m_bNeedPump = false;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponShotgun::SecondaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	pPlayer->m_nButtons &= ~IN_ATTACK2;
	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(WPN_DOUBLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration() - 0.1;
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration() - 0.1;
//	m_flSoonestPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	m_iClip1 -= 2;	// Shotgun uses same clip for primary and secondary attacks

	Vector vecSrc	 = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	// Bullet spread 50% larger than normal (final result = 9 Degrees for the player, still a bit better than vanilla)
	pPlayer->FireBullets(sk_plr_num_shotgun_pellets.GetInt() *2, vecSrc, vecAiming, GetBulletSpread() * 1.75f, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, false, false);
	pPlayer->ViewPunch( QAngle(random->RandomFloat( -6, 6 ),0,0) );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 1.0 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_SHOTGUN, 0.2 );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	if( m_iClip1 )
	{
		// pump so long as some rounds are left.
		m_bNeedPump = true;
	}

	m_iSecondaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, false, GetClassname() );
}
	
//-----------------------------------------------------------------------------
// Purpose: Override so shotgun can do mulitple reloads in a row
//-----------------------------------------------------------------------------
void CWeaponShotgun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return;
	}

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ((pOwner->m_nButtons & IN_ATTACK ) && (m_iClip1 >=1))
		{
			m_bInReload		= false;
			if (!m_bNeedPump)
			m_bDelayedFire1 = true;
		}
		// If I'm secondary firing and have two round stop reloading, pump and fire
		else if ((pOwner->m_nButtons & IN_ATTACK2) && (m_iClip1 >= 2))
		{
			m_bInReload = false;
			if (!m_bNeedPump)
			m_bDelayedFire2 = true;
		}
//		else if ((pOwner->m_nButtons & IN_RELOAD) && (m_flNextPrimaryAttack > gpGlobals->curtime))
//		{
//			m_flNextPrimaryAttack = m_flNextPrimaryAttack - 0.01;
//		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <=0)
			{
				FinishReload();
				return;
			}
			// If clip is not full and player holds reload button, reload again.
			if (m_iClip1 < GetMaxClip1())
			{
				// Chamber a round
				if ((m_bNeedPump) && (m_iClip1 == GetMaxClip1() - 1))
				{
					FinishReload();
					return;
				}
				else
				{
					Reload();
					return;
				}
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				return;
			}
		}
	}
	else
	{			
		// Make shotgun shell invisible
		SetBodygroup(1,1);
	}

	if ((m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_iClip1 >= 1))
	{
		Pump();
		return;
	}
	
	if (((pOwner->m_nButtons & IN_ATTACK2) | (pOwner->m_nButtons & IN_ATTACK)) && (m_flNextSecondaryAttack > gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 9999.0f;
	}
	else if (((pOwner->m_nButtons & IN_ATTACK2) == false) && (pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack;
	}

	// Shotgun uses same timing and ammo for secondary attack
	if ((m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire2 = false;
		
		if ( (m_iClip1 <= 1 && UsesClipsForAmmo1()))
		{
			// If only one shell is left, do a single shot instead	
			if ( m_iClip1 == 1 )
			{
				PrimaryAttack();
			}
			else if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}

		// Fire underwater?
		else if (GetOwner()->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			if ( pOwner->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			SecondaryAttack();
		}
	}
	else if ((m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;
		if ( (m_iClip1 <= 0 && UsesClipsForAmmo1()) || ( !UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType) ) )
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
			if ( pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}
			PrimaryAttack();
		}
	}

	if ( pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else 
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if ( !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime ) 
		{
			// weapon isn't useable, switch.
			if ( !(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon( this ) )
			{
				m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime )
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle( );
		return;
	}
	/*
	//Allow a refire as fast as the player can click
	if (((pOwner->m_nButtons & IN_ATTACK) == false) || ((pOwner->m_nButtons & IN_ATTACK2) == false) && (m_flSoonestPrimaryAttack < gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ((pOwner->m_nButtons & IN_ATTACK) || ((pOwner->m_nButtons & IN_ATTACK2) == false) && (m_flNextPrimaryAttack < gpGlobals->curtime) && (m_iClip1 <= 0))
	{
		DryFire();
	}
	else if (((pOwner->m_nButtons & IN_ATTACK) == false) || ((pOwner->m_nButtons & IN_ATTACK2) == false) && (m_flSoonestPrimaryAttack > gpGlobals->curtime))
	{
		m_flNextPrimaryAttack = m_flSoonestPrimaryAttack;
	}	*/
}



//-----------------------------------------------------------------------------
// Purpose: Constructor max range was 500
//-----------------------------------------------------------------------------
CWeaponShotgun::CWeaponShotgun( void )
{
	m_bReloadsSingly = true;

	m_bNeedPump		= false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;

	m_fMinRange1		= 0.0;
	m_fMaxRange1		= 720;	//25 yards
	m_fMinRange2		= 0.0;
	m_fMaxRange2		= 200;
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponShotgun::Equip(CBaseCombatCharacter *pOwner)
{
	if (pOwner->Classify() == CLASS_PLAYER_ALLY)
	{
		// Citizens are more accurate with shotguns
		m_fMaxRange1 = 1800;	//50 yards
	}
	else if (GetOwner() && GetOwner()->Classify() == CLASS_COMBINE && FClassnameIs(GetOwner(), "npc_combine_s"))
	{
		// Combine Soldiers need to be very aggressive with this
		m_fMaxRange1 = 540;		//15 yards
	}
	else
	{
		// Metrocops need to be able to shoot from longer ranges
		m_fMaxRange1 = 1440;	//40 yards
	}

	BaseClass::Equip(pOwner);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponShotgun::ItemHolsterFrame( void )
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;
	
		if ( GetOwner() == NULL )
			return;

		if ( m_iClip1 == GetMaxClip1() )
			return;

		// Just load the clip with no animations
		int ammoFill = MIN( (GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount( GetPrimaryAmmoType() ) );
		
		GetOwner()->RemoveAmmo( ammoFill, GetPrimaryAmmoType() );
		m_iClip1 += ammoFill;
	}
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponShotgun::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 1.00, 0.25 },	//poor	24
		{ 1.00, 0.30 },	//average 20
		{ 1.00, 0.40 },	//good 15
		{ 1.00, 0.60 },	//very good	10
		{ 1.00, 1.00 },	//perfect 6
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}

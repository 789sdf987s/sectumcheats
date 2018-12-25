#pragma once
#include "Hooks.h"
class CNoSpread
{
public:
	Vector SpreadFactor(int seed);
	void NoSpread(CInput::CUserCmd* pCmd);



	void CalcClient(Vector vSpreadVec, Vector ViewIn, Vector &ViewOut);
	void CalcServer(Vector vSpreadVec, Vector ViewIn, Vector &vecSpreadDir);
	void GetSpreadVec(CInput::CUserCmd*cmd, Vector &vSpreadVec);
	void CompensateInAccuracyNumeric(CInput::CUserCmd*cmd);
	void RollSpread(CBaseCombatWeapon*localWeap, int seed, CInput::CUserCmd*cmd, Vector& pflInAngles);
}; extern CNoSpread* NoSpread;

extern float RandomFloat(float flLow, float flHigh);

extern void RandomSeed(UINT Seed);

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define MAX_RANDOM_RANGE 0x7FFFFFFFUL
#define AM (1.0/IM)
#define EPS 1.2e-7
#define RNMX (1.0-EPS) 
#define CHECK_VALID( _v ) 0
#define Assert( _exp ) ((void)0)
extern float(*pfSqrt)(float x);
#define FastSqrt(x)			(*pfSqrt)(x)
#define PI 3.14159265358979323846f
#define RAD2DEG( x ) ( ( float )( x ) * ( float )( 180.0f / ( float )( PI ) ) )
#define square( x ) ( x * x )
#define M_PI			3.14159265358979323846
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#define FLOAT32_NAN_BITS     (unsigned long)0x7FC00000
#define FLOAT32_NAN          BitsToFloat( FLOAT32_NAN_BITS )
#define VEC_T_NAN FLOAT32_NAN
#include "checksum_md5.h"
class C_Random
{
private:
	int        m_idum;
	int        m_iy;
	int        m_iv[NTAB];
	int        GenerateRandomNumber(void);

public:
	void    SetSeed(int iSeed);
	float    RandomFloat(float flMinVal = 0.0f, float flMaxVal = 1.0f);
};
extern C_Random*Random;

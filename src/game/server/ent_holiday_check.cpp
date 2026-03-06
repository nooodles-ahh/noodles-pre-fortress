//===== Copyright � 1996 - 2005, Valve Corporation, All rights reserved. ========
//
// Purpose: Checks if its a specific holiday, then
//			fires an output when reached.
//
//===============================================================================

#include "cbase.h"
#include <ctime>

enum Holiday
{
	NONE = 0,
	APRIL,
	HALLOWEEN,
	CHRISTMAS
};

unsigned int IsHoliday()
{
	time_t ltime = time(0);
	const time_t* ptime = &ltime;
	struct tm* today = localtime(ptime);
	if (today)
	{
		if ( (today->tm_mon == 3 && today->tm_mday >= 12 ) || (today->tm_mon == 4 && today->tm_mday <= 31) )
		{
			return Holiday::APRIL;
		}
		else if (today->tm_mon == 9 && today->tm_mday >= 1)
		{
			return Holiday::HALLOWEEN;
		}
		else if ( (today->tm_mon == 11 && today->tm_mday >= 1) || (today->tm_mon == 0 && today->tm_mday <= 7) )
		{
			return Holiday::CHRISTMAS;
		}
	}
	return Holiday::NONE;
}

class CHolidayEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS(CHolidayEntity, CLogicalEntity);
	DECLARE_DATADESC();

	// Input function
	//void Is_holiday(inputdata_t& inputData);
	virtual void Spawn(void);

private:

	COutputEvent	m_OnRickMay, m_OnHalloween, m_OnChristmas;	// Output event

};



LINK_ENTITY_TO_CLASS(holiday_check, CHolidayEntity);

// Start of our data description for the class
BEGIN_DATADESC(CHolidayEntity)

// Links our input name from Hammer to our input member function
//DEFINE_INPUTFUNC(FIELD_VOID, "Is_holiday", Is_holiday),

// Links our output member to the output name used by Hammer
DEFINE_OUTPUT(m_OnRickMay, "Is_rick_may_holiday"),
DEFINE_OUTPUT(m_OnHalloween, "Is_halloween_holiday"),
DEFINE_OUTPUT(m_OnChristmas, "Is_christmas_holiday")

END_DATADESC()


//Checks the date to fire a holiday specific output
//so basically either through spawning it in game or just starting a map that
//already has one in it
//-Nbc66
void CHolidayEntity::Spawn()
{

	CLogicalEntity::Spawn();

	// Fire an output event
	switch(IsHoliday())
	{
		case Holiday::APRIL:
			
			m_OnRickMay.FireOutput(this, this);
			break;
		case Holiday::HALLOWEEN:
			m_OnHalloween.FireOutput(this, this);
			break;
		case Holiday::CHRISTMAS:
			m_OnChristmas.FireOutput(this, this);
			break;
		default:
			break;
	}
}
#include "stdafx.h"
#include "HitStat.hpp"

HitStat::HitStat(ObjectState* object) : object(object)
{
	time = object->time;
}
bool HitStat::operator<(const HitStat& other)
{
	return time < other.time;
}
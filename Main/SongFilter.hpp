#pragma once
#include "stdafx.h"
#include "SongSelect.hpp"
#include <Beatmap/MapDatabase.hpp>

class SongFilter
{
public:
	SongFilter() = default;
	~SongFilter() = default;

	virtual Map<int32, MapIndex*> GetFiltered(const Map<int32, MapIndex*>& source) { return source; }
	virtual String GetName() { return m_name; }
	virtual bool IsAll() { return true; }

private:
	String m_name = "All";

};

class LevelFilter : public SongFilter
{
public:
	LevelFilter(uint16 level) : m_level(level) {}
	virtual Map<int32, MapIndex*> GetFiltered(const Map<int32, MapIndex*>& source) override;
	virtual String GetName() override;
	virtual bool IsAll() override;

private:
	uint16 m_level;
};
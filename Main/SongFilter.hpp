#pragma once
#include "stdafx.h"
#include "SongSelect.hpp"
#include <Beatmap/MapDatabase.hpp>

enum FilterType
{
	All,
	Folder,
	Level
};

class SongFilter
{
public:
	SongFilter() = default;
	~SongFilter() = default;

	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source) { return source; }
	virtual String GetName() { return m_name; }
	virtual bool IsAll() { return true; }
	virtual FilterType GetType() { return FilterType::All; }

private:
	String m_name = "All";

};

class LevelFilter : public SongFilter
{
public:
	LevelFilter(uint16 level) : m_level(level) {}
	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source) override;
	virtual String GetName() override;
	virtual bool IsAll() override;
	virtual FilterType GetType() { return FilterType::Level; }


private:
	uint16 m_level;
};

class FolderFilter : public SongFilter
{
public:
	FolderFilter(String folder, MapDatabase* database) : m_folder(folder), m_mapDatabase(database) {}
	virtual Map<int32, SongSelectIndex> GetFiltered(const Map<int32, SongSelectIndex>& source);
	virtual String GetName() override;
	virtual bool IsAll() override;
	virtual FilterType GetType() { return FilterType::Folder; }


private:
	String m_folder;
	MapDatabase* m_mapDatabase;

};
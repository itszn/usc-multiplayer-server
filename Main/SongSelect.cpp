#include "stdafx.h"
#include "SongSelect.hpp"
#include "TitleScreen.hpp"
#include "Application.hpp"
#include <Shared/Profiling.hpp>
#include "Scoring.hpp"
#include "Input.hpp"
#include "Game.hpp"
#include "TransitionScreen.hpp"
#include "GameConfig.hpp"
#include "SongFilter.hpp"
#include <Audio/Audio.hpp>
#ifdef _WIN32
#include "SDL_keycode.h"
#else
#include "SDL2/SDL_keycode.h"
#endif
extern "C" {
#include "lua.h"
}
#include <iterator>
/*
	Song preview player with fade-in/out
*/
class PreviewPlayer
{
public:
	void FadeTo(AudioStream stream)
	{
		// Already existing transition?
		if(m_nextStream)
		{
			if(m_currentStream)
			{
				m_currentStream.Destroy();
			}
			m_currentStream = m_nextStream;
		}
		m_nextStream = stream;
		m_nextSet = true;
		if(m_nextStream)
		{
			m_nextStream->SetVolume(0.0f);
			m_nextStream->Play();
		}
		m_fadeTimer = 0.0f;
	}
	void Update(float deltaTime)
	{
		if(m_nextSet)
		{
			m_fadeTimer += deltaTime;
			if(m_fadeTimer >= m_fadeDuration)
			{
				if(m_currentStream)
				{
					m_currentStream.Destroy();
				}
				m_currentStream = m_nextStream;
				if(m_currentStream)
					m_currentStream->SetVolume(1.0f);
				m_nextStream.Release();
				m_nextSet = false;
			}
			else
			{
				float fade = m_fadeTimer / m_fadeDuration;

				if(m_currentStream)
					m_currentStream->SetVolume(1.0f - fade);
				if(m_nextStream)
					m_nextStream->SetVolume(fade);
			}
		}
	}
	void Pause()
	{
		if(m_nextStream)
			m_nextStream->Pause();
		if(m_currentStream)
			m_currentStream->Pause();
	}
	void Restore()
	{
		if(m_nextStream)
			m_nextStream->Play();
		if(m_currentStream)
			m_currentStream->Play();
	}

private:
	static const float m_fadeDuration;
	float m_fadeTimer = 0.0f;
	AudioStream m_nextStream;
	AudioStream m_currentStream;
	bool m_nextSet = false;
};
const float PreviewPlayer::m_fadeDuration = 0.5f;

/*
	Song selection wheel
*/
class SelectionWheel
{
	// keyed on SongSelectIndex::id
	Map<int32, Ref<SongSelectItem>> m_guiElements;
	Map<int32, SongSelectIndex> m_maps;
	Map<int32, SongSelectIndex> m_mapFilter;
	bool m_filterSet = false;

	// Currently selected selection ID
	int32 m_currentlySelectedId = 0;
	// Currently selected map ID
	int32 m_currentlySelectedMapId = 0;
	// Currently selected sub-widget
	Ref<SongSelectItem> m_currentSelection;

	// Current difficulty index
	int32 m_currentlySelectedDiff = 0;

	// Style to use for everything song select related
	Ref<SongSelectStyle> m_style;
	lua_State* m_lua;

public:
	SelectionWheel(Ref<SongSelectStyle> style) : m_style(style)
	{
	}
	bool Init()
	{
		CheckedLoad(m_lua = g_application->LoadScript("songselect/songwheel"));
		lua_newtable(m_lua);
		lua_setglobal(m_lua, "songwheel");
		return true;
	}
	virtual void Render(float deltaTime)
	{
		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
	}
	~SelectionWheel()
	{
		g_gameConfig.Set(GameConfigKeys::LastSelected, m_currentlySelectedMapId);
	}
	void OnMapsAdded(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			SongSelectIndex index(m);
			m_maps.Add(index.id, index);
		}
		if(!m_currentSelection)
			AdvanceSelection(0);
		m_SetLuaMaps();
	}
	void OnMapsRemoved(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			// TODO(local): don't hard-code the id calc here, maybe make it a utility function?
			SongSelectIndex index = m_maps.at(m->selectId * 10);
			m_maps.erase(index.id);

			auto it = m_guiElements.find(index.id);
			if(it != m_guiElements.end())
			{
				// Clear selection if a removed item was selected
				if(m_currentSelection == it->second)
					m_currentSelection.Release();

				// Remove this item from the canvas that displays the items
				m_guiElements.erase(it);
			}
		}
		if(!m_maps.Contains(m_currentlySelectedId))
		{
			AdvanceSelection(1);
		}
	}
	void OnMapsUpdated(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			// TODO(local): don't hard-code the id calc here, maybe make it a utility function?
			SongSelectIndex index = m_maps.at(m->selectId * 10);

			auto it = m_guiElements.find(index.id);
			if(it != m_guiElements.end())
			{
				it->second->SetIndex(index.GetMap());
			}
		}
	}
	void OnMapsCleared(Map<int32, MapIndex*> newList)
	{
		m_currentSelection.Release();
		for(auto g : m_guiElements)
		{
			//Remove(g.second.As<GUIElementBase>());
		}
		m_guiElements.clear();
		m_filterSet = false;
		m_mapFilter.clear();
		m_maps.clear();
		for (auto m : newList)
		{
			SongSelectIndex index(m.second);
			m_maps.Add(index.id, index);
		}
		if(m_maps.size() > 0)
		{
			// Doing this here, before applying filters, causes our wheel to go
			//  back to the top when a filter should be applied
			// TODO(local): Go through everything in this file and try to clean
			//  up all calls to things like this, to keep it from updating like 7 times >.>
			//AdvanceSelection(0);
		}
		m_SetLuaMaps();
	}
	void SelectRandom()
	{
		if(m_SourceCollection().empty())
			return;
		uint32 selection = Random::IntRange(0, (int32)m_SourceCollection().size() - 1);
		auto it = m_SourceCollection().begin();
		std::advance(it, selection);
		SelectMap(it->first);
	}
	void SelectByMapId(uint32 id)
	{
		for (const auto& it : m_SourceCollection())
		{
			if (it.second.GetMap()->id == id)
			{
				SelectMap(it.first);
				break;
			}
		}
	}

	void SelectMap(int32 newIndex)
	{
		Set<int32> visibleIndices;
		auto& srcCollection = m_SourceCollection();
		auto it = srcCollection.find(newIndex);
		if(it != srcCollection.end())
		{
			const float initialSpacing = 0.65f * m_style->frameMain->GetSize().y;
			const float spacing = 0.8f * m_style->frameSub->GetSize().y;
			static const int32 numItems = 10;
			m_currentlySelectedMapId = it->second.GetMap()->id;
			int32 istart;

			//set index in lua
			uint32 selectedPosition = std::distance(srcCollection.begin(), it);
			lua_getglobal(m_lua, "set_index");
			lua_pushinteger(m_lua, selectedPosition + 1);
			if (lua_pcall(m_lua, 1, 0, 0) != 0)
			{
				Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
				assert(false);
			}

			for(istart = 0; istart > -numItems;)
			{
				if(it == srcCollection.begin())
					break;
				it--;
				istart--;
			}

			for(int32 i = istart; i <= numItems; i++)
			{
				if(it != srcCollection.end())
				{
					SongSelectIndex index = it->second;
					int32 id = index.id;

					visibleIndices.Add(id);

					// Add a new map slot
					bool newItem = m_guiElements.find(id) == m_guiElements.end();
					Ref<SongSelectItem> item = m_GetMapGUIElement(index);
					float offset = 0;
					if(i != 0)
					{
						offset = initialSpacing * Math::Sign(i) +
							spacing * (i - Math::Sign(i));
					}

					int32 z = -abs(i);
					//slot->SetZOrder(z);

					//slot->anchor = anchor;
					//slot->autoSizeX = true;
					//slot->autoSizeY = true;
					//slot->alignment = Vector2(0, 0.5f);
					if(newItem)
					{
						// Hard set target position
						//slot->offset.pos = Vector2(0, offset);
						//slot->offset.size.x = z * 50.0f;
					}
					else
					{
						// Animate towards target position
						//item->AddAnimation(Ref<IGUIAnimation>(
						//	new GUIAnimation<Vector2>(&slot->offset.pos, Vector2(0, offset), 0.1f)), true);
						//item->AddAnimation(Ref<IGUIAnimation>(
						//	new GUIAnimation<float>(&slot->offset.size.x, z * 50.0f, 0.1f)), true);
					}

					item->fade = 1.0f - ((float)abs(i) / (float)numItems);
					item->innerOffset = item->fade * 100.0f;

					if(i == 0)
					{
						m_currentlySelectedId = newIndex;
						m_OnMapSelected(index);
					}

					it++;
				}
			}
		}
		m_currentlySelectedId = newIndex;
		
		// Cleanup invisible elements
		for(auto it = m_guiElements.begin(); it != m_guiElements.end();)
		{
			if(!visibleIndices.Contains(it->first))
			{
				//Remove(it->second.As<GUIElementBase>());
				it = m_guiElements.erase(it);
				continue;
			}
			it++;
		}
	}
	void AdvanceSelection(int32 offset)
	{
		auto& srcCollection = m_SourceCollection();
		auto it = srcCollection.find(m_currentlySelectedId);
		if(it == srcCollection.end())
		{
			if(srcCollection.empty())
			{
				// Remove all elements, empty
				m_currentSelection.Release();
				//Clear();
				m_guiElements.clear();
				return;
			}
			it = srcCollection.begin();
		}
		for(uint32 i = 0; i < (uint32)abs(offset); i++)
		{
			auto itn = it;
			if(offset < 0)
			{
				if(itn == srcCollection.begin())
					break;
				itn--;
			}
			else
				itn++;
			if(itn == srcCollection.end())
				break;
			it = itn;
		}
		if(it != srcCollection.end())
		{
			SelectMap(it->first);
		}
	}
	void SelectDifficulty(int32 newDiff)
	{
		m_currentSelection->SetSelectedDifficulty(newDiff);
		m_currentlySelectedDiff = newDiff;

		Map<int32, SongSelectIndex> maps = m_SourceCollection();
		SongSelectIndex* map = maps.Find(m_currentlySelectedId);
		if(map)
		{
			OnDifficultySelected.Call(map[0].GetDifficulties()[m_currentlySelectedDiff]);
		}
	}
	void AdvanceDifficultySelection(int32 offset)
	{
		if(!m_currentSelection)
			return;
		Map<int32, SongSelectIndex> maps = m_SourceCollection();
		SongSelectIndex map = maps[m_currentlySelectedId];
		int32 newIdx = m_currentlySelectedDiff + offset;
		newIdx = Math::Clamp(newIdx, 0, (int32)map.GetDifficulties().size() - 1);
		SelectDifficulty(newIdx);
	}

	// Called when a new map is selected
	Delegate<MapIndex*> OnMapSelected;
	Delegate<DifficultyIndex*> OnDifficultySelected;

	// Set display filter
	void SetFilter(Map<int32, MapIndex *> filter)
	{
		m_mapFilter.clear();
		for (auto m : filter)
		{
			SongSelectIndex index(m.second);
			m_mapFilter.Add(index.id, index);
		}
		m_filterSet = true;
		AdvanceSelection(0);
	}
	void SetFilter(SongFilter* filter[2])
	{
		bool isFiltered = false;
		m_mapFilter = m_maps;
		for (size_t i = 0; i < 2; i++)
		{
			if (!filter[i])
				continue;
			m_mapFilter = filter[i]->GetFiltered(m_mapFilter);
			if (!filter[i]->IsAll())
				isFiltered = true;
		}
		m_filterSet = isFiltered;
		m_SetLuaMaps();
		AdvanceSelection(0);
	}
	void ClearFilter()
	{
		if(m_filterSet)
		{
			m_filterSet = false;
			AdvanceSelection(0);
		}
	}

	MapIndex* GetSelection() const
	{
		SongSelectIndex const* map = m_SourceCollection().Find(m_currentlySelectedId);
		if(map)
			return map->GetMap();
		return nullptr;
	}
	DifficultyIndex* GetSelectedDifficulty() const
	{
		SongSelectIndex const* map = m_SourceCollection().Find(m_currentlySelectedId);
		if(map)
			return map->GetDifficulties()[m_currentlySelectedDiff];
		return nullptr;
	}

private:
	const Map<int32, SongSelectIndex>& m_SourceCollection() const
	{
		return m_filterSet ? m_mapFilter : m_maps;
	}
	void m_PushStringToTable(const char* name, const char* data)
	{
		lua_pushstring(m_lua, name);
		lua_pushstring(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushFloatToTable(const char* name, float data)
	{
		lua_pushstring(m_lua, name);
		lua_pushnumber(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushIntToTable(const char* name, int data)
	{
		lua_pushstring(m_lua, name);
		lua_pushinteger(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_SetLuaMaps()
	{
		lua_getglobal(m_lua, "songwheel");
		lua_pushstring(m_lua, "songs");
		lua_newtable(m_lua);
		int songIndex = 0;
		for (auto& song : m_SourceCollection())
		{
			lua_pushinteger(m_lua, ++songIndex);
			lua_newtable(m_lua);
			m_PushStringToTable("title", song.second.GetDifficulties()[0]->settings.title.c_str());
			m_PushStringToTable("artist", song.second.GetDifficulties()[0]->settings.artist.c_str());
			m_PushStringToTable("bpm", song.second.GetDifficulties()[0]->settings.bpm.c_str());
			m_PushIntToTable("id", song.second.GetMap()->id);
			m_PushStringToTable("path", song.second.GetMap()->path.c_str());
			int diffIndex = 0;
			lua_pushstring(m_lua, "difficulties");
			lua_newtable(m_lua);
			for (auto& diff : song.second.GetDifficulties())
			{
				lua_pushinteger(m_lua, ++diffIndex);
				lua_newtable(m_lua);
				auto settings = diff->settings;
				m_PushStringToTable("jacketPath", Path::Normalize(song.second.GetMap()->path + "/" + settings.jacketPath).c_str());
				m_PushIntToTable("level", settings.level);
				m_PushIntToTable("difficulty", settings.difficulty);
				m_PushIntToTable("id", diff->id);
				m_PushStringToTable("effector", settings.effector.c_str());
				lua_pushstring(m_lua, "scores");
				lua_newtable(m_lua);
				int scoreIndex = 0;
				for (auto& score : diff->scores)
				{
					lua_pushinteger(m_lua, ++scoreIndex);
					lua_newtable(m_lua);
					m_PushFloatToTable("gauge", score->gauge);
					m_PushIntToTable("flags", score->gameflags);
					m_PushIntToTable("score", score->score);
					lua_settable(m_lua, -3);
				}
				lua_settable(m_lua, -3);
				lua_settable(m_lua, -3);
			}
			lua_settable(m_lua, -3);
			lua_settable(m_lua, -3);
		}
		lua_settable(m_lua, -3);
		lua_setglobal(m_lua, "songwheel");
	}
	Ref<SongSelectItem> m_GetMapGUIElement(SongSelectIndex index)
	{
		auto it = m_guiElements.find(index.id);
		if(it != m_guiElements.end())
			return it->second;

		Ref<SongSelectItem> newItem = Ref<SongSelectItem>(new SongSelectItem(m_style));

		// Send first map as metadata settings
		const BeatmapSettings& firstSettings = index.GetDifficulties()[0]->settings;
		newItem->SetIndex(index);
		m_guiElements.Add(index.id, newItem);
		return newItem;
	}
	// TODO(local): pretty sure this should be m_OnIndexSelected, and we should filter a call to OnMapSelected
	void m_OnMapSelected(SongSelectIndex index)
	{
		// Update compact mode selection views
		if(m_currentSelection)
			m_currentSelection->SwitchCompact(true);
		m_currentSelection = m_guiElements[index.id];
		m_currentSelection->SwitchCompact(false);

		//if(map && map->id == m_currentlySelectedId)
		//	return;

		// Clamp diff selection
		int32 selectDiff = m_currentlySelectedDiff;
		if(m_currentlySelectedDiff >= (int32)index.GetDifficulties().size())
		{
			selectDiff = (int32)index.GetDifficulties().size() - 1;
		}
		SelectDifficulty(selectDiff);

		OnMapSelected.Call(index.GetMap());
	}
};


/*
	Filter selection element
*/
class FilterSelection
{
public:
	FilterSelection(Ref<SelectionWheel> selectionWheel) : m_selectionWheel(selectionWheel)
	{

	}
	bool Init()
	{
		SongFilter* lvFilter = new SongFilter();
		SongFilter* flFilter = new SongFilter();

		AddFilter(lvFilter, FilterType::Level);
		AddFilter(flFilter, FilterType::Folder);
		for (size_t i = 1; i <= 20; i++)
		{
			AddFilter(new LevelFilter(i), FilterType::Level);
		}
		CheckedLoad(m_lua = g_application->LoadScript("songselect/filterwheel"));
		return true;
	}
	virtual void Render(float deltaTime)
	{
		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		lua_pushboolean(m_lua, Active);
		if (lua_pcall(m_lua, 2, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
	}
	~FilterSelection()
	{
		g_gameConfig.Set(GameConfigKeys::FolderFilter, m_currentFolderSelection);
		g_gameConfig.Set(GameConfigKeys::LevelFilter, m_currentLevelSelection);
		m_levelFilters.clear();
		m_folderFilters.clear();
		m_guiElements.clear();
	}

	bool Active = false;

	bool IsAll()
	{
		bool isFiltered = false;
		for (size_t i = 0; i < 2; i++)
		{
			if (!m_currentFilters[i]->IsAll())
				return true;
		}
		return false;
	}

	void AddFilter(SongFilter* filter, FilterType type)
	{
		if (type == FilterType::Level)
			m_levelFilters.Add(filter);
		else
			m_folderFilters.Add(filter);
	}

	void SelectFilter(SongFilter* filter, FilterType type)
	{
		uint8 t = type == FilterType::Folder ? 0 : 1;
		int index = 0;
		if (type == FilterType::Folder)
		{
			index = std::find(m_folderFilters.begin(), m_folderFilters.end(), filter) - m_folderFilters.begin();
		}
		else
		{
			index = std::find(m_levelFilters.begin(), m_levelFilters.end(), filter) - m_levelFilters.begin();
		}


		lua_getglobal(m_lua, "set_selection");
		lua_pushnumber(m_lua, index + 1);
		lua_pushboolean(m_lua, type == FilterType::Folder);
		if (lua_pcall(m_lua, 2, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
		m_currentFilters[t] = filter;
		m_selectionWheel->SetFilter(m_currentFilters);
	}

	void SetFiltersByIndex(uint32 level, uint32 folder)
	{
		if (level >= m_levelFilters.size())
		{
			Log("LevelFilter out of range.", Logger::Severity::Error);
		}
		else
		{
			m_currentLevelSelection = level;
			SelectFilter(m_levelFilters[level], FilterType::Level);
		}

		if (folder >= m_folderFilters.size())
		{
			Log("FolderFilter out of range.", Logger::Severity::Error);
		}
		else
		{
			m_currentFolderSelection = folder;
			SelectFilter(m_folderFilters[folder], FilterType::Folder);
		}
	}

	void AdvanceSelection(int32 offset)
	{
		if (m_selectingFolders)
		{
			m_currentFolderSelection = ((int)m_currentFolderSelection + offset) % (int)m_folderFilters.size();
			if (m_currentFolderSelection < 0)
				m_currentFolderSelection = m_folderFilters.size() + m_currentFolderSelection;
			SelectFilter(m_folderFilters[m_currentFolderSelection], FilterType::Folder);
		}
		else
		{
			m_currentLevelSelection = ((int)m_currentLevelSelection + offset) % (int)m_levelFilters.size();
			if (m_currentLevelSelection < 0)
				m_currentLevelSelection = m_levelFilters.size() + m_currentLevelSelection;
			SelectFilter(m_levelFilters[m_currentLevelSelection], FilterType::Level);
		}
	}

	void SetMapDB(MapDatabase* db)
	{
		m_mapDB = db;
		for (String p : Path::GetSubDirs(g_gameConfig.GetString(GameConfigKeys::SongFolder)))
		{
			SongFilter* filter = new FolderFilter(p, m_mapDB);
			if(filter->GetFiltered(Map<int32, SongSelectIndex>()).size() > 0)
				AddFilter(filter, FilterType::Folder);
		}
		m_SetLuaTable();
	}

	void ToggleSelectionMode()
	{
		m_selectingFolders = !m_selectingFolders;
		lua_getglobal(m_lua, "set_mode");
		lua_pushboolean(m_lua, m_selectingFolders);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
	}

	String GetStatusText()
	{
		return Utility::Sprintf("%s / %s", m_currentFilters[0]->GetName(), m_currentFilters[1]->GetName());
	}

private:

	void m_PushStringToTable(const char* name, const char* data)
	{
		lua_pushstring(m_lua, name);
		lua_pushstring(m_lua, data);
		lua_settable(m_lua, -3);
	}
	void m_PushStringToArray(int index, const char* data)
	{
		lua_pushinteger(m_lua, index);
		lua_pushstring(m_lua, data);
		lua_settable(m_lua, -3);
	}

	void m_SetLuaTable()
	{
		lua_newtable(m_lua);
		{
			lua_pushstring(m_lua, "level");
			lua_newtable(m_lua); //level filters
			{
				for (size_t i = 0; i < m_levelFilters.size(); i++)
				{
					m_PushStringToArray(i + 1, m_levelFilters[i]->GetName().c_str());
				}
			}
			lua_settable(m_lua, -3);

			lua_pushstring(m_lua, "folder");
			lua_newtable(m_lua); //folder filters
			{
				for (size_t i = 0; i < m_folderFilters.size(); i++)
				{
					m_PushStringToArray(i + 1, m_folderFilters[i]->GetName().c_str());
				}
			}
			lua_settable(m_lua, -3);
		}
		lua_setglobal(m_lua, "filters");
	}

	Ref<SelectionWheel> m_selectionWheel;
	Vector<SongFilter*> m_folderFilters;
	Vector<SongFilter*> m_levelFilters;
	int32 m_currentFolderSelection = 0;
	int32 m_currentLevelSelection = 0;
	bool m_selectingFolders = true;
	Map<SongFilter*, void*> m_guiElements;
	SongFilter* m_currentFilters[2] = { nullptr };
	MapDatabase* m_mapDB;
	lua_State* m_lua;
};

class GameSettingsWheel{
public:
	GameSettingsWheel()
	{}
	bool Init()
	{
		CheckedLoad(m_lua = g_application->LoadScript("songselect/settingswheel"));
		m_gameFlags = GameFlags::None;
		AddSetting(L"Hard Guage", GameFlags::Hard);
		AddSetting(L"Mirror", GameFlags::Mirror);
		AddSetting(L"Random", GameFlags::Random);
		AddSetting(L"Auto BT (unused)", GameFlags::AutoBT);
		AddSetting(L"Auto FX (unused)", GameFlags::AutoFX);
		AddSetting(L"Auto Lasers (unused)", GameFlags::AutoLaser);
		return true;
	}
	virtual void Render(float deltaTime)
	{
		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		lua_pushboolean(m_lua, Active);
		if (lua_pcall(m_lua, 2, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}
	}
	bool Active = false;

	void AddSetting(WString name, GameFlags flag)
	{
		//Label* label = new Label();
		//label->SetFontSize(30);
		//label->SetText(Utility::WSprintf(L"%ls: Off", name));
		//m_guiElements[flag] = label;

		//Canvas::Slot* labelSlot = Add(label->MakeShared());
		//labelSlot->allowOverflow = true;
		//labelSlot->autoSizeX = true;
		//labelSlot->autoSizeY = true;
		//labelSlot->anchor = Anchors::Middle;
		//labelSlot->alignment = Vector2(0.5f, 0.5f);

		m_flagNames[flag] = name;
		SelectSetting((GameFlags)1);
	}
	void SelectSetting(GameFlags setting)
	{
		m_currentSelection = setting;
		for (size_t i = 0; i < m_guiElements.size(); i++)
		{
			Vector2 coordinate = Vector2(50, 0);
			GameFlags flag = (GameFlags)(1 << i);
			coordinate.y = ((int)i - log2((int)m_currentSelection)) * 40.f;
			//Canvas::Slot* labelSlot = (Canvas::Slot*)m_guiElements[flag]->slot;
			//AddAnimation(Ref<IGUIAnimation>(
			//	new GUIAnimation<Vector2>(&labelSlot->offset.pos, coordinate, 0.1f)), true);
		}
	}
	void ChangeSetting()
	{
		//Label* label = m_guiElements[m_currentSelection];
		if ((m_gameFlags & m_currentSelection) == GameFlags::None)
		{
			m_gameFlags = m_gameFlags | m_currentSelection;
			//label->SetText(Utility::WSprintf(L"%ls: On", m_flagNames[m_currentSelection]));
		}
		else
		{
			m_gameFlags = m_gameFlags & (~m_currentSelection);
			//label->SetText(Utility::WSprintf(L"%ls: Off", m_flagNames[m_currentSelection]));
		}
	}

	void AdvanceSelection(int32 offset)
	{
		int flag = 1 << ((int)log2((int)m_currentSelection) + offset);
		flag = Math::Clamp(flag, 1, (int)GameFlags::End - 1);
		SelectSetting((GameFlags)flag);
	}
	GameFlags GetGameFlags()
	{
		return m_gameFlags;
	}

private:
	GameFlags m_gameFlags;
	Map<GameFlags, void*> m_guiElements;
	Map<GameFlags, WString> m_flagNames;
	GameFlags m_currentSelection;
	lua_State* m_lua;
};

/*
	Song select window/screen
*/
class SongSelect_Impl : public SongSelect
{
private:
	Timer m_dbUpdateTimer;
	Ref<Canvas> m_canvas;
	MapDatabase m_mapDatabase;

	Ref<SongSelectStyle> m_style;
	Ref<CommonGUIStyle> m_commonGUIStyle;

	// Shows additional information about a map
	Ref<SongStatistics> m_statisticsWindow;
	// Map selection wheel
	Ref<SelectionWheel> m_selectionWheel;
	// Filter selection
	Ref<FilterSelection> m_filterSelection;
	// Game settings wheel
	Ref<GameSettingsWheel> m_settingsWheel;
	// Search field
	//Ref<TextInputField> m_searchField;
	// Panel to fade out selection wheel
	//Ref<Panel> m_fadePanel;
	
	//Ref<Label> m_filterStatus;

	// Score list canvas
	Ref<Canvas> m_scoreCanvas;
	//Ref<LayoutBox> m_scoreList;

	// Player of preview music
	PreviewPlayer m_previewPlayer;

	// Current map that has music being preview played
	MapIndex* m_currentPreviewAudio;

	// Select sound
	Sample m_selectSound;

	// Navigation variables
	float m_advanceSong = 0.0f;
	float m_advanceDiff = 0.0f;
	MouseLockHandle m_lockMouse;
	bool m_suspended = false;
	bool m_previewLoaded = true;
	bool m_showScores = false;
	uint64_t m_previewDelayTicks = 0;
	Map<Input::Button, float> m_timeSinceButtonPressed;
	Map<Input::Button, float> m_timeSinceButtonReleased;
	lua_State* m_lua;

public:
	bool Init() override
	{
		m_commonGUIStyle = g_commonGUIStyle;
		m_style = SongSelectStyle::Get(g_application);
		CheckedLoad(m_lua = g_application->LoadScript("songselect/background"));
		g_input.OnButtonPressed.Add(this, &SongSelect_Impl::m_OnButtonPressed);
		g_input.OnButtonReleased.Add(this, &SongSelect_Impl::m_OnButtonReleased);
		m_settingsWheel = Ref<GameSettingsWheel>(new GameSettingsWheel());
		if (!m_settingsWheel->Init())
			return false;
		m_selectSound = g_audio->CreateSample("audio/menu_click.wav");
		m_selectionWheel = Ref<SelectionWheel>(new SelectionWheel(m_style));
		if (!m_selectionWheel->Init())
			return false;
		m_filterSelection = Ref<FilterSelection>(new FilterSelection(m_selectionWheel));
		if (!m_filterSelection->Init())
			return false;
		m_filterSelection->SetMapDB(&m_mapDatabase);
		m_selectionWheel->OnMapSelected.Add(this, &SongSelect_Impl::OnMapSelected);
		m_selectionWheel->OnDifficultySelected.Add(this, &SongSelect_Impl::OnDifficultySelected);
		// Setup the map database
		m_mapDatabase.AddSearchPath(g_gameConfig.GetString(GameConfigKeys::SongFolder));

		m_mapDatabase.OnMapsAdded.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsAdded);
		m_mapDatabase.OnMapsUpdated.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsUpdated);
		m_mapDatabase.OnMapsCleared.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsCleared);
		m_mapDatabase.StartSearching();

		m_filterSelection->SetFiltersByIndex(g_gameConfig.GetInt(GameConfigKeys::LevelFilter), g_gameConfig.GetInt(GameConfigKeys::FolderFilter));
		m_selectionWheel->SelectByMapId(g_gameConfig.GetInt(GameConfigKeys::LastSelected));

		/// TODO: Check if debugmute is enabled
		g_audio->SetGlobalVolume(g_gameConfig.GetFloat(GameConfigKeys::MasterVolume));

		return true;
	}
	~SongSelect_Impl()
	{
		// Clear callbacks
		m_mapDatabase.OnMapsCleared.Clear();
		g_input.OnButtonPressed.RemoveAll(this);
		g_input.OnButtonReleased.RemoveAll(this);
		m_selectionWheel.Destroy();
		m_filterSelection.Destroy();
	}

	// When a map is selected in the song wheel
	void OnMapSelected(MapIndex* map)
	{
		if (map == m_currentPreviewAudio){
			if (m_previewDelayTicks){
				--m_previewDelayTicks;
			}else if (!m_previewLoaded){
				// Set current preview audio
				DifficultyIndex* previewDiff = m_currentPreviewAudio->difficulties[0];
				String audioPath = m_currentPreviewAudio->path + Path::sep + previewDiff->settings.audioNoFX;

				AudioStream previewAudio = g_audio->CreateStream(audioPath);
				if (previewAudio)
				{
					previewAudio->SetPosition(previewDiff->settings.previewOffset);
					m_previewPlayer.FadeTo(previewAudio);
				}
				else
				{
					Logf("Failed to load preview audio from [%s]", Logger::Warning, audioPath);
					m_previewPlayer.FadeTo(AudioStream());
				}
				m_previewLoaded = true;
				// m_previewPlayer.Restore();
			}
		} else{
			// Wait at least 15 ticks before attempting to load song to prevent loading songs while scrolling very fast
			m_previewDelayTicks = 15;
			m_currentPreviewAudio = map;
			m_previewLoaded = false;
		}
	}
	// When a difficulty is selected in the song wheel
	void OnDifficultySelected(DifficultyIndex* diff)
	{
		//m_scoreList->Clear();
		uint32 place = 1;

		for (auto it = diff->scores.rbegin(); it != diff->scores.rend(); ++it)
		{
			ScoreIndex s = **it;

			WString grade = Utility::ConvertToWString(Scoring::CalculateGrade(s.score));
			int badge = Scoring::CalculateBadge(s);

			WString badges[] = { L"P", L"U", L"HC", L"C", L"c" };

			//Label* text = new Label();
			//text->SetText(Utility::WSprintf(L"--%d--\n%08d\n%d%% %ls\n%ls",place, s.score, (int)(s.gauge * 100), badges[badge], grade));
			//text->SetFontSize(32);
			//LayoutBox::Slot* slot = m_scoreList->Add(text->MakeShared());
			//slot->fillX = true;
			//slot->padding = Margin(10, 5, 0, 0);
			
			if (place++ > 9)
				break;
		}


	}

	/// TODO: Fix some conflicts between search field and filter selection
	void OnSearchTermChanged(const WString& search)
	{
		if(search.empty())
			m_filterSelection->AdvanceSelection(0);
		else
		{
			String utf8Search = Utility::ConvertToUTF8(search);
			Map<int32, MapIndex*> filter = m_mapDatabase.FindMaps(utf8Search);
			m_selectionWheel->SetFilter(filter);
		}
	}
    

    void m_OnButtonPressed(Input::Button buttonCode)
    {
		if (m_suspended)
			return;
		//if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard /*&& m_searchField->HasInputFocus()*/)
		//	return;

		m_timeSinceButtonPressed[buttonCode] = 0;

	    if(buttonCode == Input::Button::BT_S && !m_filterSelection->Active && !m_settingsWheel->Active && !IsSuspended())
        {
			// NOTE(local): if filter selection is active, back doesn't work.
			// For now that sounds right, but maybe it shouldn't matter
			if (g_input.Are3BTsHeld())
			{
				m_suspended = true;
				g_application->RemoveTickable(this);
			}
			else
			{
				bool autoplay = (g_gameWindow->GetModifierKeys() & ModifierKeys::Ctrl) == ModifierKeys::Ctrl;
				MapIndex* map = m_selectionWheel->GetSelection();
				if (map)
				{
					DifficultyIndex* diff = m_selectionWheel->GetSelectedDifficulty();

					Game* game = Game::Create(*diff, m_settingsWheel->GetGameFlags());
					if (!game)
					{
						Logf("Failed to start game", Logger::Error);
						return;
					}
					game->GetScoring().autoplay = autoplay;
					m_suspended = true;

					// Transition to game
					TransitionScreen* transistion = TransitionScreen::Create(game);
					g_application->AddTickable(transistion);
				}
			}
        }
		else    
		{
			switch (buttonCode)
			{
			case Input::Button::BT_0:
			case Input::Button::BT_1:
			case Input::Button::BT_2:
			case Input::Button::BT_3:
				if (m_settingsWheel->Active)
					m_settingsWheel->ChangeSetting();
				break;

			case Input::Button::FX_1:
				if (!m_settingsWheel->Active && g_input.GetButton(Input::Button::FX_0) && !m_filterSelection->Active)
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, 0.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 1.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.75, 0.25)), true);

					m_settingsWheel->Active = true;
				}
				else if (m_settingsWheel->Active && g_input.GetButton(Input::Button::FX_0))
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, -1.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 0.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);

					m_settingsWheel->Active = false;
				}
				break;
			case Input::Button::FX_0:
				if (!m_settingsWheel->Active && g_input.GetButton(Input::Button::FX_1) && !m_filterSelection->Active)
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, 0.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 1.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.75, 0.25)), true);

					m_settingsWheel->Active = true;
				}
				else if (m_settingsWheel->Active && g_input.GetButton(Input::Button::FX_1))
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, -1.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 0.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);

					m_settingsWheel->Active = false;
				}
				break;
			case Input::Button::BT_S:
				if (m_filterSelection->Active)
				{
					m_filterSelection->ToggleSelectionMode();
				}
				if (m_settingsWheel->Active)
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, -1.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 0.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);

					m_settingsWheel->Active = false;
				}
				break;
			default:
				break;
			}

		}
    }
	void m_OnButtonReleased(Input::Button buttonCode)
	{
		if (m_suspended)
			return;
		//if (g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice) == InputDevice::Keyboard && m_searchField->HasInputFocus())
		//	return;

		m_timeSinceButtonReleased[buttonCode] = 0;

		switch (buttonCode)
		{
		case Input::Button::FX_0:
			if (m_timeSinceButtonPressed[Input::Button::FX_0] < m_timeSinceButtonPressed[Input::Button::FX_1] && !g_input.GetButton(Input::Button::FX_1) && !m_settingsWheel->Active)
			{
				if (!m_filterSelection->Active)
				{
					//g_guiRenderer->SetInputFocus(nullptr);

					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.left, 0.0, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.right, 1.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.75, 0.25)), true);
					m_filterSelection->Active = !m_filterSelection->Active;
				}
				else
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.left, -1.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.right, 0.0f, 0.2f)), true);
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);
					m_filterSelection->Active = !m_filterSelection->Active;
				}
			}
			break;
		case Input::Button::FX_1:
			if (m_timeSinceButtonPressed[Input::Button::FX_1] < m_timeSinceButtonPressed[Input::Button::FX_0] && !g_input.GetButton(Input::Button::FX_0) && !m_settingsWheel->Active)
			{
				if (!m_showScores)
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_scoreCanvas->slot)->padding.left, -200.0f, 0.2f)), true);
					m_showScores = !m_showScores;
				}
				else
				{
					//m_canvas->AddAnimation(Ref<IGUIAnimation>(
					//	new GUIAnimation<float>(&((Canvas::Slot*)m_scoreCanvas->slot)->padding.left, 0.0f, 0.2f)), true);
					m_showScores = !m_showScores;
				}
			}
			break;
		}

	}
	virtual void OnKeyPressed(int32 key)
	{
		if (m_settingsWheel->Active)
		{
			if (key == SDLK_DOWN)
			{
				m_settingsWheel->AdvanceSelection(1);
			}
			else if (key == SDLK_UP)
			{
				m_settingsWheel->AdvanceSelection(-1);
			}
			else if (key == SDLK_LEFT || key == SDLK_RIGHT)
			{
				m_settingsWheel->ChangeSetting();
			}
			else if (key == SDLK_ESCAPE)
			{
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.top, -1.0, 0.2f)), true);
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&((Canvas::Slot*)m_settingsWheel->slot)->anchor.bottom, 0.0f, 0.2f)), true);
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);

				m_settingsWheel->Active = false;
			}
		}
		else if (m_filterSelection->Active)
		{
			if (key == SDLK_DOWN)
			{
				m_filterSelection->AdvanceSelection(1);

			}
			else if (key == SDLK_UP)
			{
				m_filterSelection->AdvanceSelection(-1);
			}
			else if (key == SDLK_ESCAPE)
			{
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.left, -1.0f, 0.2f)), true);
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&((Canvas::Slot*)m_filterSelection->slot)->anchor.right, 0.0f, 0.2f)), true);
				//m_canvas->AddAnimation(Ref<IGUIAnimation>(
				//	new GUIAnimation<float>(&m_fadePanel->color.w, 0.0, 0.25)), true);
				m_filterSelection->Active = !m_filterSelection->Active;
			}
		}
		else
		{
			if (key == SDLK_DOWN)
			{
				m_selectionWheel->AdvanceSelection(1);
			}
			else if (key == SDLK_UP)
			{
				m_selectionWheel->AdvanceSelection(-1);
			}
			else if (key == SDLK_PAGEDOWN)
			{
				m_selectionWheel->AdvanceSelection(5);
			}
			else if (key == SDLK_PAGEUP)
			{
				m_selectionWheel->AdvanceSelection(-5);
			}
			else if (key == SDLK_LEFT)
			{
				m_selectionWheel->AdvanceDifficultySelection(-1);
			}
			else if (key == SDLK_RIGHT)
			{
				m_selectionWheel->AdvanceDifficultySelection(1);
			}
			else if (key == SDLK_F5)
			{
				m_mapDatabase.StartSearching();
			}
			else if (key == SDLK_F2)
			{
				m_selectionWheel->SelectRandom();
			}
			else if (key == SDLK_ESCAPE)
			{
				m_suspended = true;
				g_application->RemoveTickable(this);
			}
			else if (key == SDLK_TAB)
			{
				//if (m_searchField->HasInputFocus())
				//	g_guiRenderer->SetInputFocus(nullptr);
				//else
				//	g_guiRenderer->SetInputFocus(m_searchField.GetData());
			}
		}
	}
	virtual void OnKeyReleased(int32 key)
	{
		
	}
	virtual void Tick(float deltaTime) override
	{
		if(m_dbUpdateTimer.Milliseconds() > 500)
		{
			m_mapDatabase.Update();
			m_dbUpdateTimer.Restart();
		}
        
		//m_filterStatus->SetText(Utility::ConvertToWString(m_filterSelection->GetStatusText()));

        // Tick navigation
		if (!IsSuspended())
		{
			TickNavigation(deltaTime);
			m_previewPlayer.Update(deltaTime);

			// Ugly hack to get previews working with the delaty
			/// TODO: Move the ticking of the fade timer or whatever outside of onsongselected
			OnMapSelected(m_currentPreviewAudio);
		}


	}

	virtual void Render(float deltaTime)
	{
		if (IsSuspended())
			return;

		lua_getglobal(m_lua, "render");
		lua_pushnumber(m_lua, deltaTime);
		if (lua_pcall(m_lua, 1, 0, 0) != 0)
		{
			Logf("Lua error: %s", Logger::Error, lua_tostring(m_lua, -1));
			assert(false);
		}

		m_selectionWheel->Render(deltaTime);
		m_filterSelection->Render(deltaTime);
		m_settingsWheel->Render(deltaTime);
	}

    void TickNavigation(float deltaTime)
    {
		// Lock mouse to screen when active 
		if(g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice) == InputDevice::Mouse && g_gameWindow->IsActive())
		{
			if(!m_lockMouse)
				m_lockMouse = g_input.LockMouse();
		    //g_gameWindow->SetCursorVisible(false);
		}
		else
		{
			if(m_lockMouse)
				m_lockMouse.Release();
			g_gameWindow->SetCursorVisible(true);
		}
		
		for (size_t i = 0; i < (size_t)Input::Button::Length; i++)
		{
			m_timeSinceButtonPressed[(Input::Button)i] += deltaTime;
			m_timeSinceButtonReleased[(Input::Button)i] += deltaTime;
		}

        // Song navigation using laser inputs
		/// TODO: Investigate strange behaviour further and clean up.

        float diff_input = g_input.GetInputLaserDir(0);
        float song_input = g_input.GetInputLaserDir(1);
        
        m_advanceDiff += diff_input;
        m_advanceSong += song_input;

		int advanceDiffActual = (int)Math::Floor(m_advanceDiff * Math::Sign(m_advanceDiff)) * Math::Sign(m_advanceDiff);;
		int advanceSongActual = (int)Math::Floor(m_advanceSong * Math::Sign(m_advanceSong)) * Math::Sign(m_advanceSong);;
		
		if (m_settingsWheel->Active)
		{
			if (advanceDiffActual != 0)
				m_settingsWheel->ChangeSetting();
			if (advanceSongActual != 0)
				m_settingsWheel->AdvanceSelection(advanceSongActual);
		}
		else if (m_filterSelection->Active)
		{
			if (advanceDiffActual != 0)
				m_filterSelection->AdvanceSelection(advanceDiffActual);
			if (advanceSongActual != 0)
				m_filterSelection->AdvanceSelection(advanceSongActual);
		}
		else
		{
			if (advanceDiffActual != 0)
				m_selectionWheel->AdvanceDifficultySelection(advanceDiffActual);
			if (advanceSongActual != 0)
				m_selectionWheel->AdvanceSelection(advanceSongActual);
		}
        
		m_advanceDiff -= advanceDiffActual;
        m_advanceSong -= advanceSongActual;
    }

	virtual void OnSuspend()
	{
		m_suspended = true;
		m_previewPlayer.Pause();
		m_mapDatabase.StopSearching();
		if (m_lockMouse)
			m_lockMouse.Release();

		//g_rootCanvas->Remove(m_canvas.As<GUIElementBase>());
	}
	virtual void OnRestore()
	{
		m_suspended = false;
		m_previewPlayer.Restore();
		m_mapDatabase.StartSearching();

		//OnSearchTermChanged(m_searchField->GetText());
		//
		//Canvas::Slot* slot = g_rootCanvas->Add(m_canvas.As<GUIElementBase>());
		//slot->anchor = Anchors::Full;
	}
};

SongSelect* SongSelect::Create()
{
	SongSelect_Impl* impl = new SongSelect_Impl();
	return impl;
}

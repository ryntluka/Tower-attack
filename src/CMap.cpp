/**
 * @author Lukáš Rynt <ryntluka@fit.cvut.cz>
 * @date 24.4.2020
 */

#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <zconf.h>
#include "CMap.hpp"
#include "CMageTower.hpp"

using namespace std;
/**********************************************************************************************************************/
// INIT
CMap::CMap()
	: m_Cols(0),
	  m_Rows(0)
{}

void CMap::AssignUnitStack(shared_ptr<CUnitStack> unitStack)
{
	m_UnitStack = move(unitStack);
}

/**********************************************************************************************************************/
// LOADING

istream & operator>>(istream & in, CMap & self)
{
	self.LoadMapInfo(in);
	self.LoadMap(in);
	self.LoadEntities(in);
	self.PlaceTroops();
	return in;
}

void CMap::PlaceTroops()
{
	for (const auto & troop : m_Troops)
		m_Map.emplace(troop->GetPosition(), troop);
}

void CMap::LoadMapInfo(istream & in)
{
	in >> m_Rows >> m_Cols >> m_Gate;
}

void CMap::LoadMap(istream & in)
{
	LoadWallLine(in, 0);
	
	for (int row = 1; row < m_Rows - 1; ++row)
		LoadMapCenter(in, row);
	
	LoadWallLine(in, m_Rows - 1);
	
	if (!FindPathsFromSpawn())
		in.setstate(ios::failbit);
}

void CMap::LoadEntities(istream & in)
{
	if (!m_UnitStack)
		in.setstate(ios::failbit);
	
	while (true)	// will end with exception earlier anyway
	{
		// check first character
		char ch;
		in >> ch;
		if (m_UnitStack->IsTrooperChar(ch))
			LoadTroops(in, ch);
		else if (m_UnitStack->IsTowerChar(ch))
			LoadTowers(in, ch);
		else
		{
			in.putback(ch);
			break;
		}
	}
}

void CMap::LoadTroops(istream & in, char ch)
{
	unique_ptr<CTrooper> trooper(m_UnitStack->CreateTrooperAt(ch));
	trooper->LoadOnMap(in);
	if (m_Map.count(trooper->GetPosition()))
		in.setstate(ios::failbit);
	m_Troops.push_back(move(trooper));
	auto path = CPath{m_Map, m_Rows, m_Cols, trooper->GetPosition(), m_Gate.Position()}.FindStraightPath();
	trooper->SetPath(path);
}

void CMap::LoadTowers(istream & in, char ch)
{
	shared_ptr<CTower> tower(m_UnitStack->CreateTowerAt(ch));
	tower->LoadOnMap(in);
	if (!m_Map.count(tower->GetPosition()) || !m_Map.at(tower->GetPosition())->IsWall())
		in.setstate(ios::failbit);
	m_Towers.push_back(tower);
	m_Map.at(tower->GetPosition()) = tower;
}

void CMap::LoadWallLine(istream & in, int row)
{
	// skip whitespaces until first character and return the character itself back
	char ch = 0;
	DeleteWs(in);
	
	// load the wall line
	for (int col = 0; col < m_Cols; ++col)
	{
		in.get(ch);
		if (m_UnitStack->IsTowerChar(ch))
			LoadEntity({col, row}, ch);
		else
			LoadWallChar(ch, {col, row});
	}
}

void CMap::LoadMapCenter(istream & in, int row)
{
	// read left wall boundary - skip enter
	DeleteWs(in);
	if (!LoadWallChar(in.get(), {0, row}))
		in.setstate(ios::failbit);
	
	// load characters inside map
	for (int col = 1; col < m_Cols - 1; ++col)
		if (!LoadCenterChar(in.get(), {col, row}))
			in.setstate(ios::failbit);
	
	// load right wall boundary
	if (!LoadWallChar(in.get(), {m_Cols - 1, row}))
		in.setstate(ios::failbit);
}

bool CMap::LoadWallChar(char ch, pos_t position)
{
	CTile tile{ch};
	if (tile.IsSpawn())
	{
		if (!InitSpawner(position, ch))
			return false;
	}
	else if (tile.IsGate())
	{
		if (!InitGatePosition(position))
			return false;
	}
	else if (!tile.IsWall())
		return false;
	m_Map.emplace(position, new CTile{tile});
	return true;
}

bool CMap::LoadCenterChar(char ch, pos_t position)
{
	// check for empty character and skip troops
	if (ch == ' ')
		return true;
	
	// try wall characters first
	if (LoadWallChar(ch, position))
		return true;
	
	// if it's not load entity
	else
		return LoadEntity(position, ch);
}

void CMap::DeleteWs(istream & in)
{
	char ch = 0;
	in >> ch;
	in.putback(ch);
}

bool CMap::InitGatePosition(pos_t position)
{
	if (m_Gate.Position() != pos::npos)
		return false;
	m_Gate.Position() = position;
	return true;
}

bool CMap::LoadEntity(pos_t position, char ch)
{
	if (!m_UnitStack)
		return false;
	
	// replace tower with walls
	if (m_UnitStack->IsTowerChar(ch))
		m_Map.emplace(position, new CTile{'#'});
	// skip troops in the loading phase
	else if (!m_UnitStack->IsTrooperChar(ch))
		return false;
	return true;
}

bool CMap::InitSpawner(pos_t position, char ch)
{
	// redefinition is not allowed
	if (m_Spawns.count(ch - '0'))
		return false;
	m_Spawns.emplace(ch - '0', position);
	return true;
}

bool CMap::CheckSpawnCount(int count) const
{
	int max = 0;
	for (const auto & it : m_Spawns)
		if (max < it.first)
			max = it.first;
	return max == count;
}

bool CMap::CheckNew() const
{
	return m_Troops.empty()
		&& m_Towers.empty()
		&& m_Gate.Full()
		&& CheckSaved();
}

bool CMap::CheckSaved() const
{
	return !(m_Gate.Position() == pos::npos)
		&& !m_Spawns.empty();
}
/**********************************************************************************************************************/
// SAVING
ostream & operator<<(ostream & out, const CMap & self)
{
	
	self.SaveMapInfo(out);
	self.SaveMap(out);
	self.SaveEntities(out);
	return out << endl;
}

void CMap::SaveMapInfo(ostream & out) const
{
	out << "(M)" << endl << m_Rows << ' ' << m_Cols << ' ' << m_Gate << endl;
}

void CMap::SaveMap(ostream & out) const
{
	for (int i = 0; i < m_Rows; ++i)
	{
		string line;
		for (int j = 0; j < m_Cols; ++j)
		{
			if (!m_Map.count({j,i}) || m_Map.at({j,i})->IsTroop())	// skip troops while saving the map
				line += ' ';
			else if (m_Map.at({j,i})->IsTower())
				line += '#';
			else
				line += m_Map.at({j, i})->GetChar();
		}
		out << line << endl;
	}
}

void CMap::SaveEntities(ostream & out) const
{
	// save troops
	for (const auto & troop : m_Troops)
		troop->SaveOnMap(out) << endl;
		
	for (const auto & tower : m_Towers)
		tower->SaveOnMap(out) << endl;
}

/**********************************************************************************************************************/
// RENDER
CBuffer CMap::CreateBuffer(int windowWidth) const
{
	CBuffer buffer{windowWidth};
	buffer += m_Gate.Render(windowWidth);
	buffer += RenderMap(windowWidth);
	return buffer;
}

CBuffer CMap::RenderMap(int windowWidth) const
{
	CBuffer buffer{windowWidth};
	for (int i = 0; i < m_Rows; ++i)
	{
		buffer.Append();
		for (int j = 0; j < m_Cols; ++j)
		{
			if (!m_Map.count({j,i}))
				buffer << " ";
			else
				buffer << *m_Map.at({j, i});
		}
	}
	buffer.Center();
	return buffer;
}

/**********************************************************************************************************************/
// UPDATE
bool CMap::Update(bool & waveOn)
{
	if (!MoveTroops())
		waveOn = false;
	if (m_Gate.Fallen())
		return false;
	if (!TowerAttack())
		waveOn = false;
	return true;
}

void CMap::Spawn(vector<unique_ptr<CTrooper>> & spawns)
{
	for (auto & trooper : spawns)
	{
		trooper->SetPath(m_Paths.at(m_Spawns.at(trooper->GetSpawn())));
		trooper->Spawn(m_Map);
//		auto it = lower_bound(m_Troops.begin(), m_Troops.end(), trooper,
//				[](const unique_ptr<CTrooper> & a, const unique_ptr<CTrooper> & b){return a->DistanceToGoal() < b->DistanceToGoal();});
//		m_Troops.emplace(it, move(trooper));
	}
}

bool CMap::FindPathsFromSpawn()
{
	for (const auto & position : m_Spawns)
	{
		auto path = CPath{m_Map, m_Rows, m_Cols, position.second, m_Gate.Position()}.FindStraightPath();
		if (path.empty())
			return false;
		m_Paths.emplace(position.second, path);
	}
	return true;
}

bool CMap::MoveTroops()
{
	bool ret = true;
	for (auto troop = m_Troops.begin(); troop != m_Troops.end();)
	{
		if (!(*troop)->Move(m_Map))
		{
			troop++;
			continue;
		}
		m_Gate.ReceiveDamage((*troop)->Attack());
		troop = m_Troops.erase(troop);
		if (m_Troops.empty())
			ret = false;
	}
	return ret;
}

bool CMap::TowerAttack()
{
	// map troopers to their positions
	unordered_map<pos_t, shared_ptr<CTrooper>> troops;
	for (auto & troop : m_Troops)
		troops.emplace(troop->GetPosition(), troop);
	
	// attack troops by towers
	for (auto & tower : m_Towers)
		tower->Attack(m_Map, troops, m_Rows, m_Cols);
	
	return CheckTroopsDeaths();
}

bool CMap::CheckTroopsDeaths()
{
	bool ret = true;
	// check if a trooper has died - if so delete him from map
	for (auto troop = m_Troops.begin(); troop != m_Troops.end();)
	{
		if (!(*troop)->Died())
		{
			troop++;
			continue;
		}
		m_Map.erase((*troop)->GetPosition());
		troop = m_Troops.erase(troop);
		if (m_Troops.empty())
			ret = false;
	}
	return ret;
}

map<int, bool> CMap::SpawnsFree() const
{
	map<int, bool> res;
	for (const auto & spawn : m_Spawns)
	{
		pos_t spawnPos = m_Paths.at(spawn.second).front();
		res.emplace(spawn.first, !m_Map.count(spawnPos));
	}
	return res;
}

/**********************************************************************************************************************/
// TESTINGc
void CMap::VisualizePath(pos_t start, pos_t goal)
{
	auto path = CPath{m_Map, m_Rows, m_Cols, start, goal}.FindStraightPath();
	while (!path.empty())
	{
		CTile tile = CTile(' ');
		m_Map.emplace(path.front(), new CTile{tile});
		path.pop_front();
	}
}

void CMap::Visualize(const std::deque<pos_t> & positions)
{
	for (const auto & pos : positions)
		if (!m_Map.count(pos))
			m_Map.emplace(pos, new CTile(' ', ETileType::BULLET, Colors::BG_RED));
}

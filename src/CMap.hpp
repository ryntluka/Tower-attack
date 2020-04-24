/**
 * @author Lukáš Rynt <ryntluka@fit.cvut.cz>
 * @date 24.4.2020
 */

#pragma once

#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <queue>
#include <list>
#include "CTrooper.hpp"
#include "CTower.hpp"
#include "Colors.hpp"
#include "CPath.hpp"

// Class representing the map
class CMap
{
public:
	// INIT
	explicit CMap();
	~CMap();
	CMap(const CMap & src) = delete;
	CMap & operator=(const CMap & src) = delete;
	
	// LOADING
	void LoadSavedMap(std::istream & in);
	void LoadNewMap(std::istream & in);
	void CheckSpawnCount(int count) const;
	void SetMapDimensions(int rows, int cols);
	void SetGateHealth(int hp);
	
	// SAVING
	bool Save(const char * filename) const;
	friend std::ostream & operator<<(std::ostream & out, const CMap & map);

	// RENDER
	void Render() const;
	void RenderMap() const;
	void RenderGate() const;

	// UPDATE
	void Spawn(CTrooper * trooper);
	bool Update(bool & waveOn);
	
	void VisualizePath(std::queue<pos_t> path);
private:
	// VARIABLES
	int m_Cols;								//!< map's columns
	int m_Rows;								//!< map's rows
	std::deque<CTrooper*> m_Troops;			//!< pointers to troopers on the map
	std::deque<CTower*> m_Towers;			//!< pointers to towers on the map
	std::unordered_map<pos_t,CTile> m_Map; //!< two dimensional map
	std::map<int, pos_t> m_Spawns;			//!< spawns on the map mapped to their indexes
	std::map<pos_t, std::deque<pos_t>> m_Paths;
	pos_t m_Gate;							//!< pointer to the gate, to which the troopers must get
	int m_MaxGateHp;
	int m_GateHp;
	
	// LOADING
	void LoadSavedPositions(std::istream & in);
	void AddFromSaved(pos_t position, char ch);
	
	void LoadNewPositions(std::istream & in);
	void AddFromNew(pos_t position, char ch);
	
	void InitTower(pos_t position, char ch);
	void InitSpawner(pos_t position, char ch);
	
	// UPDATE PHASE
	void MoveTroops(bool & waveOn);
	void TowerAttack();
	void FindPaths();
	void DamageGate(int damage);
	bool GateDefeated() const;
};
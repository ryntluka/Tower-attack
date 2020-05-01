/**
 * @author Lukáš Rynt <ryntluka@fit.cvut.cz>
 * @date 24.4.2020
 */

#include "CGame.hpp"
#include "ExInvalidInput.hpp"

using namespace std;

CGame::CGame()
	: m_GameOn(true),
	  m_WaveOn(false)
{}

/**********************************************************************************************************************/
// LOADING
istream & operator>>(istream & in, CGame & self)
{
	set<char> signs;
	bool end = false;
	self.m_UnitStack = make_shared<CUnitStack>();
	self.m_Map.AssignUnitStack(self.m_UnitStack);
	self.m_Waves.AssignUnitStack(self.m_UnitStack);
	
	while (!end)
	{
		// load signature char
		char ch = CGame::LoadSignatureChar(in);
		if (!signs.insert(ch).second)
		{
			in.setstate(ios::failbit);
			return in;
		}
		
		// load appropriate object
		switch (ch)
		{
			case 'U':
				if (!(in >> *self.m_UnitStack))
					return in;
				break;
			case 'W':
				if (!(in >> self.m_Waves))
					return in;
				break;
			case 'M':
				if (!(in >> self.m_Map))
					return in;
				break;
			default:
				end = true;
		}
	}
	
	// check eof and signature chars
	if (!in.eof() || !self.CheckLoaded(signs))
	{
		in.setstate(ios::failbit);
		return in;
	}
	else
		in.clear(ios::goodbit);
	
	self.m_WaveOn = self.m_Map.WaveIsRunning();
	return in;
}

bool CGame::CheckLoaded(const set<char> & signs)
{
	m_Map.CheckSpawnCount(m_Waves.GetWaveSize());
	set<char> check{'U', 'M', 'W'};
	for (const auto & ch : check)
		if (signs.find(ch) == signs.end())
			return false;
	return true;
}

char CGame::LoadSignatureChar(istream & in)
{
	char brOp, brCl, res;
	if (!(in >> brOp >> res >> brCl)
		|| brOp != '('
		|| brCl != ')')
		return 0;
	return res;
}

/**********************************************************************************************************************/
// SAVING
ostream & operator<<(ostream & out, const CGame & self)
{
	if (!(out << *self.m_UnitStack)
		|| !(out << self.m_Waves)
		|| !(out << self.m_Map))
		return out;
	return out;
}

/**********************************************************************************************************************/
// INGAME

bool CGame::IsOn() const
{
	return m_GameOn;
}

void CGame::Update()
{
	CTrooper * troop = m_Waves.Update(m_WaveOn);
	if (troop)
		m_Map.Spawn(troop);
	if (!m_Map.Update(m_WaveOn))
		End();
}

void CGame::Render() const
{
	m_Waves.Render();
	m_UnitStack->Render();
	m_Map.Render();
}

void CGame::ProcessInput(char ch)
{
	switch (ch)
	{
		case 0:
			break;
		case '1':
			m_Waves.Cycle();
			break;
		case '2':
			m_UnitStack->Cycle();
			break;
		case 'a':
			AddTroopToWave();
			break;
		case 'p':
			StartWave();
			break;
		case 'q':
			End();
			break;
		default:
			throw invalid_input("Invalid input, read the options above.");
	}
}

void CGame::StartWave()
{
	if (m_WaveOn)
		throw invalid_input("Wave already running.");
	if (m_Waves.Release())
		m_WaveOn = true;
	else
		throw invalid_input("Cannot start an empty wave.");
}

void CGame::AddTroopToWave()
{
	if (!m_Waves.AddTroop())
		throw invalid_input("Current wave is full.");
}

void CGame::End()
{
	m_GameOn = false;
}

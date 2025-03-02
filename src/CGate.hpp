/**
 * @author Lukáš Rynt <ryntluka@fit.cvut.cz>
 * @date 29.4.2020
 */

#pragma once

#include "CPosition.hpp"
#include "CBuffer.hpp"

/**
 * Represents the enemy which the user needs to destroy in order to win
 */
class CGate
{
public:
	bool Fallen() const
	{return m_Hp <= 0;}
	
	void ReceiveDamage(int damage)
	{m_Hp -= damage;}
	
	/**
	 * Creates buffer with the lives of the gate
	 * @param width Width of the screen the in which buffer will be
	 * @return Created buffer
	 */
	CBuffer Draw(size_t width) const;
	
	pos_t & Position()
	{return m_Pos;}
	
	const pos_t & Position() const
	{return m_Pos;}
	
	bool Full() const
	{return m_Hp == m_MaxHp;}
	
	/**
	 * Loads gate status from input stream
	 * @param in Input stream
	 * @param self Gate
	 * @return in
	 */
	friend std::istream & operator>>(std::istream & in, CGate & self);
	/**
	 * Saves gate status to output stream
	 * @param out Output stream
	 * @param self Gate
	 * @return out
	 */
	friend std::ostream & operator<<(std::ostream & out, const CGate & self)
	{return out << '(' << self.m_Hp << ' ' << self.m_MaxHp << ')';}
private:
	pos_t m_Pos = pos::npos;		//!< position of the gate
	int m_Hp = 0;					//!< number of current gate's health
	int m_MaxHp = 0;				//!< number of gate's max health
};
/**
 * DStarLite.
 *
 * Based on "Improved Fast Replanning for Robot Navigation in Unknown Terrain" by
 * Sven Koenig and Maxim Likhachev
 *
 * Figure 6: D* Lite: Final Version (optimized verion).
 *
 * @package		DStarLite
 * @author		Aaron Zampaglione <azampagl@gmail.com>
 * @copyright	Copyright (C) 2011 Aaron Zampaglione
 * @license		MIT
 */
#include "planner.h"
#include <algorithm>
#include <iostream>
#include <cmath>
/*
 * @var  static const double  max steps before assuming no solution possible
 */
const double Planner::MAX_STEPS = 100000000;

/**
 * Constructor.
 *
 * @param  Map*         map
 * @param  Map::Cell*   start cell
 * @param  Map::Cell*   goal cell
 */
Planner::Planner(Map* map, Map::Cell* start, Map::Cell* goal)
{
	// Clear lists
	_open_list.clear();
	_open_hash.clear();
	_path.clear();
	
	_km = 0;

	_map = map;
	_start = start;
	_goal = goal;
	_last = _start;

	// ��Ŀ��ڵ�����ʼ�ڵ�����

	_rhs(_goal, 0.0);

	_list_insert(_goal, pair<double,double>(_h(_start, _goal), 0));
}

/**
 * Deconstructor.
 */
Planner::~Planner()
{
}

/**
 * Returns the generated path.
 *
 * @return  list<Map::Cell*>
 */
list<Map::Cell*> Planner::path()
{
	return _path;
}

/**
 * Gets/Sets a new goal.
 *
 * @param   Map::Cell* [optional]   goal
 * @return  Map::Cell*              new goal
 */
Map::Cell* Planner::goal(Map::Cell* u)
{
	if (u == NULL)
		return _goal;

	// Hack implementation ��Υ�����򣬲�������ô��ֵ��
	_goal = u;

	return _goal;
}

/**
 * Replans the path.
 *
 * @return  bool   solution found
 */
bool Planner::replan()
{
	_path.clear();
	
	bool result = _compute();
	
	// Couldn't find a solution
	if ( ! result)
	  return false;

	Map::Cell* current = _start;
	_path.push_back(current);

	// Follow the path with the least cost until goal is reached
	while (current != _goal)
	{
		if (current == NULL || _g(current) == Math::INF)
		{
			cout << "�ڵ����" << endl;
			cout << current << endl;
			/*cout << _g(current) << endl;*/
			return false;
		}
		// printf(" �滮���� λ��( %d ,%d ,%d )�����\n", current->x(), current->y(), current->z());

		current = _min_succ(current).first;

		_path.push_back(current);
	}

	return true;
}

/**
 * Gets/Sets start.
 *
 * @param   Map::Cell* [optional]   new start
 * @return  Map::Cell*              start
 */
Map::Cell* Planner::start(Map::Cell* u)
{
	if (u == NULL)
		return _start;

	_start = u;

	return _start;
}

/**
 * Update map.
 *
 * @param   Map::Cell*   cell to update
 * @param   double       new cost of the cell
 * @return  void
 */
void Planner::update(Map::Cell* u, double cost)
{
	// wyl�� Ϊʲô�������޸�Ŀ���Ĵ���ֵ��
	// 20180301 ��ѡ��ע�͵�
	/*
	if (u == _goal)
		return;
	*/

	// Update km
	_km += _h(_last, _start);
	_last = _start;

	_cell(u);

	double cost_old = u->cost;
	double cost_new = cost;
	u->cost = cost;

	Map::Cell** sucs = u->sucs();
	Map::Cell** pres = u->pres();

	double tmp_cost_old, tmp_cost_new;
	double tmp_rhs, tmp_g;

	// Update u
	for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
	{
		if (sucs[i] != NULL)
		{
			// line 40: edge cost δ����
			u->cost = cost_old;
			tmp_cost_old = _cost(u, sucs[i]);
			// line 42: edge cost δ����
			u->cost = cost_new;
			tmp_cost_new = _cost(u, sucs[i]);

			tmp_rhs = _rhs(u);
			tmp_g = _g(sucs[i]);

			if (Math::greater(tmp_cost_old, tmp_cost_new))
			{
				if (u != _goal)
				{
					_rhs(u, min(tmp_rhs, (tmp_cost_new + tmp_g)));
				}
			}
			else if (Math::equals(tmp_rhs, (tmp_cost_old + tmp_g)))
			{
				if (u != _goal)
				{
					_rhs(u, _min_succ(u).second);
				}
			}
		}
	}

	for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
	{
		if (pres[i] != NULL)
		{
			// line 40: edge cost δ����
			u->cost = cost_old;
			tmp_cost_old = _cost(u, pres[i]);
			// line 42: edge cost δ����
			u->cost = cost_new;
			tmp_cost_new = _cost(u, pres[i]);

			tmp_rhs = _rhs(u);
			tmp_g = _g(pres[i]);

			if (Math::greater(tmp_cost_old, tmp_cost_new))
			{
				if (u != _goal)
				{
					_rhs(u, min(tmp_rhs, (tmp_cost_new + tmp_g)));
				}
			}
			else if (Math::equals(tmp_rhs, (tmp_cost_old + tmp_g)))
			{
				if (u != _goal)
				{
					_rhs(u, _min_succ(u).second);
				}
			}
		}
	}


	_update(u);

	// Update neighbors
	for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
	{
		if (pres[i] != NULL)
		{
			u->cost = cost_old;
			tmp_cost_old = _cost(u, pres[i]);
			u->cost = cost_new;
			tmp_cost_new = _cost(u, pres[i]);

			tmp_rhs = _rhs(pres[i]);
			tmp_g = _g(u);

			if (Math::greater(tmp_cost_old, tmp_cost_new))
			{
				if (pres[i] != _goal)
				{
					_rhs(pres[i], min(tmp_rhs, (tmp_cost_new + tmp_g)));
				}
			}
			else if (Math::equals(tmp_rhs, (tmp_cost_old + tmp_g)))
			{
				if (pres[i] != _goal)
				{
					_rhs(pres[i], _min_succ(pres[i]).second);
				}
			}

			_update(pres[i]);
		}
	}

	for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
	{
		if (sucs[i] != NULL)
		{
			u->cost = cost_old;
			tmp_cost_old = _cost(u, sucs[i]);
			u->cost = cost_new;
			tmp_cost_new = _cost(u, sucs[i]);

			tmp_rhs = _rhs(sucs[i]);
			tmp_g = _g(u);

			if (Math::greater(tmp_cost_old, tmp_cost_new))
			{
				if (sucs[i] != _goal)
				{
					_rhs(sucs[i], min(tmp_rhs, (tmp_cost_new + tmp_g)));
				}
			}
			else if (Math::equals(tmp_rhs, (tmp_cost_old + tmp_g)))
			{
				if (sucs[i] != _goal)
				{
					_rhs(sucs[i], _min_succ(sucs[i]).second);
				}
			}

			_update(sucs[i]);
		}
	}

}

/**
 * Generates a cell.
 *
 * @param   Map::Cell*
 * @return  void
 */
void Planner::_cell(Map::Cell* u)
{
	if (_cell_hash.find(u) != _cell_hash.end())
		return;
	
	double h = Math::INF;
	_cell_hash[u] = pair<double,double>(h, h);
}

/**
 * Computes shortest path.
 *
 * @return  bool   successful
 */
bool Planner::_compute()
{
	if (_open_list.empty())
		return false;

	KeyCompare key_compare;

	int attempts = 0;

	Map::Cell* u;
	pair<double,double> k_old;
	pair<double,double> k_new;
	Map::Cell** pres;
	double g_old;
	double tmp_g, tmp_rhs;
	long int timeCount = 0;
	
	while (( ! _open_list.empty() && key_compare(_open_list.begin()->first, _k(_start))) || ! Math::equals(_rhs(_start), _g(_start)))
	{
		
		if (timeCount % 10000 == 0)
		{
			cout << "I am alive!" << endl;
		}
		// Reached max steps, quit
		// wyl: ����·���У����ͣ������
		if (++attempts > Planner::MAX_STEPS)
			return false;

		u = _open_list.begin()->second;
		k_old = _open_list.begin()->first;
		k_new = _k(u);

		tmp_rhs = _rhs(u);
		tmp_g = _g(u);
		
		if (key_compare(k_old, k_new))
		{
			_list_update(u, k_new);
		}
		else if (Math::greater(tmp_g, tmp_rhs))
		{
			timeCount++;
			_g(u, tmp_rhs);
			tmp_g = tmp_rhs;

			_list_remove(u);

			pres = u->pres();

			for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
			{
				
				if (pres[i] != NULL)
				{
					pres[i]->expandedStatus = true; // wyl ������ʴ���
					if (pres[i] != _goal)
					{
						_rhs(pres[i], min(_rhs(pres[i]), _cost(pres[i], u) + tmp_g));
					}

					_update(pres[i]);
				}
			}
		}
		else
		{
			g_old = tmp_g;
			_g(u, Math::INF);

			// Perform action for u
			if (u != _goal)
			{
				_rhs(u, _min_succ(u).second);
			}
			_update(u);

			pres = u->pres();
			// Perform action for neighbors
			for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
			{
				if (pres[i] != NULL)
				{
					pres[i]->expandedStatus = true; // wyl ������ʴ���
					if (Math::equals(_rhs(pres[i]), (_cost(pres[i], u) + g_old)))
					{
						if (pres[i] != _goal)
						{
							_rhs(pres[i], _min_succ(pres[i]).second);
						}
					}

					_update(pres[i]);
				}
			}
		}
	}
	printf(" ����planner��������: %d \n", timeCount);

	printf(" ��������: %d \n", _cell_hash.size()); 
	return true;
}

/**
 * Calculates the cost from one cell to another cell.
 * 
 * @param   Map::Cell*   cell a
 * @param   Map::Cell*   cell b
 * @return  double       cost between a and b
 */
double Planner::_cost(Map::Cell* a, Map::Cell* b)
{
	if (a->cost == Map::Cell::COST_UNWALKABLE || b->cost == Map::Cell::COST_UNWALKABLE)
		return Map::Cell::COST_UNWALKABLE;

	unsigned int dx = labs(a->x() - b->x());
	unsigned int dy = labs(a->y() - b->y());
	unsigned int dz = labs(a->z() - b->z());
	
	
	// Ϊ��������Ŀ�������Z��չ������Ŀ��㸽����dz�Ĳ����Ƚ�С

	//int goal_x = _goal->x();
	//int goal_y = _goal->y();
	//int goal_z = _goal->z();

	//double Edgedis = sqrt(pow(goal_x- b->x(),2)+ pow(goal_y- b->y(), 2) + pow(goal_z- b->z(), 2))+0.000001;  //0.00001����ֹ��ĸΪ0
	//double Linedis = sqrt(pow(goal_x - b->x(), 2) + pow(goal_y - b->y(), 2));
	//double sinValue = Linedis / Edgedis;
	//double ratio = 1 - 0.8*cos(asin(sinValue));

	/*
	if ((dx + dy) > 1)
	{
		scale = Math::SQRT2;
	}
	*/
	// scale = sqrt(pow(dx, 2) + pow(dy, 2)) + dz*ratio;
	/*scale = dx + dy + dz;*/
	double scale = dx + dy + dz*0.05;
	if (((a->cost + b->cost) / 2.0) < 1.0)
		printf(" ע�⣺costС�ڵ���2������ ");
	return scale * ((a->cost + b->cost)/2.0);
	/*return scale *b->cost;*/
}

/**
 * Gets/Sets g value for a cell.
 * 
 * @param   Map::Cell*          cell to retrieve/update
 * @param   double [optional]   new g value
 * @return  double              g value 
 */
double Planner::_g(Map::Cell* u, double value)
{
	_cell(u);
	pair<double,double>* g_rhs = &_cell_hash[u];

	if (value != DBL_MIN)
	{
		g_rhs->first = value;
	}

	return g_rhs->first;
}

/**
 * Calculates heuristic between two cells (manhattan distance).
 *
 * @param   Map::Cell*   cell a
 * @param   Map::Cell*   cell b
 * @return  double       heuristic value
 */
double Planner::_h(Map::Cell* a, Map::Cell* b)
{

	unsigned int dx = labs(a->x() - b->x());
	unsigned int dy = labs(a->y() - b->y());
	unsigned int dz = labs(a->z() - b->z());

	/*unsigned int min = labs(a->x() - b->x());
	unsigned int max = labs(a->y() - b->y());
	
	if (min > max)
	{
		unsigned int tmp = min;
		min = max;
		max = tmp;
	}
	
	return ((Math::SQRT2 - 1.0) * min + max);
	*/

	// return sqrt(pow(dx, 2) + pow(dy, 2)) + dz / 5.0;
	// return scale *b->cost;

	// double scale = sqrt(pow(dx, 2) + pow(dy, 2));
	double scale = dx+dy;
	//return scale * ((a->cost + b->cost) / 2);
	return scale;
}

/**
 * Calculates key value for cell.
 *
 * @param   Map::Cell*            cell to calculate for
 * @return  pair<double,double>   key value
 */
pair<double,double> Planner::_k(Map::Cell* u)
{
	double g = _g(u);
	double rhs = _rhs(u);
	double min = (g < rhs) ? g : rhs;
	return pair<double,double>((min + _h(_start, u) + _km), min);
}

/**
 * Inserts cell into open list.
 *
 * @param   Map::Cell*            cell to insert
 * @param   pair<double,double>   key vakue for the cell
 * @return  void
 */
void Planner::_list_insert(Map::Cell* u, pair<double,double> k)
{
	OL::iterator pos = _open_list.insert(OL_PAIR(k, u));
	_open_hash[u] = pos;
}


//typedef multimap<pair<double, double>, Map::Cell*, KeyCompare> OL;
//typedef unordered_map<Map::Cell*, OL::iterator, Map::Cell::Hash> OH;


/**
 * Removes cell from the open list.
 *
 * @param   Map::Cell*   cell to remove
 * @return  void
 */
void Planner::_list_remove(Map::Cell* u)
{
	_open_list.erase(_open_hash[u]);
	_open_hash.erase(_open_hash.find(u));
}

/**
 * Updates cell in the open list.
 *
 * @param   Map::Cell*
 * @param   pair<double,double>
 * @return  void
 */
void Planner::_list_update(Map::Cell* u, pair<double,double> k)
{
	OL::iterator pos1 = _open_hash[u];
	OL::iterator pos2 = pos1;

	if (pos1 == _open_list.end())
	{
		pos2 = _open_list.end();
	}
	else
	{
		pos2++;
	}

	_open_list.erase(pos1);
	_open_hash[u] = _open_list.insert(pos2, OL_PAIR(k, u));
}

/**
 * Finds the minimum successor cell.
 *
 * @param   Map::Cell*            root
 * @return  <Map::Cell*,double>   successor
 */
pair<Map::Cell*,double> Planner::_min_succ(Map::Cell* u)
{
	Map::Cell** sucs = u->sucs();

	double tmp_cost, tmp_g;
	
	Map::Cell* min_cell = NULL;
	double min_cost = Math::INF;

	for (unsigned int i = 0; i < Map::Cell::NUM_NBRS; i++)
	{
		if (sucs[i] != NULL)
		{
			sucs[i]-> expandedStatus= true;
			tmp_cost = _cost(u, sucs[i]);
			tmp_g = _g(sucs[i]);

			if (tmp_cost == Math::INF || tmp_g == Math::INF)
				continue;
			
			tmp_cost += tmp_g;

			if (tmp_cost < min_cost)
			{
				min_cell = sucs[i];
				min_cost = tmp_cost;
			}
		}
	}

	return pair<Map::Cell*,double>(min_cell, min_cost);
}

/**
 * Gets/Sets rhs value for a cell.
 * 
 * @param   Map::Cell*          cell to retrieve/update
 * @param   double [optional]   new rhs value
 * @return  double              rhs value
 */
double Planner::_rhs(Map::Cell* u, double value)
{
	if (u == _goal)
		return 0;

	_cell(u);  // u������_cell_hash���򲻴���
	pair<double,double>* g_rhs = &_cell_hash[u];

	if (value != DBL_MIN)  //valueΪ0���򲻸�ֵ
	{
		g_rhs->second = value;
	}
	
	return g_rhs->second;
}

/**
 * Updates cell.
 *
 * @param   Map::Cell*   cell to update
 * @return  void
 */
void Planner::_update(Map::Cell* u)
{
	bool diff = _g(u) != _rhs(u);
	bool exists = (_open_hash.find(u) != _open_hash.end());

	if (diff && exists)
	{
		_list_update(u, _k(u));
	}
	else if (diff && ! exists)
	{
		_list_insert(u, _k(u));
	}
	else if ( ! diff && exists)
	{
		_list_remove(u);
	}
}

/**
 * Key compare function.
 */
bool Planner::KeyCompare::operator()(const pair<double,double>& p1, const pair<double,double>& p2) const
{
	if (Math::less(p1.first, p2.first))				return true;
	else if (Math::greater(p1.first, p2.first))		return false;
	else if (Math::less(p1.second,  p2.second))		return true;
	else if (Math::greater(p1.second, p2.second))	return false;
													return false;
}

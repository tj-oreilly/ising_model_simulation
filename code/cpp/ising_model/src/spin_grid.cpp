#include "spin_grid.h"

void SpinGrid::SetSpin(std::size_t x, std::size_t y, int8_t spin)
{
	_spinGrid[x * _ySize + y] = spin;
}

int8_t SpinGrid::GetSpin(std::size_t x, std::size_t y) const
{
	return _spinGrid[x * _ySize + y];
}

NeighbourList SpinGrid::GetNearestNeighbours(std::size_t x, std::size_t y) const
{
	NeighbourList neighbours;

	if (x > 0)
		neighbours.push_back({ x - 1, y });
	if (x < _xSize - 1)
		neighbours.push_back({ x + 1, y });
	if (y > 0)
		neighbours.push_back({ x, y - 1 });
	if (y < _ySize - 1)
		neighbours.push_back({ x, y + 1 });

	return neighbours;
}

/// @brief Performs one iteration to try and change a random spin in the grid.
void SpinGrid::Iterate()
{
	std::size_t x = _rnd.GetRandInt(0, _xSize);
	std::size_t y = _rnd.GetRandInt(0, _ySize);

	//Calculate the energy change
	bool newSpin = GetSpin(x, y) * -1;

	double energyChange = 0.0;
	for (const auto& neighbour : GetNearestNeighbours(x, y))
		energyChange += -2 * newSpin * GetSpin(neighbour.first, neighbour.second);
	energyChange *= _interactionStrength;

	//Whether to flip spin
	if (energyChange <= 0.0 || _rnd.Get01() <= exp(-energyChange * _beta))
	{
		SetSpin(x, y, newSpin);
	}
}
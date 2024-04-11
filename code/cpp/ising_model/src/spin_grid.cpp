#include "spin_grid.h"

void SpinGrid::SetTemperature(double kBT)
{
	if (kBT > 0.0)
	{
		_beta = 1.0 / kBT;

		//Calculate probabilities
		_exps[0] = exp(-4.0 * _beta);
		_exps[1] = exp(-8.0 * _beta);
	}
}

void SpinGrid::SetSpin(std::size_t x, std::size_t y, int8_t spin)
{
	_spinGrid[x * _ySize + y] = spin;
}

int8_t SpinGrid::GetSpin(std::size_t x, std::size_t y) const
{
	return _spinGrid[x * _ySize + y];
}

void SpinGrid::SetGrid(const std::vector<int8_t>& grid)
{
	if (grid.size() == _xSize * _ySize)
	{
		_spinGrid = grid;
		CalculateTotalEnergy();
	}
}

// Periodic Boundary Conditions needed due to the method of calculating probability
NeighbourList SpinGrid::GetNearestNeighbours(std::size_t x, std::size_t y) const
{
	NeighbourList neighbours;
	
	//Periodic boundary condition
	if (x > 0)
		neighbours.push_back({ x - 1, y });
	else
		neighbours.push_back({ _xSize - 1, y });

	if (x < _xSize - 1)
		neighbours.push_back({ x + 1, y });
	else
		neighbours.push_back({ 0, y });

	if (y > 0)
		neighbours.push_back({ x, y - 1 });
	else
		neighbours.push_back({ x, _ySize - 1 });

	if (y < _ySize - 1)
		neighbours.push_back({ x, y + 1 });
	else
		neighbours.push_back({ x, 0 });

	return neighbours;
}

double SpinGrid::CalculateEnergy(std::size_t x, std::size_t y) const
{
	const int8_t spinValue = GetSpin(x, y);
	double energy = 0.0;
	for (const auto& neighbour : GetNearestNeighbours(x, y))
		energy += -spinValue * GetSpin(neighbour.first, neighbour.second);

	return energy * _interactionStrength;
}

void SpinGrid::CalculateTotalEnergy()
{
	_totalEnergy = 0.0;

	for (std::size_t x = 0; x < _xSize; ++x)
	{
		for (std::size_t y = 0; y < _ySize; ++y)
		{
			_totalEnergy += CalculateEnergy(x, y);
		}
	}

	_totalEnergy /= 2; //Double counted pairs
}

void SpinGrid::CalculateMagnetisation()
{
	_totalMagnetisation = 0;

	for (std::size_t x = 0; x < _xSize; ++x)
	{
		for (std::size_t y = 0; y < _ySize; ++y)
		{
			_totalMagnetisation += GetSpin(x, y);
		}
	}
}

double SpinGrid::GetTotalEnergy() const
{
	return _totalEnergy;
}

int64_t SpinGrid::GetMagnetisation() const
{
	return _totalMagnetisation;
}

/// @brief Performs one iteration to try and change a random spin in the grid.
void SpinGrid::Iterate()
{
	std::size_t x = _rnd.GetRandInt(0, _xSize);
	std::size_t y = _rnd.GetRandInt(0, _ySize);

	//Calculate the energy change
	int8_t newSpin = GetSpin(x, y) * -1;
	double energyChange = -2 * CalculateEnergy(x, y);

	//Use cached exponentials
	double expValue;
	if (energyChange == 4.0)
		expValue = _exps[0];
	else
		expValue = _exps[1];

	//Whether to flip spin
	if (energyChange <= 0.0 || _rnd.Get01() <= expValue)
	{
		SetSpin(x, y, newSpin);
		_totalEnergy += energyChange;
		_totalMagnetisation += 2 * newSpin;

#ifdef _DEBUG
		double cacheEnergy = _totalEnergy;
		CalculateTotalEnergy();

		if (cacheEnergy != _totalEnergy)
			printf("Energy change calculation error.");
#endif
	}
}
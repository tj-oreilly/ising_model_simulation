#include "spin_grid.h"

int8_t SpinGrid::SpinAt(std::size_t index) const
{
	return _spinGrid.at(index);
}

void SpinGrid::SetMagneticField(double fieldStrength)
{
	_magneticField = fieldStrength;
}

void SpinGrid::SetTemperature(double kBT)
{
	if (kBT > 0.0)
	{
		_beta = 1.0 / kBT;

		//Calculate probabilities
		//_exps[0] = exp(-4.0 * _beta);
		//_exps[1] = exp(-8.0 * _beta);
	}
}

void SpinGrid::SetGrid(const std::vector<int8_t>& grid)
{
	if (grid.size() == _spinGrid.size())
	{
		_spinGrid = grid;
		CalculateTotalEnergy();
		CalculateMagnetisation();
	}
}

double SpinGrid::CalculateEnergy(std::size_t index) const
{
	const int8_t spinValue = SpinAt(index);

	double energy = 0.0;
	for (std::size_t neighbour : GetNearestNeighbours(index))
		energy += -spinValue * SpinAt(neighbour);

	energy *= _interactionStrength;
	energy += -_magneticField * spinValue;

	return energy;
}

void SpinGrid::CalculateTotalEnergy()
{
	_totalEnergy = 0.0;

	for (std::size_t i = 0; i < _spinGrid.size(); ++i)
	{
		_totalEnergy += CalculateEnergy(i);
	}

	_totalEnergy /= 2; //Double counted pairs (true in 3D?)
}

void SpinGrid::CalculateMagnetisation()
{
	_totalMagnetisation = 0;

	for (std::size_t i = 0; i < _spinGrid.size(); ++i)
	{
		_totalMagnetisation += SpinAt(i);
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
	int64_t randIndex = _rnd.GetRandInt(0, _spinGrid.size());

	//Calculate the energy change
	int8_t newSpin = SpinAt(randIndex) * -1;
	double energyChange = -2 * CalculateEnergy(randIndex);

	//Use cached exponentials
	double expValue = exp(-energyChange * _beta);
	//if (energyChange == 4.0)
		//expValue = _exps[0];
	//else if (energyChange == 8.0)
		//expValue = _exps[1];

	//Whether to flip spin
	if (energyChange <= 0.0 || _rnd.Get01() <= expValue)
	{
		SetSpin(randIndex, newSpin);
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

void SpinGrid::SetSpin(std::size_t index, int8_t spin)
{
	_spinGrid[index] = spin;
}

//SpinGrid2D
std::size_t SpinGrid2D::IndexFromXY(int64_t x, int64_t y) const
{
	//Periodic boundary condition
	x = x < 0 ? (_xSize + x) : x;
	x = x >= _xSize ? (x - _xSize) : x;

	y = y < 0 ? (_ySize + y) : y;
	y = y >= _ySize ? (y - _ySize) : y;

	return x * _ySize + y;
}

/// @brief Gets the nearest neighbours for a specific spin (using periodic boundary condition).
/// @return 
NeighbourList SpinGrid2D::GetNearestNeighbours(std::size_t index) const
{
	//Get x and y from index
	int64_t x = index / _ySize;
	int64_t y = index - (x * _ySize);

	const std::array<Off2, 4> offsets = {
		Off2(1, 0), Off2(-1, 0), Off2(0, 1), Off2(0, -1)
	};

	NeighbourList neighbours;

	for (const Off2& off : offsets)
	{
		neighbours.push_back(IndexFromXY(x + off.x, y + off.y));
	}

	return neighbours;
}

//SpinGrid3D
std::size_t SpinGrid3D::IndexFromXYZ(int64_t x, int64_t y, int64_t z) const
{
	//Periodic boundary condition
	x = x < 0 ? (_xSize + x) : x;
	x = x >= _xSize ? (x - _xSize) : x;

	y = y < 0 ? (_ySize + y) : y;
	y = y >= _ySize ? (y - _ySize) : y;

	z = z < 0 ? (_zSize + z) : z;
	z = z >= _zSize ? (z - _zSize) : z;

	return x * _ySize * _zSize + y * _zSize + z;
}

NeighbourList SpinGrid3D::GetNearestNeighbours(std::size_t index) const
{
	//Get x, y, z from index
	int64_t x = index / (_ySize * _zSize);
	int64_t y = (index - (x * _ySize * _zSize)) / _zSize;
	int64_t z = index - (x * _ySize * _zSize + y * _zSize);

	NeighbourList neighbours;

	const std::array<Off3, 6> offsets = {
		Off3(1, 0, 0), Off3(-1, 0, 0), Off3(0, 1, 0), Off3(0, -1, 0), Off3(0, 0, 1), Off3(0, 0, -1)
	};

	NeighbourList neighbours;

	for (const Off3& off : offsets)
	{
		neighbours.push_back(IndexFromXYZ(x + off.x, y + off.y, z + off.z));
	}

	return neighbours;
}
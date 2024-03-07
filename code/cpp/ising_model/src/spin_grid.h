#pragma once
#include <memory>
#include <vector>
#include "random.h"

using NeighbourList = std::vector<std::pair<std::size_t, std::size_t>>;

class SpinGrid
{
public:
	SpinGrid(std::size_t xSize, std::size_t ySize, uint64_t seed, double interactionStrength = 1.0) : _xSize(xSize), _ySize(ySize), _interactionStrength(interactionStrength)
	{
		_spinGrid = std::vector<int8_t>(xSize * ySize);
		_rnd.Init(seed);
	}

	void SetTemperature(double kBT);
	void SetGrid(const std::vector<int8_t>& grid);
	void SetSpin(std::size_t x, std::size_t y, int8_t spin);
	int8_t GetSpin(std::size_t x, std::size_t y) const;
	NeighbourList GetNearestNeighbours(std::size_t x, std::size_t y) const;
	double CalculateEnergy(std::size_t x, std::size_t y) const;
	void CalculateTotalEnergy();
	void Iterate();
	double GetTotalEnergy() const;

private:
	std::size_t _xSize = 0;
	std::size_t _ySize = 0;
	std::vector<int8_t> _spinGrid;

	Random64 _rnd;

	double _totalEnergy = 0.0;

	double _interactionStrength = 1.0;
	double _beta = 1.0;
};
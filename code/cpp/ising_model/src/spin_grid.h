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
		_spinGrid = std::make_unique<int8_t[]>(xSize * ySize);
		_rnd.Init(seed);

		//Generate random spin arrangement
		for (std::size_t x = 0; x < _xSize; ++x)
		{
			for (std::size_t y = 0; y < _ySize; ++y)
			{
				if (_rnd.Get11() < 0)
					SetSpin(x, y, -1);
				else
					SetSpin(x, y, 1);
			}				
		}
	}

	void SetSpin(std::size_t x, std::size_t y, int8_t spin);
	int8_t GetSpin(std::size_t x, std::size_t y) const;
	NeighbourList GetNearestNeighbours(std::size_t x, std::size_t y) const;
	void Iterate();

private:
	std::size_t _xSize = 0;
	std::size_t _ySize = 0;
	std::unique_ptr<int8_t[]> _spinGrid = nullptr;

	Random64 _rnd;

	double _interactionStrength = 1.0;
	double _beta = 2.5;
};
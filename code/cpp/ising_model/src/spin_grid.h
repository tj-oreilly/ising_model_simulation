#pragma once
#include <memory>
#include <vector>
#include <array>
#include "random.h"

using NeighbourList = std::vector<std::size_t>;

struct Off2
{
	int64_t x, y;

	Off2(int64_t x_, int64_t y_) : x(x_), y(y_)
	{}
};

struct Off3
{
	int64_t x, y, z;

	Off3(int64_t x_, int64_t y_, int64_t z_) : x(x_), y(y_), z(z_)
	{}
};

class SpinGrid
{
public:
	SpinGrid(uint64_t seed, double magneticField, double interactionStrength = 1.0) : _magneticField(magneticField), _interactionStrength(interactionStrength)
	{
		_rnd.Init(seed);
	}

	void SetMagneticField(double fieldStrength);
	void SetTemperature(double kBT);
	void SetGrid(const std::vector<int8_t>& grid);
	double GetTotalEnergy() const;
	int64_t GetMagnetisation() const;
	void Iterate();

protected:
	void SetSpin(std::size_t index, int8_t spin);
	virtual NeighbourList GetNearestNeighbours(std::size_t index) const = 0;
	int8_t SpinAt(std::size_t index) const;
	double CalculateEnergy(std::size_t index) const;
	void CalculateTotalEnergy();
	void CalculateMagnetisation();

	Random64 _rnd;

	std::vector<int8_t> _spinGrid;

	double _totalEnergy = 0.0;
	int64_t _totalMagnetisation = 0;

	double _interactionStrength = 1.0;
	double _magneticField = 0.0;
	double _beta = 1.0;
};

/// @brief 2-dimensional grid of spins
class SpinGrid2D : public SpinGrid
{
public:
	SpinGrid2D(std::size_t xSize, std::size_t ySize, uint64_t seed, double magneticField, double interactionStrength = 1.0) : _xSize(xSize), _ySize(ySize), SpinGrid(seed, magneticField, interactionStrength)
	{
		_spinGrid = std::vector<int8_t>(xSize * ySize);
	}

private:
	std::size_t IndexFromXY(int64_t x, int64_t y) const;
	virtual NeighbourList GetNearestNeighbours(std::size_t index) const override;

	std::size_t _xSize = 0;
	std::size_t _ySize = 0;
};

/// @brief 3-dimensional grid of spins
class SpinGrid3D : public SpinGrid
{
public:
	SpinGrid3D(std::size_t xSize, std::size_t ySize, std::size_t zSize, uint64_t seed, double magneticField, double interactionStrength = 1.0) : _xSize(xSize), _ySize(ySize), _zSize(zSize), SpinGrid(seed, magneticField, interactionStrength)
	{
		_spinGrid = std::vector<int8_t>(xSize * ySize * zSize);
	}

private:
	std::size_t IndexFromXYZ(int64_t x, int64_t y, int64_t z) const;
	virtual NeighbourList GetNearestNeighbours(std::size_t index) const override;

	std::size_t _xSize = 0;
	std::size_t _ySize = 0;
	std::size_t _zSize = 0;
};
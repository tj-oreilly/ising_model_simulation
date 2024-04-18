#include <ctime>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <array>
#include "spin_grid.h"
#include "cqueue.h"
#include <ShObjIdl_core.h>

//#define _DEBUG
#define THREADED
#define GRID_2D
#define GRID_3D

typedef std::basic_string<wchar_t> wstring;
typedef long long Int;
typedef unsigned long long UInt;
typedef double Float;

//Constants
constexpr Int GRID_SIZE = 100;				//Size of grid to generate
constexpr Int TEMP_COUNT = 50;				//Number of temperature readings to take
constexpr Float TEMP_MIN = 1e-6;			//Minimum value for kBT
constexpr Float TEMP_MAX = 20.0;			//Maximum value for kBT
constexpr Int MAX_ITER_COUNT = 1000000000;		//Maximum iterations to run for each temperature
constexpr Int ITER_AVG = 1000000;					//Saved energy is averaged over the last n iterations
constexpr Int SAMPLE_GAP = 1000;				//Gap between samples for calculating the mean (reduces speed loss from mean check)
constexpr Int STD_DEV_COUNT = 2;			//Standard deviations for fluctuations to be considered at equilibrium.
constexpr Float MAGNETIC_FIELD = 1.0; //Magnetic field strength
constexpr std::array<int8_t, 10> SPIN_OPTIONS = { -1, -1, 1, 1, 1, 1, 1, 1, 1, 1 }; //Distribution of spins to use (biased one way)

struct ModelData
{
  Float temp = 0.0;
  Float energy = 0.0;
  Float magnetisation = 0.0;

	/// @brief Default constructor
	ModelData()
	{
	}

	ModelData(Float temp_, Float energy_, Float magnetisation_) : temp(temp_), energy(energy_), magnetisation(magnetisation_)
	{
	}
};

/// @brief Allows the user to pick a destination for a CSV file.
/// @return The chosen file name.
wstring FileSave(wstring title)
{
	COMDLG_FILTERSPEC ComDlgFS[2] =
	{
		{L"CSV file", L"*.csv"},
		{L"All Files",L"*.*"}
	};
		
	wstring saveFilename = L"";

	IFileSaveDialog* saveDlg = nullptr;
	IShellItem* shellItem = nullptr;

	HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void**)(&saveDlg));
	if (SUCCEEDED(hr))
	{
		hr = saveDlg->SetFileTypes(2, ComDlgFS);
		if (SUCCEEDED(hr))
		{
			saveDlg->SetTitle(title.begin()._Ptr);
			saveDlg->Show(0);

			saveDlg->GetResult(&shellItem);
			if (shellItem == nullptr)
				return L"";

			wchar_t* filename = nullptr;
			shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filename);
			if (filename != nullptr)
			{
				saveFilename = filename;
				CoTaskMemFree(filename);
			}

			shellItem->Release();
		}
		saveDlg->Release();
	}

	//Check file extension
	if (saveFilename.substr(saveFilename.size() - 4) != L".csv")
			saveFilename += L".csv";

	return saveFilename;
}

/// @brief Build a grid of random spin states
/// @param spins 
/// @param seed 
void GenRandomSpins(std::vector<int8_t>& spins, uint64_t seed)
{
	Random64 rnd;
	rnd.Init(seed);

#ifdef GRID_2D
	Int count = pow(GRID_SIZE, 2);
#elif
	Int count = pow(GRID_SIZE, 3);
#endif

	for (Int i = 0; i < count; ++i)
	{
		spins[i] = SPIN_OPTIONS[rnd.GetRandInt(0, SPIN_OPTIONS.size())];
	}
}

Float sqr(Float x)
{
	return x * x;
}

/// @brief Calculates the mean and standard deviation from the values in a queue.
/// @param buffer 
/// @param mean 
/// @param stdDevSq 
void CalculateMeanStdDev(const cqueue<Float>& buffer, Float& mean, Float& stdDevSq)
{
	mean = 0.0;
	Float meanSq = 0.0;

	Float buffVal;
	for (std::size_t i = 0; i < buffer.size(); ++i)
	{
		buffVal = buffer.back(i);
		mean += buffVal;
		meanSq += sqr(buffVal);
	}

	mean /= buffer.size();
	meanSq /= buffer.size();
			
	stdDevSq = sqrt(meanSq - sqr(mean));
}

void EnergyThread(SpinGrid& grid, Int tempNum, std::vector<ModelData>& energyValues, const std::vector<int8_t>& initialSpins)
{
	Float tempValue;
	int64_t startTime, endTime;

	//Timing
	auto clock = std::chrono::system_clock::now();
	startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

	//Temperature to test
	tempValue = TEMP_MIN + (TEMP_MAX - TEMP_MIN) * (Float(tempNum) / Float(TEMP_COUNT - 1));
	grid.SetTemperature(tempValue);
	grid.SetGrid(initialSpins);

	//Buffer for calculating mean energy
	const Int buffSize = ITER_AVG / SAMPLE_GAP;

	cqueue<Float> energyBuff(buffSize); //energy
	cqueue<int64_t> magnetisationBuff(buffSize);
	Float lastMean = 0.0;
	Float lastStdDev = 0.0; //Square std dev avoids need for sqrt

	//Iterations
	for (Int i = 0; i < MAX_ITER_COUNT; ++i)
	{
		grid.Iterate();

		if (i % SAMPLE_GAP == 0)
		{
			energyBuff.push_back(grid.GetTotalEnergy());
			magnetisationBuff.push_back(grid.GetMagnetisation());
		}

		//Check termination case
		if (i % ITER_AVG == 0)
		{
			Float mean, stdDev;
			CalculateMeanStdDev(energyBuff, mean, stdDev);

			if (i > 0)
			{
				const Float devLimit = STD_DEV_COUNT * (lastStdDev / sqrt(buffSize));
				if (abs(lastMean - mean) <= devLimit)
				{
					std::cout << "Exit early, " + std::to_string(i) + " iterations\n";
					break;
				}
			}
			
			lastMean = mean;
			lastStdDev = stdDev;
		}
	}

	//Calculate mean
	Float meanEnergy = 0.0;
	Int count = 0;
	while (!energyBuff.empty())
	{
		meanEnergy += energyBuff.back();
		energyBuff.pop_back();
		++count;
	}

	meanEnergy /= count;

	Float meanMagnetisation = 0.0;
	count = 0;
	while (!magnetisationBuff.empty())
	{
		meanMagnetisation += magnetisationBuff.back();
		magnetisationBuff.pop_back();
		++count;
	}

	meanMagnetisation /= count;
	
	energyValues[tempNum] = ModelData(tempValue, meanEnergy, meanMagnetisation);

	clock = std::chrono::system_clock::now();
	endTime = clock.time_since_epoch() / std::chrono::microseconds(1);

	Float timeTaken = (endTime - startTime) / 1000000.0;
	std::string prIntStr = "Temperature: " + std::to_string(tempValue) + ", Time Taken: " + std::to_string(timeTaken) + "\n";
	std::cout << prIntStr;
}

void CalculateEnergyValues(uint64_t seed, const std::vector<int8_t>& initialSpins, std::vector<ModelData>& energies)
{
	auto clock = std::chrono::system_clock::now();
	int64_t startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

#ifdef THREADED
	const UInt threadCount = std::thread::hardware_concurrency();

	//Initialise SpinGrid instance for each thread
#ifdef GRID_2D
	std::vector<SpinGrid2D> gridArray;
#elif GRID_3D
	std::vector<SpinGrid2D> gridArray;
#endif

	gridArray.reserve(threadCount);

	for (UInt i = 0; i < threadCount; ++i)
	{
#ifdef GRID_2D
		gridArray.push_back(SpinGrid2D(GRID_SIZE, GRID_SIZE, seed + (i + 1), MAGNETIC_FIELD));
#elif GRID_3D
		gridArray.push_back(SpinGrid3D(GRID_SIZE, GRID_SIZE, GRID_SIZE, seed + (i + 1), MAGNETIC_FIELD));
#endif
	}

	//Split temps by thread num
	const Int perThread = (Int)std::ceil(Float(TEMP_COUNT) / threadCount);

	//Start threading
	std::vector<std::thread> threads(threadCount);
	Int tempNum;

	for (Int i = 0; i < perThread; ++i)
	{
		for (UInt threadIndex = 0; threadIndex < threadCount; ++threadIndex)
		{
			//Wait for previous execution to finish
			if (threads[threadIndex].joinable())
				threads[threadIndex].join();

			//New thread
			tempNum = i * threadCount + threadIndex;
			if (tempNum >= TEMP_COUNT)
				continue;

			threads[threadIndex] = std::thread(EnergyThread, std::ref(gridArray[threadIndex]), tempNum, std::ref(energies), std::ref(initialSpins));
		}
	}

	for (auto& thread : threads)
	{
			if (thread.joinable())
				thread.join();
	}

#endif // THREADED

#ifndef THREADED
	SpinGrid grid(GRID_SIZE, GRID_SIZE, seed + 1);
	for (Int i = 0; i < TEMP_COUNT; ++i)
	{
		EnergyThread(grid, i, energies, initialSpins);
	}
#endif // !THREADED

	clock = std::chrono::system_clock::now();
	int64_t endTime = clock.time_since_epoch() / std::chrono::microseconds(1);
	std::cout << "Total Time: " + std::to_string((endTime - startTime) / 1000000.0) + " s\n";
}

void WriteEnergiesToFile(const std::vector<ModelData>& energies, const wstring& dest)
{
	if (dest == L"")
		return;

	std::ofstream fileStream;
	fileStream.open(dest);

	fileStream << "temp,energy\n";

	for (const ModelData& energyDat : energies)
	{
		fileStream << (std::to_string(energyDat.temp) + "," + std::to_string(energyDat.energy) + "\n");
	}

	fileStream.close();
}

void WriteMagnetisationToFile(const std::vector<ModelData>& energies, const wstring& dest)
{
  if (dest == L"")
		return;

  //Write to file
  std::ofstream fileStream;
  fileStream.open(dest);

  fileStream << "temp,magnetisation\n";

  for (Int energyIndex = 0; energyIndex < energies.size(); ++energyIndex)
  {
		fileStream << (std::to_string(energies[energyIndex].temp) + "," + std::to_string(energies[energyIndex].magnetisation) + "\n");
  }

  fileStream.close();
}

int main(int argc, char* argv[])
{
  //Initialisation required for using windows API
  HRESULT res = CoInitialize(NULL);
  if (!SUCCEEDED(res))
		return 0;

  //Get file names
  const wstring energyCsvFile = FileSave(L"Choose energy CSV file");
  const wstring magnetisationCsvFile = FileSave(L"Choose magnetisation CSV file");

  const uint64_t seed = std::time(nullptr); //Use time as base seed

  //Generate random spin arrangement
#ifdef GRID_2D
  std::vector<int8_t> randomSpins(GRID_SIZE*GRID_SIZE);
#elif GRID_3D
	std::vector<int8_t> randomSpins(GRID_SIZE * GRID_SIZE * GRID_SIZE);
#endif

  GenRandomSpins(randomSpins, seed + 1);
    
  //Main execution
  std::vector<ModelData> energies(TEMP_COUNT);
  CalculateEnergyValues(seed, randomSpins, energies);

  //Write to CSV files
  WriteEnergiesToFile(energies, energyCsvFile);
  WriteMagnetisationToFile(energies, magnetisationCsvFile);

  return 0;
}
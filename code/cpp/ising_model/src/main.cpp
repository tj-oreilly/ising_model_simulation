#include <ctime>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include "spin_grid.h"
#include "cqueue.h"
#include <ShObjIdl_core.h>

//#define _DEBUG
//#define THREADED

typedef std::basic_string<wchar_t> wstring;
typedef int Int;
typedef unsigned int UInt;

//Constants
constexpr Int GRID_SIZE = 100;          //Size of grid to generate
constexpr Int TEMP_COUNT = 100;         //Number of temperature readings to take
constexpr double TEMP_MIN = 1e-6;				//Minimum value for kBT
constexpr double TEMP_MAX = 4.0;        //Maximum value for kBT
constexpr Int MAX_ITER_COUNT = 1e9;    //Maximum iterations to run for each temperature
constexpr Int ITER_AVG = 1e2;           //Saved energy is averaged over the last n iterations
constexpr Int GRAD_AVG = 5;             //Points to average over for the gradient calculation (heat capacity)

struct EnergyValue // Does this need a new name because of adding magnetisation?
{
    double temp = 0.0;
    double energy = 0.0;
    double magnetisation = 0.0;
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

		constexpr int8_t spinStates[2] = {1, -1};

		for (Int x = 0; x < GRID_SIZE; ++x)
		{
				for (Int y = 0; y < GRID_SIZE; ++y)
				{
						spins[x * GRID_SIZE + y] = spinStates[rnd.GetRandInt(0, 2)];
				}
		}
}

double sqr(double x)
{
	return x * x;
}

/// @brief Calculates the mean and standard deviation from the values in a queue.
/// @param buffer 
/// @param mean 
/// @param stdDevSq 
void CalculateMeanStdDev(const cqueue<double>& buffer, double& mean, double& stdDevSq)
{
	mean = 0.0;
	double meanSq = 0.0;

	double buffVal;
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

void EnergyThread(SpinGrid& grid, Int tempNum, std::vector<EnergyValue>& energyValues, const std::vector<int8_t>& initialSpins)
{
	double tempValue;
	int64_t startTime, endTime;

	//Timing
	auto clock = std::chrono::system_clock::now();
	startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

	//Temperature to test
	tempValue = TEMP_MIN + (TEMP_MAX - TEMP_MIN) * (double(tempNum) / double(TEMP_COUNT - 1));
	grid.SetTemperature(tempValue);
	grid.SetGrid(initialSpins);

	//Buffer for calculating mean energy
	const Int buffSize = ITER_AVG;

	cqueue<double> buff(buffSize);
	double lastMean = 0.0;
	double lastStdDev = 0.0; //Square std dev avoids need for sqrt

	//Iterations
	for (Int i = 0; i < MAX_ITER_COUNT; ++i)
	{
		grid.Iterate();
		buff.push_back(grid.GetTotalEnergy());

		//Check termination case
		if (i % ITER_AVG == 0)
		{
			double mean, stdDev;
			CalculateMeanStdDev(buff, mean, stdDev);

			if (i > 0)
			{
				const double devLimit = (lastStdDev / sqrt(ITER_AVG));
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
	double meanEnergy = 0.0;
	Int count = 0;
	while (!buff.empty())
	{
		meanEnergy += buff.back();
		buff.pop_back();
		++count;
	}

	meanEnergy /= count;
	energyValues[tempNum] = EnergyValue({ tempValue, meanEnergy, grid.GetMagnetisation() });

	clock = std::chrono::system_clock::now();
	endTime = clock.time_since_epoch() / std::chrono::microseconds(1);

	double timeTaken = (endTime - startTime) / 1000000.0;
	std::string prIntStr = "Temperature: " + std::to_string(tempValue) + ", Time Taken: " + std::to_string(timeTaken) + "\n";
	std::cout << prIntStr;
}

void CalculateEnergyValues(uint64_t seed, const std::vector<int8_t>& initialSpins, std::vector<EnergyValue>& energies)
{
	auto clock = std::chrono::system_clock::now();
	int64_t startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

#ifdef THREADED
	const UInt threadCount = std::thread::hardware_concurrency();

	//Initialise SpinGrid instance for each thread
	std::vector<SpinGrid> gridArray;
	gridArray.reserve(threadCount);

	for (UInt i = 0; i < threadCount; ++i)
		gridArray.push_back(SpinGrid(GRID_SIZE, GRID_SIZE, seed + (i+1)));

	//Split temps by thread num
	const Int perThread = (Int)std::ceil(double(TEMP_COUNT) / threadCount);

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

void WriteEnergiesToFile(const std::vector<EnergyValue>& energies, const wstring& dest)
{
	if (dest == L"")
			return;

	std::ofstream fileStream;
	fileStream.open(dest);

	fileStream << "temp,energy\n";

	for (const EnergyValue& energyDat : energies)
	{
		fileStream << (std::to_string(energyDat.temp) + "," + std::to_string(energyDat.energy) + "\n");
	}

	fileStream.close();
}

void WriteHeatCapToFile(const std::vector<EnergyValue>& energies, const wstring& dest)
{
	if (dest == L"")
		return;

	//Calculate heat capacities
	std::vector<double> heatCapacities(energies.size());
	double gradient = 0.0;
	int8_t count = 0;

	for (Int energyIndex = 0; energyIndex < energies.size(); ++energyIndex)
	{
		gradient = 0.0;
		count = 0;

		//Take average either side
		for (Int i = energyIndex - 1; i > max(0, energyIndex - GRAD_AVG / 2); --i)
		{
			gradient += (energies[i + 1].energy - energies[i].energy) / (energies[i + 1].temp - energies[i].temp);
			++count;
		}
		for (Int i = energyIndex + 1; i < min(energies.size(), energyIndex + GRAD_AVG / 2); ++i)
		{
			gradient += (energies[i].energy - energies[i - 1].energy) / (energies[i].temp - energies[i - 1].temp);
			++count;
		}

		if (count != 0)
			heatCapacities[energyIndex] = gradient / count;
	}

	//Write to file
	std::ofstream fileStream;
	fileStream.open(dest);

	fileStream << "temp,heat-cap\n";

	for (Int energyIndex = 0; energyIndex < energies.size(); ++energyIndex)
	{
		fileStream << (std::to_string(energies[energyIndex].temp) + "," + std::to_string(heatCapacities[energyIndex]) + "\n");
	}

	fileStream.close();
}

void WriteMagnetisationToFile(const std::vector<EnergyValue>& energies, const wstring& dest)
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

Int main(Int argc, char* argv[])
{
  //Initialisation required for using windows API
  HRESULT res = CoInitialize(NULL);
  if (!SUCCEEDED(res))
      return 0;

  //Get file names
  const wstring energyCsvFile = FileSave(L"Choose energy CSV file");
  const wstring heatCsvFile = FileSave(L"Choose heat capacity CSV file");
  const wstring magnetisationCsvFile = FileSave(L"Choose magnetisation CSV file");

  const uint64_t seed = std::time(nullptr); //Use time as base seed

  //Generate random spin arrangement
  std::vector<int8_t> randomSpins(GRID_SIZE*GRID_SIZE);
  GenRandomSpins(randomSpins, seed + 1);
    
  //Main execution
  std::vector<EnergyValue> energies(TEMP_COUNT);
  CalculateEnergyValues(seed, randomSpins, energies);

  //Write to CSV files
  WriteEnergiesToFile(energies, energyCsvFile);
  WriteHeatCapToFile(energies, heatCsvFile);
  WriteMagnetisationToFile(energies, magnetisationCsvFile);

  return 0;
}
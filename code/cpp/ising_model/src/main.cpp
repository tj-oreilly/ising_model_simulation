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
#define THREADED

using wstring = std::basic_string<wchar_t>;

//Constants
constexpr int GRID_SIZE = 50;          //Size of grid to generate
constexpr int TEMP_COUNT = 16;           //Number of temperature readings to take
constexpr double TEMP_MIN = 0.0001;     //Minimum value for kBT
constexpr double TEMP_MAX = 4.0;        //Maximum value for kBT
constexpr int MAX_ITER_COUNT = 1e8;    //Maximum iterations to run for each temperature
constexpr int ITER_AVG = 1e6;           //Saved energy is averaged over the last n iterations
constexpr int AVG_STEP = 1e4;
constexpr int GRAD_AVG = 1;             //Points to average over for the gradient calculation (heat capacity)
constexpr double TOLERANCE = 1e-3;
const double MAX_ENERGY = GRID_SIZE * GRID_SIZE * 2.0;

struct EnergyValue
{
    double temp = 0.0;
    double energy = 0.0;
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

    for (int x = 0; x < GRID_SIZE; ++x)
    {
        for (int y = 0; y < GRID_SIZE; ++y)
        {
            spins[x * GRID_SIZE + y] = spinStates[rnd.GetRandInt(0, 2)];
        }
    }
}

void EnergyThread(SpinGrid& grid, int tempNum, std::vector<EnergyValue>& energyValues, const std::vector<int8_t>& initialSpins)
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
  const int buffSize = ITER_AVG / AVG_STEP;

  cqueue<double> buff(buffSize);
  double lastEnergy = 0.0;

  //Iterations
  for (int i = 0; i < MAX_ITER_COUNT; ++i)
  {
    grid.Iterate();

    if (i % AVG_STEP == 0)
        buff.push_back(grid.GetTotalEnergy());

    //Check terminating case
    if (i % ITER_AVG == 0)
    {
        double currentAvg = 0.0;
        int count = 0;
        for (int buffIndex = 0; buffIndex < buff.size(); ++buffIndex)
        {
            currentAvg += buff.back(buffIndex);
            ++count;
        }

        currentAvg /= count;

        //Check if average value has changed by enough to be counted
        if (abs(lastEnergy - currentAvg) < TOLERANCE * MAX_ENERGY)
        {
            std::cout << "Early exit\n"; //Output graph of energy against iterations to check out the stopping condition
            break; //Try running a lot more iterations (for small number of temperature points) see if it changes the graph significantly
            //RUn for different sized grids and compare. Leave running a long time when sure it's got a good stopping condition.
        }
    }
  }

  //Calculate mean
  double meanEnergy = 0.0;
  int count = 0;
  while (!buff.empty())
  {
    meanEnergy += buff.back();
    buff.pop_back();
    ++count;
  }

  meanEnergy /= count;
  energyValues[tempNum] = EnergyValue({ tempValue, meanEnergy });

  clock = std::chrono::system_clock::now();
  endTime = clock.time_since_epoch() / std::chrono::microseconds(1);

  double timeTaken = (endTime - startTime) / 1000000.0;
  std::string printStr = "Temperature: " + std::to_string(tempValue) + ", Time Taken: " + std::to_string(timeTaken) + "\n";
  std::cout << printStr;
}

void CalculateEnergyValues(uint64_t seed, const std::vector<int8_t>& initialSpins, std::vector<EnergyValue>& energies)
{
  auto clock = std::chrono::system_clock::now();
  int64_t startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

#ifdef THREADED
  const unsigned int threadCount = std::thread::hardware_concurrency();

  //Initialise SpinGrid instance for each thread
  std::vector<SpinGrid> gridArray;
  gridArray.reserve(threadCount);

  for (unsigned int i = 0; i < threadCount; ++i)
    gridArray.push_back(SpinGrid(GRID_SIZE, GRID_SIZE, seed + (i+1)));

  //Split temps by thread num
  const int perThread = (int)std::ceil(double(TEMP_COUNT) / threadCount);

  //Start threading
  std::vector<std::thread> threads(threadCount);
  int tempNum;

  for (int i = 0; i < perThread; ++i)
  {
    for (unsigned int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
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
  for (int i = 0; i < TEMP_COUNT; ++i)
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

  for (int energyIndex = 0; energyIndex < energies.size(); ++energyIndex)
  {
    gradient = 0.0;
    count = 0;

    //Take average either side
    for (int i = energyIndex - 1; i > max(0, energyIndex - GRAD_AVG / 2); --i)
    {
      gradient += (energies[i + 1].energy - energies[i].energy) / (energies[i + 1].temp - energies[i].temp);
      ++count;
    }
    for (int i = energyIndex + 1; i < min(energies.size(), energyIndex + GRAD_AVG / 2); ++i)
    {
      gradient += (energies[i].energy - energies[i - 1].energy) / (energies[i].temp - energies[i - 1].temp);
      ++count;
    }

    heatCapacities[energyIndex] = gradient / count;
  }

  //Write to file
  std::ofstream fileStream;
  fileStream.open(dest);

  fileStream << "temp,heat-cap\n";

  for (int energyIndex = 0; energyIndex < energies.size(); ++energyIndex)
  {
    fileStream << (std::to_string(energies[energyIndex].temp) + "," + std::to_string(heatCapacities[energyIndex]) + "\n");
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
  const wstring heatCsvFile = FileSave(L"Choose heat capacity CSV file");

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

  return 0;
}
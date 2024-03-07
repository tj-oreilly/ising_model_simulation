#include <ctime>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include "spin_grid.h"
#include "cqueue.h"
#include <ShObjIdl_core.h>

//#define _DEBUG

using wstring = std::basic_string<wchar_t>;

//Constants
constexpr int GRID_SIZE = 50;          //Size of grid to generate
constexpr int TEMP_COUNT = 50;          //Number of temperature readings to take
constexpr double TEMP_MIN = 0.1;        //Minimum value for kBT
constexpr double TEMP_MAX = 10.0;        //Maximum value for kBT
constexpr int ITER_COUNT = 1000000;    //Iterations to run for each temperature
constexpr int ITER_AVG = 10000;         //Saved energy is averaged over the last n iterations

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

void CalculateEnergyValues(SpinGrid& grid, const std::vector<int8_t>& initialSpins, std::vector<EnergyValue>& energies)
{
    double tempValue;
    int64_t startTime, endTime;

    for (int tempNum = 0; tempNum < TEMP_COUNT; ++tempNum)
    {
        //Timing
        auto clock = std::chrono::system_clock::now();
        startTime = clock.time_since_epoch() / std::chrono::microseconds(1);

        //Temperature to test
        tempValue = TEMP_MIN + (TEMP_MAX - TEMP_MIN) * (double(tempNum) / double(TEMP_COUNT - 1));
        grid.SetTemperature(tempValue);
        grid.SetGrid(initialSpins);

        //Buffer for calculating mean energy
        cqueue<double> buff(ITER_AVG);
        
        //Iterations (brute force for now)
        for (int i = 0; i < ITER_COUNT; ++i)
        {
            grid.Iterate();
            buff.push_back(grid.GetTotalEnergy());
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
        energies[tempNum] = EnergyValue({ tempValue, meanEnergy });

        clock = std::chrono::system_clock::now();
        endTime = clock.time_since_epoch() / std::chrono::microseconds(1);

        double timeTaken = (endTime - startTime) / 1000000.0;
        std::string printStr = "Temperature: " + std::to_string(tempValue) + ", Time Taken: " + std::to_string(timeTaken) + "\n";
        std::cout << printStr;
    }
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

int main(int argc, char* argv[])
{
    //Initialisation required for using windows API
    HRESULT res = CoInitialize(NULL);
    if (!SUCCEEDED(res))
        return 0;

    //Get file names (need extensions)
	const wstring energyCsvFile = FileSave(L"Choose energy CSV file");
    //const wstring heatCsvFile = FileSave(L"Choose heat capacity CSV file");

    const uint64_t seed = std::time(nullptr); //Use time as seed
    SpinGrid grid(GRID_SIZE, GRID_SIZE, seed);

    //Generate random spin arrangement
    std::vector<int8_t> randomSpins(GRID_SIZE*GRID_SIZE);
    GenRandomSpins(randomSpins, seed + 1);
    
    std::vector<EnergyValue> energies(TEMP_COUNT);
    CalculateEnergyValues(grid, randomSpins, energies);
    WriteEnergiesToFile(energies, energyCsvFile);

	return 0;
}
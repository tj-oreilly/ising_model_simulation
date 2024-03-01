import array, threading
import numpy.random as rand
import numpy as np

class SpinGrid():
    def __init__(self, sizeX, sizeY, bField, interactionStrength):
        self._sizeX = sizeX
        self._sizeY = sizeY

        self._bField = bField
        self._interactionStrength = interactionStrength
        self._beta = 0.0
        
        self._thd = None
        self._threadFinished = True

        self._iterationNum = 0
        self._lastAverageSpin = None
        self._lastTotalEnergy = None

        #Build grid of 0s (to be populated with -1 or +1 for spins)
        self._grid = array.array("i", [0 for i in range(sizeX * sizeY)])

    def SetTemperature(self, kBT):
        self._beta = 1.0 / (kBT)

    def SetSpin(self, xPos, yPos, value):
        if value == 1 or value == -1:
            self._grid[xPos * self._sizeY + yPos] = value

    def GetSpin(self, xPos, yPos):
        return self._grid[xPos * self._sizeY + yPos]

    def GetNearestNeighbours(self, xPos, yPos):
        neighbours = []
        if xPos > 0:
            neighbours.append((xPos - 1, yPos))
        if xPos < self._sizeX - 1:
            neighbours.append((xPos + 1, yPos))
        if yPos > 0:
            neighbours.append((xPos, yPos - 1))
        if yPos < self._sizeY - 1:
            neighbours.append((xPos, yPos + 1))

        return neighbours

    def CalculateEnergy(self):
        self._lastTotalEnergy = 0.0
        for xPos in range(self._sizeX):
            for yPos in range(self._sizeY):
                self._lastTotalEnergy += self.CalculateSingleEnergy(xPos, yPos)

        return self._lastTotalEnergy

    def CalculateSingleEnergy(self, x, y):
        energy = 0.0

        #Iterate over nearest neighbours
        for neighbour in self.GetNearestNeighbours(x, y):
            energy += -self.GetSpin(x, y) * self.GetSpin(neighbour[0], neighbour[1])
        
        energy *= self._interactionStrength
        energy += -self._bField * self.GetSpin(x, y)

        return energy

    def Iterate(self, repeats):

        self._threadFinished = False

        randomX = rand.randint(0, self._sizeX, repeats)
        randomY = rand.randint(0, self._sizeY, repeats)
        randomFloats = rand.random(repeats)

        totalSpinChange = 0
        totalEnergyChange = 0
        for i in range(repeats):
            #Choose a random spin
            xPos = randomX[i]
            yPos = randomY[i]

            #Calculate energy change if this spin flips
            newSpin = self.GetSpin(xPos, yPos) * -1
            energyChange = -4 * self.CalculateSingleEnergy(xPos, yPos)

            #Conditions to flip spin
            if energyChange <= 0 or randomFloats[i] <= np.exp(-energyChange * self._beta):
                self.SetSpin(xPos, yPos, newSpin)
                totalSpinChange += newSpin * 2
                totalEnergyChange += energyChange

        self._iterationNum += repeats

        #Update total energy
        if self._lastTotalEnergy == None:
            self.CalculateEnergy()
        else:
            self._lastTotalEnergy += totalEnergyChange

        #Calculate average spin for first time
        if self._lastAverageSpin == None:
            avgSpin = 0
            for x in range(self._sizeX):
                for y in range(self._sizeY):
                    avgSpin += self.GetSpin(x, y)
            avgSpin /= self._sizeX * self._sizeY

            self._lastAverageSpin = avgSpin
        else: #Adjust average only
            self._lastAverageSpin += totalSpinChange / (self._sizeX * self._sizeY)

        self._threadFinished = True

    def StartIterateThread(self, repeats):
        """Starts the iteration thread and doesn't start the next job until the results have been drawn.
        Parameters:
            repeats : How many iterations to perform before drawing to the screen.
        """

        if self._threadFinished:
            self._thd = threading.Thread(target=self.Iterate, args=(repeats,))
            self._thd.start()

    def IsThreadFinished(self):
        return self._threadFinished

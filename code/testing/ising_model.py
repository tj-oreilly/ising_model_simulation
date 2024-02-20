import array, threading, matplotlib, time
import numpy.random as rand
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import tkinter as tk

GRID_SIZE = 50
INTERACTION_STRENGTH = 1.0 #Factor multiplied by spins in hamiltonian
BETA = 1000 #Value of 1 / k_B * T

#2D for now
class SpinGrid():
    def __init__(self, sizeX, sizeY):
        self._sizeX = sizeX
        self._sizeY = sizeY
        self._thd = None
        self._threadFinished = True

        #Build grid of 0s (to be populated with -1 or +1 for spins)
        self._grid = array.array("i", [0 for i in range(sizeX * sizeY)])

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
        totalEnergy = 0.0
        for xPos in range(self._sizeX):
            for yPos in range(self._sizeY):
                #Iterate over nearest neighbours
                for neighbour in self.GetNearestNeighbours(xPos, yPos):
                    totalEnergy += -self.GetSpin(xPos, yPos) * self.GetSpin(neighbour[0], neighbour[1])

        return totalEnergy * INTERACTION_STRENGTH

    def Iterate(self, repeats):
        
        self._threadFinished = False
        for i in range(repeats):
            #Choose a random spin
            xPos = rand.randint(0, self._sizeX)
            yPos = rand.randint(0, self._sizeY)

            #Calculate energy change if this spin flips
            newSpin = self.GetSpin(xPos, yPos) * -1
            energyChange = 0.0
            for neighbour in self.GetNearestNeighbours(xPos, yPos):
                energyChange += -2 * newSpin * self.GetSpin(neighbour[0], neighbour[1])
            energyChange *= INTERACTION_STRENGTH

            #Conditions to flip spin
            if energyChange <= 0 or rand.random() <= np.exp(-energyChange * BETA):
                self.SetSpin(xPos, yPos, newSpin)

        self._threadFinished = True

    def StartIterateThread(self, repeats):
        if self._threadFinished:
            self._thd = threading.Thread(target=self.Iterate, args=(repeats,))
            self._thd.start()

    def Draw(self):
        #Only draws if thread has finished
        if not self._threadFinished:
            return

        #Build numpy array from array
        dispArray = np.zeros((self._sizeX, self._sizeY))
        for xPos in range(self._sizeX):
            for yPos in range(self._sizeY):
                dispArray[xPos][yPos] = self.GetSpin(xPos, yPos)

        cmap = matplotlib.colors.ListedColormap(['k', 'w']) #White and black for spin states
        ax.matshow(dispArray, cmap=cmap)

def InitSpins(grid):
    #Initial random spin arrangement 
    for i in range(grid._sizeX):
        for j in range(grid._sizeY):
            randNum = rand.randint(1, 3)
            if randNum == 1:
                grid.SetSpin(i, j, 1)
            else:
                grid.SetSpin(i, j, -1)

def update(frame):
    grid.StartIterateThread(10000)
    grid.Draw()

def quit():
    root.quit()
    root.destroy()
    
grid = SpinGrid(GRID_SIZE, GRID_SIZE)
InitSpins(grid)

fig, ax = plt.subplots()

root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", quit)
root.wm_title("Ising Model Simulation")

canvas = FigureCanvasTkAgg(fig, master=root)
canvas.get_tk_widget().grid(column=0,row=1)

#Causes window to not respond to events smoothly (move drawing code into thread as well?)
#Or handle rendering without matplotlib (could be easier)
anim = matplotlib.animation.FuncAnimation(fig, update, frames=(2**64), interval=1000, repeat=False, blit=False)

root.mainloop()
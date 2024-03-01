import array, threading, time, sys, os, copy
import numpy.random as rand
import numpy as np
import matplotlib.backends.backend_agg as agg
import matplotlib.pyplot as plt
import pygame, pygame_widgets
from pygame_widgets.button import Button

import spin_grid

#Model Parameters
GRID_SIZE = 200
INTERACTION_STRENGTH = 1.0 #Factor multiplied by spins in hamiltonian
TEMPERATURE = 100 #Temperature value
B_FIELD = 0.0 #External magnetic field

#Other options
WINDOW_SIZE = (1600,800)
FIG_DPI = 100
ITERATION_COUNT = 10000

class Graph():
    def __init__(self):
        self._pos = (0,0)
        self._size = (0,0)

        self._graphPoints = []
        self._xRange = [0,0]
        self._yRange = [0,0]

        self._fig = plt.figure(figsize=[0, 0], dpi=FIG_DPI)
        self._ax = self._fig.gca()

        self._matplotlibCache = None
        self._imgSize = None

        #Member constants
        self.BORDER = 0.05
    
    def SetSize(self, x, y, width, height):
        self._pos = (x, y)
        self._size = (width, height)
        self._fig.set_size_inches([self._size[0] / FIG_DPI, self._size[1] / FIG_DPI])

    def GetGraphText(self):
        return self._ax.get_xlabel(), self._ax.get_ylabel(), self._ax.get_title()

    def SetGraphText(self, xLabel, yLabel, title):
        self._ax.set_xlabel(xLabel)
        self._ax.set_ylabel(yLabel)
        self._ax.set_title(title)

    def AddPoint(self, x, y):
        self._graphPoints.append((x, y))

        if x < self._xRange[0]:
            self._xRange[0] = x
        elif x > self._xRange[1]:
            self._xRange[1] = x

        if y < self._yRange[0]:
            self._yRange[0] = y
        elif y > self._yRange[1]:
            self._yRange[1] = y

    def DrawMatPlotLib(self, screen, regen):
        
        if regen:
            #Cache graph text then clear old plot
            xLabel, yLabel, title = self.GetGraphText()
            self._ax.clear()
            self.SetGraphText(xLabel, yLabel, title)

            #Add all points to plot
            self._ax.plot([self._graphPoints[i][0] for i in range(len(self._graphPoints))], [self._graphPoints[i][1] for i in range(len(self._graphPoints))])

            canvas = agg.FigureCanvasAgg(self._fig)
            canvas.draw()
            renderer = canvas.get_renderer()
            self._matplotlibCache = renderer.tostring_rgb()

            self._imgSize = canvas.get_width_height()

        if self._matplotlibCache != None and self._imgSize != None:
            #Draw graph
            surf = pygame.image.fromstring(self._matplotlibCache, self._imgSize, "RGB")
            screen.blit(surf, self._pos)

class WindowHandler():
    def __init__(self, screen, StartAnim, StopAnim):
        self._screen = screen

        self._buttonWidth = 100
        self._buttonHeight = 30
        currentWindowSize = self._screen.get_size()

        self._graphDirty = True

        #Initialise button instances
        self._startButton = Button(screen, currentWindowSize[0] * 0.6, 10, self._buttonWidth, self._buttonHeight, text='Start', fontSize=20
                                   , margin=25, inactiveColour=(100, 255, 100), hoverColour=(50, 200, 50), pressedColour=(50, 200, 50), radius=10, onClick=StartAnim)

        self._stopButton = Button(screen, currentWindowSize[0] * 0.9 - self._buttonWidth, 10, self._buttonWidth, self._buttonHeight, text='Stop'
                                  , fontSize=20, margin=25, inactiveColour=(255, 100, 100), hoverColour=(200, 50, 50), pressedColour=(200, 50, 50), radius=10, onClick=StopAnim)

        #Initialise graph instances
        self._magnetisationGraph = Graph()
        self._magnetisationGraph.SetGraphText("Iterations", "Average Spin", "Average Spin Change")

        self._energyGraph = Graph()
        self._energyGraph.SetGraphText("Iterations", "Total Energy", "Total Energy Change")

    def SetGraphDirty(self):
        self._graphDirty = True

    def DrawUpdate(self, grid):
        currentWindowSize = self._screen.get_size()

        self._startButton.setX(currentWindowSize[0] * 0.6)
        self._stopButton.setX(currentWindowSize[0] * 0.9 - self._buttonWidth)
        self._screen.fill([200,200,200])

        #Text
        text_surface = main_font.render(f'Temperature: {TEMPERATURE} K    Iteration: {grid._iterationNum}', True, (0, 0, 0))
        self._screen.blit(text_surface, (50, 10))
    
        #Range for matrix
        matrixSize = np.min([currentWindowSize[0] / 2, currentWindowSize[1]]) - 100
        pixelSize = (matrixSize / grid._sizeX, matrixSize / grid._sizeY)

        for xIndex in range(grid._sizeX):
            xPos = 50 + pixelSize[0] * xIndex
            for yIndex in range(grid._sizeY):
                yPos = 50 + pixelSize[1] * yIndex

                if grid.GetSpin(xIndex, yIndex) == 1:
                    col = [0,0,0]
                else:
                    col = [255,255,255]

                pygame.draw.rect(self._screen, col, pygame.Rect(xPos, yPos, np.ceil(pixelSize[0]), np.ceil(pixelSize[1])))

        self._drawFinished = True

        #Draw magnetisation graph
        ratio = 1.5 #x to y
        graphSize = currentWindowSize[1] / 2 - 60 #y size
        position = (currentWindowSize[0] / 2 + (currentWindowSize[0]/2 - graphSize*ratio) /2, 50)

        self._magnetisationGraph.SetSize(position[0], position[1], graphSize*ratio, graphSize)
        self._magnetisationGraph.DrawMatPlotLib(self._screen, self._graphDirty)

        #Draw energy graph
        position = (currentWindowSize[0] / 2 + (currentWindowSize[0]/2 - graphSize*ratio) /2, 60 + graphSize)

        self._energyGraph.SetSize(position[0], position[1], graphSize * ratio, graphSize)
        self._energyGraph.DrawMatPlotLib(self._screen, self._graphDirty)

        if self._graphDirty:
            self._graphDirty = False

def InitSpins(grid):
    #Initial random spin arrangement 
    for i in range(grid._sizeX):
        for j in range(grid._sizeY):
            randNum = rand.randint(1, 3)
            if randNum == 1:
                grid.SetSpin(i, j, 1)
            else:
                grid.SetSpin(i, j, -1)

def InitPygame():

    pygame.init()
    window = pygame.display.set_mode(WINDOW_SIZE, pygame.HWSURFACE | pygame.DOUBLEBUF | pygame.RESIZABLE)
    pygame.display.set_caption("Ising Model Simulation")

    return window

START = False

def Update():
    global START
    
    if grid.IsThreadFinished():
        if grid._lastAverageSpin != None:
            windowHandle._magnetisationGraph.AddPoint(grid._iterationNum, grid._lastAverageSpin)
            windowHandle._energyGraph.AddPoint(grid._iterationNum, grid._lastTotalEnergy)

        windowHandle.DrawUpdate(grid)
        pygame_widgets.update(events)
        pygame.display.flip()

        if START:
            windowHandle.SetGraphDirty()
            grid.StartIterateThread(ITERATION_COUNT)

def StartAnim():
    global START
    START = True

def StopAnim():
    global START
    START = False

grid = spin_grid.SpinGrid(GRID_SIZE, GRID_SIZE, B_FIELD, INTERACTION_STRENGTH)
grid.SetTemperature(TEMPERATURE)
InitSpins(grid)

plt.ioff()

window = InitPygame()
clock = pygame.time.Clock()
main_font = pygame.font.Font("./font/cmu-serif-roman.ttf", size=20)

windowHandle = WindowHandler(window, StartAnim, StopAnim)

while True:
    clock.tick(30)

    events = pygame.event.get()
    for event in events:
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit(0)

    Update()
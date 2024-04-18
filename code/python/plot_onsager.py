"""Plots points for the Onsager solution to the Ising model.
"""

import numpy as np
import scipy.integrate as integrate
from scipy.misc import derivative

INTERACTION_STRENGTH = 1.0
START_TEMP = 0.1
END_TEMP = 4.0
TEMP_COUNT = 500
GRID_COUNT = 10
OUTPUT_FILE = "./onsager-energy.csv"

def Integrand(K2, w):
  return np.log(0.5 * (1.0 + np.sqrt(1.0 - K2 * np.square(np.sin(w)))))

def CalcLogLambda(beta):
  K2 = 2 * np.sinh(2 * beta * INTERACTION_STRENGTH)
  K2 /= (np.cosh(2 * beta * INTERACTION_STRENGTH)) ** 2
  K2 = K2 * K2

  logLambda = np.log(2 * np.cosh(2 * beta * INTERACTION_STRENGTH))
  logLambda += (1.0 / np.pi) * integrate.quad(lambda x: Integrand(K2, x), 0, np.pi / 2.0)[0]

  return logLambda

output = "temp,energy\n"
for tempIndex in range(TEMP_COUNT):
  currentTemp = START_TEMP + (END_TEMP - START_TEMP) * (float(tempIndex) / (TEMP_COUNT - 1))
  energy = -derivative(CalcLogLambda, 1.0 / currentTemp, dx=1e-8)

  output += str(currentTemp) + "," + str(energy) + "\n"

fileBuff = open(OUTPUT_FILE, 'w')
fileBuff.write(output)
fileBuff.close()
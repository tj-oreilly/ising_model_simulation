import numpy as np

START_TEMP = 0.1
END_TEMP = 4.0
TEMP_COUNT = 500

def CalcMagnetisation(beta):
  return np.power((1 - np.power(np.sinh(2 * beta), -4)), 0.125)

output = "temp,energy\n"
for tempIndex in range(TEMP_COUNT):
  currentTemp = START_TEMP + (END_TEMP - START_TEMP) * (float(tempIndex) / (TEMP_COUNT - 1))

  output += str(currentTemp) + "," + str(CalcMagnetisation(1.0 / currentTemp)) + "\n"

fileBuff = open("./magnetisation-onsager.csv", 'w')
fileBuff.write(output)
fileBuff.close()
# Gravity extension for Pure Data
Create random curves from up to 10 bodies and 1 black hole in a gravitational system.

Detailed information and usage in g-help.pd

Supports Windows, Linux (x64) and Linux (arm for Organelle)

* Organelle/Raspi-version: ext/linux_arm
* Linux ext/linux_x64
* Windows: windows_x64 (build does not work, TODO)

The folders contain the following files:

* grav-help.pd: help patch
* grav.pd_linux (.dll on Windows): gravity simulation
* gravf.pd_linux (.dll on Windows): some additional calculations based on the body state
* nodeapp.zip: browser-based visualization

For the visualization:

* Install Node.js
* Unzip nodeapp.zip
* Open a command prompt and change to the directory containing the unzipped files
* Type "npm i"
* Type "node server.js"
* Open the browser and open localhost:8080
* Start "Visualizer" in the help patch
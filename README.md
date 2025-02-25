
## Getting Started
The flight computer is based on the ESP32-WROOM-32E and was programmed using the ESP-IDF environment. 

The ground station uses Dash Plotly, a Python framework capable of building interactive and real-time dashboards to visualize and monitor telemetry data. This setup guide assumes that either Anaconda or Miniconda is your package manager and is thus recommended that you use it. However, you are free to use any package manager of your choosing.

Before doing anything, install the following:
- [Git](https://git-scm.com/downloads)
- [Conda](https://docs.conda.io/en/latest/) or [Miniconda](https://docs.conda.io/projects/miniconda/en/latest/) (highly recommended, also smaller than Conda)

### ESP-IDF Setup
Follow the instructions here: [https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)

Highly recommended that you install via the VSCode extension. Instrctions for getting set up are on the getting started page or you could click here: [https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md](https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md)

We used ESP-IDF version 5.4 for our build. 


### Python Environment Setup

Follow the steps below to set up the Conda environment for the project:

```bash
# 1. Clone the Repository
git clone https://github.com/bimmui/HODGE-electronics.git
cd HODGE-electronics

# 2. Create Conda Environment
conda env create -f hodge.yml -n hodge

# 3. Activate the Conda Environment
conda activate hodge

# 4. Verify Installation
conda list
```


## Getting Started
The flight computer is based on the ESP32-WROOM-32E and (I'll finish later).

The ground station is based on CustomTkinter, a Python GUI framework inspired by the original Tkinter. This setup guide assumes that either Anaconda or Miniconda is your package manager and is thus recommended that you use it. However, you are free to use any package manager of your choosing.

Before doing anything, install the following:
- [Git](https://git-scm.com/downloads)
- [Conda](https://docs.conda.io/en/latest/) or [Miniconda](https://docs.conda.io/projects/miniconda/en/latest/) (highly recommended)


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

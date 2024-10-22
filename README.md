# MtSyft

## Usage

The following is the output of `./MtSyft --help`

```
MtSyft: A tool for LTLf best-effort synthesis in multi-tier environments
Usage: ./MtSyft [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -a,--agent-file TEXT:FILE REQUIRED
                              File to agent specification
  -e,--environment-file TEXT:FILE REQUIRED
                              File to environment tiers E_1, ..., E_n (E_1 most determined, E_n least determined)
  -p,--partition-file TEXT:FILE REQUIRED
                              File to partition
  -s,--starting-player INT REQUIRED
                              Starting player (agent=1, environment=0)
  -t,--store-results          Specifies results should be stored in results.csv
  -i,--interactive            Executes the synthesized program in interactive mode
```

LTLf formulas in agent and environment files should be written in Lydia's syntax. For further details, refer to https://github.com/whitemech/lydia .

To perform best-effort synthesis for an LTLf goal in some LTLf multi-tiered environment, you have to provide both the path to the agent goal and the multi-tiered specification, e.g., `navigation_agent.ltlf` and `navigation_multitier.ltlf`, and the path to the partition file, e.g., `navigation.part` (see the `RobotNavigation` folder).

For instance, the command:

```
./MtSyft -a navigation_agent.ltlf -e navigation_multitier.ltlf -p navigation.part -s 1 -i 1
```

Performs best-effort synthesis in multi-tiered environments with the agent moving first and executes the synthesized strategy in interactive mode.

## Build from Source

Compilation instruction using CMake (https://cmake.org/). We recommend using of Ubuntu 20.04 LTS. Problems can occur between some libraries on which MtSyft relies and newer versions of Ubuntu (more information below).

### Install the dependencies

#### Flex and Bison

The project uses Flex and Bison for parsing purposes.

Firse check that you have them: `whereis flex bison`

If no item occurs, then you have to install them: `sudo apt-get install -f flex bison`

#### CUDD 3.0.0

The project depends on CUDD 3.0.0. To install, run the following commands

```
wget https://github.com/whitemech/cudd/releases/download/v3.0.0/cudd_3.0.0_linux-amd64.tar.gz
tar -xf cudd_3.0.0_linux-amd64.tar.gz
cd cudd_3.0.0_linux-amd64
sudo cp -P lib/* /usr/local/lib/
sudo cp -Pr include/* /usr/local/include/
```

Otherwise, build from source (customize `PREFIX` variable as you see fit).

```
git clone https://github.com/whitemech/cudd && cd cudd
PREFIX="/usr/local"
./configure --enable-silent-rules --enable-obj --enable-dddmp --prefix=$PREFIX
sudo make install
```

If you get an error about aclocal, this might be due to either

* Not having automake: `sudo apt-get install automake`
* Needing to reconfigure, do this before `configuring: autoreconf -i`
* Using a version of aclocal other than 1.14: modify the version 1.14 in configure accordingly.

#### MONA

The project depends on the MONA library, version v1.4 (patch 19). We require that the library is compiled with different values for parameters such as `MAX_VARIABLES`, and `BDD_MAX_TOTAL_TABLE_SIZE` (you can have a look at the details at https://github.com/whitemech/MONA/releases/tag/v1.4-19.dev0).

To install the MONA library, run the following commands:

```
wget https://github.com/whitemech/MONA/releases/download/v1.4-19.dev0/mona_1.4-19.dev0_linux-amd64.tar.gz
tar -xf mona_1.4-19.dev0_linux-amd64.tar.gz
cd mona_1.4-19.dev0_linux-amd64
sudo cp -P lib/* /usr/local/lib/
sudo cp -Pr include/* /usr/local/include
```

#### SPOT

The project relies on SPOT (https://spot.lre.epita.fr/). To install it, follows the instructions at https://spot.lre.epita.fr/install.html

#### Graphviz

The project uses Graphviz to display automata and strategies. Follow the install instructions on the official website: https://graphviz.gitlab.io/download/.

On Ubuntu, this should work:

```
sudo apt-get install libgraphviz-dev
```

#### Syft

First, install the Boots libraries

```
sudo apt-get install libboost-dev
```

If you get an error with missing Boost libraries you can use:

```
sudo apt-get install libboost-all-dev
```

For further information see https://www.boost.org/ . Then, install Syft using:

```bash 
git clone https://github.com/whitemech/Syft.git
cd Syft
git checkout v0.1.1
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
sudo make install
```

### Build from source

### Lydia

Within the project folder, clone Lydia inside the submodules folder.

```
mkdir submodules
cd submodules 
git clone https://github.com/whitemech/lydia.git --recursive
```

### Build from source

To build run the following commands.

```bash
cd ..
mkdir build && cd build
cmake ..
make
```

Building generates three executables:
*`MtSyft`. Implements LTLf best-effort synthesis in multi-tier environments (i.e., Algorithm 2);
*`cb-MtSyft`. Implements LTLf best-effort synthesis in multi-tier environments with a common base;
*`conj-MtSyft`. Implements LTLf best-effort synthesis in multi-tier environments with conjunctive refinements.

For details about the usage of each implementation insert, e.g., the command:

```
./MtSyft --help
```

## Performing the experiments.

Unzip the `zip` files. For performing the experiments on the counter game benchmarks run:

```
sudo chmod "u+x" run-benchmarks.sh 
./run-benchmarks.sh
```

To perform the experiments on the robot navigation benchmarks run:

```
sudo chmod "u+x" run-navigation.sh 
./run-navigation.sh
```

The results of the experiments will be stored into `csv` files inside the folder of the benchmarks. The running times are stored in files `res_{implementation_name}.csv`, where `{implementation_name} = {mtsyft, cb_mtsyft, conj_mtsyft}`; the time cost of each major operation is stored in `times_{implementation_name}.csv`.

To execute the synthesized program on a 2x2 robot navigation benchmark run (interactive mode): 

```
sudo chmod "u+x" running-example.sh 
./running-example.sh
```

To plot the results of the experiments shown in the paper move into the `EmpiricalResults` folder: 
* For CounterGames benchmarks run: `python3 Table1.py`, `python3 Figure_1.py`, `python3 Figure_2.py`to obtain the results shown in Table 1, and Figures 1 and 2, respectively.
* For RobotNavigation benchmarks run: `python3 Table2.py` to obtain the results shown in Table 2

To plot the results of your experiments, move into the benchmarks folder and insert the same commands as above.

Please note: performing experiments on counter game benchmarks may require weeks; performing experiments on robot navigation benchmarks may require a day or two.

## Reference

```
@inproceedings{AminofDPR24,
  author       = {Benjamin Aminof and
                  Giuseppe {De Giacomo} and
                  Gianmarco Parretti and
                  Sasha Rubin},
  title        = {Effective Approach to LTLf Best-Effort Synthesis in Multi-Tier Environments},
  booktitle    = {{IJCAI}},
  pages        = {3232--3240},
  publisher    = {ijcai.org},
  year         = {2024}
}
```

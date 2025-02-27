<h1 align="center">
  <br>
  <img src="img/logo.png" alt="ReTA" width="330">
  <br>
  ReTA framework
  <br>
</h1>
<h4 align="center">A versatile framework for modeling and analyzing arbitrary online scheduling policies for real-time systems</h4>
<p align="center">
  <a href="https://github.com/porya-gohary/ReTA/blob/master/LICENSE">
    <img src="https://img.shields.io/badge/License-GPLv3-blue.svg"
         alt="Gitter">
  </a>
    <img src="https://img.shields.io/badge/Made%20with-C++-orange">
    <img src="https://img.shields.io/badge/Python-3.8+-brightgreen">
</p>

## 📦 Dependencies and Required Packages
### Python:
Assuming that `Python` is installed in the targeted machine, we can create a virtual environment for the project by running the following command:
```bash
python -m venv venv
```
To activate the virtual environment, run the following command:
```bash
source venv/bin/activate
```
To install the required packages, run the following command:
```bash
pip install -r requirements.txt
```
### C++:
- A modern C++ compiler with support for the **C++17 standard**. Recent versions of `clang` and `g++` on Linux is known to work.
- The [CMake](https://cmake.org) build system. For installation using `apt` (Ubuntu, Debian...):
```bash
sudo apt-get -y install cmake 
```
- The [Boost](https://www.boost.org) library. For installation using `apt` (Ubuntu, Debian...):
```bash
sudo apt-get -y install libboost-dev
```
## 📋 Build Instructions
First, clone the repository and change the current directory to the cloned repository:
```bash
git clone https://github.com/porya-gohary/ReTA.git
cd ReTA
```
To compile the framework for the input codes written in `ReDo DSL`, follow the instructions below:

Consider we want to compile the framework for the example code `example1.redo` in the `examples` directory. The following commands should be executed:
```bash
./compile.sh -i ./example/example1.redo
```
The compiled code will be generated in the `build` directory. To run the compiled code, run the following command:
```bash
cd build
./reta
```

## ⚙️ Usage
The options of the analysis are as follows (`./reta -h`):
```
Usage: reta [OPTIONS]...

Options:
  -l TIMEOUT, --time-limit=TIMEOUT
                        maximum CPU time allowed (in seconds, zero means no limit)
  -n, --naive           use the naive exploration method (default: false)
  -r, --raw             print output without formatting (default: false)
  -o OUTPUTFILE, --output=OUTPUTFILE
                        name of the output file (default: out.csv)
  -e VERBOSE, --verbose=VERBOSE
                        print log messages [0-5](default: 0)
  -v, --version         show program's version number and exit
  -h, --help            show this help message and exit
```

## 🗄️ Output Files
The output files are generated in the current directory. The output files are as follows:

* `out.csv`: Contains the response times for each job.
* `out.dot`: Represents the graph of explored states. *(Requires `COLLECT_TLTS_GRAPH` to be enabled at compile time.)*

## 🌱 Contribution
With your feedback and conversation, you can assist me in developing this framework.
* Open pull request with improvements
* Discuss feedback and bugs in issues

## 📜 License
Copyright © 2023 [Pourya Gohari](https://pourya-gohari.ir)

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.


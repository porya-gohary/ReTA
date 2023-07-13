#!/bin/bash
#Colors
RED='\033[0;31m' #RED
NC='\033[0m'     # No Color
GR='\033[0;32m'  # GREEN
BL='\033[0;34m'  # BLUE
MG='\u001b[35m'  # Magenta
YL='\u001b[33m'  # Yellow

help="Usage:
      compile.sh  [-argument value]...

Options:
    -i File                   Redo code file
    -v                        show version and exit
    -h                        show this message
"
version="v0.1\nCopyright (C) 2023 Pourya Gohari"
while getopts "i:vh" opt; do
    case $opt in
        i)
            input=$OPTARG
            ;;
        v)
            echo -e $version
            exit 0
            ;;
        h)
            echo "$help"
            exit 0
            ;;
        \?)
            echo -e "${RED}[!] Invalid option: -$OPTARG${NC}" >&2
            exit 1
            ;;
        :)
            echo -e "${RED}[!] Option -$OPTARG requires an argument.${NC}" >&2
            exit 1
            ;;
    esac
done

if [ -z "$input" ]; then
    echo -e "${RED}[!] No input file specified${NC}"
    exit 1
fi

if [ ! -f "$input" ]; then
    echo -e "${RED}[!] Input file does not exist${NC}"
    exit 1
fi

# check if python3 or python is installed and set PYTHON variable
if ! command -v python &> /dev/null; then
    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}[\u2715] Python3 could not be found${NC}"
        exit 1
    else
        # set python3 to python
        PYTHON=python3
    fi
else
    # set python to python3
    PYTHON=python
fi

# check python3 or python version is greater than 3
# if python3 is installed print version
if command -v ${PYTHON}  &> /dev/null; then
    if [ "$(python3 --version | cut -d " " -f 2 | cut -d "." -f 1)" -lt "3" ]; then
        echo -e "${RED}[\u2715] Python version is less than 3${NC}"
        exit 1
    else
        echo -e "${YL}[\u2713] Python version: $(${PYTHON}  --version)${NC}"
    fi
fi

echo -e ${PYTHON}

# check if cmake is installed
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[\u2715] CMake could not be found${NC}"
    exit 1
else
    echo -e "${YL}[\u2713] CMake version: $(cmake --version | head -n 1)${NC}"
fi


# check gcc version is greater than 11
if [ "$(gcc --version | head -n 1 | cut -d " " -f 4 | cut -d "." -f 1)" -lt "11" ]; then
    echo -e "${RED}[\u2715] GCC version is less than 11${NC}"
    exit 1
else
    echo -e "${YL}[\u2713] GCC version: $(gcc --version | head -n 1)${NC}"
fi

# run ReDo parser
echo -e "${BL}--> Parsing $input${NC}"
# check for errors and print error message
if ! ${PYTHON} ./dsl/redo_parser.py "$input"; then
    echo -e "${RED}[!] Parsing failed.${NC}"
    exit 1
fi

mv -f scheduler.hpp ./include/models/
mv -f systemModel.hpp ./include/models/

# parsing completed
echo -e "${GR}[+] Parsing completed.${NC}"

# compile ReTA framework
echo -e "${BL}--> Compiling ReTA framework${NC}"
rm -rf build/*
mkdir -p build
cd build || exit
cmake ..
make -j4
echo -e "${GR}[+] Done!${NC}"

# TypeSqueezer
TypeSqueezer: When Static Recovery of Function Signatures for Binary Executables Meets Dynamic Analysis

**Repository Contents**

This repository contains the open-source portion of code for a research paper.

**Tool Description**

The tool is designed for analyzing binary files and requires the corresponding dynamic execution command. 
The output of the tool is a type result file located in the TypeSqueezer-master/TypeSqueezer/res directory.

**Dependencies**

The tool utilizes the following libraries and tools:

    dyninst 9.3.1: The content corresponding to this version of the library can be found in the TypeSqueezer-master/dyninst-9.3.1 folder.
    pin 3.27: Pin needs to be installed for the tool.
    FSROPT: Another static analysis tool from <https://github.com/ylyanlin/FSROPT/archive/refs/heads/main.zip>.
    boost 1.58.0: The tool requires the system and thread modules from this version of Boost.

You can automatically install these programs by using the following command:

    $ sh initial.sh


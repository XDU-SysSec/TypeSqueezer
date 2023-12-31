see master branch

# TypeSqueezer
TypeSqueezer: When Static Recovery of Function Signatures for Binary Executables Meets Dynamic Analysis

**Repository Contents**

This repository contains the open-source portion of code for a research paper.

**Tool Description**

The tool is designed for analyzing binary files and requires the corresponding dynamic execution command. 
The output of the tool is a type result file located in the TypeSqueezer-main/TypeSqueezer/res directory.

**Dependencies**

The tool utilizes the following libraries and tools:

    dyninst 9.3.1: The content corresponding to this version of the library can be found in the TypeSqueezer-main/dyninst-9.3.1 folder.
    pin 3.27: Pin needs to be installed for the tool.
    FSROPT: Another static analysis tool from <https://github.com/ylyanlin/FSROPT/archive/refs/heads/main.zip>.
    boost 1.58.0: The tool requires the system and thread modules from this version of Boost.

You can automatically install these programs by using the following command:

    $ sh initial.sh

**How to use it**

To use this tool, follow the instructions below:

1、Place the binary file you want to test in the TypeSqueezer-main/TypeSqueezer/bin directory.
    
2、Run the following command for static analysis

    $ sh ~/TypeSqueezer-main/TypeSqueezer/initialAnalyze.sh ${file_name}
    
Replace ${file_name} with the actual name of your binary file.

3、Next, open the TypeSqueezer-main/TypeSqueezer/pinbench/${file_name}/command.sh file and write the command(s) you want to execute. You can include multiple commands or use the testsuite command.

4、Run the following command for dynamic analysis

    $ sh ~/TypeSqueezer-main/TypeSqueezer/pinbench/pinAnalyze.sh ${file_name}
    
Again, replace ${file_name} with the name of your binary file.

5、Finally, navigate to the TypeSqueezer-main/TypeSqueezer/src directory and run the command 

    $ ./main ${file_name}
    
This will generate the result file in the TypeSqueezer-main/TypeSqueezer/res directory.

**result format**

Each *.res file contains different sections:

    [args] Regular (non-variadic) functions:

    <address> (<function symbol>) <min consumed argcount and max consumed argcount> <min consumed return argcount and max consumed return argcount> , (width for each argument and return argment) , (type for each argument and return argment)

    [icall-args] Indirect callsites:

    <address> (<function symbol>) <min consumed argcount and max consumed argcount> <min consumed return argcount and max consumed return argcount> , (width for each argument and return argment) , (type for each argument and return argment)

    [done] Marker that indicates the end of the .res


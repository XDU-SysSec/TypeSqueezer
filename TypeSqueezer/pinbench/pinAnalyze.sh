#!/bin/bash -v
cd $1
${HOME}/TypeSqueezer-master/pin-3.27/pin -follow_execv -t ${HOME}/TypeSqueezer-master/pin-3.27/source/tools/ManualExamples/obj-intel64/mypin.so -o $1 -- sh command.sh
cd ../misc/ret1
./retAnalyze $1

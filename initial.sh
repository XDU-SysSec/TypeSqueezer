#build boost-1.58.0
wget https://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz/download
tar -xzvf boost_1_58_0.tar.gz
./bootstrap.sh
sudo ./b2 install
#build pin-3.27
wget https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.27-98718-gbeaa5d51e-gcc-linux.tar.gz
tar -xzvf pin-3.27-98718-gbeaa5d51e-gcc-linux.tar.gz
mv pin-3.27-98718-gbeaa5d51e-gcc-linux pin-3.27
cp mypin.cpp pin-3.27/source/tools/ManualExamples
cd pin-3.27/source/tools/ManualExamples
mkdir -p obj-intel64
make obj-intel64/mypin.so
#build FSROPT
cd ~/TypeSqueezer-master
wget https://github.com/ylyanlin/FSROPT/archive/refs/heads/main.zip
tar -xzvf main.zip
cd FSROPT-master/typearmor-master
. ./envsetup.sh
cd di-opt
make
cd ../static
make

#make links
cd ~/TypeSqueezer-master/TypeSqueezer
ln -s ../FSROPT-master/typearmor-master/run-ta-static.sh .
ln -s ../FSROPT-master/typearmor-master/envsetup.sh .
ln -s ../FSROPT-master/typearmor-master/server-bins .
ln -s ../FSROPT-master/typearmor-master/out FSROPT-out

cd ~/TypeSqueezer-master/TypeSqueezer/directcall/Dcall_tauCFI
make
cd ~/TypeSqueezer-master/TypeSqueezer/directcall/Dcall_TypeArmor
make
cd ~/TypeSqueezer-master/TypeSqueezer/src
make
cd ~/TypeSqueezer-master/TypeSqueezer/taucfi
make
cd ~/TypeSqueezer-master/TypeSqueezer/misc/dump2
g++ dumpAnalyze.cpp -o dumpAnalyze
cd ~/TypeSqueezer-master/TypeSqueezer/misc/ret1
g++ retAnalyze.cpp -o retAnalyze
cd ../../

sudo apt install build-essential libbz2-dev # !!
cd ~/TypeSqueezer-master # !!

#build boost-1.58.0
wget https://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz
tar -xzvf boost_1_58_0.tar.gz
cd boost_1_58_0
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
#build FSROPT, download dyninst first
cd ~/TypeSqueezer-master
wget https://github.com/dyninst/dyninst/archive/v9.3.1.tar.gz
tar -zxvf v9.3.1.tar.gz
cd ~/TypeSqueezer-master
wget https://github.com/ylyanlin/FSROPT/archive/refs/heads/main.zip
tar -xzvf main.zip
cd FSROPT-master/typearmor-master
cp -f ~/TypeSqueezer-master/file_to_replace/envsetup.sh .
cp -f ~/TypeSqueezer-master/file_to_replace/Makefile.inc .
# . ./envsetup.sh
# cd di-opt
# make
# cd ../static
# make

#make links
cd ~/TypeSqueezer-master/TypeSqueezer
ln -s ../FSROPT-master/typearmor-master/run-ta-static.sh .
ln -s ../FSROPT-master/typearmor-master/envsetup.sh .
ln -s ../FSROPT-master/typearmor-master/server-bins .
ln -s ../FSROPT-master/typearmor-master/out FSROPT-out

cd ~/TypeSqueezer-master/dyninst-9.3.1/install/lib
find . -type f -name "*9.3.1" -exec sh -c '
  for file do
    base_name=$(basename "$file" .9.3.1)
    ln -s "$file" "${base_name}9.3"
    ln -s "$file" "${base_name}"
  done
' \;

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

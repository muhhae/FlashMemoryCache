#!/bin/bash

sudo apt install -y libglib2.0-dev libzstd-dev cmake
cd .. && [ ! -d libCacheSim ] && git clone https://github.com/1a1a11a/libCacheSim

cd libCacheSim && mkdir -p _build && cd _build && cmake .. &&
make -j && sudo make install &&
cd ../.. &&
cd OptimalClockOffline/python &&
pip install -r requirements.txt &&
cd .. && mkdir build && cd build

wget -o onnxruntime.tgz "https://github.com/microsoft/onnxruntime/releases/download/v1.21.0/onnxruntime-linux-x64-1.21.0.tgz" &&
tar -xvzf onnxruntime.tgz &&
cd "onnxruntime-linux-x64-1.21.0.tgz" &&

sudo cp -r include /usr/include/onnxruntime &&
sudo cp -r lib /usr/lib/onnxruntime &&
sudo ldconfig

cmake .. && make .

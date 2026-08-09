#!/bin/bash
mkdir build
   cd build
   cmake -DWANT_QT6=ON ../ || Please ensure you have met all dependencies before compiling and try again.
   make
   sudo make install || doas make install || echo "Please install either sudo or doas to install. Without propper configuration, this message may also appear."

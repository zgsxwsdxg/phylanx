# Copyright (c) 2017 R. Tohid
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

FROM stellargroup/hpx:dev

# Install Eigen library (http://eigen.tuxfamily.org/)
RUN apt-get -y install wget &&                                                   \
    apt-get -y install python3 python3-dev python3-pip &&                        \
    apt-get -y install libblas-dev liblapack-dev &&                              \
    pip3 install setuptools &&                                                   \
    pip3 install pytest &&                                                       \
    wget http://bitbucket.org/eigen/eigen/get/3.3.4.tar.bz2 &&                   \
    mkdir -p eigen/3.3.4 &&                                                      \
    mkdir eigen-src && tar xf 3.3.4.tar.bz2 -C eigen-src --strip-components 1 && \
    cd eigen-src &&                                                              \
    mkdir build && cd build &&                                                   \
    cmake -DCMAKE_INSTALL_PREFIX=/eigen/3.3.4 .. &&                              \
    make install &&                                                              \
    cd / &&                                                                      \
    rm -r /eigen-src &&                                                          \
    git clone https://github.com/pybind/pybind11.git pybind11-src &&             \
    cd pybind11-src &&                                                           \
    mkdir build && cd build &&                                                   \
    cmake  -DPYBIND11_TEST=OFF -DCMAKE_INSTALL_PREFIX=/pybind11 .. &&            \
    make &&                                                                      \
    make install &&                                                              \
    cd / &&                                                                      \
    mkdir blaze &&                                                               \
    git clone https://bitbucket.org/blaze-lib/blaze.git blaze-src &&             \
    cd blaze-src &&                                                              \
    mkdir build && cd build &&                                                   \
    cmake -DCMAKE_INSTALL_PREFIX=/blaze .. &&                                    \
    make install

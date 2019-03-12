#!/bin/bash

cd /etc/yum.repos.d

sudo wget http://people.centos.org/tru/devtools-1.1/devtools-1.1.repo 

sudo yum --enablerepo=testing-1.1-devtools-6 install devtoolset-1.1-gcc devtoolset-1.1-gcc-c++

sudo ln -s -f /opt/centos/devtoolset-1.1/root/usr/bin/* /usr/local/bin/
hash -r

#!/bin/bash

set -e

# sudo apt-get install zlib1g-dev libbz2-dev

(
	mkdir -p download;
	cd download;

	# see http://python.org/download/releases/
	wget -qc http://python.org/ftp/python/3.3.1/Python-3.3.1.tar.bz2;
	wget -qc http://python.org/ftp/python/3.2.4/Python-3.2.4.tar.bz2;
	wget -qc http://python.org/ftp/python/3.2.3/Python-3.2.3.tar.bz2;
	wget -qc http://python.org/ftp/python/3.1.5/Python-3.1.5.tar.bz2;
	wget -qc http://python.org/ftp/python/3.0.1/Python-3.0.1.tar.bz2;
	wget -qc http://python.org/ftp/python/2.7.4/Python-2.7.4.tar.bz2;
	wget -qc http://python.org/ftp/python/2.6.8/Python-2.6.8.tar.bz2;
	wget -qc http://python.org/ftp/python/2.5.6/Python-2.5.6.tar.bz2;
	wget -qc http://python.org/ftp/python/2.4.6/Python-2.4.6.tar.bz2;

	# decompress
	for i in `ls | grep bz2$`; do bzip2 -dc $i | tar -x; done

	# apply patches
	for i in `find -maxdepth 1 -type d | grep ^./P | grep -v build | grep -v env | grep -v 3.3.1 | grep -v 2.7.4 | sort`; do
	(
		cd $i;
		echo $i
		patch -s < ../../setup.py.patch;
	) done

	# build
	for i in `find -maxdepth 1 -type d | grep ^./P | grep -v build | grep -v env | sort`; do
	(
		cd $i;
		pwd;
		CXX=g++ ./configure --disable-option-checking --enable-unicode=ucs4 --with-wide-unicode --prefix=`pwd`-build > /dev/null;
		make         > /dev/null;
		make install > /dev/null;
	) done

	# virtualenv
	for i in `find -maxdepth 1 -type d | grep ^./P | grep -v build | grep -v env | grep -v 3.3 | sort`; do
	(
		echo $i;
		unset PYTHONDONTWRITEBYTECODE;
		(virtualenv -p $i-build/bin/python $i-env || virtualenv -p $i-build/bin/python3.0 $i-env || virtualenv -p $i-build/bin/python3 $i-env) > /dev/null;
		source $i-env/bin/activate;
	) done

	(
		unset PYTHONDONTWRITEBYTECODE;
		rm -rf Python-3.3.1-env;
		Python-3.3.1-build/bin/pyvenv Python-3.3.1-env;
		source Python-3.3.1-env/bin/activate;
	)
)

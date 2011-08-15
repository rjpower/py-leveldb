#!/bin/sh

# see http://python.org/download/releases/

wget -qc http://python.org/ftp/python/3.2.1/Python-3.2.1.tar.bz2
wget -qc http://python.org/ftp/python/3.1.4/Python-3.1.4.tar.bz2
wget -qc http://python.org/ftp/python/3.0.1/Python-3.0.1.tar.bz2
wget -qc http://python.org/ftp/python/2.7.2/Python-2.7.2.tar.bz2
wget -qc http://python.org/ftp/python/2.6.7/Python-2.6.7.tar.bz2
wget -qc http://python.org/ftp/python/2.5.6/Python-2.5.6.tar.bz2
wget -qc http://python.org/ftp/python/2.4.6/Python-2.4.6.tar.bz2

#for i in `ls | grep bz2$`; do bzip2 -dc $i | tar -x; done

for i in `find -maxdepth 1 -type d | grep ^./P | grep -v build | sort | grep 2.5`; do
(
	echo $i;
	cd $i;
#	./configure --prefix=`pwd`-build;
	make && make install;
)
done

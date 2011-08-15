#!/bin/sh

for i in `find -maxdepth 1 -type d | grep ^./P | grep build | sort | grep 2.5| grep -v install`; do
(
	cd $i/bin;
	PYTHON=`pwd`/`find | grep ./python | grep -v config | head --lines=1 | sed -n '1p'`;
	cd ../..;
	$PYTHON -V;
	cd ..;
	$PYTHON setup.py build --force > /dev/null && $PYTHON setup.py install --install-lib ./versions/$i-install > /dev/null;
)
done

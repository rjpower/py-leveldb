#!/bin/bash

set -e

(
	cd download;

	# virtualenv
	for i in `find -maxdepth 1 -type d | grep ^./P | grep -v build | grep -v env | sort`; do
	(
		echo $i;
		unset PYTHONDONTWRITEBYTECODE;
		source $i-env/bin/activate;
		(
			cd ../..;
			pwd;
			which python;
			python setup.py build --force > /dev/null;
			python setup.py install > /dev/null;
			(
				cd test;
				pwd;
				python test.py;
			)
		)
	) done
)

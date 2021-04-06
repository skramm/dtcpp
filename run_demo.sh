#!/bin/bash

set +x

app=build/bin/dectree

function runone
{
	case $1 in
	a)
	$app sample_data/iris.data -sep ',' -cs $2 $3 $4 $5
	;;

	b)
	$app sample_data/winequality-white.csv -sep ';' $2 $3 $4 $5
	;;

	c)
	$app sample_data/dummy_1.dat $2 $3 $4 $5
	;;

	d)
	$app sample_data/dummy_2.dat $2 $3 $4 $5
	;;

	e)
	$app sample_data/wine.data -sep ',' -cf $2 $3 $4 $5
	;;

	f)
	$app sample_data/balance-scale.csv -fl -sep ',' $2 $3 $4 $5

	;;

	esac
	if [ $? != 0 ]; then
		echo "==============================================> program failed !"
		exit 1
	fi

	if [ "$doplot" == "YES" ]; then
		make dot
		make plt
	fi
}


make
if [ $? != 0 ]; then
	echo "==============================================> build failed !"
	exit 1
fi

echo "WARNING, erasing previous data !"
make cleanout

doplot=NO
if [ "$1" == "" ]; then
	doplot=NO
	runone a $2 $3 $4 $5
	runone b $2 $3 $4 $5
	runone c $2 $3 $4 $5
	runone d $2 $3 $4 $5
	runone e $2 $3 $4 $5
	exit
fi

runone $1 $2 $3 $4 $5
make plt
make dot
xdg-open out/dectree.html




#!/bin/bash

set +x
app=build/bin/dectree
make
if [ "$1" == "" ]
then
	echo "missing arg"
	exit
fi

echo "arg=$1"

case $1 in
a)
$app sample_data/iris.data -sep ','
;;

b)
$app sample_data/winequality-white.csv -sep ';'
;;

c)
$app sample_data/dummy_1.dat
;;

d)
$app sample_data/dummy_2.dat
;;

e)
$app sample_data/wine.data -sep ',' -cf
;;

f)
$app sample_data/balance-scale.csv -fl -sep ',' $2 $3 $4 $5
;;

esac


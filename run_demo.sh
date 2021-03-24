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


#!/bin/sh

cd ~/Projects/md09082016/md/QBT
if [ -d add_directive ]
then
	echo " ready "
else
	mkdir add_directive
fi

for file in *.cpp
do
	awk 'BEGIN{NFnd=0;}{if ($1 ~ "cout"){print "#ifdef CW_DEBUG"; print $0 ; print "#endif"; NFnd++;} else {print $0;}}' $file > tmp
	awk '{if ($1 ~ "^printf"){print "#ifdef CW_DEBUG"; print $0 ; print "#endif";} else {print $0;}}' tmp > add_directive/$file
	nr=`grep "CW_DEBUG" add_directive/$file | wc | awk '{print $1;}' `
	if [ $nr -gt 0 ]
	then
	  echo $nr $file
  fi
done



#!/bin/bash

if test "$#" -ne 2 
then
  echo "Usage: $0 DIRECTORY CONFIG_FILE"
  exit 1
fi

if [ ! -d $1  ]; then
  echo "DIRECTORY NOT FOUND " 
  exit 1
fi

WK_DIR=$1
config=$2
CHANNEL_BASE=`grep channel_logger_base $WK_DIR/$config | awk '{print $3;}'`
total_channels=`grep total_channels $WK_DIR/$config |awk '{print $3;}'`

#echo $CHANNEL_BASE
#echo $total_channels

dir=`dirname $CHANNEL_BASE`
log=`basename $CHANNEL_BASE`

cat <<GAP_  > awk_src
BEGIN{seqno=1; ttGap=0;}
{
	if (\$0 !~ "parse seqno") { next;  };
	{nn=\$4;}
	if (seqno != nn){ttGap++; print "GAP", nn, " != ", seqno;};
		{seqno=nn; seqno++;}
}
END{print "\tTotal Gap=", ttGap;}
GAP_
cd $dir
#SYMBOL=$2
count=0
while [ $count -lt $total_channels ]; do 
	log_name=$CHANNEL_BASE$count".log"
	echo "Checking "$log$count
	awk -f awk_src $log_name
	count=$((count+1))
done
count=0
while [ $count -lt $total_channels ]; do 
	log_name=$CHANNEL_BASE$count".log"
	cc=`wc -l $log_name | awk '{print $1;}'` 
	echo "Rec. count : "$log$count.log" : " $cc
	count=$((count+1))
done


#!/bin/sh

if [ "$#" -ne 2   ] || ! [ -d "$1"   ]; 
then
  echo "Usage: $0 DIRECTORY CONFIG_NAME" >&2
  exit 1
fi

WK_DIR=$1
config=$2
CHANNEL_BASE=`grep channel_logger_base $WK_DIR/$config | awk '{print $3;}'`
total_channels=`grep total_channels $WK_DIR/$config |awk '{print $3;}'`

log_dir=`dirname $CHANNEL_BASE`
file_base=`basename $CHANNEL_BASE`

cd $log_dir

#SYMBOL=$2
	echo "{if (\$0 !~ /order book/){next;}}{ split(\$0, dd, \"order book\"); print dd[2];}" > awk_src
	echo "SYMBOL BOOK : "
count=0;
while [ $count -lt $total_channels ]; do
	log_name=$CHANNEL_BASE$count".log"
	echo "SYMBOL BOOK : channel "$count
	awk -f awk_src $log_name > $file_base$count".book"

	#head -10 $log_name | awk -f awk_src 
	count=$((count+1))
done

#!/bin/sh

config="asx_test.cfg"
if [ "$#" -lt 2   ] ||  [ ! -d "$1"   ]
then
  echo "Usage: $0 DIRECTORY [CONFIG_NAME] SYMBOL" 
  exit 1
fi

SYMBOL=$2
if [ "$#" -gt 2    ]; then
	config=$2
	SYMBOL=$3
fi


WK_DIR=$1
CHANNEL_BASE=`grep channel_logger_base $WK_DIR/$config | awk '{print $3;}'`
total_channels=`grep total_channels $WK_DIR/$config | awk '{print $3;}'`
base_dir=`dirname $CHANNEL_BASE`
log_base=`basename $CHANNEL_BASE`

cd $base_dir

count=0;
while [ $count -lt $total_channels ]; do
	log_name=$CHANNEL_BASE$count".log"
	echo $log_name
	count=$((count+1))
	order_id=`grep "symbol:$SYMBOL      " $log_name | awk '{split($0, a, "order book id:"); split(a[2], dd); print dd[1];}'`
	tmp=`grep "symbol:$SYMBOL      " $log_name `
	if [ ${#tmp} -le 5 ]; then
		continue
	fi
	order_id=`echo $tmp |  awk '{split($0, a, "order book id:"); split(a[2], dd); print dd[1];}'`
	echo $order_id
	if [ -n $order_id ]  ; then
		break;
	fi
done
	if [ -z $order_id ]; then
		echo "Unknown symbol: "$SYMBOL
		exit 0
	fi
echo "Checking Symbol="$SYMBOL" ==> "$order_id

cat <<_AWK_SRC_BLOCK > awk_src
{
	if ((\$0 ~ /stock:$order_id/) && (\$0 ~ /add order/)){
	    if (\$0 ~ /side:B/) {
	split(\$0, b, "shares:"); split(b[2], shrs); buy=shrs[1];
	ttB += buy; cBuy++; 
	{print cBuy, " BUY ", buy, "======", ttB, "======", (ttB - ttSell); }
	{next;}
}
	{
	split(\$0, b, "shares:"); split(b[2], shrs); sell=shrs[1];
	ttS += sell; cSell++; 
	{print cSell, " SELL ", "======", sell, "======", ttS, (ttB - ttSell);}
	{next;}
}
}
}
END{
{print cSell, " SELL ", "======", "======", "======", ttS, (ttB - ttSell); }
{print cBuy, " BUY ", "======", "======", ttB, "======", (ttB - ttSell); }
}
_AWK_SRC_BLOCK
count=0;
while [ $count -lt $total_channels ]; do
	log_name=$CHANNEL_BASE$count".log"
	echo "Checking Symbol : through channel "$count
	awk -f awk_src $log_name
	count=$((count+1))
done


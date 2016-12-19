#!/bin/sh

to_add=`ls *.cpp`

if [ ! -d CWBUG_added/  ]; 
then
	mkdir CWBUG_added/
fi

cat <<_AWK_SRCS > awk_add_CW_BUG
BEGIN{needSemiComma=0; needEndif=0;}
{
	if (\$1 ~ "^cout" || \$1 ~ "^printf"){
		print "#ifdef CW_DEBUG";
		print \$0;
		if (\$NF ~ ";"){
			print "#endif"
		} else
		{
			needSemiComma=1;
			needEndif=0;
		}
		{next;}
	}
	{print \$0;}
	if (needSemiComma==1){
		if (\$NF ~ ";"){
                        print "#endif"
			needSemiComma=0;
		}
	}
}
_AWK_SRCS
cat awk_add_CW_BUG
for i in $to_add;
do
	awk -f awk_add_CW_BUG $i > CWBUG_added/$i
	#vi CWBUG_added/$i
done

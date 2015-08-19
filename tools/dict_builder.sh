#!/bin/bash
# usage: ./dict_builder.sh DICT_SIZE

TMP_FILE="/tmp/tmp_dict"

DICT_S=http://openlab.jp/skk/dic/SKK-JISYO.S.gz
DICT_M=http://openlab.jp/skk/dic/SKK-JISYO.M.gz
DICT_ML=http://openlab.jp/skk/dic/SKK-JISYO.ML.gz
DICT_L=http://openlab.jp/skk/dic/SKK-JISYO.L.gz

usage()
{
	echo "usage: ./dict_builder.sh DICT_SIZE ROMA2KANA_FILE"
}

if test "$#" -lt 2; then
	usage
	exit 1
fi

if test ! -x sortdict; then
	make
fi

case "$1" in
s|S)
	URL=$DICT_S;;
m|M)
	URL=$DICT_M;;
ml|ML)
	URL=$DICT_ML;;
l|L)
	URL=$DICT_L;;
*)
	usage
	exit 2;;
esac

wget -q -nc $URL

FILE=`basename $URL`

cat $2 | awk '{if (NF == 3) print $0}' > $TMP_FILE

# convert to UTF-8
# remove line started by ascii printable
# remove comment of candidate
zcat $FILE \
	| nkf -w -Lu \
	| LANG=C grep -v "^[[:print:]].*" \
	| sed 's|;[^;]*/|/|g' \
	| sed 's| /|\t|; s|/$||; s|/|\t|g' >> $TMP_FILE

cat $TMP_FILE | ./sortdict

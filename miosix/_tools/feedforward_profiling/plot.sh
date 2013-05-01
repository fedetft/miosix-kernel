 
function extract_etr {
	## Using the last sed to add a trailing whitespace to each line. Without
	## that space scilab treats negative numbers as Nan, go figure...
	grep "eTr" $1 | sed "s/[[:digit:]]: eTr = //" - | sed "s/$/ /" - > $2
}

extract_etr ff_off.txt off.txt
extract_etr ff_on.txt on.txt
extract_etr ff_reinit.txt reinit.txt

scilab -f plot.sci

rm off.txt
rm on.txt
rm reinit.txt

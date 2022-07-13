#!/bin/bash

NUMQUERIES=${1:-1}
REP=${2:-5}
TARGET=${3:-127.0.0.1}

echo "run,parallel,type,table,value" > migration.csv

for ((i=1; i<=$NUMQUERIES; i++)); do
	rm migrationlog.csv
	touch migrationlog.csv
	multimigration $REP $i ./queries "host=${TARGET} user=postgres password=postgres"
	
	cat migrationlog.csv >> migration.csv
done

mv migration.csv "multimig_${NUMQUERIES}_${REP}.csv"

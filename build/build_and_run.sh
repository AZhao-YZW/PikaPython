#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh build_and_run.sh [REPL|FILE|ALL] [LEAF|PIKA]'
    exit 1
fi

echo '========================= BUILD ========================='
sh build.sh $1 $2
echo '=========================  RUN  ========================='
sh run.sh $1
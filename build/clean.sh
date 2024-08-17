#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh clean.sh [REPL|FILE|ALL]'
    exit 0
fi

#----------------- SELECT MODE -----------------#
if [ ! -n "$1" ]; then
    mode="REPL"
    echo '===== default use REPL mode ====='
elif [ "$1" = "REPL" ] || [ "$1" = "FILE" ] || [ "$1" = "ALL" ]; then
    mode="$1"
    echo "===== use $1 mode ====="
else
    echo '*ERROR*: mode is wrong, support value [REPL|FILE|ALL]'
    exit 0
fi

if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
    output_dir=./linux/repl/output
    rm -rf $output_dir
fi

if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
    output_dir=./linux/file/output
    rm -rf $output_dir
fi

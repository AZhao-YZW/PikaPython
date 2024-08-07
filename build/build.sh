if [ ! -n "$1" ]; then
    mode="REPL"
    echo '===== default use REPL mode ====='
elif [ "$1" = "REPL" ]; then
    mode="REPL"
    echo '===== use REPL mode ====='
elif [ "$1" = "FILE" ]; then
    mode="FILE"
    echo '===== use FILE mode ====='
else
    echo '*ERROR*: mode is wrong, support value [REPL|FILE]'
    exit 1
fi

if [ "$mode" = "REPL" ]; then
    output_dir=./linux/repl/output
elif [ "$mode" = "FILE" ]; then
    output_dir=./linux/file/output
fi

mkdir -p $output_dir
cd $output_dir
cmake -G "Unix Makefiles" -D MODE=$mode ../..
make
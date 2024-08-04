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
    python run_test.py ./../../build/linux/repl/output/leafpython
elif [ "$mode" = "FILE" ]; then
    echo 'todo'
fi
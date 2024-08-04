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
    repl_dir=../../build/linux/repl
    python run_test.py ${repl_dir}/output/leafpython
elif [ "$mode" = "FILE" ]; then
    file_dir=../../build/linux/file
    pika_dir=../../build/linux/pikapython
    python run_file_test.py ${file_dir}/output/leafpython ${pika_dir}
fi
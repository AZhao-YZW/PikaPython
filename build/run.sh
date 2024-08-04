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
    ./linux/repl/output/leafpython
elif [ "$mode" = "FILE" ]; then
    pika_dir=./linux/pikapython
    file_dir=./linux/file
    cp ${file_dir}/output/leafpython ${pika_dir}
    mv ${pika_dir}/main.py ${pika_dir}/main.py.bak
    cp ${file_dir}/src/main_test.py ${pika_dir}/main.py
    ${pika_dir}/leafpython ${pika_dir}/main.py
    rm -f ${pika_dir}/leafpython
    rm -f ${pika_dir}/main.py
    mv ${pika_dir}/main.py.bak ${pika_dir}/main.py
fi
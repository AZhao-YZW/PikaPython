#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh run.sh [REPL|FILE]'
    exit 0
fi

#----------------- SELECT MODE -----------------#
if [ ! -n "$1" ] || [ "$1" = "ALL" ]; then
    mode="REPL"
    echo '===== default use REPL mode ====='
elif [ "$1" = "REPL" ] || [ "$1" = "FILE" ]; then
    mode="$1"
    echo "===== use $1 mode ====="
else
    echo '*ERROR*: mode is wrong, support value [REPL|FILE]'
    exit 0
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
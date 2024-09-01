#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh run.sh [REPL|FILE] [LEAF|PIKA]'
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

#----------------- SELECT CORE -----------------#
if [ ! -n "$2" ]; then
    core="LEAF"
    echo '===== default use LEAF core ====='
elif [ "$2" = "LEAF" ] || [ "$2" = "PIKA" ]; then
    core="$2"
    echo "===== use $2 core ====="
else
    echo '*ERROR*: core is wrong, support value [LEAF|PIKA]'
    exit 0
fi

#--------------------- RUN ---------------------#
if [ "$core" = "LEAF" ]; then
    output_dir=leaf
elif [ "$core" = "PIKA" ]; then
    output_dir=pika
fi

if [ "$mode" = "REPL" ]; then
    "./linux/repl/output/$output_dir/leafpython"
elif [ "$mode" = "FILE" ]; then
    pika_dir=./linux/pikapython
    file_dir=./linux/file
    cp ${file_dir}/output/$output_dir/leafpython ${pika_dir}
    mv ${pika_dir}/main.py ${pika_dir}/main.py.bak
    cp ${file_dir}/src/main_test.py ${pika_dir}/main.py
    ${pika_dir}/leafpython ${pika_dir}/main.py
    rm -f ${pika_dir}/leafpython
    rm -f ${pika_dir}/main.py
    mv ${pika_dir}/main.py.bak ${pika_dir}/main.py
fi
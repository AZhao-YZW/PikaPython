#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh run_test.sh [REPL|FILE|ALL] [LEAF|PIKA|CPYTHON]'
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

#----------------- SELECT CORE -----------------#
if [ ! -n "$2" ]; then
    core="LEAF"
    echo '===== default use LEAF core ====='
elif [ "$2" = "LEAF" ] || [ "$2" = "PIKA" ] || [ "$2" = "CPYTHON" ]; then
    core="$2"
    echo "===== use $2 core ====="
else
    echo '*ERROR*: core is wrong, support value [LEAF|PIKA]'
    exit 0
fi

#------------------ RUN TEST -------------------#
if [ "$core" = "LEAF" ]; then
    output_dir=leaf
elif [ "$core" = "PIKA" ]; then
    output_dir=pika
fi

if [ "$core" = "LEAF" ] || [ "$core" = "PIKA" ]; then
    if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
        repl_dir=../../build/linux/repl
        python run_repl_test.py ${repl_dir}/output/$output_dir/leafpython
    fi

    if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
        file_dir=../../build/linux/file
        pika_dir=../../build/linux/pikapython
        python run_file_test.py ${file_dir}/output/$output_dir/leafpython ${pika_dir}
    fi
elif [ "$core" = "CPYTHON" ]; then
    if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
        python run_cpy_repl_test.py
    fi

    if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
        python run_cpy_file_test.py
    fi
fi

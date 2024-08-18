#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh run_test.sh [REPL|FILE|ALL] [CPYTHON]'
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

#-------------- SELECT INTERPRETER -------------#
if [ ! -n "$2" ]; then
    interpreter="DEFAULT"
    echo '===== use LEAF/PIKA interpreter ====='
elif [ "$2" = "CPYTHON" ]; then
    interpreter="$2"
    echo "===== use $2 interpreter ====="
else
    echo '*ERROR*: interpreter is wrong, support value [CPYTHON]'
    exit 0
fi

#------------------ RUN TEST -------------------#
if [ "$interpreter" = "DEFAULT" ]; then
    if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
        repl_dir=../../build/linux/repl
        python run_repl_test.py ${repl_dir}/output/leafpython
    fi

    if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
        file_dir=../../build/linux/file
        pika_dir=../../build/linux/pikapython
        python run_file_test.py ${file_dir}/output/leafpython ${pika_dir}
    fi
elif [ "$interpreter" = "CPYTHON" ]; then
    if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
        python run_cpy_repl_test.py
    fi

    if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
        python run_cpy_file_test.py
    fi
fi

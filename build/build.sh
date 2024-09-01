#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh build.sh [REPL|FILE|ALL] [LEAF|PIKA] [PRECPL]'
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
elif [ "$2" = "LEAF" ] || [ "$2" = "PIKA" ]; then
    core="$2"
    echo "===== use $2 core ====="
else
    echo '*ERROR*: core is wrong, support value [LEAF|PIKA]'
    exit 0
fi

#----------------- PRE COMPILE -----------------#
if [ "$3" = "PRECPL" ]; then
    cd ./linux/pikapython/
    echo "===== start pre-compile ====="
    ./rust-msc-latest-win10.exe >/dev/null 2>&1
    if [ $? = 0 ]; then
        echo "===== pre-compile SUCCESS, use WIN pre-compiler ====="
    else
        ./rust-msc-latest-linux >/dev/null 2>&1
        if [ $? = 0 ]; then
            echo "===== pre-compile SUCCESS, use LINUX pre-compiler ====="
        elif [ `command -v wine` ]; then
            wine ./rust-msc-latest-win10.exe
            echo "===== pre-compile SUCCESS, use WINE pre-compiler ====="
        else
            echo "===== pre-compile FAILED ====="
        fi
    fi
    wait
    cd -
elif [ -n "$3" ]; then
    echo '*ERROR*: pre-compiler is wrong, support value [PRECPL]'
    exit 1
fi

#-------------------- BUILD --------------------#
if [ "$core" = "LEAF" ]; then
    output_dir=leaf
elif [ "$core" = "PIKA" ]; then
    output_dir=pika
fi

if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
    output_path=./linux/repl/output
    mkdir -p $output_path
    mkdir -p $output_path/$output_dir
    cd $output_path/$output_dir
    cmake -G "Unix Makefiles" -D"MODE=REPL" -D"CORE=$core" "../../.."
    make
    cd ../../../../
fi

if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
    output_path=./linux/file/output
    mkdir -p $output_path
    mkdir -p $output_path/$output_dir
    cd $output_path/$output_dir
    cmake -G "Unix Makefiles" -D"MODE=FILE" -D"CORE=$core" "../../.."
    make
fi
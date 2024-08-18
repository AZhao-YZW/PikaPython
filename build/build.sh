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
    if [ `command -v wine` ]; then
        echo "===== use WINE pre-compiler ====="
        wine ./rust-msc-latest-win10.exe
    else
        echo "===== use LINUX pre-compiler ====="
        ./rust-msc-latest-linux
    fi
    wait
    cd -
elif [ -n "$3" ]; then
    echo '*ERROR*: pre-compiler is wrong, support value [PRECPL]'
    exit 1
fi

#-------------------- BUILD --------------------#
if [ "$mode" = "REPL" ] || [ "$mode" = "ALL" ]; then
    output_dir=./linux/repl/output
    mkdir -p $output_dir
    cd $output_dir
    cmake -G "Unix Makefiles" -D"MODE=REPL" -D"CORE=$core" "../.."
    make
    cd ../../../
fi

if [ "$mode" = "FILE" ] || [ "$mode" = "ALL" ]; then
    output_dir=./linux/file/output
    mkdir -p $output_dir
    cd $output_dir
    cmake -G "Unix Makefiles" -D"MODE=FILE" -D"CORE=$core" "../.."
    make
fi
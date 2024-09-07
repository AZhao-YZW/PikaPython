#-------------------- HELP ---------------------#
if [ "$1" = "-h" ]; then
    echo 'FORMAT: sh run_benchmark.sh [LEAF|PIKA]'
    exit 0
fi

#----------------- SELECT CORE -----------------#
if [ ! -n "$1" ]; then
    core="LEAF"
    echo '===== default use LEAF core ====='
elif [ "$1" = "LEAF" ] || [ "$1" = "PIKA" ]; then
    core="$1"
    echo "===== use $1 core ====="
else
    echo '*ERROR*: core is wrong, support value [LEAF|PIKA]'
    exit 0
fi

#------------------ RUN BENCH ------------------#
if [ "$core" = "LEAF" ]; then
    output_dir=leaf
elif [ "$core" = "PIKA" ]; then
    output_dir=pika
fi

pika_dir=../../build/linux/pikapython
file_dir=../../build/linux/file
benchmark_dir=.
cp ${file_dir}/output/$output_dir/leafpython ${pika_dir}
mv ${pika_dir}/main.py ${pika_dir}/main.py.bak
cp ${benchmark_dir}/mem_test.py ${pika_dir}/main.py
./${pika_dir}/leafpython ${pika_dir}/main.py
rm -f ${pika_dir}/main.py
cp ${benchmark_dir}/perf_test.py ${pika_dir}/main.py
./${pika_dir}/leafpython ${pika_dir}/main.py
rm -f ${pika_dir}/leafpython
rm -f ${pika_dir}/main.py
mv ${pika_dir}/main.py.bak ${pika_dir}/main.py
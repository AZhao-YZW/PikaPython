pika_dir=../../build/linux/pikapython
file_dir=../../build/linux/file
benchmark_dir=.
cp ${file_dir}/output/leafpython ${pika_dir}
mv ${pika_dir}/main.py ${pika_dir}/main.py.bak
cp ${benchmark_dir}/mem_test.py ${pika_dir}/main.py
./${pika_dir}/leafpython ${pika_dir}/main.py
rm -f ${pika_dir}/main.py
cp ${benchmark_dir}/perf_test.py ${pika_dir}/main.py
./${pika_dir}/leafpython ${pika_dir}/main.py
rm -f ${pika_dir}/leafpython
rm -f ${pika_dir}/main.py
mv ${pika_dir}/main.py.bak ${pika_dir}/main.py
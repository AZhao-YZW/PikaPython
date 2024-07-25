rm -rf ./output
mkdir -p output
cd output
cmake -G "Unix Makefiles" ../linux
make
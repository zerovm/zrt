#!/bin/bash

#run from project directory like as debug/copy.sh

rm ./debug/gcc/files/* -f

cp ./debug/gcc/intermediate_data/r_map2_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map3_map1 ./debug/gcc/files 
cp ./debug/gcc/intermediate_data/r_map4_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map1_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map3_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map4_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map1_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map2_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map4_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map1_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map2_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_map3_map4 ./debug/gcc/files

cp ./debug/gcc/intermediate_data/r_red5_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red6_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red7_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red8_map1 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red9_map1 ./debug/gcc/files

cp ./debug/gcc/intermediate_data/r_red5_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red6_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red7_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red8_map2 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red9_map2 ./debug/gcc/files

cp ./debug/gcc/intermediate_data/r_red5_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red6_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red7_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red8_map3 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red9_map3 ./debug/gcc/files

cp ./debug/gcc/intermediate_data/r_red5_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red6_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red7_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red8_map4 ./debug/gcc/files
cp ./debug/gcc/intermediate_data/r_red9_map4 ./debug/gcc/files


#echo copy channel emulations, to run map-reduce without zerovm
 
cp data/1input.txt ./debug/gcc/files/r_map1_stdin -f
#touch ./debug/gcc/files/r_map1_stdin

cp data/2input.txt ./debug/gcc/files/r_map2_stdin -f
#touch ./debug/gcc/files/r_map2_stdin

cp data/3input.txt ./debug/gcc/files/r_map3_stdin -f
#touch ./debug/gcc/files/r_map3_stdin

cp data/4input.txt ./debug/gcc/files/r_map4_stdin -f
#touch ./debug/gcc/files/r_map4_stdin



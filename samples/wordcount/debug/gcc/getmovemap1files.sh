
#call it after exec map1 node if changed comm data protocol, before ./copy.sh

chmod u+rw ./debug/gcc/files/w_map1_map*
mv ./debug/gcc/files/w_map1_map2 ./debug/gcc/intermediate_data/r_map2_map1
mv ./debug/gcc/files/w_map1_map3 ./debug/gcc/intermediate_data/r_map3_map1
mv ./debug/gcc/files/w_map1_map4 ./debug/gcc/intermediate_data/r_map4_map1

chmod u+rw ./debug/gcc/files/w_map1_red*
mv ./debug/gcc/files/w_map1_red5 ./debug/gcc/intermediate_data/r_red5_map1
mv ./debug/gcc/files/w_map1_red6 ./debug/gcc/intermediate_data/r_red6_map1
mv ./debug/gcc/files/w_map1_red7 ./debug/gcc/intermediate_data/r_red7_map1
mv ./debug/gcc/files/w_map1_red8 ./debug/gcc/intermediate_data/r_red8_map1
mv ./debug/gcc/files/w_map1_red9 ./debug/gcc/intermediate_data/r_red9_map1



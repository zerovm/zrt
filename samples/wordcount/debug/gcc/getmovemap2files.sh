
#call it after exec map2 node if changed comm data protocol, before ./copy.sh

chmod u+rw ./debug/gcc/files/w_map2_map*
mv ./debug/gcc/files/w_map2_map1 ./debug/gcc/intermediate_data/r_map1_map2
mv ./debug/gcc/files/w_map2_map3 ./debug/gcc/intermediate_data/r_map3_map2
mv ./debug/gcc/files/w_map2_map4 ./debug/gcc/intermediate_data/r_map4_map2

chmod u+rw ./debug/gcc/files/w_map2_red*
mv ./debug/gcc/files/w_map2_red5 ./debug/gcc/intermediate_data/r_red5_map2
mv ./debug/gcc/files/w_map2_red6 ./debug/gcc/intermediate_data/r_red6_map2
mv ./debug/gcc/files/w_map2_red7 ./debug/gcc/intermediate_data/r_red7_map2
mv ./debug/gcc/files/w_map2_red8 ./debug/gcc/intermediate_data/r_red8_map2
mv ./debug/gcc/files/w_map2_red9 ./debug/gcc/intermediate_data/r_red9_map2



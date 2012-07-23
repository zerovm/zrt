
#call it after exec map3 node if changed comm data protocol, before ./copy.sh

chmod u+rw ./debug/gcc/files/w_map3_map*
mv ./debug/gcc/files/w_map3_map1 ./debug/gcc/intermediate_data/r_map1_map3
mv ./debug/gcc/files/w_map3_map2 ./debug/gcc/intermediate_data/r_map2_map3
mv ./debug/gcc/files/w_map3_map4 ./debug/gcc/intermediate_data/r_map4_map3

chmod u+rw ./debug/gcc/files/w_map3_red*
mv ./debug/gcc/files/w_map3_red5 ./debug/gcc/intermediate_data/r_red5_map3
mv ./debug/gcc/files/w_map3_red6 ./debug/gcc/intermediate_data/r_red6_map3
mv ./debug/gcc/files/w_map3_red7 ./debug/gcc/intermediate_data/r_red7_map3
mv ./debug/gcc/files/w_map3_red8 ./debug/gcc/intermediate_data/r_red8_map3
mv ./debug/gcc/files/w_map3_red9 ./debug/gcc/intermediate_data/r_red9_map3



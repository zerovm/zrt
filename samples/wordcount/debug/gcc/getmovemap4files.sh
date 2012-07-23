
#call it after exec map4 node if changed comm data protocol, before ./copy.sh

chmod u+rw ./debug/gcc/files/w_map4_map*
mv ./debug/gcc/files/w_map4_map1 ./debug/gcc/intermediate_data/r_map1_map4
mv ./debug/gcc/files/w_map4_map2 ./debug/gcc/intermediate_data/r_map2_map4
mv ./debug/gcc/files/w_map4_map3 ./debug/gcc/intermediate_data/r_map3_map4

chmod u+rw ./debug/gcc/files/w_map4_red*
mv ./debug/gcc/files/w_map4_red5 ./debug/gcc/intermediate_data/r_red5_map4
mv ./debug/gcc/files/w_map4_red6 ./debug/gcc/intermediate_data/r_red6_map4
mv ./debug/gcc/files/w_map4_red7 ./debug/gcc/intermediate_data/r_red7_map4
mv ./debug/gcc/files/w_map4_red8 ./debug/gcc/intermediate_data/r_red8_map4
mv ./debug/gcc/files/w_map4_red9 ./debug/gcc/intermediate_data/r_red9_map4


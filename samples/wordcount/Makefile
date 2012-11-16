include $(ZRT_ROOT)/Makefile.env
CFLAGS+=-std=c99 -g -DDEBUG

LDFLAGS+=-lmapreduce -lnetworking
all: createdirs clean map.nexe reduce.nexe

map.nexe: obj/map.o obj/user_implem.o
	@$(CC) -o map.nexe obj/map.o obj/user_implem.o $(LDFLAGS)
	$(BINPATCH) map.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

reduce.nexe: obj/reduce.o obj/user_implem.o
	@$(CC) -o reduce.nexe obj/reduce.o obj/user_implem.o $(LDFLAGS)
	$(BINPATCH) reduce.nexe 4c01fc0f3148c1e2 4c01fc909048c1e2

obj/map.o: src/map.c
	@$(CC) -c -o obj/map.o ${CFLAGS} src/map.c

obj/reduce.o: src/reduce.c
	@$(CC) -c -o obj/reduce.o ${CFLAGS} src/reduce.c

obj/user_implem.o: src/user_implem.c src/user_implem.h
	@$(CC) -c -o obj/user_implem.o ${CFLAGS} src/user_implem.c -D_GNU_SOURCE

createdirs:
	chmod u+rwx genmanifest.sh mapred_conf.sh mr_start.sh
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f data/*.res
	@rm -f manifest/*.manifest	
	@rm -f *.nexe
	@rm -f nameservice.log



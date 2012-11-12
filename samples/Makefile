samples := disort hello readdir reqrep sort_paging tarimage time wordcount zshell
samples_clean = $(patsubst %,%.clean,$(samples))

.PHONY: all $(samples) $(samples_clean)

all: $(samples)

clean: $(samples_clean)

$(samples):
	$(MAKE) --directory=$@

$(samples_clean):
	$(MAKE) --directory=$(@:.clean=) clean


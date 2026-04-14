MPICC=mpicc
CFLAGS=-O2
LIBS=

ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif

main: sample.c
	$(MPICC) $(CFLAGS) $< -o sample $(LIBS)

run: main
	mpirun -np 4 ./sample $(RUN_ARGS)

clean:
	rm -f sample

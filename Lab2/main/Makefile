MPICC=mpicc
CFLAGS=-O2
LIBS=-lm

ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif


main: sample.c
	${MPICC} ${CFLAGS} $< -o a.out ${LIBS}

run:
	mpirun -np 4 ./a.out $(RUN_ARGS) 

clean:
	rm -f a.out mpi
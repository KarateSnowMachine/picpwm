SRC=main_interrupt_rx.c

CC=sdcc
FAMILY=pic16
PROC=18f4550

all: $(SRC:.c=.hex)

CFLAGS=-m${FAMILY} -p${PROC} --use-non-free 

$(SRC:.c=.hex): $(SRC)
	$(CC) --use-non-free ${CFLAGS} $^
#    /bin/bash add_cfg.sh $@


.PHONY: all clean

clean:
	rm -f $(SRC:.c=.asm) $(SRC:.c=.cod) $(SRC:.c=.hex) $(SRC:.c=.lst) $(SRC:.c=.o)

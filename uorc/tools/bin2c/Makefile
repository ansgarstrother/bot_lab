BIN2C_OBJS = main.o

all: bin2c


bin2c: $(BIN2C_OBJS)
	gcc -o $@ $(BIN2C_OBJS)

%.o: %.c
	gcc -c $<


clean:
	rm bin2c $(BIN2C_OBJS) *~

CC = clang++ -g -Wall
CFLAG = -c
LDFLAG = -lX11 -lXcomposite -lXrender -lpthread -lXtst -lXdamage

BIN  = Monitor
OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) 

all : $(BIN)
$(BIN) : ${OBJ}
	$(CC) $^ -o $@ $(LDFLAG) && rm -f *.o *~ *.orig

$(OBJ) : %.o : %.cpp
	$(CC) $(CFLAG) $< -o $@

clean :
	rm -f $(BIN)

install:
	rm -f /usr/local/bin/$(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)
	chmod +s /usr/local/bin/$(BIN)

CC = clang++ -g -Wall -O0
CFLAG = -c
LDFLAG = -lX11 -lXcomposite -lXrender -lpthread

BIN  = Mirror
OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) 

all : $(BIN)
$(BIN) : ${OBJ}
	$(CC) $^ -o $@ $(LDFLAG) && rm -f *.o *~

$(OBJ) : %.o : %.cpp
	$(CC) $(CFLAG) $< -o $@

clean :
	-rm $(BIN)

install:
	-rm /usr/local/bin/$(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)
	chmod +s /usr/local/bin/$(BIN)


SRCS = ./*.c


all: mylib.o helloworld.o
#g++ -g ${SRCS} -lmylibs -o ../.vscode/hello.exe
	gcc -o hello.exe mytest.o helloworld.o 

mylib.o:
	gcc -c ../libs/mytest.c

helloworld.o: helloworld.c
	gcc  -DMSG="\"xwh"\" -c helloworld.c
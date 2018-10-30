all: hw4.c static.a dynamic1.so dynamic2.so
	gcc -c hw4.c
	gcc hw4.o -L. staticLib.c -Wl,-rpath="." -L. firstDynamicLib.c -ldl -o hw4.out
static.a: staticLib.c
	gcc -c staticLib.c
	ar rc static.a staticLib.o
dynamic1.so: firstDynamicLib.c
	gcc -fPIC -c firstDynamicLib.c
	gcc -shared -o dynamic1.so firstDynamicLib.o
dynamic2.so: secondDynamicLib.c
	gcc -fPIC -c secondDynamicLib.c
	gcc -shared -o dynamic2.so secondDynamicLib.o
clean:
	rm *.a *.o *.so hw4.out
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdbool.h>


int mod(int a, int b);
int divo(int a, int b);
bool isGreater(int a, int b);
bool isLesser(int a, int b);
bool isEqual(int a, int b);

int main(){
	printf("%s\n", "Static library check");
	printf("%s%d\n", "mod(4, 2) == ", mod(4, 2));
	printf("%s%d\n", "div(6, 2) == ", divo(6, 2));
	printf("%s\n", "First dynamic library check");
	if (isEqual(1, 1) && isLesser(1, 10) && !isGreater(1, 10)){
		printf("%s\n", "1 equals 1, 1 is lesser than 10, 1 is not greater than 10");
	}
	else{
		printf("%s\n", "Something went wrong :/");
	}

	printf("%s\n", "Second dynamic library check");
	void* shared = dlopen("dynamic2.so", RTLD_LAZY);
	if (shared == NULL){
		dlerror();
		exit(EXIT_FAILURE);
	}
	void* addFunc = dlsym(shared, "add"); 
	void* subFunc = dlsym(shared, "sub");
	if (addFunc == NULL || subFunc == NULL){
		dlerror();
		exit(EXIT_FAILURE);
	}
	int (*addPointer) (int, int) = (int (*) (int, int)) addFunc;
	int (*subPointer) (int, int) = (int (*) (int, int)) subFunc;
	printf("%s%d%s%d%s\n", "2 + 2 is ", addPointer(2, 2), " minus 1 that`s ", subPointer(addPointer(2, 2), 1), " quick maths");
	if (dlclose(shared) != 0){
		dlerror();
	}


}
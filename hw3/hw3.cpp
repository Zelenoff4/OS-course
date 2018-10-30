#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <fstream>

int main(int argc, const char* argv[]){
	//machine code for function that returns the square of int value
	/*unsigned char asFunc[] = {0x55, 0x48, 0x89, 0xe5, 0x89, 
								0x7d, 0xfc, 0x8b, 0x45, 0xfc, 
								0x0f, 0xaf, 0x45, 0xfc, 0x5d, 0xc3};
	*/


	//machine code for function that adds constant to the int value
	//machine code works only for 8 bit signed values :c
	unsigned char asFunc[] = {0x48, 0x89, 0xf8, 0x48, 0x83, 0xc0, 4, 0xc3};
	if (argc <= 2){
		std::perror("Supposed to be two 8 bit int arguments");
		return 1;
	}
	unsigned char constant = atoi(argv[2]);
	asFunc[6] = constant;
	int argument = atoi(argv[1]);
	if (creat("Square", S_IRWXU) == -1){
		perror("Error while creating file for machine code usage");
		return 1;
	}
	//create file to write machine code into
	int readWriteDescriptor = open("Square", O_RDWR);
	if (readWriteDescriptor == -1){
		perror("Error while opening the file for machine code usage");
		return 1;
	}
	if (write(readWriteDescriptor, asFunc, sizeof(asFunc)) == -1){
		perror("Error while writing to the file for machinge code usage");
		return 1;
	}
	if (close(readWriteDescriptor) == -1){
		perror("Error while closing the file for machine code usage");
		return 1;
	}
	//open created file as RDONLY to read the machine code
	int readOnlyDescriptor = open("Square", O_RDONLY);
	if (readOnlyDescriptor == -1){
		perror("Error while opening the file for machine code usage");
		return 1;
	}
	void* pointer;
	pointer = mmap(NULL, sizeof(asFunc), PROT_READ | PROT_WRITE, MAP_PRIVATE, readOnlyDescriptor, 0);
	if (pointer == MAP_FAILED){
		perror("Error: unable to allocate memory with mmap");
		return 1;
	}
	if (mprotect(pointer, sizeof(asFunc), PROT_EXEC) == -1){
		perror("Error: unable to change allocated memory space to executable with mprotect");
		return 1;
	}
	int (*functionPointer) (int) = (int (*) (int)) pointer;
	std::cout << functionPointer(argument) << '\n';
	if (munmap(pointer, sizeof(asFunc)) == -1){
		perror("Error: unable to free allocated memory space");
		return 1;
	}
	if (close(readOnlyDescriptor) == -1){
		perror("Error while closing the file for machine usage");
		return 1;
	}
}
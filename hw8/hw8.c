#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/mman.h>
#include <ucontext.h>

const int amount = 10;
//dunno why, but i am unable to use __NGREG, so had to hard define it :c
const ssize_t _NGREG = 23;

jmp_buf jump;

void memory_handler(int signal, siginfo_t *info, void* context){
	if (signal == SIGSEGV){
		longjmp(jump, 1);
	}
}

int checkMem(char* address){
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGSEGV);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	struct sigaction action;
	action.sa_flags = SA_SIGINFO;
	action.sa_sigaction = memory_handler;
	action.sa_mask = set;
	if (sigaction(SIGSEGV, &action, NULL) < 0){
		perror("Sigaction failed");
		exit(EXIT_FAILURE);
	}
	if (setjmp(jump) == 0){
		return (int)*(address);
	}
	else {
		return 123456;
	}
}

const char *regs[] = {
        "REG_R8", "REG_R9", "REG_R10", "REG_R11", "REG_R12", "REG_R13", "REG_R14", "REG_R15", "REG_RDI", "REG_RSI",
        "REG_RBP", "REG_RBX", "REG_RDX", "REG_RAX", "REG_RCX", "REG_RSP", "REG_RIP", "REG_EFL", "REG_CSGSFS", "REG_ERR",
        "REG_TRAPNO", "REG_OLDMASK", "REG_CR2"
};

void handler(int signal, siginfo_t *info, void* context){
	if (signal == SIGSEGV){
		printf("Segmentation fault by memory address: %p\n", info->si_addr);
		printf("=======MEMORY DUMP=======\n");
		char* address = (char *) info->si_addr;
		address -= amount;
		for (int i = -amount; i < amount; i++, address++){
			char notify[] = "        ";
			//to show where exactly we've failed
			if (i == 0){
				memcpy(notify, " <=fault", 8);
			}
			if (checkMem(address) != 123456){
				printf("Address = %p : value = '%c'(%d)%s\n", address, *address, *address, notify);
			} else{
				printf("Bad address at %p%s\n", address, notify);
			}
		}
		printf("=======REGS DUMP=======\n");
		//somehow my ubuntu doesn't see mcontext, it has uc_mcontext
		mcontext_t mcontext = ((ucontext_t *) context)->uc_mcontext;
		for (ssize_t i = 0; i < _NGREG; i++){
			printf("%s = %u\n", regs[i], (unsigned int) mcontext.gregs[i]);
		}
		exit(EXIT_FAILURE);
	}
}

int main(){
	struct sigaction *action = calloc(0x00, sizeof(struct sigaction));
	action->sa_flags = SA_SIGINFO;
	action->sa_sigaction = handler;
	if (sigaction(SIGSEGV, action, NULL) < 0){
		perror("Sigaction fail");
		return 1;
	}
	char* pointer = (char *) mmap(NULL, 3, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	pointer[-1] = '0';
	return 0;
}
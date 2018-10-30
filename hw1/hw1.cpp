#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;


int main(int argc, char *argv[]){
	while (1){
		string line;
		getline(cin, line);
		if (line == "exit" || cin.eof()){
			return 0;
		}
		vector<string> rArgs;
		for (size_t i = 0; i < line.length(); i++){
			string tmp = "";
			while (i < line.length() && line[i] != ' '){
				tmp += line[i];
				i += 1;
			} 
			rArgs.push_back(tmp);
		}
		char *args[256];
		for (size_t i = 0; i < rArgs.size(); i++){
			args[i] = (char*)rArgs[i].c_str();
		}
		args[rArgs.size()] = NULL;
		pid_t pid = fork();
		if (pid < 0){
			perror("Fork failed");
			return 1; 
		}
		if (pid == 0){
			//printf("Entered child %d\n", (int) getpid());
			if (execve(args[0], args, (char* const*)NULL) < 0){
				perror("Execve failed");
				return 1;
			}
		}
		int status = 0;
		pid_t childpid = wait(&status);
		if (childpid < 0){
			perror("Wait failed");
			return 1;
		}
		cout << "Programm returned with status = " << status << '\n'; 
	}

}
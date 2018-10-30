#include <vector>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <queue>
#include <string>
#include <algorithm>
#include <cerrno>
#include <wait.h>
#include <iostream>

using namespace std;


int parseString(string word, int till = 0){
	string nums = "1234567890";
	int temp = 0, degree = 1;
	for (int k = word.length() - 1; k >= till; k--){
		if (nums.find(word[k]) > 10)
			return -1;
		int ch = word[k] - '0';
		temp += ch * degree;
		degree *= 10;
	}
	return temp;
}

struct inum
{
	int inode;

	inum(){
		this->inode = -1;
	}

	inum(int _inode){
		this->inode = _inode;
	}

};

struct nlink
{
	int n;
	nlink(){
		this->n = -1;
	}

	nlink(int _n){
		this->n = _n;
	}
};

inum parseInum(string s){
	return inum(parseString(s));
	
}

nlink parseNLink(string s){
	return nlink(parseString(s));
}


struct ssize
{
	int size = -1;
	char operand = '!';
	ssize(){
		this->size = -1;
		this->operand = '!';
	}

	ssize(int _size, char _operand){
		this->size = _size;
		this->operand = _operand;
	}
};

ssize parseSize(string s){
	ssize ans;
	if (s[0] == '+' || s[0] == '-' || s[0] == '='){
		ans.operand = s[0];
		ans.size = parseString(s, 1);
	}
	return ans;
}

struct filename
{
	string name;
	filename(string _name){
		this->name = _name;
	}

	filename(){
		this->name = "";
	}
};

filename parseFileName(string s){
	return filename(s);
}

struct execute
{
	string path;
	struct stat st;

	execute(string _path){
		this->path = _path;
	}

	execute(){
		this->path = "";
	}
};

execute parseExecute(string s){
	execute ans;
	if (stat(s.c_str(), &ans.st) < 0){
		if (errno){
			perror("");
		}
		return ans;
	}
	if ((ans.st.st_mode & S_IFMT) != S_IFREG){
		return ans;
	}
	ans.path = s;
	return ans;
}

struct directory
{
	string name, fname;
	int inode;
	char type;
};

struct linux_dirent {
    long d_ino;
    off_t d_off;
    unsigned short d_reclen;
    char d_name[];
};

std::vector<directory> get_dir(std::string path) {
    int fd, nread;
    struct linux_dirent *d;
    unsigned int nbyte = 1024;
    char buf[1024];
    int bpos;
    char d_type;
    vector<directory> dirs;

    fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
    	if (errno){
    		perror("");
    	}
        return dirs;
    }
    for (;;) {
        nread = syscall(SYS_getdents, fd, buf, nbyte);
        if (nread == 0) {
            break;
        }
        if (nread < 0) {
            if (errno){
    			perror("");
    		}
            break;
        }
        for (bpos = 0; bpos < nread;) {
            directory dir;
            d = (struct linux_dirent *) (buf + bpos);
            d_type = *(buf + bpos + d->d_reclen - 1);
            dir.name = d->d_name;
            dir.type = d_type;
            dirs.push_back(dir);
            bpos += d->d_reclen;
        }
    }
    if (close(fd) < 0) {
        if (errno){
    		perror("");
    	}
    }
    return dirs;
}

bool checkFile(ssize argSize, inum inumValue, filename fileName, nlink nlinkValue, string fname, struct stat st){
	if (nlinkValue.n != -1){
		if (!(st.st_nlink == (size_t) nlinkValue.n)){
			return false;
		}
	}
	if (inumValue.inode != -1){
		if (!(st.st_ino == (__ino_t) inumValue.inode)){
			return false;
		}
	}
	if (argSize.size != -1){
		switch (argSize.operand){
			case '-':
				if (!(st.st_size < argSize.size))
					return false;
				break;
			case '=':
				if(!(st.st_size == argSize.size)){
					return false;
				}
				break;
			case '+':
				if (!(st.st_size > argSize.size)){
					return false;
				}
				break;
		}
	}
	if (fileName.name != ""){
		if (!(fileName.name == fname)){
			return false;
		}
	}
	return true;
	//ну и гадость
}

int main(int argc, const char* argv[]){
	if (argc < 2){
		perror("Amount of arguments is less than 2.\n");
		printf("Usage: ./find [full path of directory] [-size| -nlink| -exec| -inum| -name]\n");
		return 0;
	}
	bool sizeSet = false, nlinkSet = false, nameSet = false, execSet = false, inumSet = false;
	ssize argSize;
	nlink nlinkValue;
	inum inumValue;
	execute execPath;
	filename fileName;
	struct stat st;
	string path = argv[1];
	if (stat(path.c_str(), &st) < 0){
		if (errno){
			perror(path.c_str());
			return 1;
		}
	}
	if ((st.st_mode & S_IFMT) != S_IFDIR){
		perror((path + " is not a directory").c_str());
		return 1;
	}
	for (int i = 2; i < argc; i++){
		bool correctOption = false;
		if (!strcmp(argv[i], "-size")){
			if (i + 1  < argc){
				if (sizeSet){
					perror(((string) argv[i] + " argument has already been set").c_str());
					return 1;
				}
				argSize = parseSize(argv[i + 1]);
				if (argSize.operand == '!'){
					perror(("Wrong argument for " + (string) argv[i] + " option, expected -value | +value | =value, but found " + (string) argv[i + 1]).c_str());
					return 1;
				}
				sizeSet = true;
				correctOption = true;
				i += 1;
			}
			else {
				perror(("Option " + (string) argv[i] + " doesnt have an argument").c_str());
				return 1;
			}
		}
		if (!strcmp(argv[i], "-inum")){
			if (i + 1 < argc){
				if (inumSet){
					perror(((string) argv[i] + " argument has already been set").c_str());
					return 1;
				}
				inumValue = parseInum(argv[i + 1]);
				if (inumValue.inode == -1){
					perror(("Wrong argument for " + (string) argv[i] + " option, expected int value, but found " + (string) argv[i + 1]).c_str());
					return 1;
				}
				inumSet = true;
				correctOption = true;
				i += 1;
			}
			else{
				perror(("Option " + (string) argv[i] + " doesnt have an argument").c_str());
			}
		}
		if (!strcmp(argv[i], "-nlink")){
			if (i + 1 < argc){
				if (nlinkSet){
					perror(((string) argv[i] + " argument has already been set").c_str());
					return 1;
				}
				nlinkValue = parseNLink(argv[i + 1]);
				if (nlinkValue.n == -1){
					perror(("Wrong argument for " + (string) argv[i] + " option, expected int value, but found " + (string) argv[i + 1]).c_str());
					return 1;
				}
				nlinkValue = true;
				correctOption = true;
				i += 1;
			}
			else{
				perror(("Option " + (string) argv[i] + " doesnt have an argument").c_str());
			}
		}
		if (!strcmp(argv[i], "-exec")){
			if (i + 1 < argc){
				if (execSet){
					perror(((string) argv[i] + " argument has already been set").c_str());
					return 1;
				}
				execPath = parseExecute(argv[i + 1]);
				if (execPath.path == ""){
					perror(("Wrong argument for " + (string) argv[i] + " option, expected string path, but found " + (string) argv[i + 1]).c_str());
					return 1;
				}
				execSet = true;
				correctOption = true;
				i += 1;
			}
			else{
				perror(("Option " + (string) argv[i] + " doesnt have an argument").c_str());
			}
		}
		if (!strcmp(argv[i], "-name")){
			if (i + 1 < argc){
				if (nameSet){
					perror(((string) argv[i] + " argument has already been set").c_str());
					return 1;
				}
				fileName = parseFileName(argv[i + 1]);
				if (fileName.name == ""){
					perror(("Wrong argument for " + (string) argv[i] + " option, expected string path, but found " + (string) argv[i + 1]).c_str());
					return 1;
				}
				nameSet = true;
				correctOption = true;
				i += 1;
			}
			else{
				perror(("Option " + (string) argv[i] + " doesnt have an argument").c_str());
			}
		}
		if (!correctOption){
			perror(("Invalid option. Expected [-size| -nlink| -exec| -inum| -name], found " + (string) argv[i]).c_str());
			return 1;
		}
	}

	
	directory start;
	start.type = DT_DIR;
	start.name = path.c_str();
	struct stat st_;
	if (stat(start.name.c_str(), &st_) < 0){
		if (errno){
			perror(path.c_str());
			return 1;
		}
	}
	start.inode = st_.st_ino;
	queue<directory> queue;
	vector<directory> dirs;
	queue.push(start);
	while (!queue.empty()){
		directory curr = queue.front();
		queue.pop();
		struct stat st;
		if (stat(curr.name.c_str(), &st) < 0){
			if (errno){
				perror("");
			}
		}
		curr.inode = st.st_ino;
		if (checkFile(argSize, inumValue, fileName, nlinkValue, curr.fname, st)){
			dirs.push_back(curr);
		}
		vector<directory> rec;
		if (curr.type == DT_DIR){
			rec = get_dir(curr.name);
		}
		for (size_t i = 0; i < rec.size(); i++){
			if (rec[i].name == "." || rec[i].name == ".."){
				continue;
			}
			directory tmp;
			tmp.fname = rec[i].name;
			tmp.name = curr.name + '/' + rec[i].name;
			tmp.type = rec[i].type;
			queue.push(tmp);
		}
	}
	for (size_t i = 0; i < dirs.size(); i++){
		printf("%s\n", dirs[i].name.c_str());
		if (execPath.path != ""){
			vector<string> todo;

			if (dirs[i].type == DT_REG){
				todo.push_back(dirs[i].name);
			}
			for (size_t i = 0; i < todo.size(); i++){
				 char* args[] = {const_cast<char *>(execPath.path.c_str()), const_cast<char *>(todo[i].c_str()), (char *)NULL};
				 pid_t pid = fork();
				 if (pid < 0){
					perror("Fork failed");
					return 1; 
				}
				if (pid == 0){
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
	}

	
	return 0;

}
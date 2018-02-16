#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <stack>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <unordered_map>
#include <fcntl.h>
using std::unordered_map;
using std::stack;
using std::vector;
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::cerr;


const char** getCharArgv( const vector<string>& v);
void parseArgv(string& line ,vector<string>& parse);
bool getInstruction (string& line, vector<string>& instruction,unordered_map<string, string>& var2val );
string searchPATH(const string&name,unordered_map<string, string>& var2val );
bool shellInstruction(string& line, vector<string>& instruction,unordered_map<string, string>& var2val );
bool preExecuteInstruction( vector<string> & instruction , unordered_map<string, string>& var2val);
void childProcess( vector<string>& instruction);

//return const char** from vector<string>& , called by childProcess
const char** getCharArgv(const vector<string>& v) {
  const char** ptr = new const char*[ v.size() + 1];
  for(size_t i = 0; i<v.size(); i++){
    ptr[i] = v[i].c_str();
  }
  ptr[v.size()] = NULL;
  return ptr;  
}

//parse a line of instruction into vector<string
void parseArgv(string& line ,vector<string>& parse) {
  if(line.size() == 0 || ( line.size() == 1 && line[0] == 0))  return;
  string temp;
  
  if(line[0] != ' ')
    temp += line[0];
  for(size_t i = 1; i <line.size(); i++){
    if(line[i] != ' ' ) {
      temp += line[i];
      continue;
    }
    if(line[i] == ' ' && temp.back() == '\\') {
      temp.pop_back();
      temp += line[i];
      continue;
    }

    if(line[i] == ' '&& temp.back() == ' ') {
      parse.push_back(temp);
      temp = "";
      continue;
    }
    if( line[i] == ' ' && line[i-1] != ' ' && line[i-1] != '\\') {
      parse.push_back(temp);
      temp = "";
    }
  }
  
  if(temp.size() != 0)
    parse.push_back(temp);
  return;
}

string getValueOfVar(string&name, unordered_map<string,string>& var2val) {
  auto it = var2val.find(name);
  if(it != var2val.end() ) {
    return it->second;
  }
  char* _env = getenv(name.c_str());
  return _env == NULL ? "" : string(_env);
}


//getline a line of instruction and call parseArgv() to parse the instrction to vector<string>; then replace $;  when "exit" or EOF ,  return false, other situation return true)
bool getInstruction (string& line, vector<string>& instruction, unordered_map<string,string>& var2val  ){ 
  //  cout << "in father get instruction\n";
  getline(cin, line);
  if(cin.eof()){
      cerr<<"eof()"<<endl;
      return false;
    }
  parseArgv(line , instruction);    
  //repalce $varname
  for(size_t i=0; i<instruction.size(); i++ ) {
    bool isVar = false;
    string curr = instruction[i], temp, toSwap;
     
    if(curr.size() == 0) continue;
    for(size_t j = 0; j<curr.size(); j++ ) {

      if((!isVar) && curr[j] != '$') {
	if( (j > 0 && toSwap.back() == '$')  ) {
	  isVar = true;
	  temp.push_back(curr[j]);
	  toSwap.pop_back();
	} else {
	  toSwap.push_back(curr[j]);
	}
	continue;
      }

      if((!isVar) && curr[j] == '$'){
	if(j>0 && toSwap.back() == '$') {
	  toSwap.pop_back();
	  toSwap.append(std::to_string(getpid()));
	}
	else {
	  toSwap.push_back('$');
	}
	continue;
      }
      
      if((isVar) && curr[j] == '$'){
	isVar = false;
	temp = getValueOfVar(temp, var2val);
	toSwap.append(temp+'$');
	temp = "";
	continue;
      }
      
      if((isVar) && curr[j] != '$'){
	temp.push_back(curr[j]);
	continue;
      }
    }
    if(isVar) {
      temp = getValueOfVar(temp,var2val);
      toSwap.append(temp);
    }
    instruction[i] = toSwap;
  }
  if( (instruction.size() != 0 && instruction[0] == "exit") )  {
    return false;
  }
  return true;
}

// seperate  env of PATH into reference queue, then search for the whole possible path for name
string searchPATH(const string&name, unordered_map<string,string>& var2val ){
  string path(getenv("PATH"));
  stack<string> stack_envPATH; // DFS inorder
  stack<string> stack_reverse; // get from getenv, then push into stack_envPATH to guarantee the search is in order
  int pos = 0;
  for(size_t i = 0; i<path.size() -1 ; i++){
    if(path[i] == ':'){
      stack_reverse.push(path.substr(pos, i-pos));
      pos = i+1;
    }
  }
  stack_reverse.push( path.substr( pos, path.size() - pos ) ) ;
  while(!stack_reverse.empty()){
    stack_envPATH.push( stack_reverse.top() );
    stack_reverse.pop();
  }
  
  //DFS search the possible file, find the matched path, push_back in to v
  while(!stack_envPATH.empty()){
    string current = stack_envPATH.top();
    stack_envPATH.pop();
    const char* pathname = current.c_str();
    struct dirent* filename;
    DIR* dir;
    dir = opendir( pathname );
    if(dir == NULL) {
      perror("Pathname fault:");
      continue;
    }
    while( (filename = readdir(dir)) != NULL){ //check every file in a directory
      if(strcmp(filename->d_name, ".") == 0 || strcmp( filename->d_name , "..") == 0)
	continue;

      //check if childpath is a directory, if so push into stack dfs, if not check filename==name
      string childpath = pathname + string("/") + filename->d_name;
      struct stat s;
      lstat(childpath.c_str(),&s);
      if(S_ISDIR(s.st_mode)){
	stack_envPATH.push(childpath);
	continue;
      }
      
      if(strcmp(filename->d_name, name.c_str() ) == 0 ) {
	free(dir);
	return childpath;
      }
    }
    free(dir); 
  }
  return "";
}

//to check if a varname is a valid name
bool isValidName(string& var){
  if(var.size() == 0)
    return false;
  if(var[0] == '_' || (var[0] <= 'Z' && var[0] >= 'A') || (var[0] <= 'z' && var[0] >= 'a') ) {
    for(size_t i = 1 ; i<var.size() ; i++) {
      if( !(var[i] == '_' || (var[i] <= 'Z' && var[i] >= 'A') || (var[i] <= 'z' && var[i] >= 'a') || (var[i] <= '9' && var[i] >= '0' )) )
	return false;
    }      
  }else
    return false;
  return true;
}

//built in instruction like : cd set export
bool shellInstruction(string& line, vector<string>& instruction, unordered_map<string,string>& var2val){
  if(instruction[0] == "cd") {
    if(instruction.size() == 2){
      int cd_state = chdir(instruction[1].c_str());
      if(cd_state == -1)
	perror("Error in cd command:");      
    }else
      cerr << "Error: cd command needs 1 argument" << endl;
    char* cdn = get_current_dir_name();
    setenv("PWD",cdn,1);
    free(cdn); 
    return true;
  }
  
  if(instruction[0] == "set") {
    if(instruction.size() >= 2 ) {
      if( !isValidName(instruction[1])) {
	cerr << "Error: invalid variable name" << endl;
	return true;
      }
      size_t posOfVar = line.find(instruction[1]);
      posOfVar = posOfVar + instruction[1].size() + 1;      
      if(posOfVar >= line.size() ) {
	var2val[instruction[1]] = ""; // only have 2 arguments, set var, without value
      }else {
	var2val[instruction[1]] = line.substr(posOfVar, instruction[1].size() - posOfVar);      	
      }
    }else
      cerr << "Error: set needs 2 other arguments" << endl;    
    return true;
  }
  
  if(instruction[0] == "export") {
    if( instruction.size() == 2) {
      // export
      auto it = var2val.find(instruction[1]);
      if(it != var2val.end() ) {        
	setenv(it->first.c_str(), it->second.c_str(), 1);
	var2val.erase(it);
      } else
	cerr << "variable " << instruction[1] << " not found" << endl;
    }else
      cerr << "export needs 1 argument" << endl;
    return true;
  }

  return false;
}

// verify the validity of  > < 2>
bool redirectPrepare(vector<string>& instruction, vector<string>& redirect) {
  string target[3] = {"<",">","2>"};
  for(int j = 2; j>-1; j--) {
    for(size_t i = 0 ; i<instruction.size(); i++ ) {
      size_t pos = instruction[i].find(target[j]);
      if(pos != string::npos) {
	if( instruction[i] != target[j] || i == 0 || i==instruction.size()-1) {
	  instruction.erase(instruction.begin() + i);
	  cerr << "only support > < 2> with space aruound them" << endl;
	  i--;
	  return false;
	} else {
	  redirect[j] = instruction[i+1];
	  instruction.erase(instruction.begin()+i, instruction.begin() + i + 2);
	  i--;
	}
      }
    }
    
  }    
  return true;
}



// // getInstructin, parse them, verify the validity
bool preExecuteInstruction(vector<string> & instruction, unordered_map<string,string>& var2val,vector<string>& redirect ) {
  string line;
  if( !getInstruction(line, instruction,var2val) ) { //if eof or exit, then exit
    exit(EXIT_SUCCESS);
  }
  if (instruction.size() == 0 )
    return false; //when you input space and enter or just enter
  
  if(instruction[0].find('/') == string::npos) {

    //call shellInstruction eg: cd set export ...
    if( shellInstruction(line, instruction, var2val) )
      return false;
    //cout<<"in preExecution(), before searchPATH:" << instruction[0] <<endl;
    string inPATH = searchPATH(instruction[0], var2val);

    if( inPATH.size() == 0) {
      cout<< "Command " << instruction[0] << " not found" << endl;
      return false; // 
    } else
      instruction[0] = inPATH;
  }else if(instruction[0][0] == '.') {
    instruction[0].erase(0,2);
  }
  
  if(!redirectPrepare(instruction, redirect)) {
    return false;
  }
  
  return true;
}


  

//cpid == 0, get char** from vector<string>, execute child process, if instruction.size() == 0, then exit child process
void childProcess( vector<string>& instruction, vector<string>& redirect) {
 
  int fd[3] = {-1};
  const char** charArgv = getCharArgv(instruction);
  if(redirect[0].size() != 0) {
    fd[0] = open(redirect[0].c_str(), O_RDONLY);
    if( fd[0]==-1 ) {
      perror("file open fail in <:");
      exit(EXIT_FAILURE);
    }
    if ( dup2(fd[0],0) == -1) {
      perror("dup fd[0] fail:");
      exit(EXIT_FAILURE);
    }
  }

  if(redirect[1].size() != 0) {
    fd[1] = open(redirect[1].c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd[1] == -1 ) {
      perror("file open fail in >:");
      exit(EXIT_FAILURE);
    }
    if ( dup2(fd[1],1) == -1) {
      perror("dup fd[1] fail:");
      exit(EXIT_FAILURE);
    }
    
  }
  
  if(redirect[2].size() != 0) {
    fd[2] = open(redirect[2].c_str(),O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd[2] == -1 ) {
      perror("file open fail in 2>:");
      exit(EXIT_FAILURE);
    }
    if ( dup2(fd[2],2) == -1) {
      perror("dup fd[2] fail:");
      exit(EXIT_FAILURE);
    }    
  }

  int e = execve(charArgv[0],(char*const*) charArgv,(char*const*) environ);  
  
  if(e == -1){
    cerr << "Error from execve:\n";
    exit(EXIT_FAILURE);
  }
  for(int i = 0 ; i < 3 ; i++ ) {
    if(fd[i] != -1 ) {
      close(fd[i]);
    }
  }
  delete[] charArgv;
  exit(EXIT_SUCCESS);
}

int main(int argc, char** argv , char** envv){
  unordered_map<string,string> var2val; //map variable name to value  

  while( true ){
    
    cout << "myShell:" << getenv("PWD") <<" $";
    vector<string> instruction;
    vector<string> redirect(3,"");
    if(!preExecuteInstruction(instruction , var2val, redirect )) 
      continue;   
    pid_t cpid,w;
    int status;

    cpid = fork();
    if(cpid == -1){ // fork failure
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if(cpid == 0) { // child
      childProcess(instruction,redirect);
    }

    else {      
      w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
      if( w == -1) {
	perror("waitpid");
	exit( EXIT_FAILURE);
      }
      
      if(WIFEXITED(status)) {
	printf("Program exited with status %d \n", WEXITSTATUS(status));
      }else if(WIFSIGNALED(status )) {
	printf("Program was killed by signal %d \n",WTERMSIG(status));//kill
      }      
    }            
  }
  exit(EXIT_SUCCESS);
}

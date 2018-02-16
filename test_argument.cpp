#include <iostream>
#include <stdlib.h>
#include <vector>
#include <stack>
#include <unistd.h>
#include <string>
//#include <regex>
using namespace std;

int main(int argc, char** argv, char** envv)
{
  string str;
  int i = 0;
  while(*(argv+i) != NULL){
    //std::cout << str << std::endl;
    string a(* (argv+i) );
    cout<< i<<" argument: " << a << " has " <<a.size()<<" characters."<< endl;
    i++;
  }
  return 0;
}

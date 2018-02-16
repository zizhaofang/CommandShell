#include <errno.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>

int main(){
  std::cerr << "this is from error!" << std::endl;
  std::cout << "this is from stdout!" << std::endl;
  return EXIT_SUCCESS;
}

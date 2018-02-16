myShell:myShell.cpp
	g++ -o myShell myShell.cpp -Wall -Werror -std=gnu++11
test_argument:test_argument.cpp
	g++ -o test_argument myShell.cpp -Wall -Werror -std=gnu++11

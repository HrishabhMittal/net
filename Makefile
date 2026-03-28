all:
	g++ test/server.cpp -o server -std=c++23 -Iinclude
	g++ test/client.cpp -o client -std=c++23 -Iinclude

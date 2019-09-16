all: server client

server: client
	g++ -std=c++11 -o messenger_server server.cpp

client:
	g++ -std=c++11 -o messenger_client client.cpp -lpthread

clean:
	@rm messenger_server messenger_client
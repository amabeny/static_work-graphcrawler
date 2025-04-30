CXXFLAGS=-I./rapidjson/include -pthread
LDFLAGS=-lcurl
LD=g++
CC=g++

all: sequential_client parallel_client

sequential_client: sequential_client.o
	$(LD) $< -o $@ $(LDFLAGS)
sequential_client.o: sequential_client.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@ 

parallel_client: level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

level_client.o: level_client.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@ 

clean:
	-rm -f sequential_client sequential_client.o parallel_client level_client.o 

CXXFLAGS=-I./rapidjson/include
LDFLAGS=-lcurl
LD=g++
CC=g++

par_level_client: par_level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CC) -c par_level_client.cpp $(CXXFLAGS)

level_client: level_client.o
	$(LD) $< -o $@ $(LDFLAGS)

level_client.o: level_client.cpp
	$(CC) -c level_client.cpp $(CXXFLAGS)

clean:
	-rm level_client level_client.o par_level_client par_level_client.o

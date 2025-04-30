CXXFLAGS=-I./rapidjson/include -pthread
LDFLAGS=-lcurl
LD=g++
CC=g++

all: level_client par_level_client

level_client: level_client.o
    $(LD) $< -o $@ $(LDFLAGS)

par_level_client: par_level_client.o
    $(LD) $< -o $@ $(LDFLAGS)

par_level_client.o: par_level_client.cpp
    $(CC) $(CXXFLAGS) -c $< -o $@

level_client.o: level_client.cpp
    $(CC) $(CXXFLAGS) -c $< -o $@

clean:
    -rm -f level_client level_client.o par_level_client par_level_client.o

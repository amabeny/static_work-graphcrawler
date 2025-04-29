#CXXFLAGS=-I path/to/rapidjson
LDFLAGS=-lcurl
LD=g++
CC=g++

par_level_client: par_level_client.o
	$(LD) $< -o $@ $(LDFLAGS)


level_client: level_client.o
	$(LD) $< -o $@ $(LDFLAGS)



clean:
	-rm level_client level_client.o

CXX = g++
CXXFLAGS = -I./rapidjson/include
LDFLAGS = -lcurl

all: level_client par_level_client

level_client: level_client.o
	$(CXX) level_client.o -o level_client $(LDFLAGS)

level_client.o: level_client.cpp
	$(CXX) -c level_client.cpp $(CXXFLAGS)

par_level_client: par_level_client.o
	$(CXX) par_level_client.o -o par_level_client $(LDFLAGS)

par_level_client.o: par_level_client.cpp
	$(CXX) -c par_level_client.cpp $(CXXFLAGS)

clean:
	rm -f level_client level_client.o par_level_client par_level_client.o

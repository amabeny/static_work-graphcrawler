#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <stdexcept>
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include <chrono>
#include <queue>

using namespace std;
using namespace rapidjson;

bool debug = false;

const string SERVICE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

struct ParseException : std::runtime_error, rapidjson::ParseResult {
    ParseException(rapidjson::ParseErrorCode code, const char* msg, size_t offset) :
        std::runtime_error(msg),
        rapidjson::ParseResult(code, offset) {}
};

#define RAPIDJSON_PARSE_ERROR_NORETURN(code, offset) \
    throw ParseException(code, #code, offset)

// Function to HTTP encode parts of URLs. for instance, replace spaces with '%20' for URLs
string url_encode(CURL* curl, string input) {
    char* out = curl_easy_escape(curl, input.c_str(), input.size());
    string s = out;
    curl_free(out);
    return s;
}

// Callback function for writing response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// Function to fetch neighbors using libcurl
string fetch_neighbors(CURL* curl, const string& node) {
    string url = SERVICE_URL + url_encode(curl, node);
    string response;

    if (debug)
        cout << "Sending request to: " << url << endl;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Sequential-GraphCrawler/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        cerr << "CURL error for " << node << ": " << curl_easy_strerror(res) << endl;
        return "{}";
    } else {
        if (debug)
            cout << "CURL request successful for " << node << "!" << endl;
    }

    if (debug)
        cout << "Response received for " << node << ": " << response << endl;

    return response;
}

// Function to parse JSON and extract neighbors
vector<string> get_neighbors(const string& json_str) {
    vector<string> neighbors;
    try {
        Document doc;
        doc.Parse(json_str.c_str());

        if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
            for (const auto& neighbor : doc["neighbors"].GetArray())
                neighbors.push_back(neighbor.GetString());
        }
    } catch (const ParseException& e) {
        std::cerr << "Error while parsing JSON: " << json_str << std::endl;
        throw e;
    }
    return neighbors;
}

// Sequential BFS Traversal Function
vector<vector<string>> sequential_bfs(CURL* curl, const string& start_node, int depth) {
    vector<vector<string>> levels;
    unordered_set<string> visited;
    queue<string> q;

    levels.push_back({start_node});
    visited.insert(start_node);
    q.push(start_node);

    for (int d = 0; d < depth && !q.empty(); ++d) {
        levels.push_back({});
        int level_size = q.size();
        for (int i = 0; i < level_size; ++i) {
            string current_node = q.front();
            q.pop();

            try {
                for (const auto& neighbor : get_neighbors(fetch_neighbors(curl, current_node))) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        levels.back().push_back(neighbor);
                        q.push(neighbor);
                    }
                }
            } catch (const ParseException& e) {
                std::cerr << "Error fetching/parsing neighbors for " << current_node << ": " << e.what() << std::endl;
            }
        }
    }
    return levels;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <node_name> <depth>\n";
        return 1;
    }

    string start_node = argv[1];
    int depth;
    try {
        depth = stoi(argv[2]);
    } catch (const exception& e) {
        cerr << "Error: Depth must be an integer.\n";
        return 1;
    }

    CURL* curl_main = curl_easy_init();
    if (!curl_main) {
        cerr << "Failed to initialize CURL" << endl;
        return -1;
    }

    auto start_time = chrono::steady_clock::now();
    vector<vector<string>> results = sequential_bfs(curl_main, start_node, depth);
    auto end_time = chrono::steady_clock::now();
    chrono::duration<double> elapsed_seconds = end_time - start_time;

    for (size_t i = 0; i < results.size(); ++i) {
        cout << "Level " << i << " (" << results[i].size() << " nodes):\n";
        for (const auto& node : results[i]) {
            cout << "- " << node << "\n";
        }
    }
    cout << "Time to crawl (sequential): " << elapsed_seconds.count() << "s\n";

    curl_easy_cleanup(curl_main);

    return 0;
}

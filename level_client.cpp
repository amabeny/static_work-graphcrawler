#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <future>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <stdexcept>
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"
#include "rapidjson/reader.h"
#include <chrono>

using namespace std;
using namespace rapidjson;

bool debug = true; // Enable debugging output

// Updated service URL
const string SERVICE_URL = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// Maximum number of threads to use
const int MAX_THREADS = 8;

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

    cout << "Sending request to: " << url << endl; // Debug: Print the URL

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Set a User-Agent header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Parallel-GraphCrawler/1.0");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        cerr << "CURL error for " << node << ": " << curl_easy_strerror(res) << endl;
        return "{}";
    } else {
        cout << "CURL request successful for " << node << "!" << endl;
    }

    cout << "Response received for " << node << ": " << response << endl; // Debug: Print the raw response

    return response;
}

// Function to parse JSON and extract neighbors
vector<string> get_neighbors(const string& json_str) {
    vector<string> neighbors;
    cout << "Parsing JSON: " << json_str << endl; // Debug: Print the JSON string being parsed
    try {
        Document doc;
        doc.Parse(json_str.c_str());

        if (doc.HasMember("neighbors") && doc["neighbors"].IsArray()) {
            cout << "Found 'neighbors' array." << endl; // Debug
            for (const auto& neighbor : doc["neighbors"].GetArray()) {
                cout << "Neighbor found: " << neighbor.GetString() << endl; // Debug
                neighbors.push_back(neighbor.GetString());
            }
        } else {
            cout << "Warning: 'neighbors' key not found or is not an array." << endl; // Debug
        }
        if (doc.HasMember("node") && doc["node"].IsString()) {
            cout << "Node in response: " << doc["node"].GetString() << endl; // Debug
        }
    } catch (const ParseException& e) {
        std::cerr << "Error while parsing JSON: " << json_str << std::endl;
        throw e;
    }
    return neighbors;
}

// Structure to hold results from a thread
struct ThreadResult {
    vector<string> next_level_nodes;
    unordered_set<string> visited_nodes;
};

// Function to be executed by each thread
ThreadResult process_nodes(const vector<string>& nodes_to_process, unordered_set<string>& visited, mutex& visited_mutex) {
    ThreadResult result;
    CURL* curl_handle = curl_easy_init(); // Initialize a CURL handle for this thread
    if (!curl_handle) {
        cerr << "Error initializing CURL in thread." << endl;
        return result; // Return early if initialization fails
    }

    for (const string& node : nodes_to_process) {
        try {
            for (const auto& neighbor : get_neighbors(fetch_neighbors(curl_handle, node))) {
                {
                    lock_guard<mutex> lock(visited_mutex);
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        result.next_level_nodes.push_back(neighbor);
                        result.visited_nodes.insert(neighbor);
                    }
                }
            }
        } catch (const ParseException& e) {
            std::cerr << "Error processing node " << node << ": " << e.what() << std::endl;
        }
    }
    curl_easy_cleanup(curl_handle); // Cleanup the CURL handle for this thread
    return result;
}

// Parallel BFS Traversal Function (modified to pass no CURL* initially)
vector<vector<string>> parallel_bfs(const string& start_node, int depth) {
    vector<vector<string>> levels;
    unordered_set<string> visited;
    mutex visited_mutex;

    levels.push_back({start_node});
    visited.insert(start_node);

    for (int d = 0; d < depth; ++d) {
        if (levels[d].empty()) {
            break;
        }

        vector<string> current_level_nodes = levels[d];
        levels.push_back({});
        vector<future<ThreadResult>> futures;
        int num_nodes = current_level_nodes.size();
        int num_threads = min((int)num_nodes, MAX_THREADS);
        int nodes_per_thread = num_nodes / num_threads;
        int remaining_nodes = num_nodes % num_threads;
        int start_index = 0;

        cout << "Level " << d << ": Processing " << num_nodes << " nodes with " << num_threads << " threads." << endl; // Debug

        for (int i = 0; i < num_threads; ++i) {
            int end_index = start_index + nodes_per_thread + (i < remaining_nodes ? 1 : 0);
            vector<string> nodes_for_thread(current_level_nodes.begin() + start_index, current_level_nodes.begin() + end_index);
            // Pass by value for visited and visited_mutex, as each thread gets its own copy's reference
            futures.push_back(async(launch::async, process_nodes, nodes_for_thread, ref(visited), ref(visited_mutex)));
            start_index = end_index;
        }

        unordered_set<string> next_level_unique_nodes;
        for (auto& future : futures) {
            ThreadResult result = future.get();
            for (const string& node : result.next_level_nodes) {
                next_level_unique_nodes.insert(node);
            }
        }
        levels[d + 1].assign(next_level_unique_nodes.begin(), next_level_unique_nodes.end());
        cout << "Level " << d + 1 << " found " << levels[d + 1].size() << " unique nodes." << endl; // Debug
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

    // Initialize CURL in the main thread
    CURL* curl_main = curl_easy_init();
    if (!curl_main) {
        cerr << "Failed to initialize CURL in main thread" << endl;
        return -1;
    }

    cout << "Parallel BFS Traversal:\n";
    const auto start_parallel = chrono::steady_clock::now();
    vector<vector<string>> result_parallel = parallel_bfs(start_node, depth); // Don't pass curl_main here
    const auto finish_parallel = chrono::steady_clock::now();
    const chrono::duration<double> elapsed_seconds_parallel = finish_parallel - start_parallel;

    for (size_t i = 0; i < result_parallel.size(); ++i) {
        cout << "Level " << i << " (" << result_parallel[i].size() << " nodes):\n";
        for (const auto& node : result_parallel[i]) {
            cout << "- " << node << "\n";
        }
    }
    cout << "Time for parallel crawl: " << elapsed_seconds_parallel.count() << "s\n\n";

    curl_easy_cleanup(curl_main); // Cleanup the CURL handle from the main thread

    return 0;
}

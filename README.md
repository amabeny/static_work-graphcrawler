# static_work-graphcrawler
Parallel Graph Traversal with Web API - BFS

Usage:
    make
    ./parallel_bfs "Tom Hanks" 3

Description:
    This program performs a parallel BFS traversal using threads and calls the web API at:
    http://hollywood-graph-crawler.bridgesuncc.org/neighbors/{node}
    It uses level-by-level parallel expansion to query neighbors concurrently.

Dependencies:
    sudo dnf install libcurl-devel nlohmann-json-devel

Example Output:
    Visited nodes:
    Tom Hanks
    Forrest_Gump
    Saving_Private_Ryan
    Cast_Away
    ...

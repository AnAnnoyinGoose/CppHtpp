#ifndef REACTPP_SERVER_H
#define REACTPP_SERVER_H

#include "loadView.h"
#include <algorithm>
#include <arpa/inet.h>
#include <condition_variable>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <queue>
#include <sstream>
#include <string>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @namespace Core
 * This namespace contains the core components of the React++ framework.
 */
namespace Core { /// Core namespace

struct Response {
  std::string status, body;
  int socket;
  void send() const {
    std::string response = "HTTP/1.1 " + status + "\r\n\r\n" + body;
    ::send(socket, response.c_str(), response.size(), 0);
    close(socket);
  }
};

/** Request
 * This struct represents an HTTP request. It contains the method, path, and
 * body of the request.
 */
struct Request {
  std::string method;
  std::string path;
  std::string body;
};

/** Route
 * This struct represents a route. It contains the path and handler function of
 * the route.
 * @param path - a string which contains the path of the route.
 * @param handler - a function which handles the request and response.
 */
struct Route {
  std::string path;
  std::function<void(Request *, Response *)> handler;
  std::string method;
};

class Server {
private:
  int socket, port, MAX_REQUESTS = 80, TIME_PERIOD = 10;
  std::string ip;
  std::vector<std::thread> threads;
  std::unordered_map<std::string, Route *> routes;
  std::map<std::string, int> clientRequests;
  std::map<std::string, time_t> lastRequestTime;
  bool devMode = false;

  /** Get IP Address
   * This function takes an integer parameter `clientSocket` which represents
   * the socket descriptor of the client. It returns a string which contains the
   * IP address of the client.
   *
   * @param clientSocket - an integer value which represents the socket
   * descriptor of the client.
   *
   * @return A string which contains the IP address of the client.
   *
   * @description
   * This function uses the `getpeername` function to get the IP address of the
   * client. The `getpeername` function retrieves the address of the peer
   * connected to the socket referred to by `clientSocket`, and stores the
   * address in the `addr` structure. Then, it uses the `inet_ntoa` function to
   * convert the binary representation of the IP address into a string, which is
   * returned by the function.
   */
  static std::string getIPAddress(int clientSocket);
  /** Serve File
   * This function serves the file specified by `path` and sets the response
   * `body` to the contents of the file. The `res` parameter is updated with the
   * contents of the file, and then the response is sent.
   *
   * @param path - a reference to a constant string which contains the path to
   * the file to be served.
   * @param res - a reference to a Response object which will be updated with
   * the contents of the file.
   *
   * @return void.
   *
   * @description
   * This function opens the file specified by `path` in binary mode using an
   * `ifstream` object. If the file is opened successfully, the contents of the
   * file are read into a `stringstream` object using its `rdbuf` function. The
   * `body` of the response object `res` is set to the contents of the
   * `stringstream`. Then, the file is closed and the response is sent using the
   * `send` function of the `Response` object. If the file fails to open, an
   * error message is logged using the `LOG_ERROR` macro and no response is
   * sent.
   */
  static void serveFile(const std::string &path, Response &res);
  /** Rate Limiting
   * @brief A function to rate limit client requests based on IP address.
   *
   * This function takes in the client's socket, the maximum number of requests
   * allowed within a given time period, and the time period in seconds. It uses
   * the client's IP address to keep track of the number of requests made within
   * the given time period. If the number of requests exceeds the maximum
   * allowed within the time period, the function returns true to indicate that
   * the client should be rate limited. Otherwise, it returns false to indicate
   * that the client is within the allowed limit.
   *
   * @param clientSocket The client's socket.
   * @param maxRequests The maximum number of requests allowed within the time
   * period.
   * @param timePeriod The time period in seconds.
   *
   * @return true if the client should be rate limited, false otherwise.
   *
   * @see getIPAddress
   */
  bool rateLimit(int clientSocket, int maxRequests, int timePeriod);
  /** Serve static file
   *
   * This function serves static files such as CSS, JS, and image files. It
   * checks if the path of the request starts with /css, /js, or /img. If it
   * does, it serves the file. Otherwise, it returns false.
   *
   * @param path The path of the request.
   * @param res The response object.
   *
   * @return true if the file was served, false otherwise.
   *
   * @see serveFile
   * @see Response
   */
  static bool serveStaticFile(const std::string &path, Response res);
  /** Client Handler
   * This function handles incoming client connections.
   *
   * @param clientSocket The socket of the client that connected.
   */
  void clientHandler(int clientSocket);
  /** Init Workspace
   * This function initializes the workspace by creating the public folder and
   * the index.html file.
   */
  void initWorkspace();
public:
  /** Constructor
   * This constructor initializes the server's port and IP address.
   *
   * @param port The port to listen on.
   * @param ip The IP address to listen on.
   */
  explicit Server(int port, std::string ip = "0.0.0.0")
      : port(port), ip(std::move(ip)) {
    // Create css, js, and img directories if they don't already exist, in
    // (public folder)
    mkdir("public/css", 0777);
    mkdir("public/js", 0777);
    mkdir("public/img", 0777);

    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
      std::cerr << "Could not create socket" << std::endl;
      exit(1);
    }
    int opt = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
      std::cerr << "Could not set socket options" << std::endl;
      exit(1);
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(this->ip.c_str());
    address.sin_port = htons(port);
    if (bind(socket, (sockaddr *)&address, sizeof(address)) < 0) {
      std::cerr << "Could not bind socket" << std::endl;
      exit(1);
    }
    if (listen(socket, 3) < 0) {
      std::cerr << "Could not listen on socket" << std::endl;
      exit(1);
    }
  }
  /** Start
   * Starts the server and begins listening for client connections. This
   * method will continuously run in a loop and accept incoming client
   * connections on the specified port. It will then launch a new thread to
   * handle each client connection using the `clientHandler()` method.
   *
   * Note that this method never returns and will run indefinitely until the
   * program is terminated.
   *
   * @noreturn
   */
  [[noreturn]] void start();
  /** GET method
   * Adds a GET route to the server. When a GET request is received with a
   * URL that matches one of the provided paths, the specified handler
   * function will be called to handle the request.
   *
   * @param paths A vector of string paths to match against incoming GET
   * requests.
   * @param handler A function pointer that will be called to handle
   * matching GET requests.
   */
  void get(std::vector<std::string> paths,
           const std::function<void(Request *, Response *)> &handler);
  /** POST method
   * Adds a POST route to the server.
   *
   * @param paths A vector of URL paths to match for the route.
   * @param handler A function pointer that will be called to handle the
   * route request.
   */
  void post(std::vector<std::string> paths,
            const std::function<void(Request *, Response *)> &handler);
  /** PUT method
   * Adds a PUT route to the server.
   *
   * @param paths A vector of URL paths to match for the route.
   * @param handler A function pointer that will be called to handle the
   * route request.
   */
  void put(std::vector<std::string> paths,
           const std::function<void(Request *, Response *)> &handler);
  /** DELETE method
   * Deletes a matching route from the server.
   * @param paths A vector of URL paths to match for the route.
   */
  void del(std::vector<std::string> paths);
  /** Page Not Found (PNF) method
   * Adds a PNF (Page Not Found) route to the server. This route will be
   * matched if no other route matches the incoming request.
   *
   * @param handler A function pointer that will be called to handle the PNF
   * request.
   */
  void pnf(std::function<void(Request *, Response *)> handler);
  /** getAddress
   * @return The server's address as a string.
   */
  std::string getAddress();
  /** setTimeouts
   * Sets the maximum number of requests that can be made in a given time
   * period. This is used to prevent brute force attacks.
   *
   * @param maxRequests The maximum number of requests that can be made in
   * the given time period.
   * @param timePeriod The time period in seconds.
   */
  void setTimeouts(int maxRequests, int timePeriod);

  ~Server() {
    close(socket);
    for (auto &thread : threads) {
      thread.join();
    }
  }
};

} // namespace Core
#endif // REACTPP_SERVER_H

#include "server.h"

namespace Core {
std::string Server::getIPAddress(int clientSocket) {
  struct sockaddr_in addr {};
  socklen_t len = sizeof(addr);
  getpeername(clientSocket, (struct sockaddr *)&addr, &len);
  return inet_ntoa(addr.sin_addr);
};
void Server::serveFile(const std::string &path, Response &res) {
  std::ifstream file(path.substr(1), std::ios::binary);
  if (file.is_open()) {
    std::stringstream buffer{};
    buffer << file.rdbuf();
    res.body = std::move(buffer).str();
    file.close();
  } else {
  }
  res.send();
}
bool Server::rateLimit(int clientSocket, int maxRequests, int timePeriod) {
  std::string clientIP = getIPAddress(
      clientSocket); // Implement a function to get the client's IP address
  time_t now = time(nullptr);
  if (clientRequests.find(clientIP) == clientRequests.end()) {
    clientRequests[clientIP] = 1;
  } else {
    int requests = clientRequests[clientIP];
    double timeDiff = difftime(now, lastRequestTime[clientIP]);
    if (timeDiff < timePeriod && requests >= maxRequests) {
      return true;
    } else if (timeDiff >= timePeriod) {
      clientRequests[clientIP] = 1;
    } else {
      clientRequests[clientIP] = requests + 1;
    }
  }
  lastRequestTime[clientIP] = now;
  return false;
}
bool Server::serveStaticFile(const std::string &path, Response res) {
  if (path.find("/css") == 0 || path.find("/js") == 0 ||
      path.find("/img") == 0) {
    serveFile("/public" + path, res);
    return true;
  }
  return false;
}
void Server::clientHandler(int clientSocket) {
  const int BUFFER_SIZE = 1024;
  const std::string DELIMITER = "\r\n\r\n";

  if (rateLimit(clientSocket, MAX_REQUESTS, TIME_PERIOD)) {
    Response res{"429 Too Many Requests", "", clientSocket};
    res.send();
    return;
  }

  char buffer[BUFFER_SIZE] = {0};
  ssize_t valRead = read(clientSocket, buffer, BUFFER_SIZE);
  if (valRead < 0) {

    return;
  }

  std::string request(buffer, valRead);
  std::istringstream iss(request);
  std::string method, path, protocol;
  iss >> method >> path >> protocol;

  std::replace(path.begin(), path.end(), '\\', '/');

  path.erase(std::unique(path.begin(), path.end(),
                         [](char a, char b) { return a == '/' && b == '/'; }),
             path.end());

  std::string body = request.substr(request.find(DELIMITER) + DELIMITER.size());

  Request req{method, path, body};

  Response res{"200 OK", "", clientSocket};

  if (routes.count(path)) {
    routes[path]->handler(&req, &res);
  } else if (serveStaticFile(path, res)) {
  } else if (path == "/reactpp/api/routes") {
    if (!devMode) {
      res.status = "403 Forbidden";
      res.body =
          R"(<style>body{font-size:30px;background-color:
                       #282828; color: #ebdbb2; font-family:
                       monospace;}</style><h1>403 Forbidden</h1><p>Sorry but
                       dev-mode is not enabled thus you cannot access this
                       route.</p><p>Please enable dev-mode in order to access
                       this route.</p>)";
      res.send();
    }
    res.status = "200 OK";
    std::string routesList = "<ul>";
    for (auto const &[key, val] : routes) {
      routesList += "<li><a href=\"" + key + "\">" + key + "</a>    " +
                    val->method + "</li>";
    }
    routesList += "</ul>";
    res.body =
        R"(<style>body{font-size:30px;background-color:
                       #282828; color: #ebdbb2; font-family:
                       monospace;}</style><h1>200 OK</h1><p>Here are all the
                       routes that are currently defined:</p>)";
    res.body += routesList;
    res.send();
  } else {
    res.status = "404 Not Found";
    routes["*"]->handler(&req, &res);
  }
}
void Server::initWorkspace() {
  mkdir("public", 0777);
  mkdir("public/css", 0777);
  mkdir("public/js", 0777);
  mkdir("public/img", 0777);
  std::ofstream indexFile("public/index.html");
}
void Server::start() {
  initWorkspace();
  while (true) {
    sockaddr_in address{};
    socklen_t addressLength = sizeof(address);
    int clientSocket = accept(socket, (sockaddr *)&address, &addressLength);
    if (clientSocket < 0) {

      continue;
    }
    threads.emplace_back(&Server::clientHandler, this, clientSocket);
  }
}
void Server::get(std::vector<std::string> paths,
                 const std::function<void(Request *, Response *)> &handler) {
  for (auto &path : paths) {
    // Convert path to lowercase
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Add route to map
    auto *route = new Route{path, handler};
    this->routes[path] = route;
  }
}
void Server::post(std::vector<std::string> paths,
                  const std::function<void(Request *, Response *)> &handler) {
  for (auto &path : paths) {
    // Convert path to lowercase
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Add route to map
    auto *route = new Route{path, handler};
    this->routes[path] = route;
  }
}
void Server::put(std::vector<std::string> paths,
                 const std::function<void(Request *, Response *)> &handler) {
  for (auto &path : paths) {
    // Convert path to lowercase
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Add route to map
    auto *route = new Route{path, handler};
    this->routes[path] = route;
  }
}
void Server::del(std::vector<std::string> paths) {
  for (auto &path : paths) {
    // Convert path to lowercase
    std::transform(path.begin(), path.end(), path.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Delete route from map
    this->routes.erase(path);
  }
}
void Server::pnf(std::function<void(Request *, Response *)> handler) {
  this->routes["*"] = new Route{"*", std::move(handler)};
}
std::string Server::getAddress() {
  return "http://" + ip + ":" + std::to_string(port);
}
void Server::setTimeouts(int maxRequests, int timePeriod) {
  MAX_REQUESTS = maxRequests;
  TIME_PERIOD = timePeriod;
}

} // namespace Core

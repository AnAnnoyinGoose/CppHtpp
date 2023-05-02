#include "React++.h"

int main() {
  Core::Server server(8080);
  Core::View view;
//  server.setDevMode(true);

  server.get({"/", "/index.html"}, [&view](Core::Request *request,
                                           Core::Response *response) {
    response->body =
        view.use("hello.html", custom::data{{"name", "AnAnnoyinGoose"}})
            .render();
    response->send();
  });

  // if the page doesnt exist, return 404
  server.pnf(
      [&view, &server](Core::Request *request, Core::Response *response) {
        response->status = "404 Not Found";
        response->body =
            view.use("404.html", custom::data{{"url", server.getAddress()}})
                .render();
        response->send();
      });

  server.start();
}

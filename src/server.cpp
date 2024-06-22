#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

std::string path;

std::string getUrl(char *buffer) {
  std::string result;
  char *urlp = strtok(buffer, "/");
  urlp = strtok(NULL, "/");
  while (*urlp != ' ') {
    result += *urlp;
    urlp++;
  }
  return result;
}

std::string getTextFromFile(std::string filename) {
  std::ifstream file(path + filename, std::ios::in);
  std::stringstream filebuf;
  filebuf << file.rdbuf();
  return filebuf.str();
}

void writeTextToFile(std::string filename, const std::string &text) {
  std::ofstream file(path + filename, std::ios::out);
  file.write(text.c_str(), text.length());
  // if (file){
  //   std::cerr << path + filename;
  // }
}
std::string getBodyText(const std::string& request) {
  auto bodyPos = request.find_last_of("\r\n");
  //std::cerr << request.substr(bodyPos + 1, request.length() - 1);
  return request.substr(bodyPos + 1, request.length() - 1);
}

void workWithClient(int client_fd) {
  char buffer[1024];

  auto size = read(client_fd, buffer, 1024);

  std::string OK("HTTP/1.1 200 OK\r\n");
  std::string ERROR("HTTP/1.1 404 Not Found\r\n\r\n");

  std::string request(buffer);
  std::string url(getUrl(buffer));
  if (request.starts_with("GET")) {
    if (url.starts_with("echo")) {
      OK += "Content-Type: text/plain\r\nContent-Length: ";
      OK += std::to_string(url.length() - 5) + "\r\n\r\n" +
            url.substr(5, url.length() - 1);
      send(client_fd, OK.c_str(), OK.length(), 0);
    } else if (url.starts_with("user-agent")) {
      auto userAgentPos = request.find("User-Agent: ");
      userAgentPos += 12;
      std::string userAgentValue;
      std::cout << request;
      while (request[userAgentPos] != '\r') {
        userAgentValue += request[userAgentPos];
        userAgentPos++;
      }
      OK += "Content-Type: text/plain\r\nContent-Length: ";
      OK +=
          std::to_string(userAgentValue.length()) + "\r\n\r\n" + userAgentValue;
      send(client_fd, OK.c_str(), OK.length(), 0);

    } else if (url.starts_with("files")) {
      if (url.length() == 6) {
        send(client_fd, ERROR.c_str(), 30, 0);
      } else {
        std::string filebuf = getTextFromFile(url.substr(6, url.length() - 1));
        if (filebuf.empty()) {
          send(client_fd, ERROR.c_str(), 30, 0);
        } else {
          OK += "Content-Type: application/octet-stream\r\nContent-Length: ";
          OK += std::to_string(filebuf.length()) + "\r\n\r\n" + filebuf;
          send(client_fd, OK.c_str(), OK.length(), 0);
        }
      }
    } else if (url.empty()) {
      OK += "\r\n";
      send(client_fd, OK.c_str(), 23, 0);
    } else {
      send(client_fd, ERROR.c_str(), 30, 0);
    }
  } else if (request.starts_with("POST")) {
    if (url.starts_with("files")) {
      std::string filename = url.substr(6, url.length() - 1);
      writeTextToFile(filename, getBodyText(request));
      send(client_fd, "HTTP/1.1 201 Created\r\n\r\n", 28, 0);
    }
  }
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  if (argc > 2 && strcmp(argv[1], "--directory") == 0) {
    path += argv[2];
  }

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  std::cout << "Waiting for a client to connect...\n";

  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);

  std::vector<std::thread> threads(std::thread::hardware_concurrency());

  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);
    std::cout << "Client connected\n";
    threads.emplace_back(std::thread(workWithClient, client_fd));
  }
  for (auto &thr : threads) {
    if (thr.joinable()) {
      thr.join();
    }
  }
  close(server_fd);
  return 0;
}

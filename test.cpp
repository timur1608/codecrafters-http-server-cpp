#include <iostream>
#include <cstring>

int main()
{
    using namespace std;
    char buf[] = "GET /user-agent HTTP/1.1\r\n Host: localhost:4221\r\n User-Agent: foobar/1.2.3\r\n Accept: *";
    char* bufp = strtok(buf, "User-Agent: ");
    bufp = strtok(NULL, "User-Agent: ");
    //bufp = strtok(NULL, "User-Agent: ");
    // while (*bufp != '\0')
    // {
    std::cout << *(bufp + 3);
    //     bufp++;
    // }
}
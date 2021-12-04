#include "hussar.h"

void hello_world(hus::Request& req, hus::Response& resp)
{
    if (req.GET.find("name") != req.GET.end()) {
        resp.body = "<h2>Hello, " + hus::HtmlEscape(req.GET["name"]) + "!</h2>";
    } else {
        resp.body = "<h2>Hello, World!</h2>";
    }
}

void welcome(hus::Request& req, hus::Response& resp)
{
    resp.body = R"(
        <h1>Welcome to a new world!</h1>
        <p>Click <a href="/hello">HERE</a> to go to the hello world, or fill in and submit the form below</p><br>
        <form action="/hello">
        <label for="name">Enter your name: </label>
        <input type="text" id="name" name="name" required>
        <input type="submit" value="Submit">
        </form>
    )";
}

int main()
{
    hus::Hussar h("127.0.0.2", 8080, 0, true);

    h.Router.DEFAULT(&welcome);
    h.Router.GET("/hello", &hello_world);

    h.Listen();
}

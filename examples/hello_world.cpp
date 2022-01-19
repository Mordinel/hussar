/**
*     Copyright (C) 2022 Mason Soroka-Gill
* 
*     This program is free software: you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation, either version 3 of the License, or
*     (at your option) any later version.
* 
*     This program is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
* 
*     You should have received a copy of the GNU General Public License
*     along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "hussar.h"

void redirect_home(hus::Request& req, hus::Response& resp) {
    resp.code = "302";
    resp.headers["Location"] = "/";
}

void home(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        resp.body = "<h1>Welcome to the website, <b>" + hus::html_escape(hus::read_session(req.session_id, "username")) + "</b>!</h1><br><p>Click <a href=\"/logout\">HERE</a> to log out.</p>";
    } else {
        resp.body = "<h1>Welcome to the website!</h1><br><p>Click <a href=\"/login\">HERE</a> to go to the login page</p>";
    }
}

void login_page(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        redirect_home(req, resp);
        return;
    }

    // multiline string
    resp.body = R"(
<form action="/login" method="post">
<label for="name">Username: </label>
<input type="text" id="username" name="username" required><br>
<label for="password">Password: </label>
<input type="password" id="password" name="password" required><br>
<input type="submit" value="Submit">
    )";

    // checking if parameter exists
    if (req.get.find("message") != req.get.end()) {
        resp.body += "<br><p>" + hus::html_escape(req.get["message"]) + "</p>";
    }
}

void login(hus::Request& req, hus::Response& resp) {
    if (hus::read_session(req.session_id, "username") != "") {
        redirect_home(req, resp);
        return;
    }

    // Checking if post parameters exist
    if (req.post.find("username") == req.post.end()) goto login_redirect;
    if (req.post.find("password") == req.post.end()) goto login_redirect;

    // TODO get creds from a db query and do a hash of some kind
    if (req.post["password"] != "lemon42") goto login_redirect; // TODO compute hash from given password and compare with hash from db

    if (hus::write_session(req.session_id, "username", req.post["username"])) {
        redirect_home(req, resp);
    } else {
        goto login_redirect;
    }

    return;

    // If anything here fails, return to the login page with a generic error
login_redirect:
    resp.code = "302";
    resp.headers["Location"] = "/login?message=Failed+to+login.";
}

void logout(hus::Request& req, hus::Response& resp) {
    resp.code = "302";
    if (hus::delete_session(req.session_id, "username")) {
        resp.headers["Location"] = "/login?message=Logged+out.";
    } else {
        resp.headers["Location"] = "/login?message=Not+logged+in.";
    }
}

int main() {

    // initialize HUSSAR server
    //            Socket Host
    //            |            Socket Port
    //            |            |     Thread count (0 is default count)
    //            |            |     |  SSL Private key
    //            |            |     |  |          SSL public cert
    //            |            |     |  |          |           Verbosity enabled
    //            |            |     |  |          |           |
    hus::Hussar s("127.0.0.2", 8443, 0, "key.pem", "cert.pem", true);
    
    // non-ssl constructor is available too
    //hus::Hussar s("127.0.0.2", 8080, 0, true);

    // register routes
    s.router.fallback(&redirect_home);       // fallback route is everything other than registered routes
    s.router.get("/", &home);
    s.router.get("/login", &login_page);
    s.router.post("/login", &login);
    s.router.get("/logout", &logout);

    // BLOCKING listen for the server
    // perhaps put in a thread for non-blocking style behaviour so
    // multiple hus::Hussar instances can exist in the same process
    s.serve();

    return 0;
}


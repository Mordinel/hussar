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
    resp.Headers["Location"] = "/";
}

void home(hus::Request& req, hus::Response& resp) {
    if (hus::SessionExists(req.SessionID) && hus::ReadSessionData(req.SessionID, "username") != "") {
        resp.body = "<h1>Welcome to the website, <b>" + hus::HtmlEscape(hus::ReadSessionData(req.SessionID, "username")) + "</b>!</h1><br><p>Click <a href=\"/logout\">HERE</a> to log out.</p>";
    } else {
        resp.body = "<h1>Welcome to the website!</h1><br><p>Click <a href=\"/login\">HERE</a> to go to the login page</p>";
    }
}

void login_page(hus::Request& req, hus::Response& resp) {
    if (hus::SessionExists(req.SessionID) && hus::ReadSessionData(req.SessionID, "username") != "") {
        redirect_home(req, resp);
    }

    // multiline string
    resp.body = R"(
<form action="/login" method="POST">
<label for="name">Username: </label>
<input type="text" id="username" name="username" required><br>
<label for="password">Password: </label>
<input type="password" id="password" name="password" required><br>
<input type="submit" value="Submit">
    )";

    // checking if parameter exists
    if (req.GET.find("message") != req.GET.end()) {
        resp.body += "<br><p>" + hus::HtmlEscape(req.GET["message"]) + "</p>";
    }
}

void login(hus::Request& req, hus::Response& resp) {
    if (hus::SessionExists(req.SessionID) && hus::ReadSessionData(req.SessionID, "username") != "") {
        redirect_home(req, resp);
    }

    // Checking if POST parameters exist
    if (req.POST.find("username") == req.POST.end()) goto login_redirect;
    if (req.POST.find("password") == req.POST.end()) goto login_redirect;

    // TODO get creds from a db query and do a hash of some kind
    if (req.POST["password"] != "lemon42") goto login_redirect; // TODO compute hash from given password and compare with hash from db

    if (hus::SessionExists(req.SessionID)) {
        if (hus::WriteSessionData(req.SessionID, "username", req.POST["username"])) {
            redirect_home(req, resp);
        } else {
            goto login_redirect;
        }
    } else {
        resp.code = "302";
        resp.Headers["Location"] = "/login?message=Please+enable+cookies.";
    }

    return;

    // If anything here fails, return to the login page with a generic error
login_redirect:
    resp.code = "302";
    resp.Headers["Location"] = "/login?message=Failed+to+login.";
}

void logout(hus::Request& req, hus::Response& resp) {
    if (hus::SessionExists(req.SessionID)) {
        if (hus::DeleteSessionData(req.SessionID, "username")) {
            resp.code = "302";
            resp.Headers["Location"] = "/login?message=Logged+out.";       
        } else {
            resp.code = "302";
            resp.Headers["Location"] = "/login?message=Not+logged+in.";       
        }
    } else {
        resp.code = "302";
        resp.Headers["Location"] = "/login?message=Please+enable+cookies.";       
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
    s.Router.DEFAULT(&redirect_home);       // default route is everything other than registered routes
    s.Router.GET("/", &home);
    s.Router.GET("/login", &login_page);
    s.Router.POST("/login", &login);
    s.Router.GET("/logout", &logout);

    // BLOCKING listen for the server
    // perhaps put in a thread for non-blocking style behaviour so
    // multiple hus::Hussar instances can exist in the same process
    s.Listen();

    return 0;
}





char *
LCWEB_http_html_login (void) {
    char *login_form =
        "HTTP/1.1 200 OK\nConnection: close\n\
        Content-Type: text/html\n\n\n\
        <html lang=\"en-US\">\
        <head>\
        <title>LCWEB</title>\
        </head>\
        <body>\
        <form name=\"input\" action=\"#\" method=\"post\">\
        <table>\
        <tr><td>Username: </td><td><input type=\"text\" name=\"user\"></td></tr>\
        <tr><td>Password: </td><td><input type=\"password\" name=\"password\"></td></tr>\
        <tr><td></td><td><input type=\"submit\" value=\"Submit\"></td></tr>\
        </table></form></body></html>";
    return login_form;
}

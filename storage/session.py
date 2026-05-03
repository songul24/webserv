#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import warnings

warnings.filterwarnings("ignore", category=DeprecationWarning)

import os
import cgi
import http.cookies
from string import Template
from html import escape

STATIC_DIR = os.environ.get("SCRIPT_ROOT", "")
LOGIN_PAGE = STATIC_DIR + "/storage/session_login.html"
HOME_PAGE = STATIC_DIR +  "/storage/session_home.html"



def load_page(filename):
    """Return the contents of the named file as a string."""
    with open(filename, "r") as file:
        contents = file.read()
    return contents


def set_cookie(name, value, expires=None, path="/"):
    """Set a cookie with the given name and value."""
    cookie = http.cookies.SimpleCookie()
    cookie[name] = value
    if expires:
        cookie[name]["expires"] = expires
    cookie[name]["path"] = path
    print(cookie.output())


def get_cookie(name):
    """Retrieve the value of the specified cookie."""
    cookie_str = os.environ.get("HTTP_COOKIE", "")
    cookies = http.cookies.SimpleCookie(cookie_str)
    return cookies.get(name)


def render_login_form():
    """Render the login form."""
    print("Content-Type: text/html\n")
    print(load_page(LOGIN_PAGE), end="")


def render_home_page(username):
    """Render the home page with the provided username."""
    print("Content-Type: text/html\n")
    page = load_page(HOME_PAGE)
    page = Template(page).safe_substitute(username=escape(username.title()))
    print(page, end="")


def main():
    username_cookie = get_cookie("username")

    form = cgi.FieldStorage()
    if "username" in form:
        username = form.getvalue("username")
        set_cookie("username", username)
        render_home_page(username)
    elif "logout" in form:
        set_cookie("username", "", expires="Thu, 01 Jan 1970 00:00:00 GMT")
        render_login_form()
    elif username_cookie:
        username = username_cookie.value
        render_home_page(username)
    else:
        render_login_form()


if __name__ == "__main__":
    try:
        main()
    except:
        cgi.print_exception()
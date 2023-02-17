> ***IMPORTANT:***
> _This is a dead, no longer maintained, project!_
>

# gospy-applet

gospy-applet is a GNOME applet for monitoring changes on servers and web pages.. You can add a illimited number of sources to monitor. It is possible to detect changes in HTTP header fields, IP associated with domain name, page content, page status, page load time and so on.

## Installation
```bash
$ ./autogen.sh --prefix=/usr
$ make
$ su -c "make install"
```
Then restart your GNOME session.

## FAQ

1. How can I build gurlchecker without libesmtp support?
> Just pass `--disable-libesmtp` to `./configure`.
2. How can I build gurlchecker with/without GNUTLS support?
> Just pass `--disable-gnutls` to `./configure`.

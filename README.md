# http.cc

A simple asynchronous http 1.1 server written in C++20.

**Hobby project: not a production-ready server.** This simple server was made only to test my skills when:

- Using linux [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html) API.
- Parsing complex data structures in an asynchronous context.
- Using state-machines to verify and parse http request.
- Learning async programming without the async/await syntax sugar modern languages provide.

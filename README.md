# Screensharing 

This is a small and simple multithreaded web server implemented in c++.
Host exposes its screen and plays as a chat server, and the Client is a web application that shows what the server exposes and also allows to send/receive short messages.

Supported features:
+ multiple simultaneous clients
+ still picture of the screen, that is updated periodically
+ a single global "chat room" accessible without authentication
+ newly connected clients do not receive any chat history
+ server stores only last 500 messages
+ every message marked with ip-address it received from
+ host displays how many clients are connected

Server developed in c++ using libevent library (http://libevent.org)

To compile open screensharing.sln solution in Microsoft Visual Studio 2015 and select platform x86. Before starting MSVS set LIBEVENT_HOME environment variable to point to your copy of compiled libevent. You can find precompiled version in libevent-vs2015-x86 directory.
To compile and run tests set GOOGLE_TEST_HOME environment variable to point to your copy of Google Test framework.

When executing server ensure that _index.html_ file (can be found in server/index.html) is in a current directory — server considers current directory as web root.
Http requests to server are limited and hard encoded. You can only put a new message, get screenshort and newly available messages.

Command line would look like: _server.exe 192.168.1.1 5555_

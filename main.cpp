/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
// LIVE555 Proxy Server
// main program

/*! \file main.cpp
*   \brief      LIVE555 Proxy Server - main program
*   \details    This is the unchanged version of LIVE555 Proxy Server, with a changed name and doxygen documentation. Also added ReceivingInterfaceAddr for setup in multihomed environments
*   \author     Live Networks, Inc.
*   \copyright  GNU Lesser General Public License
*   \copyright  Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved
*/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"


char const* progName;
UsageEnvironment* env;
UserAuthenticationDatabase* authDB;

// Default values of command-line parameters:
/// Defaukt verbosity level of the output
int verbosityLevel = 0;
/// Stream RTP over TCP or not
Boolean streamRTPOverTCP = False;
/// Tunnel over a HTTP connection on port <http-port>
portNumBits tunnelOverHTTPPortNum = 0;
/// Username for a backend stream
char* username = NULL;
/// Password for a backend stream
char* password = NULL;
/// Handle incoming "REGISTER" requests by proxying the specified stream
Boolean proxyREGISTERRequests = False;

/*!
 * \brief Helper function for creating a new RTSPServer Object
 * \param port Port to listen on
 * \return new RTSPServer-Object
 */
static RTSPServer* createRTSPServer(Port port) {
    if (proxyREGISTERRequests) {
        return RTSPServerWithREGISTERProxying::createNew(*env, port, authDB);
    } else {
        return RTSPServer::createNew(*env, port, authDB);
    }
}

/*!
 * \brief print the usage of this program
 */
void usage() {
    *env << "Usage: " << progName
         << " [-v|-V]"
         << " [-t|-T <http-port>]"
         << " [-u <username> <password>]"
         << " [-R]"
         << " [-b <IP-adress>]"
         << " <rtsp-url-1> ... <rtsp-url-n>\n";
    exit(1);
}

/*!
 * \brief main function of the program
 * \param -v|-V                     Verbose / more verbose output
 * \param -t                        Stream RTP and RTCP over the TCP 'control' connection (This is for the 'back end' (i.e., proxied) stream only.)
 * \param -T <http-port>            Stream RTP and RTCP over a HTTP connection on port <http-port>
 * \param -u <username> <password>  Specify a username and password (to be used if the 'back end' (i.e., proxied) stream requires authentication)
 * \param -R                        Handle incoming "REGISTER" requests by proxying the specified stream
 * \param -b <IP-Adress>            specify an IP to bind this proxy to.
 * \param <rtsp-url>                rtsp-URLs to be proxied by this program
 * \return Never returns except on cancel / sigterm / sigkill.
 */
int main(int argc, char** argv) {
    // Increase the maximum size of video frames that we can 'proxy' without truncation.
    // (Such frames are unreasonably large; the back-end servers should really not be sending frames this large!)
    OutPacketBuffer::maxSize = 100000; // bytes

    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    *env << "LIVE555 Proxy Server\n"
         << "\t(LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ")\n\n";

    // Check command-line arguments: optional parameters, then one or more rtsp:// URLs (of streams to be proxied):
    progName = argv[0];
    if (argc < 2) usage();
    while (argc > 1) {
        // Process initial command-line options (beginning with "-"):
        char* const opt = argv[1];
        if (opt[0] != '-') break; // the remaining parameters are assumed to be "rtsp://" URLs

        switch (opt[1]) {
        case 'v': { // verbose output
            verbosityLevel = 1;
            break;
        }

        case 'V': { // more verbose output
            verbosityLevel = 2;
            break;
        }

        case 't': {
            // Stream RTP and RTCP over the TCP 'control' connection.
            // (This is for the 'back end' (i.e., proxied) stream only.)
            streamRTPOverTCP = True;
            break;
        }

        case 'T': {
            // stream RTP and RTCP over a HTTP connection
            if (argc > 3 && argv[2][0] != '-') {
                // The next argument is the HTTP server port number:
                if (sscanf(argv[2], "%hu", &tunnelOverHTTPPortNum) == 1
                        && tunnelOverHTTPPortNum > 0) {
                    ++argv; --argc;
                    break;
                }
            }

            // If we get here, the option was specified incorrectly:
            usage();
            break;
        }

        case 'u': { // specify a username and password (to be used if the 'back end' (i.e., proxied) stream requires authentication)
            if (argc < 4) usage(); // there's no argv[3] (for the "password")
            username = argv[2];
            password = argv[3];
            argv += 2; argc -= 2;
            break;
        }

        case 'R': { // Handle incoming "REGISTER" requests by proxying the specified stream:
            proxyREGISTERRequests = True;
            break;
        }

        case 'b': {
            //Da das Interface angeben, an das man sich binden will
            if (argc > 3 && argv[2][0] != '-') {
                ReceivingInterfaceAddr = our_inet_addr(argv[2]);
                *env << "Binding to Interface with IP: " << argv[2] << "\n";
                ++argv; --argc;
                break;
            }
        }

        default: {
            usage();
            break;
        }
        }

        ++argv; --argc;
    }
    if (argc < 2 && !proxyREGISTERRequests) usage(); // there must be at least one "rtsp://" URL at the end
    // Make sure that the remaining arguments appear to be "rtsp://" URLs:
    int i;
    for (i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "rtsp://", 7) != 0) usage();
    }
    // Do some additional checking for invalid command-line argument combinations:
    if (streamRTPOverTCP) {
        if (tunnelOverHTTPPortNum > 0) {
            *env << "The -t and -T options cannot both be used!\n";
            usage();
        } else {
            tunnelOverHTTPPortNum = (portNumBits)(~0); // hack to tell "ProxyServerMediaSession" to stream over TCP, but not using HTTP
        }
    }

    authDB = NULL;
#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    // Create the RTSP server.  Try first with the default port number (554),
    // and then with the alternative port number (8554):
    RTSPServer* rtspServer;
    portNumBits rtspServerPortNum = 554;
    rtspServer = createRTSPServer(rtspServerPortNum);
    if (rtspServer == NULL) {
        rtspServerPortNum = 8554;
        rtspServer = createRTSPServer(rtspServerPortNum);
    }
    if (rtspServer == NULL) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }

    // Create a proxy for each "rtsp://" URL specified on the command line:
    for (i = 1; i < argc; ++i) {
        char const* proxiedStreamURL = argv[i];
        char streamName[30];
        if (argc == 2) {
            sprintf(streamName, "%s", "proxyStream"); // there's just one stream; give it this name
        } else {
            sprintf(streamName, "proxyStream-%d", i); // there's more than one stream; distinguish them by name
        }
        ServerMediaSession* sms
                = ProxyServerMediaSession::createNew(*env, rtspServer,
                                                     proxiedStreamURL, streamName,
                                                     username, password, tunnelOverHTTPPortNum, verbosityLevel);
        rtspServer->addServerMediaSession(sms);

        char* proxyStreamURL = rtspServer->rtspURL(sms);
        *env << "RTSP stream, proxying the stream \"" << proxiedStreamURL << "\"\n";
        *env << "\tPlay this stream using the URL: " << proxyStreamURL << "\n";
        delete[] proxyStreamURL;
    }

    if (proxyREGISTERRequests) {
        *env << "(We handle incoming \"REGISTER\" requests on port " << rtspServerPortNum << ")\n";
    }

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
        *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
    } else {
        *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
    }

    // Now, enter the event loop:
    env->taskScheduler().doEventLoop(); // does not return

    return 0; // only to prevent compiler warning
}

/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** -------------------------------------------------------------------------*/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <stdio.h>
#include <string>
#include <iostream>

#include "logger.h"
#include "rtsp2ws.h"

int LogLevel=NOTICE;

/* ---------------------------------------------------------------------------
**  end condition
** -------------------------------------------------------------------------*/
int stop=0;

/* ---------------------------------------------------------------------------
**  SIGINT handler
** -------------------------------------------------------------------------*/
void sighandler(int)
{ 
       printf("SIGINT\n");
       stop =1;
}

/* ---------------------------------------------------------------------------
**  main
** -------------------------------------------------------------------------*/
int main(int argc, char* argv[]) 
{	
	int verbose=0;
	int c = 0;
	const char * port = "8080";
	std::string sslCertificate;
	std::string webroot = "vuejs/dist";
	std::string nbthreads;

	while ((c = getopt (argc, argv, "hv::" "P:c:p:N:" )) != -1)
	{
		switch (c)
		{
			case 'v': verbose = 1; if (optarg && *optarg=='v') verbose++;  break;

			case 'P': port = optarg; break;
			case 'c': sslCertificate = optarg; break;
			case 'N': nbthreads = optarg; break;
			case 'p': webroot = optarg; break;		

			case 'h':
			{
				std::cout << argv[0] << " [-v[v]] [-P httpport] [-c sslkeycert] [-R rtspport] [-W width] [-H height] [-F fps] [-G <w>x<h>x<f>] url" << std::endl;
				std::cout << "\t -v               : verbose " << std::endl;
				std::cout << "\t -v v             : very verbose " << std::endl;
				std::cout << "\t -P port          : server port (default "<< port << ")" << std::endl;
				std::cout << "\t -p path          : server root path (default "<< webroot << ")" << std::endl;
				std::cout << "\t -c sslkeycert    : path to private key and certificate for HTTPS" << std::endl;

				exit(0);
			}
		}
	}

	// initialize log4cpp
	initLogger(verbose);


	// http options
	std::vector<std::string> options;
	options.push_back("document_root");
	options.push_back(webroot);
	options.push_back("enable_directory_listing");
	options.push_back("no");
	options.push_back("additional_header");
	options.push_back("X-Frame-Options: SAMEORIGIN");
	options.push_back("access_control_allow_origin");
	options.push_back("*");		
	options.push_back("listening_ports");
	options.push_back(port);
	if (!sslCertificate.empty()) {
		options.push_back("ssl_certificate");
		options.push_back(sslCertificate);
	}		
	if (!nbthreads.empty()) {
		options.push_back("num_threads");
		options.push_back(nbthreads);
	}		
	std::string url;
	if (optind<argc)
	{
			url = argv[optind];
			optind++;
	}

	// api server
	Rtsp2Ws server(url, options, verbose);
	if (server.getContext() == NULL)
	{
		LOG(WARN) << "Cannot listen on port:" << port; 
	}
	else
	{		
		LOG(NOTICE) << "Started on port:" << port << " webroot:" << webroot;
		signal(SIGINT,sighandler);	 
		while (!stop) {
			sleep(1); 
		}
	}
	
	return 0;
}

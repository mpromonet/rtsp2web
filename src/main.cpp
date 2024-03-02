/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** -------------------------------------------------------------------------*/

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <stdio.h>
#include <string>
#include <iostream>

#include "cxxopts.hpp"

#include "rtsp2ws.h"

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
	int rtptransport = RTSPConnection::RTPOVERTCP;

	cxxopts::Options options(argv[0], " - command line options");
	options.allow_unrecognised_options();
	options.add_options()
		("h,help"      , "Print usage")

		("P,port"      , "Listening port"       , cxxopts::value<std::string>()->default_value("8080")) 
		("N,thread"    , "Server number threads", cxxopts::value<std::string>()->default_value(""))
		("v,verbose"   , "Verbose"              , cxxopts::value<int>()->default_value("0"))
		("p,path"      , "Server root path"     , cxxopts::value<std::string>()->default_value("html"))
		("c,sslkeycert", "Path to private key and certificate for HTTPS", cxxopts::value<std::string>()->default_value(""))
		("M"           , "RTP over Multicast")
		("U"           , "RTP over Unicast")
		("H"           , "RTP over HTTP")
		;

	auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }

	std::string port = result["port"].as<std::string>();
	std::string webroot = result["path"].as<std::string>();
	int verbose = result["verbose"].as<int>();
	std::string sslCertificate = result["sslkeycert"].as<std::string>();
	std::string nbthreads = result["thread"].as<std::string>();

	if (result.count("M")) rtptransport = RTSPConnection::RTPUDPMULTICAST;
	if (result.count("U")) rtptransport = RTSPConnection::RTPUDPUNICAST;
	if (result.count("H")) rtptransport = RTSPConnection::RTPOVERHTTP;

	std::vector<std::string> urls;
	for (auto arg : result.unmatched()) {
		urls.push_back(arg);
	}

	// http options
	std::vector<std::string> opts;
	opts.push_back("document_root");
	opts.push_back(webroot);
	opts.push_back("enable_directory_listing");
	opts.push_back("no");
	opts.push_back("additional_header");
	opts.push_back("X-Frame-Options: SAMEORIGIN");
	opts.push_back("access_control_allow_origin");
	opts.push_back("*");		
	opts.push_back("listening_ports");
	opts.push_back(port);
	if (!sslCertificate.empty()) {
		opts.push_back("ssl_certificate");
		opts.push_back(sslCertificate);
	}		
	if (!nbthreads.empty()) {
		opts.push_back("num_threads");
		opts.push_back(nbthreads);
	}		

	// api server
	Rtsp2Ws server(urls, opts, rtptransport, verbose);
	if (server.getContext() == NULL)
	{
		std::cout << "Cannot listen on port:" << port << std::endl; 
	}
	else
	{		
		std::cout << "Started on port:" << port << " webroot:" << webroot << std::endl;
		signal(SIGINT,sighandler);	 
		while (!stop) {
			sleep(1); 
		}
		std::cout << "Exiting..." << std::endl;
	}
	
	return 0;
}

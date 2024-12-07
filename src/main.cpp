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
#include <fstream>

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
	Json::Value config;

	cxxopts::Options options(argv[0]);
	options.allow_unrecognised_options();
	options.add_options()
		("h,help"        , "Print usage")
		("V,version"     , "Print version and exit")
		("v,verbose"     , "Verbose"                                      , cxxopts::value<int>()->default_value("0"))

		("P,port"        , "Listening port"                               , cxxopts::value<std::string>()->default_value("8080s")) 
		("N,thread"      , "Server number threads"                        , cxxopts::value<std::string>()->default_value(""))
		("p,path"        , "Server root path"                             , cxxopts::value<std::string>()->default_value("www"))
		("c,sslkeycert"  , "Path to private key and certificate for HTTPS", cxxopts::value<std::string>()->default_value("keycert.pem"))

		("C,config"      , "Config"                                       , cxxopts::value<std::string>() ) 

		("r,rtptransport", "RTP transport(udp,tcp,multicast,http)"        , cxxopts::value<std::string>()->default_value("tcp"))
		;

	auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    std::cout << "Version: " << VERSION << std::endl;
    if (result.count("version"))
    {
      exit(0);
    }

	std::string port = result["port"].as<std::string>();
	std::string webroot = result["path"].as<std::string>();
	int verbose = result["verbose"].as<int>();
	std::string sslCertificate = result["sslkeycert"].as<std::string>();
	std::string nbthreads = result["thread"].as<std::string>();
	std::string rtptransport = result["rtptransport"].as<std::string>();

	if (result.count("config")) {
		std::string configFile = result["config"].as<std::string>();
		std::ifstream ifs(configFile.c_str());
		if (ifs.good()) {
			ifs >> config;
		}
	}

	int idx = 0;
	for (auto arg : result.unmatched()) {
		config["urls"]["stream" + std::to_string(idx++)]["video"]=arg;
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
	Rtsp2Ws server(config, opts, rtptransport, verbose);
	if (server.getContext() == NULL)
	{
		std::cout << "Cannot listen on port:" << port << std::endl; 
	}
	else
	{		
		std::cout << "Started on port:" << port << " webroot:" << webroot << std::endl;
		signal(SIGINT,sighandler); 
		while (!stop) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		std::cout << "Exiting..." << std::endl;
	}
	
	return 0;
}

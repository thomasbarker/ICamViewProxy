/*
 * Project: ICamViewProxy
 * File:	Main.cpp
 * Author:	Thomas Barker
 * Ref:		http://www.barkered.com
*/


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <memory>

#ifndef _MSC_VER
#include <unistd.h>
#endif

// SDL Headers
#include "SDL.h"
#include "SDL_net.h"
#include "ICamViewSocket.h"

// TCP sending buffer
char tcp_out_buff[65535];

// Frames per second
// This is a very loose figure as i dont time the method and subtract
// literally just 1000/fps
// Doesnt seem worth changing since its only supposed to be a simple program
const int fps = 6;
const int attempts_per_image = 3;

void display_usage()
{
	printf("||----------------------------------------------------------------------||\n");
	printf("||        ICamViewProxy 2010 - Thomas Barker - www.barkered.com         ||\n");
	printf("||----------------------------------------------------------------------||\n");
	printf("|| ICamViewProxy originally provided support for (HCV73/HCV72)          ||\n"); 
	printf("|| ICamViewPlus  servers. Functionality has since been added to support ||\n");
	printf("|| HCAMV based servers.                                                 ||\n");
	printf("||----------------------------------------------------------------------||\n");
	printf("|| Any queries/comments/patches, please visit the ICamViewProxy website ||\n"); 
	printf("|| Contact me at: http://www.barkered.com or boreddead@barkered.com     ||\n");
	printf("||----------------------------------------------------------------------||\n\n");
	printf("Usage:                                                               \n");
	printf("\tICamViewProxy -camid 1 -camhost 192.168.1.3 -camport 9001 -camuser user -campass password [ -proxyport 8888 | -move [up|down|left|right] ]\n");
	printf("\nExample usage for (HCV73/HCV72) ICamViewPlus  servers:\n");
	printf("ICamViewProxy -camhost 192.168.1.3 -camport 9001 -camuser user -campass password -proxyport 8888\n");
	printf("\nExample usage for HCAMV based servers:\n");
	printf("ICamViewProxy -HCAMV -camhost 192.168.1.3 -camport 9001 -camuser user -campass password -proxyport 8888\n");
	printf("\nExample usage for moving the HCAMV based servers:\n");
	printf("ICamViewProxy -HCAMV -camhost 192.168.1.3 -camport 9001 -camuser user -campass password -move up|down|left|right\n");
}

// Main app
int main(int argc, char **argv)
{
	// SDL 
	IPaddress ip,*remoteip;
	TCPsocket server,client;
	Uint32 ipaddr;

	// Bool to represent mode of execution
	bool bHCAMMode = false;
	
	// initialize SDL
	if(SDL_Init(0)==-1)
	{
		printf("SDL_Init: %s\n",SDL_GetError());
		exit(1);
	}

	// initialize SDL_net
	if(SDLNet_Init()==-1)
	{
		printf("SDLNet_Init: %s\n",SDLNet_GetError());
		exit(2);
	}
	
	// Read arguments from command line
	int nargs_set = 0;
	std::string camhost, camuser, campass, move;
	int proxyport, camport;
	unsigned short camID = 1;

	// Hardcoded defaults
	proxyport = 8888;
	camport = 9001;

	int i =0;
	for(i=0; i < argc; i++)
	{
		if( strcmp(argv[i],"-camid") == 0)
		{
			++i;
			camID = atoi(argv[i]);

			// Do not increment nargs_set as we default this option to cam 1
		}
		else if( strcmp(argv[i],"-camhost") == 0 ) 
		{
			++i;
			camhost = argv[i];
			++nargs_set;
		}
		else if( strcmp(argv[i],"-camport") == 0 )
		{
			++i;
			camport = atoi(argv[i]);
			++nargs_set;
		}
		else if( strcmp(argv[i], "-proxyport") == 0 )
		{
			++i;
			proxyport = atoi(argv[i]);
			++nargs_set;
		}
		else if( strcmp(argv[i],"-camuser") == 0 )
		{
			++i;
			camuser = argv[i];
			++nargs_set;
		}
		else if( strcmp(argv[i],"-campass") == 0 )
		{
			++i;
			campass = argv[i];
			++nargs_set;
		}
		else if(strcmp(argv[i],"-HCAMV") == 0 )
		{
			bHCAMMode = true;
			++nargs_set; 
		}
		else if( strcmp(argv[i],"-move") == 0 )
		{
			++i;
			move = argv[i];
			++nargs_set; 
		}

	}


	// Verify we have all args set
	if(nargs_set != 5 || (bHCAMMode && nargs_set != 6))
	{
		display_usage();
		return 0;
	}

	// Create cam class
	ICamViewSocket *pCamView = new ICamViewSocket(bHCAMMode, camID);

	//initialise 
	pCamView->Initialise(camhost,camport, camuser, campass);
	
	// If we are moving the camera, perform the operation and then exit
	if (move.length() > 0) 
	{
	  printf("Moving Camera then exiting\n");
	  pCamView->Login();
	  pCamView->Movement(move);
	  pCamView->Logout();
	  exit(0);
	}
	// Resolve the argument into an IPaddress type
	if(SDLNet_ResolveHost(&ip,NULL,proxyport)==-1)
	{
		printf("SDLNet_ResolveHost: %s\n",SDLNet_GetError());
		exit(3);
	}

	// open the server socket
	server=SDLNet_TCP_Open(&ip);
	if(!server)
	{
		printf("SDLNet_TCP_Open: %s\n",SDLNet_GetError());
		exit(4);
	}

	// This could easily be expanded to support multiple clients
	// but for the majority of purposes there is no need.
	while(1) // wait for one client to connect, then only service that. ie zone alarm
	{
		// try to accept a connection
		client = SDLNet_TCP_Accept(server);

		if(!client)// no connection accepted, retry
		{ 
			SDL_Delay(100); //sleep 1/10th of a second
			continue;
		}
		
		// get the clients IP and port number
		remoteip=SDLNet_TCP_GetPeerAddress(client);
		if(!remoteip)
		{
			printf("SDLNet_TCP_GetPeerAddress: %s\n",SDLNet_GetError());
			continue;
		}

		// print out the clients IP and port number
		ipaddr=SDL_SwapBE32(remoteip->host);
		printf("Accepted a connection from %d.%d.%d.%d port %hu\n",
			ipaddr >> 24,
			(ipaddr >> 16)&0xff,
			(ipaddr >> 8)&0xff,
			ipaddr & 0xff,
			remoteip->port);

		// Reset output buffer
		memset(tcp_out_buff,0,65535);
	
		std::string boundary;
		// Send header on accepted stream
		boundary += "HTTP/1.0 200 OK\r\nServer: ICamviewRelay-Relay\r\nConnection: close\r\nMax-Age: 0\r\nExpires: 0\r\nCache-Control: no-cache, private\r\nPragma: no-cache\r\nContent-Type: multipart/x-mixed-replace; boundary=--BoundaryString";

		// Cpy string into buffer
		strcpy(tcp_out_buff, boundary.c_str() );

		//tcp send
		SDLNet_TCP_Send(client, tcp_out_buff,  (int)boundary.length());

		// Boolean to control the connectivity of the socket
		bool bstreaming = true;

		while(bstreaming)
		{
			//get image data	
			bool bvalid = false;
            int nfs = -1; //filesize
			static bool no_image = true;
			int ii = 0;

			// Get valid image
			// This camera is wholely unreliable this makes sure only a valid image is passed to ZM
			while( !bvalid )
			{
				// printf("Attempt: %d\n", ii++);
				
				// Login
				pCamView->Login();

				// How big is the image, <=0 is error
				nfs = pCamView->RequestImage();

				// Logout
				// TODO this takes too much time.
				pCamView->Logout();
				
				if(nfs > 0)
				{
					no_image = false; // We now have a valid image stored for good
					bvalid = true;	  // Image is valid
				}
				if( ii > attempts_per_image && !no_image) // If after n attempts there is no image received, use old
				{
					nfs = pCamView->pimagesize;	// Filesize is old filesize
					bvalid = true;	// Now valid
				}
			} // End get valid image
		
			// Convert int to str
			char tmpstr[100];
			sprintf(tmpstr, "%d", nfs);

			// Packet for a single jpeg send
			boundary = "\r\n\r\n--BoundaryString\r\nContent-type: image/jpeg\r\nContent-Length: ";
			boundary += tmpstr;
			boundary += "\r\n\r\n";
		
			// cpy the string into the buffer
			strcpy(tcp_out_buff, boundary.c_str() );

			// cpy into packet to send
			// TODO why did I comment this out so long ago and manually copy bytes?!
			// std::memcpy(tcp_out_buff + boundary.length(), pCamView->pgoodimage, nfs);
			for(int i=0; i<nfs; i++)
            {
				tcp_out_buff[i + boundary.length()] = (char)pCamView->pgoodimage[i];
            }

			int lentosend = (int)boundary.length()+nfs;
			int nbytesent = SDLNet_TCP_Send(client, tcp_out_buff,  lentosend);
			
			// check the send happened, ie socket still alive.
			if(nbytesent == 0 ||nbytesent == -1)
			{
				bstreaming = false;
				printf("Disconnect\n");
			}
			
			SDL_Delay((Uint32)(1000.0/(double)fps));

		}// end fps loop, bstreaming


		printf("Disconnect or socket fail\n");
		
		// failed or disconnect
		SDLNet_TCP_Close(client); //close client
	}
	
	// shutdown SDL_net
	SDLNet_Quit();

	// shutdown SDL
	SDL_Quit();

	return(0);
}

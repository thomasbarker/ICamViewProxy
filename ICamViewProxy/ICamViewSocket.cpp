/*
 * Project: ICamViewProxy
 * File:	ICamViewSocket.cpp
 * Author:	Thomas Barker
 * Ref:		http://www.barkered.com
*/

#include "ICamViewSocket.h"
#include <fstream>
#include <iostream>

#define ERROR (0xff)

using namespace std;

// maximum size of our buffers, 64k
#define BUFFERSIZE 65535
#define PACKET_BUFFER_LEN 60 // tbh we are never going to receive this many, but better to have a higher upper limit

/*
* CTOR - construct buffers
*/
ICamViewSocket::ICamViewSocket(bool bHCAMVMode, unsigned short camID) :
m_shost("192.168.1.3"),
m_nport(9001),
m_nCameraID(camID),
m_bHCAMVMode(bHCAMVMode)
{
	// pre allocate packets for quick retrieval
	pResponses = SDLNet_AllocPacketV(PACKET_BUFFER_LEN, BUFFERSIZE );

	// single 6 byte packet for request
	pRequestImage = SDLNet_AllocPacket(6);

	// single 8 byte packet for request
	pRequestMovement = SDLNet_AllocPacket(8);

	// single 72 byte packet for login
	pLogin = SDLNet_AllocPacket(72);

	// single response packet, tmp packet
	pResponse = SDLNet_AllocPacket(BUFFERSIZE);	

	// image buffer
	// temporary working image
	pimage = new char[BUFFERSIZE];
	// known good image
	pgoodimage= new char[BUFFERSIZE];

}

/*
* Destroy buffers
*/
ICamViewSocket::~ICamViewSocket(void)
{
	SDLNet_FreePacketV(pResponses);
	SDLNet_FreePacket(pLogin);
	SDLNet_FreePacket(pRequestImage);
	SDLNet_FreePacket(pRequestMovement);
	SDLNet_FreePacket(pResponse);
	delete [] pimage;
	delete [] pgoodimage;
}


/*
* Send method
*/
int ICamViewSocket::Send(int channel, UDPpacket *out)
{
	if( !SDLNet_UDP_Send(m_socket, channel, out) )
	{
		printf("SDLNet_UDP_Send: %s\n",SDLNet_GetError());
	}
	else
	{
		// printf("Packet sent\n");
	}

	return 0;
}

/*
* Receive a UDP packet with time out parameters
*/
int ICamViewSocket::Receive( UDPpacket *in, Uint32 delay, int timeout)
{
	Uint32 ticks, ticks2;
	int err;

	in->data[0]=0;
	ticks = SDL_GetTicks();
	do
	{
		ticks2 = SDL_GetTicks();

		if(ticks2 - ticks > (Uint32) timeout)
		{
			// printf("timed out in rec ...\n"); // this is commented to look nicer...
			return(0);
		}
		err = SDLNet_UDP_Recv(m_socket, in);

		// if packet is received.
		//  1 = packet
		//  0 = nothing
		// -1 = error
		if(err == 1)
		{
			return 1;
		}
		else if(err == 0)
		{
			SDL_Delay(delay);
		}

	} while( err == 0 );

	return  0;

}

/*
* Send a Login packet to the webserver
*/
int ICamViewSocket::Login()
{
	// reset data
	memset(pLogin->data,0,72);
	memset(pResponse->data,0,BUFFERSIZE);

	pResponse->len =0;
	pLogin->len = 0;

	// create login http header
	std::string header;
	header = "HTTP/1.0 200 OK\r\nServer: ICamviewRelay-Relay\r\nConnection: close\r\nMax-Age: 0\r\nExpires: 0\r\nCache-Control: no-cache, private\r\nPragma: no-cache\r\nContent-Type: multipart/x-mixed-replace; boundary=--BoundaryString";

	// message id
	int byteid = 0;

	// ID of login message, matters not here as it is not checked
	pLogin->data[byteid++] = '0';
	pLogin->data[byteid++] = '1';

	// message type
	// login
	pLogin->data[byteid++] = '4';
	pLogin->data[byteid++] = '3';
	pLogin->data[byteid++] = '2';
	pLogin->data[byteid++] = '1';

	// cam id, should really be configurable.
	pLogin->data[byteid++] = '0';
	pLogin->data[byteid++] = (char)(m_nCameraID + 48);

	// TODO these for loops should be memcpys really
	// username 32 bytes
	int tmpstrlen = (int)m_susername.size();
	for(int i=0; i<tmpstrlen; i++)
	{
		pLogin->data[byteid + i] = m_susername.c_str()[i];
	}
	byteid += 32;

	// password
	tmpstrlen = (int)m_spassword.size();
	for(int i=0; i<tmpstrlen; i++)
	{
		pLogin->data[byteid + i] = m_spassword.c_str()[i];
	}
	pLogin->len = 72;

	// send packet
	int nstatus = Send(0, pLogin);

	// receive response but dont do anything with it as its useless
	nstatus = Receive(pResponse, 0, 100);

	return 0;
}


/*
* Send a Logout packet to the webserver
*/
int ICamViewSocket::Logout()
{
	//reset data
	memset(pLogin->data,0,6);
	memset(pResponse->data,0,BUFFERSIZE);

	pResponse->len =0;
	pLogin->len = 0;

	//message id
	int byteid = 0;

	//ID of login message, matters not here as it is not checked
	pLogin->data[byteid++] = '0';
	pLogin->data[byteid++] = '1';

	// message type
	//logout
	pLogin->data[byteid++] = '0';
	pLogin->data[byteid++] = '0';
	pLogin->data[byteid++] = '1';
	pLogin->data[byteid++] = '1';

	pLogin->len = byteid;

	//send packet
	int nstatus = Send(0, pLogin);

	//receive response but dont do anything with it as its useless
	nstatus = Receive(pResponse, 0, 100);

	return 0;
}


/*
* Send a Movement packet to the webserver
*/
int ICamViewSocket::Movement(std::string direction)
{
	//reset data
	memset(pRequestMovement->data,0,6);
	memset(pResponse->data,0,BUFFERSIZE);

	pRequestMovement->len = 0;

	int byteid = 0;

	pRequestMovement->data[byteid++] = '2';
	pRequestMovement->data[byteid++] = '0';

	pRequestMovement->data[byteid++] = '5';
	pRequestMovement->data[byteid++] = '0';
	pRequestMovement->data[byteid++] = '0';
	pRequestMovement->data[byteid++] = '5';

	// 0 = small
	// 1 = big
	pRequestMovement->data[byteid++] = '0';

	// 1 = up
	// 2 = down
	// 3 = left
	// 4 = right
	if (direction.compare("up") == 0)
	  pRequestMovement->data[byteid++] = '1';
	else if (direction.compare("down") == 0)
	  pRequestMovement->data[byteid++] = '2';
	else if (direction.compare("right") == 0)
	  pRequestMovement->data[byteid++] = '3';
	else if (direction.compare("left") == 0)
	  pRequestMovement->data[byteid++] = '4';
	else {
	  std::cout << "Unknown movement direction: " << direction << "\n";
	  return -1;
	}

	pRequestMovement->len = byteid;

	int nstatus = Send(0, pRequestMovement);
	nstatus = Receive(pResponse, 0, 100);

	return 0;
}

/*
* Initialise the class with username password etc
* Also set up the socket
*/
int ICamViewSocket::Initialise(std::string host, unsigned int port, std::string username, std::string password)
{
	m_shost = host;
	m_nport = port;
	m_susername = username;
	m_spassword = password;

	//resolve ip
	if(SDLNet_ResolveHost(&m_ip, m_shost.c_str(), m_nport)==-1)
	{
		printf("SDLNet_ResolveHost: %s\n",SDLNet_GetError());
		exit(1);
	}

	// open udp client socket
	if(!(m_socket=SDLNet_UDP_Open(0)))
	{
		printf("SDLNet_UDP_Open: %s\n",SDLNet_GetError());
		exit(2);
	}

	// bind server address to channel 0
	if(SDLNet_UDP_Bind(m_socket, 0, &m_ip)==-1)
	{
		printf("SDLNet_UDP_Bind: %s\n",SDLNet_GetError());
		exit(3);
	}

	return 0;
}

/* 
* Request an image from the web server, this is the complicated part
*/
int ICamViewSocket::RequestImage()
{

	// 0 data structures
	memset(pRequestImage->data,0,6);
	pRequestImage->len =0;

	// clean
	for(int i=0;i<PACKET_BUFFER_LEN;i++)
	{
		pResponses[i]->len = 0;
		memset(pResponses[i]->data,0,BUFFERSIZE);
	}

	// reset image
	memset(pimage, 0, BUFFERSIZE);
	// for speed cant honestly get away with just setting len's but for now they stay

	int byteid=0;

	static int requestid  = 0;

	// never let the id get out of range
	if (requestid > 9)
	{
		requestid = 0;
	}

	// set id for reception of data
	char tmprequestid[4];
	sprintf(tmprequestid,"%02d", requestid);

	pRequestImage->data[byteid++] = tmprequestid[0];
	pRequestImage->data[byteid++] = tmprequestid[1]; //this will be ignored

	// inc here incase we return
	requestid++;

	// type 3003
	// ie new image
	pRequestImage->data[byteid++] = '3';
	pRequestImage->data[byteid++] = '0';
	pRequestImage->data[byteid++] = '0';
	if(m_bHCAMVMode)
	{
		pRequestImage->data[byteid++] = '2';
	}
	else
	{
		pRequestImage->data[byteid++] = '3';
	}

	pRequestImage->len = 6;

	// receive response
	int nresponse = 1;
	int nindex = 0;

	// send packet to the webserver
	int nstatus = Send(0, pRequestImage);

	// get response, never go over our 'buffer' len
	while( nresponse == 1 && nindex < PACKET_BUFFER_LEN )
	{
		// set len to 0
		pResponses[nindex]->len = 0;

		// works local
		nresponse = Receive(pResponses[nindex], 25, 50);//was 25,50

		// if we have a valid response, count it.
		if(	nresponse == 1)
		{
			nindex++;
		}
	}

	// supposedly we have all the packets for a message now
	// knowing ICamView this could be a lie!

	long nDataStart = 0;
	int nfilesize = 100; // just to get things going
	int nbytesrec = 0;
	UDPpacket *ppacket = NULL;
	long nDataLength = 0; 
	Uint8 *pData = NULL;

	// loop through all packets we received
	for(int i =0; i<nindex; i++)
	{
		// set current packet ptr
		ppacket = pResponses[i];

		if(ppacket->len > 0)
		{

			// get packet length for processing
			nDataLength = ppacket->len;

			// set data pointer
			pData = ppacket->data;

			int nrec = nDataLength;

			char filesize[8];
			memset(filesize,0,8);

			char offset[6];
			memset(offset,0,6);

			char sid[2];
			int recid = 0;
			// cpy id into buffer
			memcpy(&sid, &pData[0], 2);
			recid = atoi(sid);
			int noffset = 0;

			// is this sane?
			// Does the id of this image bite match the id i requested
			if(requestid-1 == recid)
			{

				// get offset type
				memcpy(offset, &pData[2], 6);
				noffset = atoi( offset );
				if( noffset > BUFFERSIZE) {
				  printf("Offset too high\n");
				  return -1;
				}

				if( noffset == 0) // initial packet of image
				{
					if(nrec > 8) // if we actually have 8 bytes worth of data, ie filesize
					{
						nbytesrec = 0;
						// get image size
						// 8 bytes from byte 8 = filesize
						memcpy(filesize, &pData[8], 8);
						nfilesize = atoi( filesize );
					}
					else
					{
//						printf("not enough data to read noffset\n");
						return -1;
					}

					// dont try and work with big images
					// basically trashed data, we wont be receiving 64k+ data
					if(nfilesize > BUFFERSIZE)
					{
//						printf("file size greater than buffer %d\n",nfilesize);
						return -1;
					}

					// copy initial bytes into start of image
					if(m_bHCAMVMode)
					{
						memcpy( &pimage[nbytesrec], &pData[25], nrec - 25);
						nbytesrec += (nrec - 25);
					}
					else
					{
						memcpy( &pimage[nbytesrec], &pData[37], nrec - 37);
						nbytesrec += (nrec - 37);
					}
				}
				else if(pimage != NULL)
				{	
					if(nrec > 8 && nrec < nfilesize) // sanity check
					{
						memcpy( &pimage[noffset], &pData[8], nrec - 8);
						nbytesrec += (nrec - 8);
					}
					else
					{
//						printf("not enough data to set data\n");
						return -1;
					}
				}

				nDataStart += nDataLength ;

			}// end if pid check
		}// end >0 len check
	}

	//if we think we have a complete good image
	if(nbytesrec == nfilesize)
	{
		// TODO should really just swap buffers here
		pimagesize = nbytesrec;
		memcpy(pgoodimage, pimage, nbytesrec);

		return nbytesrec;
	}
	else
	{
		return -1;
	}

	return 0;
}


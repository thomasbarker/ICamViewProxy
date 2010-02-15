/*
 * Project: ICamViewProxy
 * File:	ICamViewSocket.h
 * Author:	Thomas Barker
 * Ref:		http://www.barkered.com
*/

#ifndef ICAMVIEWSOCKET

#include "SDL.h"
#include "SDL_net.h"
#include <string>

class ICamViewSocket
{
public:
	ICamViewSocket(bool bHCAMVMode = false, unsigned short camID = 1);
public:
	virtual ~ICamViewSocket(void);
	int Send(int channel, UDPpacket *out);
	int Receive( UDPpacket *in, Uint32 delay, int timeout);
	int Login();
	int Logout();
	int Movement(std::string direction);
	int Initialise(std::string host, unsigned int port, std::string username, std::string password);
	int RequestImage();

public:
	std::string m_shost;
	std::string m_susername;
	std::string m_spassword;
	unsigned int m_nport;
	unsigned short m_nCameraID;
	bool m_bHCAMVMode;
	UDPsocket m_socket;
	IPaddress m_ip;
	UDPpacket **pResponses;

	UDPpacket *pLogin;
	UDPpacket *pResponse;
	UDPpacket *pRequestImage;
	UDPpacket *pRequestMovement;
	char *pimage;
	char *pgoodimage;

	long pimagesize;
};

#endif

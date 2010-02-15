#ifndef ICAMVIEWSOCKET

#include "SDL.h"
#include "SDL_net.h"

#include <string>

class ICamViewSocket
{
public:
	ICamViewSocket(void);
public:
	virtual ~ICamViewSocket(void);
	int Send(int channel, UDPpacket *out);
	int Receive( UDPpacket *in, Uint32 delay, int timeout);
	int Login();
	int Initialise(std::string host, unsigned int port, std::string username, std::string password);
	int RequestImage();

public:
	std::string m_shost;
	std::string m_susername;
	std::string m_spassword;
	unsigned int m_nport;
	UDPsocket m_socket;
	IPaddress m_ip;
	UDPpacket **pResponses;

	UDPpacket *pLogin;
	UDPpacket *pResponse;
	UDPpacket *pRequestImage;
	char *pimage;
	char *pgoodimage;

	long pimagesize;
};

#endif

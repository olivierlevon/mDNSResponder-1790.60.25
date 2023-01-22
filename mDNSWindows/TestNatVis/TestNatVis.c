// TestNatVis.c : Test NatVis files
//

/*

general network structures

in_addr
in6_addr
sockaddr_in
sockaddr_in6
sockaddr
sockaddr_storage

mdns network structures

UTF8str255
domainlabel
mDNSOpaque16
mDNSAddr
mDNSOpaque32
mDNSOpaque128
mDNSOpaque48
v6addr_t


*/

#include	<stdio.h>
#include	<stdlib.h>

#include	<winsock2.h>
#include	<Ws2tcpip.h>

#include	"mDNSEmbeddedAPI.h"
#include	"DNSCommon.h"

int main()
{
	mDNSv4Addr				ipv4;
	struct sockaddr_in		sa4;
	struct sockaddr         inAddr;
	mDNSIPPort              port;
	struct sockaddr_storage *ss = (struct sockaddr_storage *)&inAddr;

	ipv4.NotAnInteger 	= ( (const struct sockaddr_in *) &inAddr )->sin_addr.s_addr;
	memset( &sa4, 0, sizeof( sa4 ) );
	sa4.sin_family 		= AF_INET;
	sa4.sin_port 		= port.NotAnInteger;
	sa4.sin_addr.s_addr = ipv4.NotAnInteger;



	struct sockaddr_in6 *		sa6p;
	struct sockaddr_in6			sa6;
	
	sa6p = (struct sockaddr_in6 *) &inAddr;
		
	// Bind the socket to the desired port
	memset( &sa6, 0, sizeof( sa6 ) );
	sa6.sin6_family		= AF_INET6;
	sa6.sin6_port		= port.NotAnInteger;
	sa6.sin6_flowinfo	= 0;
	sa6.sin6_addr		= sa6p->sin6_addr;
	sa6.sin6_scope_id	= sa6p->sin6_scope_id;






    DebugBreak();

	return 0;
}

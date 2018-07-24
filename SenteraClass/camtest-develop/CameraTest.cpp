// Define IS_WINDOWS if compiled on a Windows machine
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
#define IS_WINDOWS (1==1)
#endif

// *****************************************************************************
// ************************** System Includes **********************************
// *****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "Bufferizer.h"
#include "Packetizer.h"

// Include additional libraries & declare variables based on operating system
#if IS_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Ws2def.h>
#include <Windows.h>
#include <Winbase.h>
#include <conio.h>
#include "UnixForWindows.h"

// Tell linker Ws2_32.lib file is needed
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/time.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#endif

// *****************************************************************************
// ************************** User Includes ************************************
// *****************************************************************************
#include "crc.h"
#include "suas_types.h"
#include "packets.h"
#include "Bufferizer.h"
#include "Packetizer.h"

// *****************************************************************************
// ************************** Global Variables *********************************
// *****************************************************************************
#define BUFLEN 512

uint8 buf[BUFLEN];                                          // Buffer to hold content of constructed packet

struct sockaddr_in si_other_send;
int slen_send = sizeof(si_other_send);

struct sockaddr_in si_other_rec;
int slen_rec = sizeof(si_other_send);

char server_ipaddr[80] = "192.168.143.141";                 // Default IP of camera
char local_ipaddr[80] = "192.168.143.130";                  // Default local IP
uint16 port = 60530;                                        // Default port of camera

int num_cameras = 1;                                        // Default to a single camera
const int FILE_HISTORY_SIZE = 10;                           // The number of saved files to store
fw_imager_data_ready_t recent_images[8][FILE_HISTORY_SIZE]; // Store individual history of the last 8 images recorded in a circular buffer
int recent_images_length[8];                                // The number of recent images stored in the buffer
int recent_images_start[8];                                 // The current index of the circular buffer
fw_system_time_ack_t recent_time_ack;						// The most recent system time acknowledgement data

fw_payload_metadata_t camera_metadata[8];                   // Store up to 8 IDs worth of camera info
bool camera_metadata_valid[8];                              // Indicates whether each ID was valid 
unsigned long long camera_metadata_last_update_us[8];       // Timestamp of last update 
uint8 trigger_mask = 0x03;                                  // Hex mask to determine which cameras to send triggers to

#if IS_WINDOWS
typedef uint32_t in_addr_t;                                 // unit32_t not defined

SOCKET s_send;                                              // Windows sending socket ref 
SOCKET s_rec;                                               // Windows receiving socket ref

HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);           // Windows console handle for console cursor location
CONSOLE_SCREEN_BUFFER_INFO sb_info;                         // Winows console screen info
COORD curs_og;                                              // Windows coordinate for cursor in console
#else
int s_send;                                                 // UNIX sending socket ref
int s_rec;                                                  // UNIX receiving socket ref
#endif

// ********************************************************************************
// ************************** Socket Configuration Functions **********************
// ********************************************************************************
// *
// *  Both the configure_socket and configure_recieve functions prepare a socket 
// *  and return a reference. However, sockets are referenced differently on 
// *  Windows and UNIX systems. Thus, the functions are discretely defined for both.
// *
// ********************************************************************************
#if IS_WINDOWS
// Winsock function for configuring a socket
// Configures a given socket returning a socket ID, -1 indicates failure
SOCKET configure_socket(int myport, sockaddr_in& si_other, bool bind_socket)
{
    struct in_addr local_interface;
    SOCKET s;
    char broadcast = 1;
    char opt = 1;

    // Open network connections
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
    {
        fprintf(stderr, "!! Failed to create socket !!\n");
        return -1;
    }

	// Configure socket for sending broadcast data 
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
    {
        fprintf(stderr, "!! Failed allow broadcasts. !!\n");
        return -1;
    }

	// Allow binding to address already in use
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR)
    {
        fprintf(stderr, "!! Failed allow reuse address. !!\n");
        return -1;
    }

	// Configure interface for multicast traffic
    local_interface.s_addr = inet_addr(local_ipaddr); //inet_addr("192.168.143.242"); //  INADDR_ANY
    if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
                              (char *)&local_interface, sizeof(local_interface)) < 0)
    {
        fprintf(stderr, "!! Multicast interface assignment failed !!\n");
    }

	// Refer IP address and port to the socket
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(myport);
	if (inet_pton(AF_INET, server_ipaddr, &(si_other.sin_addr)) == 0)
	{
		fprintf(stderr, "!! inet_pton() failed !!\n");
		return -1;
	}

    // An odd case occurs when you try to bind to a port that already contains a bind.  
    // I'm not sure exactly what is happened, but somehow the socket reference, s becomes
    // invalid, even without the return code explicitely marking a failure.  I'd love to
    // sort this out later, but for now we simply skip it when binding on the send port 
    // I don't think the sender has to bind?
    if (bind_socket)
    {
        struct sockaddr_in bind_addr;
        memset(&bind_addr, 0, sizeof(struct sockaddr_in));
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = htons(myport); // locally bound port
        if((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0) 
        {
            fprintf(stderr, "bind error with port\n");
            return -1;
        }
    }
    
    return s;
}

// WinSock function for configuring the receiving socket
SOCKET configure_receive(int myport, sockaddr_in& si_other)
{
	struct in_addr local_interface;
	SOCKET s;
	char broadcast = 1;
	char opt = 1;

	// Open network connections
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		fprintf(stderr, "!! Failed to create socket !!\n");
		return -1;
	}

	// Configure for sending broadcast data
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == SOCKET_ERROR)
	{
		fprintf(stderr, "!! Failed allow broadcasts. !!\n");
		return -1;
	}

	// Allow binding to address already in use
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR)
	{
		fprintf(stderr, "!! Failed allow reuse address. !!\n");
		return -1;
	}

	// Refer IP address and port to the socket
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(myport);
	if (inet_pton(AF_INET, server_ipaddr, &(si_other.sin_addr)) == 0)
	{
		fprintf(stderr, "!! inet_pton() failed !!\n");
		return -1;
	}

	// Bind to the multicast address now
	struct sockaddr_in bind_addr;
	memset(&bind_addr, 0, sizeof(struct sockaddr_in));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(myport); // locally bound port
	if ((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0)
	{
		fprintf(stderr, "bind error with receive port\n");
		return -1;
	}

	// Make socket non-blocking
	u_long nonblocking = 1;
	if (ioctlsocket(s, FIONBIO, &nonblocking) != NO_ERROR)
	{
		fprintf(stderr, "!! Failed to make socket non-blocking !!");
	}


	// Attempt to join the multicast group for the camera
	// If we specified a multicast IP for the camera, this should succeed
	// If not, fallback to unicast binding

	// An odd case occurs when you try to bind to a port that already contains a bind.
	// I'm not sure exactly what is happened, but somehow the socket reference, s becomes
	// invalid, even without the return code explicitely marking a failure.  I'd love to
	// sort this out later, but for now we simply skip it when binding on the send port
	// Receiver has to bind
	struct ip_mreq mreq; 
	mreq.imr_interface.s_addr = inet_addr(local_ipaddr);
	mreq.imr_multiaddr.s_addr = inet_addr(server_ipaddr);
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0) {
		fprintf(stderr, "!! Could not join multicast group, Falling back to unicast !!");
	}

	return s;
}

#else
// Configures a given socket returning a socket ID, -1 indicates failure
// UNIX socket-friendly function for configuring sockets
int configure_socket(int myport, sockaddr_in& si_other, bool bind_socket)

{
	struct in_addr local_interface;
	int i, s;
	int broadcast = 1;
	int opt = 1;

	// Open network connections
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)

		if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			fprintf(stderr, "!! Failed to create socket !!\n");
			return -1;
		}

	// Configure socket for sending broadcast data 
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
	{
		fprintf(stderr, "!! Failed allow broadcasts. !!\n");
		return -1;
	}

	// Allow binding to address already in use
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		fprintf(stderr, "!! Failed allow reuse address. !!\n");
		return -1;
	}

	// Configure interface for multicast traffic
	local_interface.s_addr = inet_addr(local_ipaddr); //inet_addr("192.168.143.242"); //  INADDR_ANY
	if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
		(char *)&local_interface, sizeof(local_interface)) < 0)
	{
		fprintf(stderr, "!! Multicast interface assignment failed !!\n");
	}

	// Refer IP address and port to the socket
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(myport);
	if (inet_aton(server_ipaddr, &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "!! inet_aton() failed !!\n");
		return -1;
	}


	// An odd case occurs when you try to bind to a port that already contains a bind.  
	// I'm not sure exactly what is happened, but somehow the socket reference, s becomes
	// invalid, even without the return code explicitely marking a failure.  I'd love to
	// sort this out later, but for now we simply skip it when binding on the send port 
	// I don't think the sender has to bind?
	if (bind_socket)
	{
		struct sockaddr_in bind_addr;
		memset(&bind_addr, 0, sizeof(struct sockaddr_in));
		bind_addr.sin_family = AF_INET;
		bind_addr.sin_port = htons(myport); // locally bound port
		if ((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0)
		{
			fprintf(stderr, "bind error with port\n");
			return -1;
		}
	}

	return s;
}

// UNIX socket-friendly function for configuring the receiving socket
int configure_receive(int myport, sockaddr_in& si_other)
{
    struct in_addr local_interface;
    int i, s;
    int broadcast = 1;
    int opt = 1;

    // Open network connections
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        fprintf(stderr, "!! Failed to create socket !!\n");
        return -1;
    }

	// Configure socket for sending broadcast data 
    if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
    {
        fprintf(stderr, "!! Failed allow broadcasts. !!\n");
        return -1;
    }

	// Allow binding to address already in use
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        fprintf(stderr, "!! Failed allow reuse address. !!\n");
        return -1;
    }

	// Refer IP address and port to the socket
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(myport);
    if(inet_aton(server_ipaddr, &si_other.sin_addr)==0)
    {
      fprintf(stderr, "!! inet_aton() failed !!\n");
      return -1;
    }

    // Bind to the multicast address now
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(struct sockaddr_in));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(myport); // locally bound port
    if((bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr))) < 0)
    {
        fprintf(stderr, "bind error with receive port\n");
        return -1;
    }

    // Attempt to join the multicast group for the camera
    // If we specified a multicast IP for the camera, this should succeed
    // If not, fallback to unicast binding

    // An odd case occurs when you try to bind to a port that already contains a bind.
    // I'm not sure exactly what is happened, but somehow the socket reference, s becomes
    // invalid, even without the return code explicitely marking a failure.  I'd love to
    // sort this out later, but for now we simply skip it when binding on the send port
    // Receiver has to bind
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr=inet_addr(server_ipaddr);
    mreq.imr_interface.s_addr=inet_addr(local_ipaddr);
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
        fprintf(stderr, "!! Could not join multicast group, Falling back to unicast !!");
    }

    return s;
}
#endif

// ***********************************************************************************
// **************************** Query Packets ****************************************
//************************************************************************************
// *
// *  Retrieves the most recent status packet if available
// *  This includes both imager status packets and recent image packets if available
// *  This will also include any ACK packets that we have to listen to
// *
// ***********************************************************************************
int query_status_packet()
{
    uint8 rec_buf[BUFLEN];
    int rec_data;
    int current_packet = 0;
    errno = 0;
    int newdata_received = 0;

    // Keep reading and processing until the packets are emtpy.
    // Since each packet can be from a different imagerID, we process each one
    // old buffers will override new ones.
    // Non-blocking receives with no data will simply not write to the buffer (tested)
    do 
    {
		// Check for receiving packet; recvfrom returns length of packet
		#if IS_WINDOWS
		current_packet = recvfrom(s_rec, (char*)rec_buf, BUFLEN, 0, (struct sockaddr *) &si_other_rec, (socklen_t*)&slen_rec);
		if (current_packet == SOCKET_ERROR)
		{
			// perror returns no error, so do nothing for now
			//
			// Would block and eagain just say there is no data
			//if (!(WSAGetLastError() == EWOULDBLOCK || WSAGetLastError() == EAGAIN)) {
			//	perror("Error receiving data packet.");
			//	return -1;
			//}
		}
		#else
		current_packet = recvfrom(s_rec, rec_buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *) &si_other_rec, (socklen_t*)&slen_rec);
		if (current_packet <= 0)
		{
			// Would block and eagain just say there is no data
			if (!(errno == EWOULDBLOCK || errno == EAGAIN)) {
				perror("Error receiving data packet.");
				return -1;
			}
		}
		#endif
			// Minimum size for both status and new data events
        // As well as FW Header check
        else if (current_packet < 24 || !(rec_buf[0] == 0x46 && rec_buf[1] == 0x57))
        {    
            return 0;
        }
        else if (rec_buf[2] == 0x81) 
        {
            // Make sure our packet length is long enough
            if (!(rec_buf[3] >= 0x19 && rec_buf[4] == 0x00)) return 0;

            // Good packet! Go ahead and process.
            int n = 5;
            fw_payload_metadata_t status;
            status.imagerID       = rec_buf[n++];
            status.imagerType     = rec_buf[n++];
            status.imagerVersion  = rec_buf[n++];
            status.imagerVersion += rec_buf[n++] << 8;
            status.imagerVFOV     = rec_buf[n++];
            status.imagerVFOV    += rec_buf[n++] << 8;
            status.imagerHFOV     = rec_buf[n++];
            status.imagerHFOV    += rec_buf[n++] << 8;
            status.imagerZoom     = rec_buf[n++];
            status.imagerZoom    += rec_buf[n++] << 8;
            status.memCapacity    = rec_buf[n++];
            status.memCapacity   += rec_buf[n++] << 8;
            status.memUsed        = rec_buf[n++];
            status.mntRoll        = rec_buf[n++];
            status.mntRoll       += rec_buf[n++] << 8;
            status.mntPitch       = rec_buf[n++];
            status.mntPitch      += rec_buf[n++] << 8;
            status.mntYaw         = rec_buf[n++];
            status.mntYaw        += rec_buf[n++] << 8;
            status.pwrMode        = rec_buf[n++];
            status.sessionStatus  = rec_buf[n++];
            status.sessionImgCnt  = rec_buf[n++];
            status.sessionImgCnt += rec_buf[n++] << 8;
            status.sessionImgCnt += rec_buf[n++] << 16;
            status.sessionImgCnt += rec_buf[n++] << 24;

            // Rev S Updates
            // Rev S = 41 bytes + 5 header + crc
            if (current_packet >= 47) {
                status.captureStatus  = rec_buf[n++];
                status.minHFOV        = rec_buf[n++];
                status.minHFOV       += rec_buf[n++] << 8;
                status.minVFOV        = rec_buf[n++];
                status.minVFOV       += rec_buf[n++] << 8;
                status.maxHFOV        = rec_buf[n++];
                status.maxHFOV       += rec_buf[n++] << 8;
                status.maxVFOV        = rec_buf[n++];
                status.maxVFOV       += rec_buf[n++] << 8;
                status.videoStatus    = rec_buf[n++];
                status.videoDstIP     = rec_buf[n++];
                status.videoDstIP    += rec_buf[n++] << 8;
                status.videoDstIP    += rec_buf[n++] << 16;
                status.videoDstIP    += rec_buf[n++] << 24;
                status.videoDstPort   = rec_buf[n++];
                status.videoDstPort  += rec_buf[n++] << 8;
            } 
            else
            {
                status.captureStatus  = 0;
                status.minHFOV        = 0;
                status.minVFOV        = 0; 
                status.maxHFOV        = 0; 
                status.maxVFOV        = 0; 
                status.videoStatus    = 0; 
                status.videoDstIP     = 0; 
                status.videoDstPort   = 0; 
            }

            // Get the time we received the packet (approx)
            struct timeval currTv;
            gettimeofday(&currTv, NULL);
            unsigned long long timestamp = (((unsigned long long)currTv.tv_sec) * 1000000) + ((unsigned long long)currTv.tv_usec);

            // Store the most recent packet for each camera
            // A packet can be for more than one camera.
            for(int i = 0; i < 8; i++) 
            {
                if (status.imagerID & (0x01 << i)) 
                {
                    camera_metadata[i] = status;
                    camera_metadata_valid[i] = true;
                    camera_metadata_last_update_us[i] = timestamp;
                } 
            }

            newdata_received = 1;
    } 
    // Handle new image avilable packets
    else if (rec_buf[2] == 0x85)
    {
            // Make sure our packet length is long enough
            if (!(rec_buf[3] >= 0x21 && rec_buf[4] == 0x00)) return 0;

            // Good packet! Go ahead and process.
            int n = 5;
            fw_imager_data_ready_t new_image;
            new_image.imagerID    = rec_buf[n++];
            for (int i = 0; i < 48; ++i)
            {
                new_image.fileName[i] = rec_buf[n++];
            } 

            // Store the packet in the appropriate location of the circular buffer
            for(int i = 0; i < num_cameras; i++) 
            {
                if (new_image.imagerID & (0x01 << i)) 
                {
                    // Handle circular buffer wraparound
                    recent_images_length[i]++;
                    if (recent_images_length[i] > FILE_HISTORY_SIZE)
                    {
                        recent_images_length[i] = FILE_HISTORY_SIZE;
                        recent_images_start[i]++;
                        recent_images_start[i] = recent_images_start[i] % FILE_HISTORY_SIZE;
                    }


                    int cur_idx = (recent_images_start[i] + recent_images_length[i] - 1) % FILE_HISTORY_SIZE; 
                    recent_images[i][cur_idx] = new_image;
                } 
            }

            newdata_received = 1;

    }
    // Handle Time Ack Packets
    else if (rec_buf[2] == 0x8F)
    {
    	// Make sure our packet length is long enough
    	if (!(rec_buf[3] >= 0x12 && rec_buf[4] == 0x00 )) return 0;

    	// Good time ack packet, temporarily store the response
    	int n = 5;
    	recent_time_ack.requestID =  rec_buf[n++] << 0;
    	recent_time_ack.requestID += rec_buf[n++] << 8;
    	recent_time_ack.timeStamp =  (unsigned long long) rec_buf[n++] << 0;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 8;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 16;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 24;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 32;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 40;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 48;
    	recent_time_ack.timeStamp += (unsigned long long) rec_buf[n++] << 56;
    	recent_time_ack.bootTime  =  (unsigned long long) rec_buf[n++] << 0;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 8;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 16;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 24;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 32;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 40;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 48;
    	recent_time_ack.bootTime  += (unsigned long long) rec_buf[n++] << 56;
    }
    else
    {   
	printf("INCOMPLETE PACKET of Length: %d", current_packet);

    }

    } while (current_packet > 0);

    return newdata_received;
}

// ***********************************************************************************************************
// ************************************* Update Prompt *******************************************************
// ***********************************************************************************************************
// * Updates the shell prompt with sensor status information.  Updates are handled differently for Windows
// * due to differneces in system console APIs.
// ***********************************************************************************************************
#if IS_WINDOWS
void update_prompt(bool newpacket)
{
    // Get current Time 
    struct timeval currTv;
    gettimeofday(&currTv, NULL);
    unsigned long long cur_time = (((unsigned long long)currTv.tv_sec) * 1000000) + ((unsigned long long)currTv.tv_usec);

    // Lost comms for more than 5 seconds triggers a No Communication 
    const int MAX_NO_COMMS_US = 5000000;

	// Set console cursor position using position following the most recent command
	COORD coord;
	coord.X = curs_og.X;
	coord.Y = curs_og.Y;
	SetConsoleCursorPosition(console, coord);

    // Now, update each prompt in turn
    for (int i=0; i < num_cameras; i++)
    {
        printf("\r");

        // Update our user prompt
        if ((cur_time - camera_metadata_last_update_us[i]) > MAX_NO_COMMS_US || 
                !camera_metadata_valid[i])
        {
            printf("Sentera %d-[ No Communication ]", i+1);
        }
        else 
        {
            // Modify the current prompt to display updated info
            printf("Sentera %d-[", i+1);

            // Status
            if (camera_metadata[i].sessionStatus == 0x00)
            {
                printf("R");
                printf("-");
                printf("%03d", camera_metadata[i].sessionImgCnt); // Current Image Count
            }
            else
            {
                printf("Idle ");
            }

            printf(" ");
            printf("%3d%%", camera_metadata[i].memUsed); // Used
            printf(" / ");
            printf("%2d GB", camera_metadata[i].memCapacity / 100); // Capacity
            printf("]");

        }

        // Don't move down a line if we are on the last line
        if ((i+1) < num_cameras)
        {
            printf("\n");
        }
    }

    // Restore cursor position
    printf(" > ");
	fflush(stdout);
}

#else
// Updates the shell prompt with our status info 
void update_prompt(bool newpacket)
{
	// Get current Time 
	struct timeval currTv;
	gettimeofday(&currTv, NULL);
	unsigned long long cur_time = (((unsigned long long)currTv.tv_sec) * 1000000) + ((unsigned long long)currTv.tv_usec);

	// Lost comms for more than 5 seconds triggers a No Communication 
	const int MAX_NO_COMMS_US = 5000000;

	// Save cursor position (This will NOT work on windows)
	printf("\033[s");

	// Since we are on the lower prompt, the lowest indexed camera is back num_cameras -1 
	if (num_cameras > 1) printf("\033[%dA", num_cameras - 1);

	// Now, update each prompt in turn
	for (int i = 0; i < num_cameras; i++)
	{
		printf("\r");

		// Update our user prompt
		if ((cur_time - camera_metadata_last_update_us[i]) > MAX_NO_COMMS_US ||
			!camera_metadata_valid[i])
		{
			printf("Sentera %d-[ No Communication ]", i + 1);
		}
		else
		{
			// Modify the current prompt to display updated info
			printf("Sentera %d-[", i + 1);

			// Status
			if (camera_metadata[i].sessionStatus == 0x00)
			{
				printf("R");
				printf("-");
				printf("%03d", camera_metadata[i].sessionImgCnt); // Current Image Count
			}
			else
			{
				printf("Idle ");
			}

			printf(" ");
			printf("%3d%%", camera_metadata[i].memUsed); // Used
			printf(" / ");
			printf("%2d GB", camera_metadata[i].memCapacity / 100); // Capacity
			printf("]");

		}

		// Don't move down a line if we are on the last line
		if ((i + 1) < num_cameras)
		{
			printf("\n");
		}
	}

	// Restore cursor position
	printf(" > ");
	printf("\033[u");
	fflush(stdout);
}
#endif

int main(int argc, char *argv[])
{
    int port;
    int port_status;
    char user_input [80];
    int j;
    int packet_length = 0;
    struct timeval currTv;
    char mainSessionName[64];
    char fpfName[96];
    bool new_packet;

    fw_video_adjust_t video_adjust;
    memset(&video_adjust, 0, sizeof(video_adjust));
    video_adjust.aeTarget = 1024;
    video_adjust.bitrate = 2000000;
    video_adjust.eStab = 0;
    video_adjust.gain = 64;
    video_adjust.gop = 30;

    // Check for the right number of arguments.
    if(((argc < 4) && (argc != 2) && (argc != 1)) || ((argc > 1) && (strcmp(argv[1], "help") == 0)))
    {
        printf("Usage: \n");
        printf("\n");
        printf("Specify all Arguments: \n");
        printf("%s <ip address of camera> <port> <local ip address> <number of cameras>\n", argv[0]);
        printf("\n");
        printf("Use Default Port and Number of Cameras: \n");
        printf("%s <ip address of camera>\n", argv[0]);
        printf("\n");
        printf("Use All Defaults: \n");
        printf("%s\n", argv[0]);
        printf("\n");
        return -1;
    }

    // Initialize the IP address for the camera server
    if(argc > 2)
    {
    	printf("Using Specific IP! \n");
        strcpy(server_ipaddr, argv[1]);
        port = (uint16)atoi(argv[2]);
        port_status = port + 1;
    } else 
    {
    	printf("Using Default IP! \n");
    	strncpy(server_ipaddr, "192.168.143.141", sizeof(server_ipaddr) - 1);
    	port = 60530;
    	port_status = port + 1;
    }
    
    // Initialize the IP address for the local machine
    if(argc > 3)
    {
    	printf("Using Local IP! \n");
        strcpy(local_ipaddr, argv[3]);
    } else 
    {
    	printf("Using Default Interface! \n");
    	strncpy(local_ipaddr,"192.168.143.10", sizeof(local_ipaddr) - 1);
    }

    // Initialize the number of cameras
    if(argc > 4)
    {
        num_cameras = atoi(argv[4]);
    } else
    {
    	num_cameras = 1;
    }

    fw_payload_metadata_t status;

    // Assume we start without a connection
    for(int i=0; i<8; i++) {
        camera_metadata_valid[i] = false;
        camera_metadata_last_update_us[i] = 0;
    }

    // Reset our circular buffer
    for(int i=0; i< 8; i++) {
        recent_images_length[i] = 0;
        recent_images_start[i] = 0;    
    }

    // Print testing and socket information to the console
    printf("\n");
    printf("-----------------------------------------------------------------\n");
    printf("Sentera Camera Test\n");
    printf("Payload Protocol: Rev. S3\n");
    printf("-----------------------------------------------------------------\n");
    printf("\n");
    printf("Settings:\n");
    printf("Camera IP Address: %s\n", server_ipaddr);
    printf("Multicast interface: %s\n", local_ipaddr); 
    printf("\n");
    printf("Command Port: %d\n", port);
    printf("Status Port: %d\n", port_status);
    printf("\n");

	// Windows specific intializations for WinSock and console formatting
	#if IS_WINDOWS
	// Initialize WSA to initiate use of WinSock DLL
	WSADATA wsaData;
	int init_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (init_result != 0)
	{
		fprintf(stderr, "!! WSAStartup failed. !!");
		return -1;
	}

	// Console cursor jitters uncontrollably when repeatedly writing to the same lines
	// in the terminal so it is simply hidden using the Windows API
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO curs;
	curs.bVisible = false;
	curs.dwSize = 1;
	SetConsoleCursorInfo(console, &curs);
	#endif

	// If we run locally on the camera, don't bind to the port or we fail
	bool bind_send_socket = true;
	if (strcmp(local_ipaddr, server_ipaddr) == 0)
	{
		fprintf(stderr, "!! Local camera running, skipping bind to %d !!", port);
		bind_send_socket = false;
	}

	// Configure Sending Socket
	// If we try to bind sending socket, and it is multicast, the program breaks
	s_send = configure_socket(port, si_other_send, bind_send_socket);
	if (!s_send)
	{
		fprintf(stderr, "!! Unable to configure sending socket. !!");
		return -1;
	}

	// Configure Receiving Socket
	s_rec = configure_receive(port_status, si_other_rec);
	if (!s_rec)
	{
		fprintf(stderr, "!! Unable to configure receiving socket. !!");
		return -1;
	}

    printf("\n");
    printf("-----------------------------------------------------------------\n");
    printf("\n");

	//For polling user input on UNIX systems
	#if !IS_WINDOWS
	struct pollfd mypoll = { STDIN_FILENO, POLLIN | POLLPRI };
	int bytes_avail = 0;
	#endif

    //Drop into application shell
    while(1)
    {
        printf("\n");
		
		#if IS_WINDOWS
		// Windows machines grab the current cursor position. This will be referenced when overwriting 
		// status data for each camera.  Cursor location is reset in update_prompt().
		GetConsoleScreenBufferInfo(console, &sb_info);
		curs_og.X = sb_info.dwCursorPosition.X;
		curs_og.Y = sb_info.dwCursorPosition.Y;
		#endif	

        // Print shell prompt
        // 3 Fields : 1 = S/R for Session Running/Stopped, 2 = XXX% for percentage of card, 3 = XX GB for card size
        // 4 - Picture number
        //               1 444 222% / 33 GB
        //     "Sentera [S-001   0% /  0 GB] > ");s
        for (int i=0; i<num_cameras; i++) {
            printf("Sentera %d-[                  ]", i+1);

            // Only print a newline if we aren't on the last pass
            if ((i+1) == num_cameras) {
                printf(" > ");
            } else {
                printf("\n");
            }
        }

        fflush(stdout);

        // While waiting for input, update based on status packets
        // Poll at twice the 'blink rate' for the status signals
        bool receivedData = false;
        while (!receivedData) {

            // Update our packet
            new_packet = (query_status_packet() == 1);
            update_prompt(new_packet);
			
			// Poll for user input before querying next status packet; system type will determine
			// the method of polling for user input; status will not update between time user enters
			// keystroke and submits command.
			#if IS_WINDOWS
			// If keystroke waiting in input buffer, prompt user for input; else, keep querying packets
			if (_kbhit())
			#else
			// Poll last so our prompt updates immediately
			// Wait for 500 ms to see if the user entered a command 
			bytes_avail = poll(&mypoll, 1, 500);
			if (bytes_avail > 0)
			#endif
			{
				scanf("%s", user_input);
				receivedData = true;
			}
        }
			
        // ------------------------- Begin Interpreter -------------------------------

        // Proceed accordingly based on command (long if,else if statement)
        if(strcmp(user_input, "session") == 0)
        {
            // Construct new session packet
            fw_imager_session_t imager_session = Packetizer::session();

            // Load the new packet into the buffer
            packet_length = Bufferizer::session(imager_session, buf);
        }
        // Imager select command
        else if (strcmp(user_input, "is") == 0)
        {
            // Set new trigger mask from user input
            uint8 tmp_u8;
            printf("Current Trigger Mask: 0x%02X\n", trigger_mask);
            printf("New Mask Value (0x00-0xFF): ");
            scanf("%hhx", &tmp_u8);
            trigger_mask = tmp_u8;
            printf("Mask Set To: 0x%02X\n", trigger_mask);
            packet_length = 0;
        }
        // Trigger command
        else if(strcmp(user_input, "trigger") == 0)
        {
            // Construct new image trigger packet from trigger mask
            fw_imager_trigger_t imager_trigger = Packetizer::trigger(trigger_mask);

            // Load the new packet into the buffer
            packet_length = Bufferizer::trigger(imager_trigger,buf);
        }
        // Zoom command 
        else if(strcmp(user_input, "zoom") == 0)
        {
            // Construct new zoom packet from the trigger mask
            fw_imager_zoom_t imager_zoom = Packetizer::zoom(trigger_mask);

            // Load the new packet into the buffer
            packet_length = Bufferizer::zoom(imager_zoom, buf);
        }
        // Preview stream command
        else if(strcmp(user_input, "preview") == 0)
        {
            // Construct new image preview stream packet
            fw_imager_preview_stream_setup_t imager_preview = Packetizer::preview_stream_setup(trigger_mask);
            
            // Load the new packet into the buffer
            packet_length = Bufferizer::preview_stream_setup(imager_preview, buf);
        }
        else if(strcmp(user_input, "el") == 0)
        {
            fw_elevation_metadata_t elevation = Packetizer::elevation_metadata();
            packet_length = Bufferizer::elevation_metadata(elevation, buf);
        }        
        else if (strcmp(user_input, "loc") == 0)
        {
            fw_aircraft_metadata_t aircraft = Packetizer::location_metadata();
        	packet_length = Bufferizer::aircraft_metadata(aircraft, buf);
        }
        else if(strcmp(user_input, "video") == 0)
        {
            fw_video_session_t video_session = Packetizer::video();
            packet_length = Bufferizer::video(video_session,buf);
        }
        else if(strcmp(user_input, "videotime") == 0)
        {
            fw_video_session_t video_session = Packetizer::videotime(currTv);
            packet_length = Bufferizer::videotime(video_session, buf);
        }
        else if(strcmp(user_input, "videoadvanced") == 0)
        {
            fw_video_session_advanced_t video_session_adv = Packetizer::videoadvanced(currTv);
            packet_length = Bufferizer::videoadvanced(video_session_adv, buf);
        }
        else if(strcmp(user_input, "videoadjust") == 0)
        {
            // video_adjust object is scoped to the main function to enable saving values and typing less
            uint32 tmp_u32;

            printf("Bitrate (bps[100,000:10,000,000): ");
            scanf("%u", &video_adjust.bitrate);

            printf("Group of Pictures (GOP, [1:30]): ");
            scanf("%u", &tmp_u32);
            video_adjust.gop = tmp_u32;

            printf("Estab (0=disabled, 1=enabled): ");
            scanf("%u", &tmp_u32);
            video_adjust.eStab = tmp_u32;

            printf("Auto Exposure Target [1:4095] (rec=1024): ");
            scanf("%u", &tmp_u32);
            video_adjust.aeTarget = tmp_u32;

            printf("Gain: [1:255] (rec=64): ");
            scanf("%u", &tmp_u32);
            video_adjust.gain = tmp_u32;

            packet_length = Bufferizer::videoadjust(video_adjust, buf);
        }
        else if(strcmp(user_input, "ea") == 0)
        {
            fw_exposure_adjust_t exposure_adjust = Packetizer::exposureadjust();
            packet_length = Bufferizer::exposureadjust(exposure_adjust, buf);
        }
        else if(strcmp(user_input, "brightness") == 0)
        {
            // video_adjust object is scoped to the main function to enable saving values and typing less
            uint32 tmp_u32;

            printf("Auto Exposure Target [1:4095] (rec=1024): ");
            scanf("%u", &tmp_u32);
            video_adjust.aeTarget = tmp_u32;

            printf("Gain: [1:255] (rec=64): ");
            scanf("%u", &tmp_u32);
            video_adjust.gain = tmp_u32;

            packet_length = Bufferizer::videoadjust(video_adjust, buf);
        }
        else if(strcmp(user_input, "estab") == 0)
        {
            // video_adjust object is scoped to the main function to enable saving values and typing less
            uint32 tmp_u32;

            printf("Estab (0=disabled, 1=enabled): ");
            scanf("%u", &tmp_u32);
            video_adjust.eStab = tmp_u32;

            packet_length = Bufferizer::videoadjust(video_adjust, buf);
        }
        else if(strcmp(user_input, "bitrate") == 0)
        {
            // video_adjust object is scoped to the main function to enable saving values and typing less
            uint32 tmp_u32;

            printf("Bitrate (bps[100,000:10,000,000): ");
            scanf("%u", &video_adjust.bitrate);

            packet_length = Bufferizer::videoadjust(video_adjust, buf);
        }
        else if(strcmp(user_input, "gop") == 0)
        {
            // video_adjust object is scoped to the main function to enable saving values and typing less
            uint32 tmp_u32;

            printf("Group of Pictures (GOP, [1:30]): ");
            scanf("%u", &tmp_u32);
            video_adjust.gop = tmp_u32;

            packet_length = Bufferizer::videoadjust(video_adjust, buf);
        }
        else if(strcmp(user_input, "var") == 0)
		{
			fw_video_adjust_relative_t va_relative = Packetizer::video_adjust_relative(trigger_mask);
			packet_length = Bufferizer::video_adjust_relative(va_relative, buf);
		}
        else if(strcmp(user_input, "focus") == 0)
        {
            fw_focus_session_t focus_session = Packetizer::focus();
            packet_length = Bufferizer::focus(focus_session, buf);
        }
        else if(strcmp(user_input, "sf") == 0)
        {
            fw_still_focus_session_t still_focus_session = Packetizer::sf();
            packet_length = Bufferizer::sf(still_focus_session, buf);
        }
        else if (strcmp(user_input, "time") == 0) 
        {
        	fw_system_time_t system_time = Packetizer::system_time();
        	packet_length = Bufferizer::system_time(system_time, buf);

		// Send this one early, since we wait for a response
		if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send)==-1)
		{
		    printf("Failed to send packet: %d", errno);
		    return -1;
		}
		packet_length = 0;  // Only send once, so set this to 0 so we don't send again

		// Wait for a matching response
        struct timeval startTime;
		struct timeval curTime;
        gettimeofday(&startTime, NULL);
		bool timeout = false;
		while (system_time.requestID != recent_time_ack.requestID && !timeout)
		{
			usleep(100);
			query_status_packet();
			gettimeofday(&curTime, NULL);
			if ((curTime.tv_sec - startTime.tv_sec) > 1)
			{
				timeout=true;
			}
		}

        	// Display the current time
        	if (system_time.requestID != recent_time_ack.requestID) {
        		printf("\n");
        		printf("******** NO RESPONSE TO TIME REQUEST ********");
        		printf("\n");
        	} else {
        		printf("\n");
        		printf("Time Response Received: \n");
        		printf("ID        : %u\n", recent_time_ack.requestID);
        		printf("Timestamp (us): %lu\n", recent_time_ack.timeStamp);
        		printf("BootTime  (ns): %lu\n", recent_time_ack.bootTime);
			printf("Diff      (us): %ld\n", (int64_t)((recent_time_ack.bootTime / 1000) - recent_time_ack.timeStamp));
        		printf("\n");
        	}
        }
        else if(strcmp(user_input, "spoof") == 0)
        {
            fw_imager_trigger_t imager_trigger = Packetizer::spoof(trigger_mask);
            packet_length = Bufferizer::trigger(imager_trigger, buf);

            // Get user input for interval
            uint32 tmp_u32;
            int q = 0;
            printf("Current Trigger Mask: 0x%02X\n", trigger_mask);
            printf("Interval (us): ");
            scanf("%u", &tmp_u32);
            printf("Spoofing autopilot, sending triggers.");

            while(1)
            {
                printf("Sending trigger[%d]\n", q); q++;
                if (sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send)==-1)
                {
                    printf("Failed to send packet");
                    return -1;
                }
                usleep(tmp_u32);
            
            }
        }
        else if(strcmp(user_input, "status") == 0)
        {

            // Display all information about the most recent status packet
            printf("\n");
            printf("-- Most Recent Status Packets --\n");

            for(int i=0; i<num_cameras; i++) {
                printf("\n");

                if (!camera_metadata_valid[i]) {
                    printf("-- Imager ID %d --\n", (i+1));
                    printf("\n");
                    printf("NO VALID PACKETS RECEIVED YET!\n");
                }
                else
                {
                    printf("-- Imager ID %d --\n", (i+1));
                    printf("\n");
                    printf("Imager Type    : %d\n", camera_metadata[i].imagerType);
                    printf("Imager Version : %d\n", camera_metadata[i].imagerVersion);
                    printf("Imager VFOV    : %01.02f deg\n", camera_metadata[i].imagerVFOV / 100.0);
                    printf("VFOV Range     : %01.02f - %01.02f deg\n", camera_metadata[i].minVFOV / 100.0, camera_metadata[i].maxVFOV / 100.0);
                    printf("Imager HFOV    : %01.02f deg\n", camera_metadata[i].imagerHFOV / 100.0);
                    printf("HFOV Range     : %01.02f - %01.02f deg\n", camera_metadata[i].minHFOV / 100.0, camera_metadata[i].maxHFOV / 100.0);
                    printf("Imager Zoom    : %01.02f%%\n", camera_metadata[i].imagerZoom / 100.0);
                    printf("\n");
                    printf("Card Capacity  : %d GB\n", camera_metadata[i].memCapacity);
                    printf("Used Capacity  : %d%%\n", camera_metadata[i].memUsed);
                    printf("\n");
                    printf("Mount Roll     : %01.02f deg\n", camera_metadata[i].mntRoll / 100.0);
                    printf("Mount Pitch    : %01.02f deg\n", camera_metadata[i].mntPitch / 100.0);
                    printf("Mount Yaw      : %01.02f deg\n", camera_metadata[i].mntYaw / 100.0);
                    printf("\n");
                    printf("Power Mode     : %s\n", (camera_metadata[i].pwrMode == 0x00) ? "Normal":"Low Power");
                    printf("\n");
                    printf("Session Status : %s\n", (camera_metadata[i].sessionStatus == 0x00) ? "Open":"Closed");
                    printf("Trigger Type   : %s\n", (camera_metadata[i].captureStatus & 0x01) ? "Auto":"Manual");
                    printf("Session Images : %d\n", camera_metadata[i].sessionImgCnt);
                    printf("\n");
                    printf("Video Recording: %s\n", (camera_metadata[i].captureStatus & 0x02) ? "True":"False");
                    printf("Video Streaming: %s\n", (camera_metadata[i].videoStatus == 0x01) ? "True":"False");
                    printf("Video IP       : %d.%d.%d.%d\n", (camera_metadata[i].videoDstIP >> 24) & 0xFF, 
                                                             (camera_metadata[i].videoDstIP >> 16) & 0xFF, 
                                                             (camera_metadata[i].videoDstIP >>  8) & 0xFF, 
                                                             (camera_metadata[i].videoDstIP      ) & 0xFF);
                    printf("Video Port     : %d\n", camera_metadata[i].videoDstPort);
                    printf("\n");
                }
            }
            printf("\n");
            packet_length = 0;
        }
        else if(strcmp(user_input, "files") == 0)
        {
            for(int i=0; i<num_cameras; i++) {
                printf("\n");

                printf("-- Imager ID %d --\n", (i+1));
                printf("\n");

                if (recent_images_length[i] <= 0) {
                    printf("No Images Taken Yet!");
                }

                for(int j=0; j<recent_images_length[i]; j++) {
                    int cur_idx = (recent_images_start[i] + j) % FILE_HISTORY_SIZE;
                    printf("%s\n", recent_images[i][cur_idx].fileName);
                }
            }

            printf("\n");
            packet_length = 0;
        }
        else if(strcmp(user_input, "quit") == 0)
        {
            break;
        }
        else
        {
            if(strcmp(user_input, "help") != 0)
            {
                printf("Command not recognized\n\n");
            }

            printf("Usage:\n");
            printf("  session\n");
            printf("  trigger\n");
            printf("  is (imager select)\n");
            printf("  video\n");
            printf("  videotime\n");
            printf("  videoadvanced\n");
            printf("  videoadjust\n");
            printf("     bitrate\n");
            printf("     gop\n");
            printf("     estab\n");
            printf("     brightness\n");
            printf("  var (video adjust relative)\n");
            printf("  ea (exposure adjust)\n");
            printf("  focus\n");
            printf("  preview\n");
            printf("  sf (still focus)\n");
            printf("  status\n");
            printf("  files\n");
            printf("  zoom\n");
            printf("  spoof (spoof an autopilot sending periodic triggers)\n");
            printf("  loc (set location, spoof autopilot info pakcet)\n");
            printf("  el (send elevation metadata)\n");
            printf("  time\n");
            printf("  quit\n");

            continue;
        }

        // Packets are buit in the packet if/else construct, stored in buf, and sent here.
        if (packet_length > 0 && sendto(s_send, (char*)buf, packet_length, 0, (const struct sockaddr *)&si_other_send, slen_send)==-1)
        {
            printf("Failed to send packet: %d", errno);
            return -1;
        }

    }
    printf("\n");

	// Close the sockets
	#if IS_WINDOWS
	closesocket(s_send);
	closesocket(s_rec);
	#else
	close(s_send);
	close(s_rec);
	#endif
    return 0;
}

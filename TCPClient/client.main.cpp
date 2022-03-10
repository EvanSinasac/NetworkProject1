#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512						// Default buffer length of our buffer in characters
#define DEFAULT_PORT "27020"					// The default port to use
#define SERVER "127.0.0.1"						// The IP of our server


struct ClientInfo {
	// Buffer information (this is basically you buffer class)
	WSABUF dataBuf;
	char buffer[DEFAULT_BUFLEN];
	int bytesRECV;

	std::string name;
	std::vector<std::string> room_name;
};

//Identifiers
bool SerializeAndSendMessage(SOCKET connectSocket, const char* sendbuf, std::string room_name);
bool SerializeAndJoinRoom(SOCKET connectSocket, std::string room_name);
bool SerializeAndLeaveRoom(SOCKET connectSocket, std::string room_name);
bool SerializeAndSendName(SOCKET connectSocket, std::string name);

//Protocols
//uint32_t ProtocolHeader(int packet_length, int message_id);																							//[packet_length][message_id]
//uint32_t ProtocolSendMessage(uint32_t header, int rnLength, std::string room_name, int mLength, char* message);										//[header][length][room_name][length][message]
//uint32_t ProtocolReceiveMessage(uint32_t header, int nLength, std::string name, int mLenght, char* message, int rnLength, std::string room_name);	//[header][length][name][length][message][length][room_name]
//uint32_t ProtocolJoinRoom(uint32_t header, int rnLength);																							//[header][length][room_name]
//uint32_t ProtocolLeaveRoom(uint32_t header, int rnLength);																							//[header][length][room_name]


//Globals
std::string sInput;
ClientInfo* client = new ClientInfo;

int main(int argc, char** argv)
{
	std::cout << "Hello!  Welcome to the chat service.  Please enter a name: ";
	std::cin >> client->name;
	std::cout << "Hello " << client->name << ".  To enter a room, type in \"/join \" and the room name, then press enter." << std::endl
		<< "To leave a room, type in \"/leave \" and the room name, then press enter." << std::endl
		<< "To message a group, type in \"/message \" and the room name, then the message then press enter." << std::endl
		<< "Now attempting to connect to server..." << std::endl;
	
	
	bool quit = false;
	bool isConnected = true;
	bool backSpace = false;

	//Initialize our chat client
	WSADATA wsaData;							// holds Winsock data
	SOCKET connectSocket = INVALID_SOCKET;		// Our connection socket used to connect to the server

	struct addrinfo* infoResult = NULL;			// Holds the address information of our server
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	DWORD Flags = 0;

	char recvbuf[DEFAULT_BUFLEN];				// The maximum buffer size of a message to send
	int result;									// code of the result of any command we use
	int recvbuflen = DEFAULT_BUFLEN;			// The length of the buffer we receive from the server

	// Step #1 Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error: %d\n", result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Step #2 Resolve the server address and port
	result = getaddrinfo(SERVER, DEFAULT_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}


	// Step #3 Attempt to connect to an address until one succeeds
	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(infoResult);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	isConnected = true;

	std::cout << "Connected to server!" << std::endl;
	if (!SerializeAndSendName(connectSocket, client->name))
	{
		std::cout << "Couldn't send name to server!" << std::endl;
		return -1;
	}

	while (!quit)
	{
		if (_kbhit())
		{
			//printf("\n");
			//Add character to our message string
			//if "delete" (backspace) (8), remove a character from the message
			//if "enter" (10), serialize and send message

			char  key = _getch();

			if (key == 27)				//ESC
			{
				quit = true;
			}

			if (key == 8)				//Backspace
			{
				sInput.pop_back();
				backSpace = true;
			}

			if (key == 13)				//Enter
			{
				std::stringstream ss;
				ss << sInput;
				std::string nextToken;

				ss >> nextToken;
				if (nextToken == "/join")
				{
					//	/join room_name
					ss >> nextToken;
					if (!SerializeAndJoinRoom(connectSocket, nextToken))
					{
						std::cout << "SerializeAndJoinRoom failed!" << std::endl;
					}
					client->room_name.push_back(nextToken);
				}
				else if (nextToken == "/leave")
				{
					//	/leave room_name
					ss >> nextToken;
					for (int i = 0; i < client->room_name.size(); i++)
					{
						if (client->room_name[i] == nextToken)
						{
							client->room_name.erase(client->room_name.begin() + i);
							i--;
						}
					}
					if (!SerializeAndLeaveRoom(connectSocket, nextToken))
					{
						std::cout << "SerializeAndLeaveRoom failed!" << std::endl;
					}
				}
				else if (nextToken == "/message")
				{
					//	/message room_name message...
					std::string room_name;
					ss >> room_name;
					bool inRoom = false;

					for (unsigned int i = 0; i < client->room_name.size(); i++)
					{
						if (room_name == client->room_name[i])
						{
							inRoom = true;
							break;
						}
					}

					if (inRoom)
					{
						std::string message = "";
						while (ss >> nextToken)
						{
							message.append(nextToken);
							message.append(" ");
						}

						if (!SerializeAndSendMessage(connectSocket, message.c_str(), room_name))
						{
							std::cout << "Send Message failed!" << std::endl;
						}
					}
					else
					{
						std::cout << std::endl << "***You are not in " << room_name << " to send a message***" << std::endl;
					}
					
				}

				//if (sInput == "join")
				//{
				//	std::cout << std::endl << "Please enter the room name: ";
				//	std::string newRoom;
				//	std::cin >> newRoom;
				//	client->room_name.push_back(newRoom);
				//	if (!SerializeAndJoinRoom(connectSocket, newRoom))
				//	{
				//		std::cout << "SerializeAndJoinRoom failed!" << std::endl;
				//	}
				//}
				//else if (sInput == "leave")
				//{
				//	std::cout << std::endl << "Please enter the room name you want to leave: ";
				//	std::string leaveRoom;
				//	std::cin >> leaveRoom;
				//	for (int i = 0; i < client->room_name.size(); i++)
				//	{
				//		if (client->room_name[i] == leaveRoom)
				//		{
				//			client->room_name.erase(client->room_name.begin() + i);
				//			i--;
				//		}
				//	}
				//	if (!SerializeAndLeaveRoom(connectSocket, leaveRoom))
				//	{
				//		std::cout << "SerializeAndLeaveRoom failed!" << std::endl;
				//	}
				//}
				//else
				//{
				//	// Step #4 Send the message to the server
				//	if (!SerializeAndSendMessage(connectSocket, sInput.c_str()))	//send the message
				//	{
				//		std::cout << "SerializeAndSendMessage failed!" << std::endl;
				//	}
				//	//printf("\n");								//new line
				//	
				//	
				//}

				int size = sInput.size();					//how many characters in the message
				for (unsigned int i = 0; i < size; i++)		//erase them from our vector
				{
					sInput.pop_back();
				}
				backSpace = true;
				std::cout << std::endl;
				ss.str("");
			} //end of enter

			if (!backSpace)
			{
				sInput.push_back(key);
			}
			else
			{
				backSpace = false;
			}
			
			printf("%c", key);
		} //end of !quit

		if (isConnected)
		{
			// Set recv to be NonBlocking using ioctlsocket
			// recv
			u_long iMode = 1;
			result = ioctlsocket(connectSocket, FIONBIO, &iMode);
			if (result != NO_ERROR)
			{
				printf("ioctlsocket failed with error: %d\n", result);
			}

			result = recv(connectSocket, recvbuf, recvbuflen, 0);
			/*result = recv(connectSocket,
				client->dataBuf.buf,
				client->dataBuf.len,
				Flags);*/

			if (result == SOCKET_ERROR)
			{
				if (WSAGetLastError() == 10035)
				{
					//ignore this error
				}
				else
				{
					//Tell the user there was an error
					//Shut down properly.
					printf("recv failed with error: %d\n", WSAGetLastError());

					// Step #5 shutdown the connection since no more data will be sent
					result = shutdown(connectSocket, SD_SEND);
					if (result == SOCKET_ERROR)
					{
						printf("shutdown failed with error: %d\n", WSAGetLastError());
						closesocket(connectSocket);
						WSACleanup();
						return 1;
					}
				}
			}
			else if (result == 0)
			{
				//Connection closed
				isConnected = false;
			}
			else
			{
				//TODO - Modify to receive the Receive Message protocol
				//Print the received message
				//printf("Bytes received: %d\n", result);
				//printf("Message: %s\n", recvbuf);

				//Parse message
				std::string received(recvbuf);
				std::cout << "Received: " << received << std::endl;

				std::stringstream rs;
				rs << received;
				
				uint32_t packet_length;
				rs >> packet_length;
				
				int message_id;
				rs >> message_id;
				
				int name_length;
				rs >> name_length;

				std::string name;
				rs >> name;

				
				int message_length;
				std::string smessage_length;
				rs >> smessage_length;
				message_length = atoi(smessage_length.c_str());

				std::string message = "";
				std::string nextToken;
				for (unsigned int x = 0; x < message_length; x++)
				{
					rs >> nextToken;
					message.append(nextToken);
					message.append(" ");
				}

				int room_nameLength;
				rs >> room_nameLength;

				std::string room_name;
				rs >> room_name;

				std::cout << "[" << name << "] sent: \"" << message << "\" to " << room_name << std::endl;
			
				recvbuf[0] = ' ';
				recvbuflen = 1;
			}
		} //end of isConnected

	} //end of while


	// Step #5 shutdown the connection since no more data will be sent
	result = shutdown(connectSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}


	// Step #7 cleanup
	closesocket(connectSocket);
	WSACleanup();

	system("pause");

} //end of main


bool SerializeAndSendMessage(SOCKET connectSocket, const char* sendbuf, std::string room_name)
{
	//TO-DO: Setup protocol stuff for sending and receiving
	int result;

	std::stringstream ss;
	//							packet_length	message_id	length of room name	room_name	length of message		message
	uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + room_name.size() +  sizeof(int) + (int)strlen(sendbuf);// + /*5*/3/* * sizeof(" ")*/;
	
	//Header
	ss << packet_length << " ";		//Packet Length
	ss << 1 << " ";					//message_id
	
	//Send Message
	ss << room_name.size() << " ";		//room_name length
	ss << room_name << " ";				//room_name

	ss << (int)strlen(sendbuf) << " ";			//length of message
	ss << sendbuf;

	// Step #4 Send the message to the server
	result = send(connectSocket, ss.str().c_str(), packet_length, 0);

	if (result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return false;
	}
} //end of serialize and send message


bool SerializeAndJoinRoom(SOCKET connectSocket, std::string room_name)
{
	int result;
	std::stringstream ss;

	uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + room_name.size();

	//Header
	ss << packet_length << " ";
	ss << 3 << " ";

	//Join room
	ss << room_name.size() << " ";
	ss << room_name;

	// Step #4 Send the message to the server
	result = send(connectSocket, ss.str().c_str(), packet_length, 0);
	//result = send(connectSocket, (char*)buffer.readUInt32BE(0), packet_length, 0);

	if (result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return false;
	}
} //end of serialize and join room

bool SerializeAndLeaveRoom(SOCKET connectSocket, std::string room_name)
{
	int result;
	std::stringstream ss;
	uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + room_name.size();

	//Header
	ss << packet_length << " ";
	ss << 4 << " ";

	//Join room
	ss << room_name.size() << " ";
	ss << room_name;

	// Step #4 Send the message to the server
	result = send(connectSocket, ss.str().c_str(), packet_length, 0);

	if (result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return false;
	}
} //end of serialize and leave room

bool SerializeAndSendName(SOCKET connectSocket, std::string name)
{
	int result;
	
	std::stringstream ss;
	uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + name.size();// + /*5*/3/* * sizeof(" ")*/;
	ss << packet_length << " ";		//Packet Length
	ss << 0 << " ";					//message_id

	//Send Client Name
	ss << name.size() << " ";
	ss << name;

	// Step #4 Send the message to the server
	result = send(connectSocket, ss.str().c_str(), packet_length, 0);

	if (result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return false;
	}
} //end of serialize and send name

//uint32_t ProtocolHeader(int packet_length, int message_id)
//{
//	return 0;
//}
//uint32_t ProtocolSendMessage(uint32_t header, int rnLength, std::string room_name, int mLength, char* message)
//{
//	return 0;
//}
//uint32_t ProtocolReceiveMessage(uint32_t header, int nLength, std::string name, int mLenght, char* message, int rnLength, std::string room_name)
//{
//	return 0;
//}
//uint32_t ProtocolJoinRoom(uint32_t header, int rnLength)	
//{
//	return 0;
//}
//uint32_t ProtocolLeaveRoom(uint32_t header, int rnLength)
//{
//	return 0;
//}
#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 16
#define DEFAULT_PORT "27020"

// Client structure
struct ClientInfo {
	SOCKET socket;

	// Buffer information (this is basically you buffer class)
	WSABUF dataBuf;
	char buffer[DEFAULT_BUFLEN];
	int bytesRECV;

	std::string name;
	std::vector<std::string> room_name;
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

}

int main(int argc, char** argv)
{
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	// #1 Socket
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	// Create a SOCKET for connecting to the server
	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		// -1 -> Actual Error Code
		// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	// #2 Bind - Setup the TCP listening socket
	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	// We don't need this anymore
	freeaddrinfo(addrResult);

	// #3 Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}

	// Change the socket mode on the listening socket from blocking to
	// non-blocking so the application will not block waiting for requests
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	DWORD flags;
	DWORD RecvBytes;
	DWORD SentBytes;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		// Initialize our read set
		FD_ZERO(&ReadSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &ReadSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		// Call our select function to find the sockets that
		// require our attention
		//printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			//printf("select() is successful!\n");
		}

		// #4 Check for arriving connections on the listening socket
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");

					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					printf("New client connected on socket %d\n", (int)acceptSocket);
				}
			}
		}

		// #5 recv & send
		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;
				client->dataBuf.buf = client->buffer;
				client->dataBuf.len = DEFAULT_BUFLEN;


				DWORD Flags = 0;
				/*iResult = WSARecv(
					client->socket,
					&(client->dataBuf),
					1,
					&RecvBytes,
					&Flags,
					NULL,
					NULL
				);*/

				iResult = recv(client->socket,
					client->dataBuf.buf,
					client->dataBuf.len,
					Flags);



				std::string received(client->dataBuf.buf);
				// buffer.WriteString(received);
				// packetLength = buffer.ReadUInt32LE();

				/*int value = 0;
				value |= client->dataBuf.buf[0] << 24;
				value |= client->dataBuf.buf[1] << 16;
				value |= client->dataBuf.buf[2] << 8;
				value |= client->dataBuf.buf[3];*/

				//printf("The value received is: %d\n", value);

				std::cout << "RECVd: " << received << std::endl;

				if (iResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					printf("WSARecv() is OK!\n");
					if (iResult == 0)						//was RecvBytes
					{
						RemoveClient(i);
					}
					else if (iResult == SOCKET_ERROR)		//was RecvBytes
					{
						printf("recv: There was an error..%d\n", WSAGetLastError());
						continue;
					}
					else
					{
						std::stringstream ss;
						ss << received;

						//Parse message
						//Header
						uint32_t packet_length;
						ss >> packet_length;	// packet_length;
						std::cout << "packet_length: " << packet_length << std::endl;
						int message_id;
						ss >> message_id;	// message_id;
						std::cout << "mesage_id: " << message_id << std::endl;


						if (message_id == 0)
						{
							//client name
							int nameSize;
							ss >> nameSize;
							std::string name;
							ss >> name;
							client->name = name;
							std::cout << "Client name: " << client->name << std::endl;
						}
						else if (message_id == 1)//message_id == 1)
						{
							//Send Message
							int length_room_name;
							ss >> length_room_name;
							std::string room_name;
							ss >> room_name;

							int messageReceivedLength;
							ss >> messageReceivedLength;	//messageReceivedLength;
							std::cout << "messageReceivedLength: " << messageReceivedLength << std::endl;

							std::string messageReceived = "";
							std::string nextToken;
							while (ss >> nextToken)
							{
								messageReceived.append(nextToken);
								messageReceived.append(" ");
							}

							//std::string fullMessage = "";
							//int temp = messageReceivedLength;
							//ss.get();
							/*for (int i = 0; i < messageReceivedLength; i++)
							{
								messageReceived = ss.get();
								std::cout << "messageReceived: " << messageReceived << std::endl;
								fullMessage.append(messageReceived);
							}*/
							std::cout << "Full Message: " << messageReceived << std::endl;

							ss.str("");

							//client->dataBuf.buf = (char*)fullMessage.c_str();
							//client->dataBuf.len = fullMessage.size();
							
							//Now send the message to peers
							
							// RecvBytes > 0, we got data
							/*iResult = WSASend(
								client->socket,
								&(client->dataBuf),
								1,
								&SentBytes,
								Flags,
								NULL,
								NULL
							);*/

							//TODO - modify to go through clients and send the full packet message for receive message protocol

							for (unsigned int x = 0; x < TotalClients; x++)
							{
								ClientInfo* tempClient = ClientArray[x];

								for (unsigned int y = 0; y < tempClient->room_name.size(); y++)
								{
									if (room_name == tempClient->room_name[y])
									{

										std::stringstream outMes;
										//std::string message = client->name + " has left room: " + room_name;
										//						packet_length		message_id	  length of name		name		length of message	message		length of room_name	room_name
										uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + client->name.size() + sizeof(int) + messageReceived.size() + sizeof(int) + room_name.size();
										outMes << packet_length << " ";
										outMes << 2 << " ";
										outMes << client->name.length() << " ";
										outMes << client->name << " ";
										outMes << messageReceived.size() << " ";
										outMes << messageReceived << " ";
										outMes << room_name.size() << " ";
										outMes << room_name;

										tempClient->buffer[0] = (char)outMes.str().c_str();
										tempClient->dataBuf.buf = tempClient->buffer;
										tempClient->dataBuf.len = outMes.str().size();

										/*tempClient->dataBuf.buf = (char*)messageReceived.c_str();
										tempClient->dataBuf.len = messageReceived.size();*/

										iResult = send(tempClient->socket,
											tempClient->dataBuf.buf,
											tempClient->dataBuf.len,
											Flags);

										if (iResult == SOCKET_ERROR)		//was SentBytes
										{
											printf("send error %d\n", WSAGetLastError());
										}
										else if (iResult == 0)			//was SentBytes
										{
											printf("Send result is 0\n");
										}
										else
										{
											printf("Successfully sent %d bytes!\n", iResult);
										}
									} //end of if
								} //end of for y
							} //end of for x
						} //end of message_id == 1
						else if (message_id == 2)//message_id == 2)
						{
							//Receive Message?
							//Receive message protocol is for clients receiving messages from the server (which are usually broadcasts by the server)
						}
						else if (message_id == 3)//message_id == 3)
						{
							//Join Room
							int roomNameLength;
							ss >> roomNameLength;
							std::cout << "roomNameLength: " << roomNameLength << std::endl;

							std::string room_name;
							ss >> room_name;
							std::cout << "room_name: " << room_name << std::endl;

							client->room_name.push_back(room_name);
							//testing
							for (unsigned int x = 0; x < client->room_name.size(); x++)
							{
								std::cout << "Room Name " << x + 1 << ": " << client->room_name[x] << std::endl;
							}

							for (unsigned int x = 0; x < TotalClients; x++)
							{
								//if (x != i)
								//{
								ClientInfo* tempClient = ClientArray[x];

									//std::stringstream js;
									//js << /*client->name << */"Bob has joined room: " << room_name;
								//std::string outMes = "Bob has joined room";
								//std::cout << "outMes" << outMes << std::endl;
								std::stringstream outMes;
								std::string message = client->name + " has joined room: " + room_name;
								//						packet_length		message_id	  length of name		name		length of message	message		length of room_name	room_name
								uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + client->name.size() + sizeof(int) + message.size() + sizeof(int) + room_name.size();
								outMes << packet_length << " ";
								outMes << 2 << " ";
								outMes << client->name.length() << " ";
								outMes << client->name << " ";
								outMes << message.size() << " ";
								outMes << message << " ";
								outMes << room_name.size() << " ";
								outMes << room_name;

								tempClient->buffer[0] = (char)outMes.str().c_str();
								tempClient->dataBuf.buf = tempClient->buffer;
								tempClient->dataBuf.len = outMes.str().size();

								/*iResult = WSASend(
									tempClient->socket,
									&(tempClient->dataBuf),
									1,
									&SentBytes,
									Flags,
									NULL,
									NULL
								);*/
								iResult = send(tempClient->socket,
									tempClient->dataBuf.buf,
									tempClient->dataBuf.len,
									Flags);

									/*iResult = send(tempClient->socket,
										tempClient->dataBuf.buf,
										tempClient->dataBuf.len,
										Flags);*/

									// Example using send instead of WSASend...
									//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

								if (iResult == SOCKET_ERROR)			//was SentBytes
								{
									printf("send error %d\n", WSAGetLastError());
								}
								else if (iResult == 0)				//was SentBytes
								{
									printf("Send result is 0\n");
								}
								else
								{
									printf("Successfully sent %d bytes!\n", iResult);
								}
								//}
							}

						}
						else if (message_id == 4)//message_id == 4)
						{
							//Leave Room
							int roomNameLength;
							ss >> roomNameLength;
							std::cout << "roomNameLength: " << roomNameLength << std::endl;

							std::string room_name;
							ss >> room_name;
							std::cout << "room_name: " << room_name << std::endl;

							for (unsigned int x = 0; x < client->room_name.size(); x++)
							{
								if (client->room_name[x] == room_name)
								{
									std::cout << "Erasing " << x + 1 << ": " << client->room_name[x] << std::endl;
									client->room_name.erase(client->room_name.begin() + x);
								}
							}

							for (unsigned int x = 0; x < TotalClients; x++)
							{
								//if (x != i)
								//{
								ClientInfo* tempClient = ClientArray[x];

								//std::stringstream js;
								//js << client->name << " has left room: " << room_name;
								std::stringstream outMes;
								std::string message = client->name + " has left room: " + room_name;
								//						packet_length		message_id	  length of name		name		length of message	message		length of room_name	room_name
								uint32_t packet_length = sizeof(uint32_t) + sizeof(int) + sizeof(int) + client->name.size() + sizeof(int) + message.size() + sizeof(int) + room_name.size();
								outMes << packet_length << " ";
								outMes << 2 << " ";
								outMes << client->name.length() << " ";
								outMes << client->name << " ";
								outMes << message.size() << " ";
								outMes << message << " ";
								outMes << room_name.size() << " ";
								outMes << room_name;
									
								tempClient->buffer[0] = (char)outMes.str().c_str();
								tempClient->dataBuf.buf = tempClient->buffer;
								tempClient->dataBuf.len = outMes.str().size();

									/*iResult = WSASend(
										tempClient->socket,
										&(tempClient->dataBuf),
										1,
										&SentBytes,
										Flags,
										NULL,
										NULL
									);*/

								iResult = send(tempClient->socket,
									tempClient->dataBuf.buf,
									tempClient->dataBuf.len,
									Flags);

									// Example using send instead of WSASend...
									//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

								if (iResult == SOCKET_ERROR)		//was SentBytes
								{
									printf("send error %d\n", WSAGetLastError());
								}
								else if (iResult == 0)			//was SentBytes
								{
									printf("Send result is 0\n");
								}
								else
								{
									printf("Successfully sent %d bytes!\n", iResult);
								}
								//}
							}
						}

						ss.str("");

					} //else received ok
				} //else no error
			} //if read
			
		} //end of for totalClients
		for (unsigned int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];
			client->dataBuf.buf = (char*)"";
			client->dataBuf.len = 0;
		}

	} //end of while(true)



	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;

} //end of main
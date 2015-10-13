//-----------------------------------------------------------
//-----------------------------------------------------------
//
//             System: IQFeed
//       Program Name: ConsoleBenchmarkTool_VC.exe
//        Module Name: ConsoleBenchmarkTool.cpp
//
//-----------------------------------------------------------
//
//            Proprietary Software Product
//
//           Data Transmission Network Inc.
//           9110 West Dodge Road Suite 200
//               Omaha, Nebraska  68114
//
//    Copyright (c) by Data Transmission Network 2010
//                 All Rights Reserved
//
//
//-----------------------------------------------------------
// Module Description: Implementation of Level 1 Streaming Quotes
//         References: None
//           Compiler: Microsoft Visual C++ Version 2005
//             Author: Steven Schumacher
//        Modified By: 
//
//-----------------------------------------------------------
//-----------------------------------------------------------
#include "stdafx.h"
#include <stdio.h>

#include <string>
#include <fstream>
#include <time.h>
#include <winsock2.h>
#include <Windows.h>
#include <iostream>



bool bIQFeedIsReady(false);
const int iSckBufferSize(1048576);
char sckBuffer[iSckBufferSize];
std::string commandArray[10000];
int iNumCommands(0);

//-----------------------------------------------------------
//
//  int _tmain(int argc, _TCHAR* argv[])	 
//
/**

    Main

    @return void
*/
//  Globals:  
//
//  Notes:  
//
//-----------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	//using time
	SYSTEMTIME sys_time;
	FILE * pFile;
	pFile = fopen ("myfile.csv","a");

// Initialize Winsock
	printf("Initializing winsock...\n");
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR)
	{
		printf("Error at WSAStartup()...\n");
	}

	SetConsoleTitle("C++ Console Benchmark Tool");

	// Launch IQFeed
	printf("Launching IQConnect...\n");
	SHELLEXECUTEINFO lpExecInfo; 
	memset(&lpExecInfo, 0, sizeof(SHELLEXECUTEINFO));
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO); 
	lpExecInfo.lpFile = "IQConnect.exe"; 
	lpExecInfo.lpParameters = "-product IQFEED_DEMO -version 1"; 
	lpExecInfo.lpDirectory = NULL; 
	lpExecInfo.lpVerb = NULL; 
	lpExecInfo.nShow = SW_SHOWNORMAL; 
	lpExecInfo.fMask = 0; 

	if (!ShellExecuteEx(&lpExecInfo))
	{
		printf("Unable to launch IQConnect.exe.  Error Code: %d hInst: %p\n", GetLastError(), lpExecInfo.hInstApp);
	}
	else
	{
		// next we connect to the admin port (9300) to get the status of IQFeed and/or wait until the
		// user hits the connect button.
		SOCKET sockAdmin = NULL;
		sockAdmin = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (sockAdmin != INVALID_SOCKET)
		{
			printf("Admin socket created...\n");

			// set the address to 127.0.0.1:9300
			sockaddr_in addrAdmin;
			addrAdmin.sin_family = AF_INET;
			// IQFeed uses port 9300 by default for the Admin connection
			unsigned short usIQFeedAdminPort(9300);
			addrAdmin.sin_port = htons(usIQFeedAdminPort);
			// All connections to IQFeed MUST originate from the same machine and connect to 127.0.0.1
			addrAdmin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			memset(&addrAdmin.sin_zero, 0, sizeof(addrAdmin.sin_zero));

			// Connect the socket
			if (connect(sockAdmin, (sockaddr*)&addrAdmin, sizeof(addrAdmin)) == 0) 
			{
				// once we connect, if IQConnect was launched by this app, we just monitor the S,STATS messages
				// until one comes in indicating we are now connected to the feed.  If IQConnect was already running,
				// we want to send a S,REGISTER CLIENT APP command to make sure our app gets registered first.
				printf("Admin socket connected...\n");
				int nRead(0);
				memset(sckBuffer, 0, sizeof(sckBuffer));
				printf("Start admin Socket traffic\n--------------------------\n");
				while (((nRead = recv(sockAdmin, sckBuffer, sizeof(sckBuffer) - 1, 0)) != 0) && !bIQFeedIsReady)
				{
					printf("%s\n", sckBuffer);
					char * pStrFoundAt = strstr(sckBuffer, ",Connected,");
					if (pStrFoundAt != NULL)
					{
						printf("IQFeed Connected...\n");
						bIQFeedIsReady = true;
					}
					else
					{
						// this is only here to provide feedback to the user as to what the app is doing
						// while we wait for IQConnect to connect to the server.
						pStrFoundAt = strstr(sckBuffer, ",Not Connected,");
						if (pStrFoundAt != NULL)
						{
							printf("Not Connected.  Waiting for IQConnect...\n");
						}
					}
					memset(sckBuffer, 0, sizeof(sckBuffer));
				}
				printf("------------------------\nEnd admin socket traffic\n");
				
				// At this point we know that IQConnect is connected and running.  If this were a production application
				// we would continue to monitor this connection for changes in the status of the feed.  But for this application,
				// we just wanted to verify the feed was running so we will go ahead and cleanup the socket.
			}
			else
			{
				int iErr = GetLastError();
				printf("Unable to connect to Admin Port. Error: %d\n", iErr);
			}
			printf("Closing Admin Socket...\n");
			closesocket(sockAdmin);
		}
		else
		{
			int iErr = GetLastError();
			printf("Unable to create Admin socket. Error: %d\n", iErr);
		}

		// IQFeed is connected.  Lets read in our commands file that was supplied in the command line arguments.
		std::string strFile("");
        if (argc > 1)
        {
            strFile = argv[1];
		}else{
			strFile = "ConsoleCommands.txt";
		}
		printf("Start reading commands from file %s\n", strFile.c_str());

        if (strFile.length() > 0)
        {
            // read commands in from the file
            int iCounter(0);
			std::string strLine("");

            // Read the file and load it into the array.
			std::ifstream fsCommandFile(strFile.c_str());
			char szBuf[256];
			GetModuleFileName(NULL, szBuf, 256);
			if (fsCommandFile.is_open())
			{
				while (!fsCommandFile.eof())
				{
					getline(fsCommandFile, strLine);
					// load each line into an array
					if (strLine.length() > 0)
					{
						commandArray[iNumCommands] = strLine + "\r\n";
						iNumCommands++;
					}
				}
				fsCommandFile.close();
			}
			else 
			{
				printf("Unable to open file %s...\n", strFile.c_str());
			}
        }
        printf("Done reading commands from file %s...\n", strFile.c_str());

		// Now we will connect to the level 1 port and make our requests to the feed.
		// Start by creating a new socket
		SOCKET sockLevel1 = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (sockLevel1 != INVALID_SOCKET) 
		{
			struct sockaddr_in addrLevel1;

			// set the address to 127.0.0.1:5009
			addrLevel1.sin_family = AF_INET;
			// IQFeed uses port 5009 by default for the Level1 connection
			unsigned short usIQFeedLevel1Port(5009);
			addrLevel1.sin_port = htons(usIQFeedLevel1Port);
			// All connections to IQFeed MUST originate from the same machine and connect to 127.0.0.1
			addrLevel1.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			memset(&addrLevel1.sin_zero, 0, sizeof(addrLevel1.sin_zero));

			// connect to the server
			if (connect(sockLevel1, (struct sockaddr*)&addrLevel1, sizeof(addrLevel1))==0)
			{
				// loop through our list of commands and send each one to IQFeed.
				int iCurrentCommand(0);
				for (iCurrentCommand = 0; iCurrentCommand < iNumCommands; iCurrentCommand++)
				{
					int iBytesToSend((int)commandArray[iCurrentCommand].length());
					int iBytesSent = send(sockLevel1, commandArray[iCurrentCommand].c_str(), iBytesToSend, 0);
					if (iBytesToSend == iBytesSent)
					{
						printf("Sent command: %s to IQFeed...\n", commandArray[iCurrentCommand].substr(0, commandArray[iCurrentCommand].length() - 2).c_str());
					}
					else
					{
						printf("Unable to send command: %s to IQFeed...\n", commandArray[iCurrentCommand].substr(0, commandArray[iCurrentCommand].length() - 2).c_str());
					}
				}

				// start processing data received from IQConnect.
				// variables for data processing
				int iBytesReceived(0);
				int iTotalBytesParsed(0);
				int iBytesLeftover(0);
				int iBytesToParse(0);
				unsigned char ucNewLine(10);

				// variables for stats tracking
				int iFMessages(0);
				int iPMessages(0);
				int iQMessages(0);
				int iTMessages(0);
				int iSMessages(0);
				int iNMessages(0);
				int iRMessages(0);
				int iMsgsLastSecond(0);
				__int64 i64TotalMsgs(0);
				__int64 i64TotalMsgsPerSecond(0);

				time_t tStart = time(NULL);
				time_t tNow = tStart;
				__int64 i64Seconds(0);

				char * pCurrentMsg = sckBuffer;
				char * pNextMsg = sckBuffer;
				char * lastRowMsg = sckBuffer;
				// here is where all the message processing takes place.  Continue processing as long as we dont get a socket error.
				while ((iBytesReceived = recv(sockLevel1, pCurrentMsg, sizeof(sckBuffer) - iBytesLeftover - 1, 0)) != SOCKET_ERROR)
				{
					//System Time
				//	GetLocalTime( &sys_time );
					// unlike with the admin port connection and with the other example apps,
					// with this connection, we ARE worried about efficiency of data processing.
					// Additionally, want to make sure this app is using the least amount of CPU possible.
					// As a result, we are not converting data to strings for processing.
					// Since we only want to know what type of message was received so we only need to
					// look at the starting character of each message and search for the delimiting character.

					// calculate our number of bytes to process
					iBytesToParse = iBytesReceived;
					iBytesToParse += iBytesLeftover;
					iBytesLeftover = 0;
					iTotalBytesParsed += iBytesReceived;
					
					// move our pointer back to the beginning of the buffer
					pCurrentMsg = sckBuffer;
					//char newmsg[60] = "";

					while ((pNextMsg = strchr(pCurrentMsg, ucNewLine)) != NULL)
					{
						// we only get in here if we have another complete message to process
						i64TotalMsgs++;
						iMsgsLastSecond++;

						char *newmsg;
						newmsg = new char[pNextMsg -pCurrentMsg+1]();
						

						switch (pCurrentMsg[0])
						{
							case 81: // Q
								// Update Message
								iQMessages++;
							//	lastRowMsg = strrchr(pCurrentMsg,'Q');
							//	pNextMsg = strchr(lastRowMsg, ucNewLine); //end char
								
								
								
								memcpy(newmsg,pCurrentMsg,pNextMsg -pCurrentMsg);
								printf("%s\n", newmsg);

								//std::cout<<newmsg<<std::endl;
							
								
								//printf("%s", pCurrentMsg);
								fprintf (pFile, "%s\n", newmsg);
							//	std::cout << sys_time.wHour << ":"<< sys_time.wMinute << ":"<<sys_time.wSecond << "."<< sys_time.wMilliseconds <<"," << pCurrentMsg;
							//	printf("Next Message %s", pNextMsg);
								break;
							case 84: // T
								// Timestamp Message
								iTMessages++;
								// we trigger our output to the screen when we receive a timestamp message.

								// update our timing variables
								time(&tNow);

								//System Time
								GetLocalTime( &sys_time );

							//	struct tm *timeinfo;
							//	struct tm ts;
							//	char       buf[80];

								// skip the first writeout if a complete second hasn't elapsed to prevent division by zero
								i64Seconds = tNow;
								i64Seconds -= tStart;
								if (i64Seconds > 0)
								{
									i64TotalMsgsPerSecond = i64TotalMsgs;
									i64TotalMsgsPerSecond /= i64Seconds;
									//printf("F:%d\tP:%d\tQ:%d\tT:%d\tS:%d\tN:%d\tR:%d\tLM:%d\tTMS:%I64d\tTIME:%d\n", iFMessages, iPMessages, iQMessages, iTMessages, iSMessages, iNMessages, iRMessages, iMsgsLastSecond, i64TotalMsgsPerSecond, tNow);
								/*	
									struct tm  ts;
									char       buf[80];
									// Get current time
									//time(&now);
									// Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
									ts = *localtime(&tNow);
									strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
									printf("%s\n", buf);
								*/

									//time (&rawtime);
								//	timeinfo = localtime (&tNow);
								//	ts = *localtime (&tNow);
									//printf ("Current local time and date: %s", asctime(timeinfo));
									memcpy(newmsg,pCurrentMsg,pNextMsg - pCurrentMsg);
									printf("%s Local Time %d:%d.%d\n", newmsg,sys_time.wMinute,sys_time.wSecond,sys_time.wMilliseconds);

									fprintf(pFile, "%s Local Time %d:%d.%d\n", newmsg,sys_time.wMinute,sys_time.wSecond,sys_time.wMilliseconds);
			
									//std::cout<<newmsg<<std::endl;

								//	 strftime(buf, sizeof(buf), "%M:%S.%Z", &ts);
								//	 printf("%s Local Time %s\n", newmsg,buf);
									//printf("%s", pCurrentMsg);
							//		fprintf (pFile, "%s Local Time %s \n", newmsg,asctime(timeinfo));
									// reset our "Messages in the last second" counter
									iMsgsLastSecond = 0;
								}
								break;
							case 70: // F
								// Fundamental Message
								iFMessages++;
								break;
							case 80: // P
								// Summary Message
								iPMessages++;
								break;
							case 82: // R
								// Regional Message
								iRMessages++;
								break;
							case 78: // N
								// News Message
								iNMessages++;
								break;
							case 83: // S
								// System Message
								iSMessages++;
								break;
						}
						// update our pointer to the next msg
						pCurrentMsg = pNextMsg;
						pCurrentMsg++;

						delete newmsg;
						newmsg = NULL;

					}
					// now we need to check for an incomplete message and copy it to the beginning of the buffer for the next read from the socket
					if (iTotalBytesParsed < iBytesToParse)
					{
						// we have an incomplete message
						iBytesLeftover = iBytesToParse;
						iBytesLeftover -= iTotalBytesParsed;
						// copy the left over bytes to the front of the buffer
						memcpy_s(sckBuffer, iSckBufferSize, pCurrentMsg, iBytesLeftover);
						// reset our pointer to the current location in the message buffer for the next read from the socket
						pCurrentMsg = sckBuffer;
						pCurrentMsg += iBytesLeftover;
					}
					else
					{
						pCurrentMsg = sckBuffer;
					}
					// zero out any of the buffer that was used in the read starting at the end of what we copied to the front
					memset(pCurrentMsg, 0, iSckBufferSize - iBytesLeftover);
				}
			} 
			else 
			{
				int iErr = GetLastError();
				printf("Unable to connect to Level 1 socket. Error: %d", iErr);
			}
			// Close the socket
			closesocket(sockLevel1);
			 fclose (pFile);
		}
		else
		{
			int iErr = GetLastError();
			printf("Unable to create Level 1 socket. Error: %d", iErr);
		}
	}

	return 0;
}


#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

std::string boardLocation;
int processIndex;
bool generationEnded = false;

DWORD WINAPI MonitorServer(LPVOID lpParam)
{
	SOCKET clientSocket = *(SOCKET*)lpParam;
	char buffer[1024];
	int bytesReceived;

	while (true)
	{
		bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0)
		{
			break;
		}

		buffer[bytesReceived] = '\0';
		std::string message(buffer);

		if (message == "GenerationEnded") {
			generationEnded = true;
			break;
		}
	}
	return 0;
}

void SendIdea(SOCKET clientSocket, std::string idea)
{
	std::ofstream outFile(boardLocation, std::ios::app);
	outFile << "Process " << processIndex + 1 << " idea: " << idea << std::endl;
	outFile.close();

	const std::string completedMessage = "FileProcessed";
	send(clientSocket, completedMessage.c_str(), completedMessage.length(), 0);

	std::cout << "Idea: " << idea << std::endl;
}

int PrintIdeas(SOCKET clientSocket)
{
	std::ifstream input(boardLocation);

	if (!input)
		std::cerr << "Error opening file for reading!" << std::endl;

	std::cout << "\nFinal board with ideas:" << std::endl;

	std::string line;
	int counter = 1;

	while (std::getline(input, line))
	{
		std::cout << counter << ". " << line << std::endl;
		counter++;
	}

	input.close();
	counter--;

	const std::string completedMessage = "FileRead";
	send(clientSocket, completedMessage.c_str(), completedMessage.length(), 0);

	return counter;
}

void Vote(SOCKET clientSocket, int ideasNum)
{
	if (ideasNum < 3)
	{
		std::string msg = "NotEnoughIdeas";
		send(clientSocket, msg.c_str(), msg.length(), 0);
		std::cout << "There aren't enough ideas to vote" << std::endl;
		return;
	}

	int vote1, vote2, vote3;
	while (true) {
		std::cout << "Enter three best ideas: ";
		std::cin >> vote1 >> vote2 >> vote3;

		if (vote1 >= 1 && vote1 <= ideasNum && vote2 >= 1 && vote2 <= ideasNum && vote3 >= 1 && vote3 <= ideasNum
			&& vote1 != vote2 && vote1 != vote3 && vote2 != vote3)
			break;
		else
			std::cout << "Error. Choose 1 - " << ideasNum << " with no repeats." << std::endl;
	}

	std::string voteMessage = "Vote " + std::to_string(vote1) + " " + std::to_string(vote2) + " " + std::to_string(vote3);
	send(clientSocket, voteMessage.c_str(), voteMessage.length(), 0);
}


void HandleServerConnection(SOCKET clientSocket)
{
	char buffer[1024];
	int bytesReceived;

	while (true)
	{
		std::string idea;
		HANDLE hServerMonitor = CreateThread(NULL, 0, MonitorServer, &clientSocket, 0, NULL);

		std::cout << "Enter the idea: ";
		std::getline(std::cin, idea);

		while (true)
		{
			if (idea == "")
			{
				std::cout << "Enter the correct idea: ";
				std::getline(std::cin, idea);
			}
			else
				break;
		}

		if (generationEnded)
		{
			CloseHandle(hServerMonitor);
			std::cout << "Your last idea wasn't written to the board. The server closed the generation";
			int ideas_num = PrintIdeas(clientSocket);
			Vote(clientSocket, ideas_num);
			break;
		}

		TerminateThread(hServerMonitor, 0);
		CloseHandle(hServerMonitor);

		const std::string requestMessage = "RequestFile";
		send(clientSocket, requestMessage.c_str(), requestMessage.length(), 0);

		bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0)
		{
			break;
		}

		buffer[bytesReceived] = '\0';
		std::string message(buffer);

		if (message == "PermissionGranted")
		{
			SendIdea(clientSocket, idea);
		}
		else if (message == "GenerationEnded")
		{
			const std::string completedMessage = "FileProcessed";
			send(clientSocket, completedMessage.c_str(), completedMessage.length(), 0);
			std::cout << "Your last idea wasn't written to the board. The server closed the generation";
			int ideas_num = PrintIdeas(clientSocket);
			Vote(clientSocket, ideas_num);
			break;
		}
	}

	closesocket(clientSocket);
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cerr << "Incorrect arguments" << std::endl;
		return 1;
	}

	boardLocation = argv[1];
	processIndex = std::stoi(argv[2]);

	WSADATA wsaData;
	SOCKET clientSocket;
	struct sockaddr_in serverAddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed!" << std::endl;
		return 1;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed!" << std::endl;
		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(13000);

	if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
		std::cerr << "Invalid address or Address not supported!" << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Connection failed!" << std::endl;
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	HandleServerConnection(clientSocket);

	WSACleanup();

	Sleep(4000);

	return 0;
}
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

#define MAX_IDEAS 50

std::string boardLocation;
int processIndex;

std::string ideas[MAX_IDEAS] = {
    "AI", "Blockchain", "Quantum Computing", "Cloud", "Big Data", "IoT",
    "5G", "Cybersecurity", "Robotics", "Augmented Reality", "Virtual Reality",
    "Machine Learning", "Neural Networks", "Deep Learning", "Data Analytics",
    "Autonomous Vehicles", "Sustainability", "Space Exploration",
    "Healthcare Tech", "E-commerce", "Smart Cities", "Fintech",
    "Digital Twins", "3D Printing", "Bioinformatics", "Nanotechnology",
    "Clean Energy", "Gaming", "Metaverse", "Education Tech", "AgriTech",
    "Edge Computing", "Wearables", "Smart Homes", "Personal AI Assistants",
    "Voice Recognition", "Cryptocurrency", "AR Gaming", "FoodTech",
    "Digital Marketing", "Social Media Analytics", "Cyber Threat Intelligence",
    "Green Tech", "Biometrics", "Renewable Resources", "AR Advertising",
    "AI in Education", "AI in Medicine", "AI in Finance", "AI Ethics"
};

void HandleServerConnection(SOCKET clientSocket)
{
    char buffer[1024];
    int bytesReceived;

    while (true)
    {
        const std::string requestMessage = "RequestFile";
        send(clientSocket, requestMessage.c_str(), requestMessage.length(), 0);

        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string message(buffer);

        if (message == "PermissionGranted")
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            srand(static_cast<unsigned>(time(0)) + GetCurrentProcessId());
            int ideaIndex = rand() % MAX_IDEAS;

            std::ofstream outFile(boardLocation, std::ios::app);
            outFile << "Process " << processIndex + 1 << " idea: " << ideas[ideaIndex] << std::endl;
            outFile.close();

            const std::string completedMessage = "FileProcessed";
            send(clientSocket, completedMessage.c_str(), completedMessage.length(), 0);

            std::cout << "Idea: " << ideas[ideaIndex] << std::endl;
        }
        else if (message == "VoteEnded")
        {
            int vote1, vote2, vote3;
            while (true) {
                std::cout << "Enter three best ideas: ";
                std::cin >> vote1 >> vote2 >> vote3;

                if (vote1 >= 1 && vote1 <= 50 && vote2 >= 1 && vote2 <= 50 && vote3 >= 1 && vote3 <= 50) {
                    break;
                }
                else {
                    std::cout << "Error. Choose 1 - 50" << std::endl;
                }
            }

            std::string voteMessage = "Vote " + std::to_string(vote1) + " " + std::to_string(vote2) + " " + std::to_string(vote3);
            send(clientSocket, voteMessage.c_str(), voteMessage.length(), 0);
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
    return 0;
}
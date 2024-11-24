using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System;
using System.Threading;
using System.Collections.Generic;

class Program
{
	const string PROCESS_LOCATION = "D:\\Example\\Process\\Console3D.exe";
	const string BOARD_LOCATION = "D:\\Example\\board.txt";
	const int PROCESS_COUNT = 4;
	const int MILLISECONDS_TIME = 30000; //180000

	static TcpListener server;
	static TcpClient[] clients = new TcpClient[PROCESS_COUNT];

	static Thread[] clientThreads = new Thread[PROCESS_COUNT];

	static Queue<TcpClient> clientsQueue = new Queue<TcpClient>();
	static bool isWaitingForEnd = false;

	static bool isEnd = false;

	public static void Main(string[] args)
	{
		FileStream board = File.Create(BOARD_LOCATION);
		board.Close();

		CreateServer();

		for(int i = 0; i < PROCESS_COUNT; i++)
		{
			ProcessStartInfo startInfo = new ProcessStartInfo
			{
				FileName = PROCESS_LOCATION,
				Arguments = BOARD_LOCATION + " " + i.ToString(),
				UseShellExecute = true
			};
			Process process = Process.Start(startInfo);
			clients[i] = server.AcceptTcpClient();
			int temp = i;
			(clientThreads[i] = new Thread(() => HandleClient(temp))).Start();
		}

		(new Thread(HandleQueue)).Start();

		Thread.Sleep(MILLISECONDS_TIME);

		byte[] buffer = System.Text.Encoding.ASCII.GetBytes("GenerationEnded");
		for (int i = 0; i < PROCESS_COUNT; i++)
		{
			clients[i].GetStream().Write(buffer, 0, buffer.Length);
		}

		isEnd = true;

		//Console.ReadKey();
	}

	static void CreateServer()
	{
		server = new TcpListener(IPAddress.Parse("127.0.0.1"), 13000);
		server.Start();
	}

	static void HandleQueue()
	{
		byte[] buffer = System.Text.Encoding.ASCII.GetBytes("PermissionGranted");
		int currentCount = 0;
		while (!isEnd)
		{
			if (!isWaitingForEnd)
			{
				if (clientsQueue.Count > 0)
				{
					Console.WriteLine("PermissionGranted");
					clientsQueue.Peek().GetStream().Write(buffer, 0, buffer.Length);
					currentCount = clientsQueue.Count;
					isWaitingForEnd = true;
				}
			}
		}
	}

	static void HandleClient(int index)
	{
		NetworkStream clientStream = clients[index].GetStream();

		while (true)
		{
			byte[] buffer = new byte[1024];
			int i = clientStream.Read(buffer, 0, buffer.Length);
			if (i != 0)
			{
				string data = System.Text.Encoding.ASCII.GetString(buffer, 0, i);
				Console.WriteLine(data);
				if (data == "RequestFile")
				{
					clientsQueue.Enqueue(clients[index]);
				}
				else if (data == "FileProcessed")
				{
					clientsQueue.Dequeue();
					isWaitingForEnd = false;
				}
			}
		}
	}
}
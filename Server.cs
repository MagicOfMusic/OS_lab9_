using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System;
using System.Threading;
using System.Collections.Generic;
using System.Linq;

class Server
{
	const string PROCESS_LOCATION = "D:\\MyProjects\\Моє\\c++\\ProjectWithTests\\delete2\\x64\\Debug\\delete2.exe";
	const string BOARD_LOCATION = "D:\\Example\\board.txt";
	const int PROCESS_COUNT = 4;
	const int MILLISECONDS_TIME = 30000; //180000

	static TcpListener server;
	static TcpClient[] clients = new TcpClient[PROCESS_COUNT];

	static Thread[] clientThreads = new Thread[PROCESS_COUNT];

	static Queue<TcpClient> clientsQueue = new Queue<TcpClient>();
	static Thread queueHandlingThread;
	static bool isWaitingForEnd = false;

	static bool isEnd = false;

	public static void Main(string[] args)
	{
		FileStream board = File.Create(BOARD_LOCATION);
		board.Close();

		CreateServer();

		CreateProcesses();

		Thread.Sleep(MILLISECONDS_TIME);

		EndGeneration();

		string[] lines = File.ReadAllLines(BOARD_LOCATION);
		DisplayBoard(lines);

		GetVotes(lines);
	}

	static void CreateServer()
	{
		server = new TcpListener(IPAddress.Parse("127.0.0.1"), 13000);
		server.Start();
	}

	static void CreateProcesses()
	{
		for (int i = 0; i < PROCESS_COUNT; i++)
		{
			ProcessStartInfo startInfo = new ProcessStartInfo
			{
				FileName = PROCESS_LOCATION,
				Arguments = BOARD_LOCATION + " " + i.ToString(),
				UseShellExecute = true
			};
			Process.Start(startInfo);
			clients[i] = server.AcceptTcpClient();
			int temp = i;
			(clientThreads[i] = new Thread(() => HandleClient(temp))).Start();
		}

		(queueHandlingThread = new Thread(HandleQueue)).Start();
	}

	static void EndGeneration()
	{
		Console.WriteLine("GenerationEnded");
		byte[] buffer = System.Text.Encoding.ASCII.GetBytes("GenerationEnded");
		for (int i = 0; i < PROCESS_COUNT; i++)
		{
			clients[i].GetStream().Write(buffer, 0, buffer.Length);
		}

		while (clientsQueue.Count > 0);

		isEnd = true;

		queueHandlingThread.Join();

		for (int i = 0; i < PROCESS_COUNT; i++)
		{
			clientThreads[i].Join();
		}
	}

	static void DisplayBoard(string[] lines)
	{
		for (int i = 0; i < lines.Length; i++)
		{
			Console.WriteLine(lines[i]);
		}
	}

	static void GetVotes(string[] lines)
	{
		File.AppendAllText(BOARD_LOCATION, "----------------------------------------" + Environment.NewLine + "Best 3 ideas:" + Environment.NewLine);

		Dictionary<int, int> allVotes = new Dictionary<int, int>();
		for (int i = 0; i < clients.Length; i++)
		{
			NetworkStream clientStream = clients[i].GetStream();
			byte[] buffer = new byte[1024];
			int bytesRead = clientStream.Read(buffer, 0, buffer.Length);
			if (bytesRead != 0)
			{
				string[] data = System.Text.Encoding.ASCII.GetString(buffer, 0, bytesRead).Split(' ');
				Console.WriteLine(data[0] + " " + data[1] + " " + data[2] + " " + data[3]);
				string command = data[0];
				if (command == "Vote")
				{
					for (int voteIndex = 1; voteIndex < data.Length; voteIndex++)
					{
						int vote = int.Parse(data[voteIndex]);
						if (allVotes.ContainsKey(vote))
						{
							allVotes[vote]++;
						}
						else
						{
							allVotes[vote] = 1;
						}
					}
				}
			}
		}

		Tuple<int, int>[] bestVotes = new Tuple<int, int>[3];
		for(int i = 0; i < bestVotes.Length; i++)
		{
			bestVotes[i] = new Tuple<int, int>(-1, 0);
		}
		int notFilledIndex = 0;
		foreach(KeyValuePair<int, int> votePair in allVotes)
		{
			if (notFilledIndex < bestVotes.Length)
			{
				int insertIndex = notFilledIndex;
				for (int i = 0; i < notFilledIndex; i++)
				{
					if (votePair.Value > bestVotes[i].Item2)
					{
						insertIndex = i;
					}
				}

				if(insertIndex == notFilledIndex)
				{
					bestVotes[notFilledIndex] = new Tuple<int, int>(votePair.Key, votePair.Value);
				}
				else
				{
					for (int i = notFilledIndex; i > insertIndex; i--)
					{
						bestVotes[i] = bestVotes[i-1];
					}

					bestVotes[insertIndex] = new Tuple<int, int>(votePair.Key, votePair.Value);
				}
				notFilledIndex++;
			}
			else
			{
				for (int i = 0; i < bestVotes.Length; i++)
				{
					if (votePair.Value > bestVotes[i].Item2)
					{
						bestVotes[i] = new Tuple<int, int>(votePair.Key, votePair.Value);
					}
				}
			}
		}

		for (int i = 0; i < bestVotes.Length; i++)
		{
			File.AppendAllText(BOARD_LOCATION, lines[bestVotes[i].Item1 - 1] + "   Votes: " + bestVotes[i].Item2 + Environment.NewLine);
		}
	}

	static void HandleQueue()
	{
		byte[] buffer = System.Text.Encoding.ASCII.GetBytes("PermissionGranted");
		while (!isEnd)
		{
			if (!isWaitingForEnd)
			{
				if (clientsQueue.Count > 0)
				{
					Console.WriteLine("PermissionGranted");
					clientsQueue.Peek().GetStream().Write(buffer, 0, buffer.Length);
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
			int bytesRead = clientStream.Read(buffer, 0, buffer.Length);
			if (bytesRead != 0)
			{
				string data = System.Text.Encoding.ASCII.GetString(buffer, 0, bytesRead);
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
				else if(data == "FileRead")
				{
					break;
				}
			}
		}
	}
}

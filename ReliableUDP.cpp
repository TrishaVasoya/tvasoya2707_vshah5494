/*
	Name        : Vrushti Shah (8825494) and Trisha Vasoya (8872707)
	Project Name: ReliableUDP
	Date        : 10th Febuary,2024
	Description : This C++ program demonstrates reliable communication between a client and server, managing flow control and measuring transmission speed.
*/

/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <chrono>


#include "Net.h"
#pragma warning (disable : 4996)



//#define SHOW_ACKS

using namespace std;
using namespace net;
using namespace std::chrono;


const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

// Class for managing flow control
class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}
	// Reset flow control parameters
	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}
	// Update flow control based on round trip time (RTT)
	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;
		// Switch statement to handle flow control based on current mode
		switch (mode)
		{
		case Good:
			// If RTT exceeds threshold, switch to bad mode and adjust penalty time
			if (rtt > RTT_Threshold)
			{
				printf("* dropping to bad mode *\n");
				mode = Bad;
				// Increase penalty time if conditions are good for less than 10 seconds and penalty time is less than 60 seconds
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
			}
			else
			{
				// Increment good conditions time and adjust penalty time if applicable
				good_conditions_time += deltaTime;
				penalty_reduction_accumulator += deltaTime;

				if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
				{
					penalty_time /= 2.0f;
					if (penalty_time < 1.0f)
						penalty_time = 1.0f;
					printf("penalty time reduced to %.1f\n", penalty_time);
					penalty_reduction_accumulator = 0.0f;
				}
			}
			break;

		case Bad:
			// If RTT is below threshold, increment good conditions time
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;
			// If good conditions time exceeds penalty time, switch to good mode
			if (good_conditions_time > penalty_time)
			{
				printf("* upgrading to good mode *\n");
				good_conditions_time = 0.0f;// Reset good conditions time
				penalty_reduction_accumulator = 0.0f;// Reset penalty reduction accumulator
				mode = Good;
			}
			break;
		}
	}

	// Get the send rate based on current mode
	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:
	// Enum for flow control mode
	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};
/*
	Function Name: calculateXORChecksum
	Description: Calculate XOR checksum for a vector of bytes.
	Parameters:
		- const std::vector<uint8_t>& data: Input data for which checksum is calculated.
	Returns:
		- uint16_t: Calculated XOR checksum.
*/
uint16_t calculateXORChecksum(const std::vector<uint8_t>& data) {
	uint16_t checksum = 0;
	for (auto byte : data) {
		checksum ^= byte; // XOR each byte with the checksum
	}
	return checksum;
}

// Main function
int main(int argc, char* argv[])
{
	// parse command line
	static int packetCounter = 0;

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	if (argc >= 2)
	{
		int a, b, c, d;
#pragma warning(suppress : 4996)
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;

	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}

		if (mode == Client) {
			connection.Connect(address);


			// Assuming 'connection' and 'address' are set up as shown in your provided code
			auto start = high_resolution_clock::now();
			// Open the file in binary mode
			std::ifstream file("C:\\Users\\trish\\Desktop\\trisha.txt", std::ios::binary | std::ios::ate);
			if (!file.is_open()) {
				std::cerr << "Failed to open file.\n";
				return 1;
			}

			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);

			// Extract filename from path
			std::string path = "C:\\Users\\trish\\Desktop\\trisha.txt";
			std::string filename = path.substr(path.find_last_of("/\\") + 1);

			// Send filename packet (Type 0x01)
			unsigned char filenamePacket[PacketSize];
			memset(filenamePacket, 0, sizeof(filenamePacket));
			filenamePacket[0] = 0x01; // Packet type for filename
			memcpy(filenamePacket + 1, filename.c_str(), filename.length());
			connection.SendPacket(filenamePacket, sizeof(filenamePacket));

			// Send file size packet (Type 0x02)
			unsigned char fileSizePacket[PacketSize];
			memset(fileSizePacket, 0, sizeof(fileSizePacket));
			fileSizePacket[0] = 0x02; // Packet type for file size
			memcpy(fileSizePacket + 1, &size, sizeof(size));
			connection.SendPacket(fileSizePacket, sizeof(fileSizePacket));
		}
		unsigned char contentPacket[PacketSize];
		contentPacket[0] = 0x03; // Packet type for file content

		for (std::streamsize bytesRead = 0; bytesRead < size; ) {
			memset(contentPacket + 1, 0, PacketSize - 1);
			std::streamsize readSize = (std::min)(size - bytesRead, static_cast<std::streamsize>(PacketSize - 1));
			if (!file.read(reinterpret_cast<char*>(contentPacket + 1), readSize)) {
				std::cerr << "Failed to read from file.\n";
				break;
			}
			bytesRead += readSize;
			connection.SendPacket(contentPacket, readSize + 1);
		}

		// Send End of File packet (Type 0x04)
		unsigned char eofPacket[PacketSize];
		memset(eofPacket, 0, sizeof(eofPacket));
		eofPacket[0] = 0x04; // Packet type for end of file
		connection.SendPacket(eofPacket, sizeof(eofPacket));

		file.close();
		// Assuming connection is your UDP connection object and sendAccumulator, DeltaTime, and sendRate are properly defined.

		std::string fileName = "test.txt"; // Example file name

		// Create a packet for the file name
		unsigned char fileNamePacket[PacketSize];
		memset(fileNamePacket, 0, sizeof(fileNamePacket));
		fileNamePacket[0] = 0x01; // Packet type for file name
		strncpy((char*)fileNamePacket + 1, fileName.c_str(), sizeof(fileNamePacket) - 2); // Ensure there's room for null terminator

		// Send the file name packet
		connection.SendPacket(fileNamePacket, sizeof(fileNamePacket));

		// Existing logic to send "Hello World" messages
		sendAccumulator += DeltaTime;
		while (sendAccumulator > 1.0f / sendRate) {
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));
			sprintf((char*)packet, "Hello World <<%d>>", packetCounter++);
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;
		}



		// This loop is inside your main server loop, where it processes incoming packets.
		while (true) {
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0) break; // No more packets

			if (packet[0] == 0x01) { // Check if it's a file name packet
				// Extract the file name from the packet
				char fileName[255];
				strncpy(fileName, (char*)packet + 1, sizeof(packet) - 2);
				fileName[sizeof(packet) - 2] = '\0'; // Ensure null termination
				printf("Received file name: %s\n", fileName);
			}
			else {
				// Process other types of packets (e.g., "Hello World" messages)
				printf("Received packet: %s\n", packet + 1); // +1 to skip the type byte
			}
		}



		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}

	ShutdownSockets();

	return 0;
}
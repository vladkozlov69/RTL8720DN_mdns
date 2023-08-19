#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include <WiFi.h>
//#include <WiFiUdp.h>
#include <wifi_Udp.h>

#define DEBUG_STATISTICS      // Record how many incoming packets fitted into data_buffer.
#define DEBUG_OUTPUT          // Send packet summaries to Serial.
#define DEBUG_RAW             // Send HEX and ASCII encoded raw packet to Serial.

#define MDNS_TYPE_A     0x0001
#define MDNS_TYPE_PTR   0x000C
#define MDNS_TYPE_HINFO 0x000D
#define MDNS_TYPE_TXT   0x0010
#define MDNS_TYPE_AAAA  0x001C
#define MDNS_TYPE_SRV   0x0021

#define MDNS_TARGET_PORT 5353
#define MDNS_SOURCE_PORT 5353
#define MDNS_TTL 255

// Make this as big as memory limitations allow.
// This default value can be overridden using the max_packet_size_ parameter of
// MDns().
#define MAX_PACKET_SIZE 1024

// The mDNS spec says this should never be more than 256 (including trailing '\0').
#define MAX_MDNS_NAME_LEN 256  

namespace mdns {

// A single mDNS Query.
typedef struct Query {
#ifdef DEBUG_OUTPUT
	unsigned int buffer_pointer; // Position of Answer in packet. (Used for debugging only.)
#endif
	char qname_buffer[MAX_MDNS_NAME_LEN]; // Question Name: Contains the object, domain or zone name.
	unsigned int qtype; // Question Type: Type of question being asked by client.
	unsigned int qclass; // Question Class: Normally the value 1 for Internet (“IN”)
	bool unicast_response;                  //
	bool valid;           // False if problems were encountered decoding packet.

	void Display(Print * debug) const;    // Display a summary of this Answer on Serial port.
} Query;

// A single mDNS Answer.
typedef struct Answer {
#ifdef DEBUG_OUTPUT
	unsigned int buffer_pointer; // Position of Answer in packet. (Used for debugging only.)
#endif
	char name_buffer[MAX_MDNS_NAME_LEN];  // object, domain or zone name.
	char rdata_buffer[MAX_MDNS_NAME_LEN]; // The data portion of the resource record.
	unsigned int rrtype;                  // ResourceRecord Type.
	unsigned int rrclass; // ResourceRecord Class: Normally the value 1 for Internet (“IN”)
	unsigned long int rrttl; // ResourceRecord Time To Live: Number of seconds ths should be remembered.
	bool rrset;                    // Flush cache of records matching this name.
	bool valid;           // False if problems were encountered decoding packet.
	IPAddress ipAddress = INADDR_NONE;
	uint16_t port = 0;
	void Display(Print * debug) const;    // Display a summary of this Answer on Serial port.
} Answer;

class MDns;

class Callback {
public:
	virtual ~Callback()
	{
		//
	}
	virtual void onPacket(const MDns* packet) {};
	virtual void onQuery(const Query* query) {};
	virtual void onAnswer(const Answer* answer) {};
};

class MDns {
private:
	Print * debug = NULL;
public:

	MDns(WiFiUDP& udp, byte *data_buffer_ = NULL, int max_packet_size_ = MAX_PACKET_SIZE, Print * debug_ = &Serial):
#ifdef DEBUG_STATISTICS
		buffer_size_fail(0), largest_packet_seen(0), packet_count(0),
#endif
		buffer_pointer(0), max_packet_size(max_packet_size_)
	{
		if (data_buffer_ != NULL)
		{
			data_buffer = data_buffer_;
		}
		else {
			data_buffer = new byte[max_packet_size_];
		}
		this->udp = &udp;
		this->debug = debug_;
	};

	~MDns();

// added to call startUdpMulticast
	void begin();

	// Call this regularly to check for an incoming packet.
	bool loop();

	// Send this MDns packet.
	void Send() const;

	// Send this MDns packet to a unicast address
	void SendUnicast(IPAddress) const;

	// Resets everything to represent an empty packet.
	// Do this before building a packet for sending.
	void Clear();

	// Add a query to packet prior to sending.
	// May only be done before any Answers have been added.
	bool AddQuery(const Query &query);

	// Add an answer to packet prior to sending.
	bool AddAnswer(const Answer &answer);

	// Display a summary of the packet on Serial port.
	void Display() const;

	// Display the raw packet in HEX and ASCII.
	void DisplayRawPacket() const;

	// Get the source IP address of the packet
	IPAddress getRemoteIP();

	// Get the destination IP address of the packet (unicast or multicast)
	IPAddress getDestinationIP();

	void setCallback(Callback * newCallback) {
		this->_callback = newCallback;
	}

	Callback * getCallback() {
		return this->_callback;
	}

#ifdef DEBUG_STATISTICS
	// Counter gets increased every time an incoming mDNS packet arrives that does
	// not fit in the data_buffer.
	unsigned int buffer_size_fail;

	// Track the largest mDNS packet that has arrived.
	// Useful for knowing what size to make data_buffer.
	unsigned int largest_packet_seen;

	// How many mDNS packets have arrived so far.
	unsigned int packet_count;
#endif
private:
	// Initializes udp multicast
	uint8_t startUdpMulticast();

	void Parse_Query(Query &query);
	void Parse_Answer(Answer &answer);
	unsigned int PopulateName(const char *name_buffer);
	void PopulateAnswerResult(Answer *answer);
	void PrintHex(const unsigned char data) const;

	Callback * _callback = NULL;

	WiFiUDP* udp;

	// Position in data_buffer while processing packet.
	unsigned int buffer_pointer;

	// Buffer containing mDNS packet.
	byte *data_buffer = NULL;

	// Buffer size for incoming MDns packet.
	unsigned int max_packet_size;

	// Size of mDNS packet.
	unsigned int data_size = 0;

	// Query or Answer
	bool type = false;

	// Whether more follows in another packet.
	bool truncated = false;

	// Number of Queries in the packet.
	unsigned int query_count = 0;

	// Number of Answers in the packet.
	unsigned int answer_count = 0;

	unsigned int ns_count = 0;
	unsigned int ar_count = 0;

	// source & destination IP for incoming UDP packet
	IPAddress srcIP;
	IPAddress destIP;
};

// Display a byte on serial console in hexadecimal notation,
// padding with leading zero if necessary to provide evenly tabulated display data.
void PrintHex(unsigned char data);

// Extract Name from DNS data. Will follow pointers used by Message Compression.
// TODO Check for exceeding packet size.
int nameFromDnsPointer(char *p_name_buffer, int name_buffer_pos,
		const int name_buffer_len, const byte *p_packet_buffer,
		int packet_buffer_pos);
int nameFromDnsPointer(char *p_name_buffer, int name_buffer_pos,
		const int name_buffer_len, const byte *p_packet_buffer,
		int packet_buffer_pos, const bool recurse);

bool writeToBuffer(const byte value, char *p_name_buffer,
		int *p_name_buffer_pos, const int name_buffer_len);

int parseText(char *data_buffer, const int data_buffer_len, int const data_len,
		const byte *p_packet_buffer, int packet_buffer_pos);

} // namespace mdns

#endif  // MDNS_H

/*
 * MDNSClient.cpp
 *
 *  Created on: Aug 18, 2023
 *      Author: vkozlov
 */

#include "MDNSClient.h"

MDNSClient::MDNSClient(mdns::MDns &mdns, Print& debug) {
	_mdns = &mdns;
	_debug = &debug;
}

MDNSClient::~MDNSClient() {
	// TODO Auto-generated destructor stub
}

IPAddress MDNSClient::lookupHost(const char *hostName, uint16_t timeout) {
	IPAddress result = INADDR_NONE;
	question = (char*) malloc(strlen(hostName) + 1);
	Callback * oldCallback = _mdns->getCallback();
	strcpy(question, hostName);
	_mdns->setCallback(this);
	unsigned long startedAt = millis();
	_mdns->Clear();
	struct Query query;
	query.qclass = 1;    // "INternet"
	query.qtype = MDNS_TYPE_A;
	query.unicast_response = 0;
	strncpy(query.qname_buffer, question, MAX_MDNS_NAME_LEN);
	_mdns->AddQuery(query);
	lookupType = LOOKUP_HOST;
	clearHostsCache();
	_mdns->Send();

	while (millis() - startedAt < timeout) {
		_mdns->loop();
		if(hosts[0].host == question and hosts[0].ip != INADDR_NONE)
		{
			result = hosts[0].ip;
			break;
		}
	}
	lookupType = LOOKUP_NONE;
	_mdns->setCallback(oldCallback);
	free(question);
	question = NULL;

	return result;
}

int MDNSClient::lookupService(const char *svcName, uint16_t timeout) {
	int result = 0;
	question = (char*) malloc(strlen(svcName) + 1);
	Callback * oldCallback = _mdns->getCallback();
	strcpy(question, svcName);
	_mdns->setCallback(this);
	unsigned long startedAt = millis();
	_mdns->Clear();
	struct Query query;
	query.qclass = 1;    // "INternet"
	query.qtype = MDNS_TYPE_PTR;
	query.unicast_response = 0;
	strncpy(query.qname_buffer, question, MAX_MDNS_NAME_LEN);
	_mdns->AddQuery(query);
	lookupType = LOOKUP_SERVICE;
	clearHostsCache();
	_mdns->Send();

	while (millis() - startedAt < timeout) {
		_mdns->loop();
		for (int i = 0; i < MAX_HOSTS; i++) {
			if (hosts[i].host != "" and hosts[i].service != "" and hosts[i].port != 0 and hosts[i].ip != INADDR_NONE) {
				result++;
			}
		}
		if (result > 0) {
			break;
		}
	}
	lookupType = LOOKUP_NONE;
	_mdns->setCallback(oldCallback);
	free(question);
	question = NULL;

	return result;
}

void MDNSClient::onAnswer(const Answer *answer) {
	switch (lookupType) {
		case LOOKUP_HOST:
			processHostAnswer(answer);
			break;
		case LOOKUP_SERVICE:
			processServiceAnswer(answer);
			break;
		default:
			break;
	}

#ifdef DEBUG_OUTPUT
	if (_debug) {
		_debug->println("======================= RESULTS ===================");
		for (int i = 0; i < MAX_HOSTS; ++i) {
			if (hosts[i].service != "" || hosts[i].host != "") {
				_debug->print(">  ");
				_debug->print(hosts[i].service);
				_debug->print("    ");
				_debug->print(hosts[i].port);
				_debug->print("    ");
				_debug->print(hosts[i].host);
				_debug->print("    ");
				_debug->println(hosts[i].ip);
			}
		}
		_debug->println("===================================================");
	}
#endif
}


void MDNSClient::processHostAnswer(const Answer* answer) {
	// A typical A record matches an FQDN to network ipv4 address.
	// eg:
	//   name:    twinkle.local
	//   address: 192.168.192.9
	if (answer->rrtype == MDNS_TYPE_A and strcmp(answer->name_buffer, question) == 0 ) {
		hosts[0].host = answer->name_buffer;
		hosts[0].ip = answer->ipAddress;
	}
}

void MDNSClient::processServiceAnswer(const Answer* answer) {
	// A typical PTR record matches service to a human readable name.
	// eg:
	//  service: _mqtt._tcp.local
	//  name:    Mosquitto MQTT server on twinkle.local
	if (answer->rrtype == MDNS_TYPE_PTR
			and strstr(answer->name_buffer, question) != 0) {
		unsigned int i = 0;
		for (; i < MAX_HOSTS; ++i) {
			if (hosts[i].service == answer->rdata_buffer) {
				// Already in hosts[][].
				break;
			}
			if (hosts[i].service == "") {
				// This hosts[][] entry is still empty.
				hosts[i].service = answer->rdata_buffer;
				break;
			}
		}
		if (i == MAX_HOSTS and _debug) {
			_debug->print(" ** ERROR ** No space in buffer for ");
			_debug->print('"');
			_debug->print(answer->name_buffer);
			_debug->print('"');
			_debug->print("  :  ");
			_debug->print('"');
			_debug->println(answer->rdata_buffer);
			_debug->print('"');
		}
	}

	// A typical SRV record matches a human readable name to port and FQDN info.
	// eg:
	//  name:    Mosquitto MQTT server on twinkle.local
	//  data:    p=0;w=0;port=1883;host=twinkle.local
	if (answer->rrtype == MDNS_TYPE_SRV) {
		unsigned int i = 0;
		for (; i < MAX_HOSTS; ++i) {
			if (hosts[i].service == answer->name_buffer) {
				// This hosts entry matches the name of the host we are looking for
				// so parse data for port and hostname.
				hosts[i].port = answer->port;
				char *host_start = strstr(answer->rdata_buffer, "host=");
				if (host_start) {
					host_start += 5;
					hosts[i].host = host_start;
				}
				break;
			}
		}
		if (i == MAX_HOSTS and _debug) {
			_debug->print(" Did not find ");
			_debug->print('"');
			_debug->print(answer->name_buffer);
			_debug->print('"');
			_debug->println(" in hosts buffer.");
		}
	}

	// A typical A record matches an FQDN to network ipv4 address.
	// eg:
	//   name:    twinkle.local
	//   address: 192.168.192.9
	if (answer->rrtype == MDNS_TYPE_A) {
		int i = 0;
		for (; i < MAX_HOSTS; ++i) {
			if (hosts[i].host == answer->name_buffer) {
				hosts[i].ip = answer->ipAddress;
				break;
			}
		}
		if (i == MAX_HOSTS and _debug) {
			_debug->print(" Did not find ");
			_debug->print('"');
			_debug->print(answer->name_buffer);
			_debug->print('"');
			_debug->println(" in hosts buffer.");
		}
	}
}


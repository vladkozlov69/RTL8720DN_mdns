/*
 * MDNSClient.cpp
 *
 *  Created on: Aug 18, 2023
 *      Author: vkozlov
 */

#include "MDNSClient.h"

MDNSClient::MDNSClient(mdns::MDns &mdns) {
	_mdns = &mdns;
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
	struct Query query_host;
	strncpy(query_host.qname_buffer, question, MAX_MDNS_NAME_LEN);
	query_host.qtype = MDNS_TYPE_A;
	query_host.qclass = 1;    // "INternet"
	query_host.unicast_response = 0;
	_mdns->AddQuery(query_host);
	lookupType = LOOKUP_HOST;
	clearHostsCache();
	_mdns->Send();

	while (millis() - startedAt < timeout) {
		_mdns->loop();
		if(hosts[0][HOSTS_HOST_NAME] == question and addresses[0] != INADDR_NONE)
		{
			result = addresses[0];
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
	strncpy(query.qname_buffer, question, MAX_MDNS_NAME_LEN);
	query.qtype = MDNS_TYPE_PTR;
	query.qclass = 1;    // "INternet"
	query.unicast_response = 0;
	_mdns->AddQuery(query);
	lookupType = LOOKUP_SERVICE;
	clearHostsCache();
	_mdns->Send();

	while (millis() - startedAt < timeout) {
		_mdns->loop();
//		if(hosts[0][HOSTS_HOST_NAME] == question and addresses[0] != INADDR_NONE)
//		{
//			result = addresses[0];
//			break;
//		}
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

	Serial.println("======================= RESULTS ===================");
	for (int i = 0; i < MAX_HOSTS; ++i) {
		if (hosts[i][HOSTS_SERVICE_NAME] != "" || hosts[i][HOSTS_HOST_NAME] != "") {
			Serial.print(">  ");
			Serial.print(hosts[i][HOSTS_SERVICE_NAME]);
			Serial.print("    ");
			Serial.print(hosts[i][HOSTS_PORT]);
			Serial.print("    ");
			Serial.print(hosts[i][HOSTS_HOST_NAME]);
			Serial.print("    ");
			Serial.println(hosts[i][HOSTS_ADDRESS]);
		}
	}
	Serial.println("===================================================");
}


void MDNSClient::processHostAnswer(const Answer* answer) {
	// A typical A record matches an FQDN to network ipv4 address.
	// eg:
	//   name:    twinkle.local
	//   address: 192.168.192.9
	if (answer->rrtype == MDNS_TYPE_A and strcmp(answer->name_buffer, question) == 0 ) {
		hosts[0][HOSTS_HOST_NAME] = answer->name_buffer;
		hosts[0][HOSTS_ADDRESS] = answer->rdata_buffer;
		addresses[0] = answer->ipAddress;
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
			if (hosts[i][HOSTS_SERVICE_NAME] == answer->rdata_buffer) {
				// Already in hosts[][].
				break;
			}
			if (hosts[i][HOSTS_SERVICE_NAME] == "") {
				// This hosts[][] entry is still empty.
				hosts[i][HOSTS_SERVICE_NAME] = answer->rdata_buffer;
				break;
			}
		}
		if (i == MAX_HOSTS) {
			Serial.print(" ** ERROR ** No space in buffer for ");
			Serial.print('"');
			Serial.print(answer->name_buffer);
			Serial.print('"');
			Serial.print("  :  ");
			Serial.print('"');
			Serial.println(answer->rdata_buffer);
			Serial.print('"');
		}
	}

	// A typical SRV record matches a human readable name to port and FQDN info.
	// eg:
	//  name:    Mosquitto MQTT server on twinkle.local
	//  data:    p=0;w=0;port=1883;host=twinkle.local
	if (answer->rrtype == MDNS_TYPE_SRV) {
		unsigned int i = 0;
		for (; i < MAX_HOSTS; ++i) {
			if (hosts[i][HOSTS_SERVICE_NAME] == answer->name_buffer) {
				// This hosts entry matches the name of the host we are looking for
				// so parse data for port and hostname.
				char *port_start = strstr(answer->rdata_buffer, "port=");
				if (port_start) {
					port_start += 5;
					char *port_end = strchr(port_start, ';');
					char port[1 + port_end - port_start];
					strncpy(port, port_start, port_end - port_start);
					port[port_end - port_start] = '\0';

					if (port_end) {
						char *host_start = strstr(port_end, "host=");
						if (host_start) {
							host_start += 5;
							hosts[i][HOSTS_PORT] = port;
							hosts[i][HOSTS_HOST_NAME] = host_start;
						}
					}
				}
				break;
			}
		}
		if (i == MAX_HOSTS) {
			Serial.print(" Did not find ");
			Serial.print('"');
			Serial.print(answer->name_buffer);
			Serial.print('"');
			Serial.println(" in hosts buffer.");
		}
	}

	// A typical A record matches an FQDN to network ipv4 address.
	// eg:
	//   name:    twinkle.local
	//   address: 192.168.192.9
	if (answer->rrtype == MDNS_TYPE_A) {
		int i = 0;
		for (; i < MAX_HOSTS; ++i) {
			if (hosts[i][HOSTS_HOST_NAME] == answer->name_buffer) {
				hosts[i][HOSTS_ADDRESS] = answer->rdata_buffer;
				addresses[i] = answer->ipAddress;
				break;
			}
		}
		if (i == MAX_HOSTS) {
			Serial.print(" Did not find ");
			Serial.print('"');
			Serial.print(answer->name_buffer);
			Serial.print('"');
			Serial.println(" in hosts buffer.");
		}
	}
}


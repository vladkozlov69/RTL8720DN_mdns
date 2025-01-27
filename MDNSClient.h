/*
 * MDNSClient.h
 *
 *  Created on: Aug 18, 2023
 *      Author: vkozlov
 */

#ifndef LIBRARIES_RTL8720DN_MDNS_MDNSCLIENT_H_
#define LIBRARIES_RTL8720DN_MDNS_MDNSCLIENT_H_

#include "mdns.h"

#define MAX_HOSTS 4
#define HOSTS_SERVICE_NAME 0
#define HOSTS_PORT 1
#define HOSTS_HOST_NAME 2
#define HOSTS_ADDRESS 3

using namespace mdns;

struct HostInfo {
	String service;
	String host;
	uint16_t port;
	IPAddress ip;
};

class MDNSClient : public Callback {
public:
	enum LookupType {
		LOOKUP_NONE,
		LOOKUP_HOST,
		LOOKUP_SERVICE
	};
	MDNSClient(MDns& mdns, Print& debug = Serial);
	MDNSClient(MDns * mdns, Print * debug = &Serial);
	virtual ~MDNSClient();
	IPAddress lookupHost(const char * hostName, uint16_t timeout = 5000);
	int lookupService(const char *svcName, uint16_t timeout = 5000);
	virtual void onAnswer(const Answer* answer);
private:
	Print * _debug;
	MDns * _mdns;
	HostInfo hosts[MAX_HOSTS];
	char * question = NULL;
	LookupType lookupType = LOOKUP_NONE;
	void processHostAnswer(const Answer* answer);
	void processServiceAnswer(const Answer* answer);
	void clearHostsCache() {
		for (int i = 0; i < MAX_HOSTS; i++) {
			hosts[i].host = "";
			hosts[i].service = "";
			hosts[i].ip = INADDR_NONE;
			hosts[i].port = 0;
		}
	}
};

#endif /* LIBRARIES_RTL8720DN_MDNS_MDNSCLIENT_H_ */

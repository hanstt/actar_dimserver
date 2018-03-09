#include <cstdio>
#include <iostream>
#include <dis.hxx>
#include <dic.hxx>

#define HEARTBEAT_S 3

class ActarHeartBeat: public DimStampedInfo
{
	public:
		ActarHeartBeat(char const *name):
			DimStampedInfo(name, 1, 1),
      m_last_timestamp(time(NULL))
    {}

		bool isActarAlive() {
			time_t currentTime = time(NULL);
			return abs(currentTime - m_last_timestamp) <= HEARTBEAT_S;
		}

		void infoHandler() {
			std::string message(getString());
			if (message == "ActarHeartBeat") {
				m_last_timestamp = time(NULL);
			}
		}

	private:
		int m_last_timestamp;
};

int main()
{
	char const *dns_addr = getenv("DIM_DNS_HOSTNAME");
	if (NULL == dns_addr) {
		std::cout << "DIM_DNS_HOSTNAME not set, I'll use \"pccore21\".\n";
		dns_addr = "pccore21";
	}
	char const *dns_port_str = getenv("DIM_DNS_PORT");
	if (NULL == dns_port_str) {
		std::cout << "DIM_DNS_PORT not set, I'll use \"2505\".\n";
		dns_port_str = "2505";
	}
	unsigned dns_port = atoi(dns_port_str);

	DimServer::setDnsNode(dns_addr, dns_port);
	DimClient::setDnsNode(dns_addr, dns_port);

	char *message = strdup("COMPASSHeartBeat");
	DimService heartBeatService("COMPASSHeartBeat", message);

	int run_number = 0;

	ActarHeartBeat actarHeartBeat("ActarHeartBeat");

	DimInfo actarRunNumber("ActarRunNumber", -1);

	DimServer::start("CompassDAQ");

	// Add place-holder for run number in start command.
	char *start_run = strdup("StartRun ..........");
	char *stop_run = strdup("StopRun");

	for (int flip = 0;; flip = (flip + 1) & 15) {
		if (!actarHeartBeat.isActarAlive()) {
			std::cerr << "Actar be ded.\n";
		}
		usleep(500000);
		if (0 == flip) {
			++run_number;
			// Put run-number in start command so they always travel together.
			sprintf(start_run + 9, "%u", run_number);
      std::cout << start_run << '\n';
			DimClient::sendCommand("ActarRunControl", start_run);
		} else if (12 == flip) {
      std::cout << "ACTAR run number=" << actarRunNumber.getInt() << '\n';
      std::cout << stop_run << '\n';
			DimClient::sendCommand("ActarRunControl", stop_run);
		}
	}
}

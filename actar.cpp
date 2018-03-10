#include <sys/stat.h>
#include <sys/wait.h>
#include <cerrno>
#include <ctime>
#include <cstdarg>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <err.h>
#include <dis.hxx>
#include <dic.hxx>

#define HEARTBEAT_S 3

namespace {
char const *g_lmd_path;
char const *g_log_path;
int g_run_number;
DimService *g_run_number_service;
pid_t g_pid = -1;
void logg(char const *a_fmt, ...)
{
  time_t now = time(NULL);
  std::string ts = ctime(&now);
  ts.resize(ts.length() - 1);
  FILE *file = fopen(g_log_path, "a");
  if (NULL == file) {
    err(EXIT_FAILURE, "fopen(%s)", g_log_path);
  }
  fprintf(file, "%s: ", ts.c_str());
  va_list args;
  va_start(args, a_fmt);
  vfprintf(file, a_fmt, args);
  va_end(args);
  fprintf(file, "\n");
  fclose(file);

  printf("%s: ", ts.c_str());
  va_start(args, a_fmt);
  vprintf(a_fmt, args);
  va_end(args);
  printf("\n");
}

void ucesb_start(char const *a_run_number)
{
  logg(" Starting UCESB...");
  unsigned run_number = strtol(a_run_number, NULL, 10);
  char const *ucesb_splitter = "/LynxOS/mbsusr/mbsdaq/actar_dimserver/ucesb_splitter.bash";
  std::ostringstream oss;
  oss << run_number;
  g_pid = fork();
  if (g_pid < 0) {
    logg("fork failed: %s.", strerror(errno));
  } else if (0 == g_pid) {
    setsid();
    logg(" %s %s", ucesb_splitter, oss.str().c_str());
    execl(ucesb_splitter, ucesb_splitter, oss.str().c_str(), NULL);
    logg(" execl failed: %s.", strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    g_run_number = run_number;
    g_run_number_service->updateService();
    logg(" Started UCESB!");
  }
}

void ucesb_stop()
{
  if (-1 == g_pid) {
    return;
  }
  logg(" Killing UCESB...");
  kill(-g_pid, SIGINT);
  int status = 0;
  waitpid(g_pid, &status, 0);
  if (WIFEXITED(status)) {
    logg(" UCESB exited.");
  }
  g_pid = -1;
  logg(" Killed UCESB!");
  g_run_number = 0;
  g_run_number_service->updateService();
}

void sighandler(int a_signum)
{
  (void)a_signum;
  logg("Caught signal %u, cleaning up.", a_signum);
  if (-1 != g_pid) {
    ucesb_stop();
  }
  exit(EXIT_FAILURE);
}
}

class COMPASSHeartBeat: public DimStampedInfo
{
  public:
    COMPASSHeartBeat(char const *name):
      DimStampedInfo(name, 1, 1),
      m_last_timestamp(time(NULL))
    {}

    bool isCOMPASSAlive() {
      time_t currentTime = time(0);
      return abs(currentTime - m_last_timestamp) < HEARTBEAT_S;
    }

    void infoHandler() {
      std::string message(getString());
      if (message == "COMPASSHeartBeat") {
        m_last_timestamp = time(NULL);
      }
    }

  private:
    int m_last_timestamp;
};

class ActarRunControl: public DimCommand
{
  public:
    ActarRunControl(char const *name):
      DimCommand(name, "C") {}

    void commandHandler() {
      std::string command(getString());
      std::cout << command << '\n';
      if (0 == command.compare(0, 9, "StartRun ")) {
        logg("RUN START");
        ucesb_stop();
        ucesb_start(command.c_str() + 9);
      } else if (command == "StopRun") {
        logg("RUN STOP");
        ucesb_stop();
      } else {
        logg("Unknown command \"%s\"!", command.c_str());
      }
    }
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
  g_lmd_path = getenv("DIM_LMD_PATH");
  if (NULL == g_lmd_path) {
    std::cerr << "DIM_LMD_PATH not set, please do!\n";
    exit(EXIT_FAILURE);
  }
  g_log_path = getenv("DIM_LOG_PATH");
  if (NULL == g_log_path) {
    std::cerr << "DIM_LOG_PATH not set, please do!\n";
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);

  DimServer::setDnsNode(dns_addr, dns_port);
  DimClient::setDnsNode(dns_addr, dns_port);

  char *message = strdup("ActarHeartBeat");
  DimService heartBeatService("ActarHeartBeat", message);

  g_run_number_service = new DimService("ActarRunNumber", g_run_number);

  ActarRunControl run_control("ActarRunControl");

  COMPASSHeartBeat compassHeartBeat("COMPASSHeartBeat");

  DimServer::start("ActarDAQ");

  for (;;) {
    if (!compassHeartBeat.isCOMPASSAlive()) {
      std::cerr << "COMPASS be ded.\n";
    }
    usleep(500000);
  }
}

//#define LIBSSH_API
#define WIN32_LEAN_AND_MEAN
#include  <Windows.h>
#include  <WinSock2.h>
#include  <iostream>
#include  <boost/lexical_cast.hpp>
extern "C"
{
#include  <libssh/libssh.h>
}
#include  <cstdlib>
#include  <string>
#pragma comment(lib, "ssh.lib")
#include  <cstring>
#include  <ctype.h>

int verify_knownhost(ssh_session session)
{
  int state, hlen;
  unsigned char *hash = NULL;
  char *hexa;
  char buf[10];
  state = ssh_is_server_known(session);
  hlen = ssh_get_pubkey_hash(session, &hash);

  if (hlen < 0)    return -1;
  switch (state)  {    case SSH_SERVER_KNOWN_OK:      break;
 /* ok */    case SSH_SERVER_KNOWN_CHANGED:      fprintf(stderr, "Host key for server changed: it is now:\n");
      ssh_print_hexa("Public key hash", hash, hlen);
      fprintf(stderr, "For security reasons, connection will be stopped\n");
      free(hash);
      return -1;
    case SSH_SERVER_FOUND_OTHER:      fprintf(stderr, "The host key for this server was not found but an other"        "type of key exists.\n");
      fprintf(stderr, "An attacker might change the default server key to"        "confuse your client into thinking the key does not exist\n");
      free(hash);
      return -1;
    case SSH_SERVER_FILE_NOT_FOUND:      fprintf(stderr, "Could not find known host file.\n");
      fprintf(stderr, "If you accept the host key here, the file will be"       "automatically created.\n");
      /* fallback to SSH_SERVER_NOT_KNOWN behavior */    case SSH_SERVER_NOT_KNOWN:      hexa = ssh_get_hexa(hash, hlen);
      fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
      fprintf(stderr, "Public key hash: %s\n", hexa);
      //free(hexa);
      if (fgets(buf, sizeof(buf), stdin) == NULL)      {        free(hash);
        return -1;
      }
      if (std::string(buf) == std::string("yes"))
      {
        free(hash);
        return -1;
      }      if (ssh_write_knownhost(session) < 0)      {        fprintf(stderr, "Error %s\n", strerror(errno));
        free(hash);
        return -1;
      }      break;
    case SSH_SERVER_ERROR:      fprintf(stderr, "Error %s", ssh_get_error(session));
      free(hash);
      return -1;
  }
  //free(hash);

  return 0;

}

int show_remote_processes(ssh_session session){  ssh_channel channel;
  int rc;
  char buffer[8192] = {0};
  unsigned int nbytes;
  channel = ssh_channel_new(session);
  if (channel == NULL)    return SSH_ERROR;
  rc = ssh_channel_open_session(channel);
  if (rc != SSH_OK)  {    ssh_channel_free(channel);
    return rc;
  }
  rc = ssh_channel_request_exec(channel, "tail -f /usr/local/src/a.txt");
  //rc = ssh_channel_request_exec(channel, "ps aux");
  if (rc != SSH_OK)  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return rc;
  }
  nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
  //while (nbytes > 0)
  while(true)
  {
    //std::cout << buffer << std::endl;
    //wchar_t res[1024] = {0};
    //::MultiByteToWideChar(CP_ACP, 0, buffer, -1, res, _countof(res));
    std::cout << buffer << std::endl;
    //::OutputDebugStringA(buffer);
    ::memset(buffer, 0, _countof(buffer));
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    //::Sleep(1000);
  }
  if (nbytes < 0)
  {
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return SSH_ERROR;
  }
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
  return SSH_OK;
}

int wmain()
{
  std::locale::global(std::locale(""));
  ssh_session my_ssh_session = ssh_new();
  if(my_ssh_session != NULL)
  {
    const int verbosity = SSH_LOG_PROTOCOL;
    const int port = 22;
    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, "IPv4");
    ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
    const int rc = ssh_connect(my_ssh_session);
    if(rc == SSH_OK)
    {
      std::cout << "SSH_OK" << std::endl;
      if(0 <= verify_knownhost(my_ssh_session))
      {
        const int rc2 = ::ssh_userauth_password(my_ssh_session, "root", "rootroot");
        if(rc2 == SSH_AUTH_SUCCESS)
        {
          std::cout << "success: SSH_AUTH" << std::endl;
          const int a = show_remote_processes(my_ssh_session);
          std::cout << a << std::endl;
        }
        else
        {
          std::cout << "failure: SSH_AUTH" << std::endl;
        }
      }
      else
      {
        std::cout << "failure: verify_knownhost" << std::endl;
      }
    }
    else
    {
      std::cout << ssh_get_error(my_ssh_session);
      std::cout << "NOT SSH_OK" << std::endl;
    }
  }
  else
  {
    std::cout << "failure" << std::endl;
  }

  ssh_free(my_ssh_session);
  std::cin.get();
  return 0;
}

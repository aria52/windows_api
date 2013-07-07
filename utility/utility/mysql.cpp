#include  <iostream>
#include  <WinSock2.h>
#include  <mysql.h>
#pragma comment(lib, "libmysql")

int wmain()
{
  MYSQL *mysql = mysql_init(nullptr);
  mysql_real_connect(mysql, NULL,"root","root", "test", 0,NULL,0);
  if(mysql_errno(mysql) == 0)
  {
    std::cout << "ok" << mysql_error(mysql) << std::endl;
    ::mysql_query(mysql, "SELECT * FROM AA");
    if(::mysql_errno(mysql) != 0)
    {
      std::cout << mysql_error(mysql) << std::endl;
      std::cin.get();
    }
    MYSQL_RES *results = ::mysql_store_result(mysql);
    if(::mysql_errno(mysql) != 0)
    {
      std::cin.get();
    }
    while(MYSQL_ROW record = ::mysql_fetch_row(results))
    {
      std::cout << record[0] << std::endl;
    }
  }
  else
  {
    std::cout << "failed" << mysql_error(mysql) << std::endl;
  }
  std::cin.get();
  return 0;
}

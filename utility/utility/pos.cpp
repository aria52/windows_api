#include  <vector>
#include  <iostream>
#include  <algorithm>
#include  <postgres_ext.h>
#include  <libpq-fe.h>
#pragma comment(lib, "postgres")
#pragma comment(lib, "libpq.lib")

int wmain()
{
  /* DB‚Æ‚ÌÚ‘± */
  PGconn *con = PQconnectdb("host=* port=5432 dbname=test user=postgres password=root");
  if ( PQstatus(con) == CONNECTION_BAD ) { /* Ú‘±‚ª¸”s‚µ‚½‚Æ‚«‚ÌƒGƒ‰[ˆ— */
    fprintf(stderr,"%s",PQerrorMessage(con));
    exit(1);
  }
  /* select•¶‚Ì”­s */
  const char *del_target_table = "SELECT tablename FROM pg_tables WHERE tablename LIKE '%test%'";
  PGresult *res = ::PQexec(con, del_target_table);
  std::vector<std::string> tables;
  for(int i = 0; i != 2; ++i)
  {
    tables.emplace_back(::PQgetvalue(res, i, 0));
  }
  std::for_each(std::begin(tables), std::end(tables),
    [con](const std::string &tablename)
  {
    char table[255] = {0};
    ::_snprintf_s(table, _countof(table), "DROP TABLE %s;", tablename.c_str());
    ::PQexec(con, table);
  });

  PQclear(res);
  std::cin.get();
  return 0;
}

#include "../lib/p4net.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <mysql/mysql.h>

int db_init(MYSQL **conn)
{
    char    *host = "localhost";    
    char    *user = "admin";
    char    *pwd  = "admin";
    char    *db   = "P4db";

    *conn = mysql_init(NULL);
    /* Connect to database */
    if (!mysql_real_connect(*conn, host, user,
                        pwd, db, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(*conn));
        exit(1);
    }
    
    if (mysql_query(*conn, "set names utf8")) {
        fprintf(stderr, "%s\n", mysql_error(*conn));
        exit(1);
    }

    return 0;
}

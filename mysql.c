#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>

int acm_mysql_connect(MYSQL *db) {
    /* initialize databse */
    db = mysql_init(db);
    if(db == NULL) {
        syslog(LOG_ERR, "%s", "mysql object cannot be initialized");
        return 0;
    }
    if(mysql_real_connect(
                db,
                NULL,
                "root",
                "codmysql",
                "ibdoor",
                0,
                NULL,
                0) == NULL) {
        syslog(LOG_ERR, "error at mysql connection: %d - %s\n", mysql_errno(db),
                mysql_error(db));
        return 1;
    }
    return 0;
}


int acm_mysql_getUsers(MYSQL *db, MYSQL_RES *res)
{
    char *str;

    if(db == NULL) {
        syslog(LOG_ERR, "cannot connect because mysql is not initialized");
        return 1;
    }

    str = "SELECT * FROM ibdoor_user";
    mysql_real_query(db, str, strlen(str));

    res = mysql_store_result(db);

    return 0;
}




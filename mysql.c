#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>

#include "globals.h"


int acm_mysql_connect(MYSQL *db) {
    /* initialize databse */
    db = mysql_init(NULL);
    if(db == NULL) {
        syslog(LOG_ERR, "%s", "mysql object cannot be initialized");
        return 0;
    }
    if(mysql_real_connect(
                db,
                NULL,
                "root",
                "codm",
                "ibdoor",
                0,
                NULL,
                0) == NULL) {
        if(verbose)
            printf("error at mysql connection: %d - %s\n", mysql_errno(db),
                    mysql_error(db));
        syslog(LOG_ERR, "error at mysql connection: %d - %s\n", mysql_errno(db),
                mysql_error(db));
        return 1;
    }
    else
        if(verbose)
            printf("mysql connection successfully established\n");
    return 0;
}


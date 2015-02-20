all:
	gcc -o accessm main.c -pedantic -ansi -Wall -I/usr/include/mysql -lmysqlclient

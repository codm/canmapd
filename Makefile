all:
	gcc -o accessm -g main.c -g websock.c -g cansock.c -g isotp.c  -pedantic -ansi -Wall -I/usr/include/mysql -lpthread -lmysqlclient

#include "libnetfiles.h"


int main(int argc, char **argv)
{
	/*int a = newclientfd("127.0.0.1", "12002");
	char *blacks = (char *)malloc(128);
	char *whites = "restfile.txt";
	readbuf *test;
	int greys  = O_RDWR;
	char *pinks = "Open";
	char *magenta = "Close";
	sprintf(blacks, "%d\n%s\n%s\n%d\n", 26, pinks, whites, greys); 
	write(a, blacks, 29);

	test = readbuf_init(a);

	serv_readln(a, blacks, BUFSIZE, test);
	greys = serv_readln(a, blacks, BUFSIZE, test);

	close(a);
	a = newclientfd("127.0.0.1", "12002");

	free(blacks);
	blacks = (char *)malloc(128);
	sprintf(blacks, "%d\n%s\n%d\n", 10, magenta, atoi(blacks));

	write(a, blacks, 13);

	readbuf_free(test);
	test = readbuf_init(a);
	serv_readln(a, blacks, BUFSIZE, test);
	serv_readln(a, blacks, BUFSIZE, test);

	close(a);
	a = newclientfd("127.0.0.1", "12002");


	sprintf(blacks, "%d\n%s\n%d\n%d\n", 20, "Read", 6, 10);
	write(a, blacks, 15);
	
	readbuf_free(test);
	test = readbuf_init(a);
	serv_readln(a, blacks, BUFSIZE, test);
	serv_readln(a, blacks, BUFSIZE, test);
	serv_readln(a, blacks, BUFSIZE, test);

	close(a);
	readbuf_free(test);

	a = newclientfd("127.0.0.1", "12002");

	test = readbuf_init(a);

	sprintf(blacks, "%d\n%s\n%d\n%d\n%s\n", 23, "Write", 6, 11, "Gasthekikes");

	write(a, blacks, 26);

	serv_readln(a, blacks, BUFSIZE, test);
	printf("%s\n", blacks);
	serv_readln(a, blacks, BUFSIZE, test);
	printf("%s\n", blacks);

	close(a);
	readbuf_free(test);*/

	
	netserverinit("127.0.0.1");
	int test = netopen("restfile.txt", O_RDWR);
	printf("%d\n", test);

	char *gasgasgas = (char *)malloc(11);
	printf("%ld\n", (long)gasgasgas);
	gasgasgas[9] = '\0';
	netread(test, gasgasgas, 10);
	
	gasgasgas[10] = '\0';
	printf("%s\n", gasgasgas);
	

	printf("%ld\n", (long)gasgasgas);
	netwrite(test, gasgasgas, 10);
	printf("%ld\n", (long)gasgasgas);
	free(gasgasgas);
	test = netclose(test);
	printf("%d\n", test);
	close(test);
	


	return 0;
}

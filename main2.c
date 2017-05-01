#include "libnetfiles.h"




int main(int argc, char **argv)
{
	netserverinit("kill.cs.rutgers.edu", M_EXCLUSIVE);
	int rest = netopen("restfile.txt", O_RDWR);
	printf("%d\n", rest);
	return 0;
}

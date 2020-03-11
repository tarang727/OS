#include <stdio.h>
#include <string.h> //strtok(),strcasecmp()
#include <stdlib.h>	//malloc(),exit()
#include <limits.h>	//INT_MAX
#include <sys/types.h>	//open()
#include <sys/stat.h>	//open()
#include <fcntl.h>	//open()
#include <unistd.h>	//ftruncate()
#include <error.h>	//perror()

int main(int argc, char** argv)
{
	int fd=open(argv[1], O_CREAT|O_RDWR, S_IRWXU);
	int status=ftruncate(fd,atoi(argv[2]));
	if(status==-1)
	{
		perror("ftruncate() error: ");
		exit(1);
	} 

	return 0;
}
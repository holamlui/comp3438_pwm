#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int beep;
int fd;
int b_fd;
int main()
{

	
	char current_values[1];
	int  ret;	
	b_fd = open("/dev/button2", O_RDONLY);
	fd = open("/dev/a2", O_WRONLY);
	if(fd == -1)
	{
		printf("Fail to open device lab4!\n");
		goto finish;	
	}
	while(1)
	{
		ret = read(b_fd, current_values, sizeof(current_values) );
		if (ret != sizeof(current_values) ) 
		{
			printf("Read key error:\n");
			goto finish;
		}
		if(current_values[0] == '1')
		{
			beep = 1400;
		}
		else if(current_values[0] == '2')
		{
			beep = 0;
		} 
		write(fd, &beep, 1);
		printf("Outout is %c \n", beep);
	}
	close(fd);
	close(b_fd);

finish:	
	return 0;
}
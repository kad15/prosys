#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>



int main(int argc, char * argv[]){
	

	int nb,i,fd;
	char buf[512];
	int d='a'-'A';
	
	if (argc > 1)
	{
		fd = open(argv[1],O_RDONLY);
		if(fd==-1)
		{
			perror("ouverture");
			exit(1);
		}

		else
		{	
			close(0);
			dup(fd);
			close(fd);
		}
	}

 	while((nb=read(0, buf, sizeof(buf)))>0) {
		for (i=0; i < nb; i++){
			if (buf[i]>='a'&& buf[i]<='z')
				buf[i]-=d;
		}
		write(1,buf,nb);
	}

	return EXIT_SUCCESS;
}

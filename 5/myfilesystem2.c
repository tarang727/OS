/*The commands to be used are exactly the same as mentioned in the assignment.
Assumptions:
1. Maximum filesize that can be accommodated is limited by size of datablocks array in inode.
2. No of inodes, datablocks and their sizes are all fixed.
3. All files created reside in root folder. 
4. While copying files from one filesystem to another, the size of datablock of one should
   be either equal, multiple or sub-multiple of other.*/

#include <stdio.h>
#include <string.h> //strtok(),strcasecmp()
#include <stdlib.h>	//malloc(),exit()
#include <limits.h>	//INT_MAX
#include <sys/types.h>	//open()
#include <sys/stat.h>	//open()
#include <fcntl.h>	//open()
#include <unistd.h>	//ftruncate()
#include <error.h>	//perror()

/*defining all constants*/

#define MAX_IBM_SIZE 100
#define MAX_DBM_SIZE 5000
#define SUPERBLOCK_SIZE sizeof(Superblock)
#define INODE_SIZE sizeof(Inode)
#define MAX_DATA_BLOCKS_PER_INODE MAX_DBM_SIZE/MAX_IBM_SIZE
#define MAX_NO_OF_COMMANDS 50
#define MAX_COMMAND_LENGTH 1024
#define MAX_FILE_NAME 20
#define MAX_FILESYSTEM 10
#define debug(s,n) //(n==0?fprintf(stderr, "%s\n",s):fprintf(stderr, "%s%d\n", s,n))
#define prompt "tarangfs> "
#define clear_screen() printf("\033[H\033[J")
#define set (unsigned char)1	//to set bit in inode_bitmap
#define clear (unsigned char)0	//to clear bit in inode_bitmap

/*typedef of all structs used*/

/*superblock structure*/
typedef struct {
	long fs_size;
	int block_size;
	int inode_size;
	int data_block_count;
	int inode_count;
	int free_data_block_count;
	int free_inode_count;
	int data_block_start_location;
	int inode_start_location;
	int root_inode_no;
	unsigned char inode_bitmap[MAX_IBM_SIZE];
	unsigned char data_block_bitmap[MAX_DBM_SIZE];

} Superblock;

/*inode structure*/
typedef struct {
	char file_name[MAX_FILE_NAME];
	char type;
	long size;
	int data_block_count;
	int datablocks[MAX_DATA_BLOCKS_PER_INODE];

} Inode;

/*this structure is to store information about each file in the data block of the parent folder*/
typedef struct {
	char file_name[MAX_FILE_NAME];
	int inode_no;
	long size;
	
}FileInfo;

/*list of filesystems mounted yet*/
typedef struct {
	char file_name[MAX_FILE_NAME];	//name of osfile
	char fs_name;
	Superblock sb;
	Inode root_inode;
	int fd;
} FS_list;

FS_list fs[MAX_FILESYSTEM];
int fscount=0;
 

/*function prototype declarations*/

void shell_prompt();
char* read_command();
char** parse_command(char* read_line);
int execute_command(char** subcommands);
int create_filesystem(char** subcommands);
int mount_filesystem(char** subcommands);
int copy_file(char** subcommands);
int show_filesystem(char** subcommands);
int delete_file(char** subcommands);
int move_file(char** subcommands);
int writeSuperblock(int fd, Superblock sb);
int readSuperblock(int fd, Superblock* sb);
void printSuperblock(Superblock sb);
int writeInode(int fd, int offset, Inode inode);
int readInode(int fd, int offset, Inode* inode);
void printInode(Inode inode);
void printFS(FS_list fs);
int writeDatablock(int fd, int offset, void* buffer, int data_block_size);
int readDatablock(int fd, int offset, void* buffer, int data_block_size);
int find_free_index(unsigned char bitmap[], int size);
int search_file(int fs_index, char* filename, Inode* inode);
int search_drive(char drive);


int main()
{
	clear_screen();
	shell_prompt();
	return 0;
}

/*showing the shell prompt and reading commands*/
void shell_prompt()
{
	char *read_line;
	char **subcommands;
	int status=1;
	while(status)
	{
		printf("%s",prompt);
		read_line=read_command();	//reading line
		if(read_line[0]=='\0')
			continue;
		subcommands=parse_command(read_line);	//parsing the commands into subcommands
		status=execute_command(subcommands);	//executing the commands
	}
}

char* read_command()
{
	char* read_line=(char*)malloc(MAX_COMMAND_LENGTH*sizeof(char));
	int i=0;
	char c;
	while(1)
	{
		c=getchar();
		if(c=='\n')
		{
			read_line[i]='\0';
			return read_line;
		}
		else
			read_line[i]=c;

		i++;
	}

}

char** parse_command(char* read_line)
{
	char** subcommands=(char**)malloc(MAX_NO_OF_COMMANDS*sizeof(char*));
	int i=0;
	char* token=strtok(read_line," ");
	while(token!=NULL)
	{
		subcommands[i]=token;
		token=strtok(NULL," ");
		i++;
	}
	subcommands[i]=NULL;
	return subcommands;

}

int execute_command(char** subcommands)
{
	int status;
	if(strcasecmp(subcommands[0],"mkfs")==0)
	{
		status=create_filesystem(subcommands);
	}
	else if(strcasecmp(subcommands[0],"use")==0)
	{
		status=mount_filesystem(subcommands);
	}
	else if(strcasecmp(subcommands[0],"cp")==0)
	{
		status=copy_file(subcommands);
	}
	else if(strcasecmp(subcommands[0],"ls")==0)
	{
		status=show_filesystem(subcommands);
	}
	else if(strcasecmp(subcommands[0],"rm")==0)
	{
		status=delete_file(subcommands);
		if(status==-1)
		{
			fprintf(stderr, "File not found.\n");
			status=1;
		}
		if(status==1)
			fprintf(stderr, "File successfully deleted.\n");
	}
	else if(strcasecmp(subcommands[0],"mv")==0)
	{
		status=move_file(subcommands);
	}
	else if(strcasecmp(subcommands[0],"exit")==0)
	{
		int ch;
		fprintf(stderr, "Do you want to save the changes made ?\n1.yes\n2.no\nEnter: ");
		scanf("%d",&ch);
		if(ch==1)
		{
		/*Finally writing back the superblocks and inodes*/

			for(int i=0;i<fscount;i++)
			{
				status=writeSuperblock(fs[i].fd,fs[i].sb);
				if(status==0)
				{
					fprintf(stderr, "Error in writeSuperblock()\n");
					return 0;
				} 
				printf("\nSuperblock written:\n");
				printSuperblock(fs[i].sb);
				printf("\n");
				
				status=writeInode(fs[i].fd, SUPERBLOCK_SIZE, fs[i].root_inode);
				if(status==0)
				{
					fprintf(stderr, "Error in writeInode()\n");
					return 0;
				}
				printf("\nRoot inode written:\n");
				printInode(fs[i].root_inode);

				close(fs[i].fd);
			}
		}

		status=0;
	}
	else
	{
		printf("Wrong Command");
		status=0;
	}

	return status;

}

/*function to create the filesystem*/
int create_filesystem(char** subcommands)
{
	Superblock sb,sb1;
	Inode root_inode,inode1;
	int i,fd,status,offset;
	long fs_size_mb,fs_size;
	fd=open(subcommands[1], O_CREAT|O_RDWR, S_IRWXU);
	status=sscanf(subcommands[3],"%ld",&fs_size_mb);
	if(status!=1)
	{
		fprintf(stderr, "Wrong value of filesize in mkfs\n");
		return 0;
	}
	fs_size=fs_size_mb*1024*1024;	//megabytes to bytes
	status=ftruncate(fd,fs_size);
	if(status==-1)
	{
		perror("ftruncate() error: ");
		return 0;
	} 

	/*Initializing superblock*/

	sb.fs_size=fs_size;
	sb.block_size=atoi(subcommands[2]);
	sb.inode_size=INODE_SIZE;
	sb.inode_count=MAX_IBM_SIZE;
	sb.free_inode_count=sb.inode_count;	//no inode declared, so all inodes are free
	sb.inode_start_location=SUPERBLOCK_SIZE;
	sb.data_block_start_location=sb.inode_start_location+(sb.inode_count*sb.inode_size);
	sb.data_block_count=(sb.fs_size-sb.data_block_start_location)/sb.block_size;
	sb.free_data_block_count=sb.data_block_count;
	sb.root_inode_no=0; 

	/*Initializing inode bitmap*/
	for(i=0;i<MAX_IBM_SIZE;i++)
		sb.inode_bitmap[i]=clear;

	/*Initializing data blocks bitmap*/
	for(i=0;i<MAX_DBM_SIZE;i++)
		sb.data_block_bitmap[i]=clear;

	sb.inode_bitmap[0]=set;
	sb.free_inode_count--;

	status=writeSuperblock(fd,sb);
	if(status==0)
	{
		fprintf(stderr, "Error in writeSuperblock()\n");
		return 0;
	} 
	printf("\nSuperblock :\n");
	printSuperblock(sb); 

	/*status=readSuperblock(fd,&sb1);
	if(status==0)
	{
		fprintf(stderr, "Error in readSuperblock()\n");
		return 0;
	}
	printf("\nSuperblock read:\n");
	printSuperblock(sb1); */	

	/*Initializing root inode*/
	root_inode.type='d';	//d=directory, f=file
	root_inode.size=0;	//there exists nothing in the root folder
	root_inode.data_block_count=0;	//no data blocks assigned till now

	/*Initializing root inode datablocks*/
	for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
		root_inode.datablocks[i]=-1;

	offset=sb.inode_start_location+(sb.root_inode_no*INODE_SIZE);

	status=writeInode(fd, offset, root_inode);
	if(status==0)
	{
		fprintf(stderr, "Error in writeInode()\n");
		return 0;
	}
	printf("\nRoot inode :\n");
	printInode(root_inode);

	return 1;
}

/*mounting the filesystem*/
int mount_filesystem(char** subcommands)
{
	int new_mount,status;
	new_mount=fscount;

	fs[new_mount].fs_name=subcommands[3][0];	// char to denote filesystem
	memset(fs[new_mount].file_name,'\0',MAX_FILE_NAME);
	strcpy(fs[new_mount].file_name,subcommands[1]);	//name of the osfile
	fs[new_mount].fd=open(fs[new_mount].file_name,O_RDWR);	//opening the osfile and storing the fd
	debug(fs[new_mount].file_name,0);
	status=readSuperblock(fs[new_mount].fd,&(fs[new_mount].sb));	//reading and storing superblock
	if(status==0)
	{
		fprintf(stderr, "Error in readSuperblock()\n");
		return 0;
	}

	status=readInode(fs[new_mount].fd, SUPERBLOCK_SIZE, &fs[new_mount].root_inode);	//reading and storing inode
	if(status==0)
	{
		fprintf(stderr, "Error in readSuperblock()\n");
		return 0;		
	}

	printFS(fs[new_mount]);

	fscount++;
	return 1;
}

/*to copy files from one location to another*/
int copy_file(char** subcommands)
{
	struct stat stat_buf;
	void *buffer;
	char *src, *src1;
	Inode inode,root_inode;
	FileInfo file1;
	int i,fd1,flag,fs_index,offset,no_of_datablocks,status,free_inode_index,free_data_block_index;
	char c,c1;
	if((src=(char*)strchr(subcommands[1],':'))!=NULL)
	{
		/* copying files from one filesystem to another*/
		if((src1=(char*)strchr(subcommands[2],':'))!=NULL)	
		{
			int fs_index1,inode_index,inode_index1;
			void *buffer1;
			Inode inode1;
			src++;	//now this contains filename1
			src1++;	//this contains filename2
			c=subcommands[1][0];	//name of first filesystem
			c1=subcommands[2][0];	//name of second filesystem

			/*finding whether filesystem has been mounted or not*/
			fs_index=search_drive(c);
			if(fs_index==-1)
			{
				fprintf(stderr, "Filesystem 1 not yet mounted. Please mount it first.\n");
				return 1;
			}
			fs_index1=search_drive(c1);
			if(fs_index1==-1)
			{
				fprintf(stderr, "Filesystem 2 not yet mounted. Please mount it first.\n");
				return 1;
			}
			/*searching for the files in particular filesystem*/
			inode_index=search_file(fs_index, src, &inode);
			if(inode_index==0)
			{
				fprintf(stderr, "Error in copy_file()\n");
				return 0;
			}
			if(inode_index==-1)
			{
				fprintf(stderr, "Source file does not exist.\n");
				return 1;				
			}
			inode_index1=search_file(fs_index1, src1, &inode1);
			if(inode_index1==0)
			{
				fprintf(stderr, "Error in copy_file()\n");
				return 0;
			}

			/*Destination filename already exists. So overwrite has to be done. 
			Destination file is deleted first, then new file will be written.*/
			if(inode_index1!=-1)
			{
				debug("1",0);
				char* temp_filename[]={"", subcommands[2]};
				status=delete_file(temp_filename);	//calling delete_file()
				if(status==0)
				{
					fprintf(stderr, "Error in copy_file()\n");
					return 0;
				}
			}
			debug("ok",0);
			memset(file1.file_name, '\0', MAX_FILE_NAME);
			strcpy(file1.file_name,src1);	//copying filename into file structure

			debug(file1.file_name,0);

			/*This is for the inode of the file to be written later on*/
			free_inode_index=find_free_index(fs[fs_index1].sb.inode_bitmap, MAX_IBM_SIZE);
			if(free_inode_index==-1)
			{
				fprintf(stderr, "No free inodes left.\n");
				return 0;
			}
			/*This is for the data block of the root inode containing information about the file*/
			free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
			if(free_data_block_index==-1)
			{
				fprintf(stderr, "No free data blocks left.\n");
				return 0;
			}
			/*Modifying data block of root inode*/		

			file1.inode_no=free_inode_index;	//this is the inode for the file
			file1.size=inode.size;	//this is the size of the file

			/*this is to store the file info as a datablock*/
			buffer=malloc(fs[fs_index1].sb.block_size);
			memset(buffer,0,fs[fs_index1].sb.block_size);
			memcpy(buffer, &file1, sizeof(file1));

			/*finding proper offset*/
			offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);

			debug("Datablock offset: ",offset);

			/* Finally datablock of root inode is written*/
			status=writeDatablock(fs[fs_index1].fd, offset, buffer, fs[fs_index1].sb.block_size);
			if(status==0)
			{
				fprintf(stderr, "Error in writeDatablock()\n");
				return 0;
			}
			free(buffer);

			debug("datablock of root written",0);
			/*modifying superblock accordingly*/

			fs[fs_index1].sb.free_data_block_count--;
			fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
			printSuperblock(fs[fs_index1].sb);

			/*Modifying root inode*/
			fs[fs_index1].root_inode.size +=fs[fs_index1].sb.block_size;	//size increases by 1 block
			for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
			{
				if(fs[fs_index1].root_inode.datablocks[i]==-1)
				{
					fs[fs_index1].root_inode.datablocks[i]=free_data_block_index;
					fs[fs_index1].root_inode.data_block_count++;
					break;
				}
			}
			/* root inode modified */

			fs[fs_index1].sb.free_inode_count--;
			fs[fs_index1].sb.inode_bitmap[free_inode_index]=set;	
			Inode file_inode;
			/*Now preparing the inode for the file to be created*/
			file_inode.type='f';	//new inode to be created
			file_inode.size=inode.size;	//size of the file
			strcpy(file_inode.file_name, src1);	//name of the file stored in file structure
			file_inode.data_block_count=0;

			/*Initializing inode datablocks*/
			for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
				file_inode.datablocks[i]=-1;

			int datablock_index=0;

			/*size of datablock of both filesystems are same*/
			if(fs[fs_index].sb.block_size==fs[fs_index1].sb.block_size)
			{
				buffer=malloc(fs[fs_index1].sb.block_size);
				i=0;

				/*finding which datablocks to copy from, then copying their contents to destination filesystem*/
				while(inode.datablocks[i]!=-1)
				{
					memset(buffer,0,fs[fs_index].sb.block_size);
					offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
					/*reading the datablock*/
					status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in readDatablock()\n");
						return 0;
					}
					/*finding free datablocks to write to*/
					free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
					debug("free_data_block_index: ",free_data_block_index);
					if(free_data_block_index==-1)
					{
						fprintf(stderr, "Error while copying file. No datablocks found\n");
						return 0;
					}
					else
					{
						offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
						debug("offset :",offset);
						/*writing the datablock*/
						status=writeDatablock(fs[fs_index1].fd, offset, buffer, fs[fs_index1].sb.block_size);
						if(status==0)
						{
							fprintf(stderr, "Error in writeDatablock()\n");
							return 0;
						}

						fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;	//the particular bit in inode_bitmap is set
						fs[fs_index1].sb.free_data_block_count--;	//free data block count decreased
						file_inode.data_block_count++;
						file_inode.datablocks[datablock_index++]=free_data_block_index;
						printInode(file_inode);

					}
					i++;
				}
				/*Finally inode of the file is written*/

				offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
				status=writeInode(fs[fs_index1].fd, offset, file_inode);
				if(status==0)
				{
					fprintf(stderr, "Error in writeInode()\n");
					return 0;
				}	/* file inode written */

				free(buffer);	

			}
			/*if destination block size is smaller, source block is broken into smaller pieces and copied in parts*/
			else if(fs[fs_index].sb.block_size>fs[fs_index1].sb.block_size)
			{
				/*blocking factor is calculated*/
				int blocking_factor=(fs[fs_index].sb.block_size/fs[fs_index1].sb.block_size);
				i=0;
				buffer=malloc(fs[fs_index].sb.block_size);
				buffer1=malloc(fs[fs_index1].sb.block_size);

				while(inode.datablocks[i]!=-1)
				{
					memset(buffer,0,fs[fs_index].sb.block_size);
					offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
					/*reading source block*/
					status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in readDatablock()\n");
						return 0;
					}

					/*here the segmentation of the block takes place*/
					for(int j=0;j<blocking_factor;j++)
					{
						memset(buffer1,0,fs[fs_index1].sb.block_size);
						/*copying from source block part by part*/
						memcpy(buffer1, (buffer+(j*fs[fs_index1].sb.block_size)), fs[fs_index1].sb.block_size);
						free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
						debug("free_data_block_index: ",free_data_block_index);
						if(free_data_block_index==-1)
						{
							fprintf(stderr, "Error while copying file. No datablocks found\n");
							return 0;
						}
						else
						{
							offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
							debug("offset :",offset);
							status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
							if(status==0)
							{
								fprintf(stderr, "Error in writeDatablock()\n");
								return 0;
							}

							fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
							fs[fs_index1].sb.free_data_block_count--;
							file_inode.data_block_count++;
							file_inode.datablocks[datablock_index++]=free_data_block_index;
							printInode(file_inode);

						}						
					}
					i++;					

				}

				/*Finally inode of the file is written*/

				offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
				status=writeInode(fs[fs_index1].fd, offset, file_inode);
				if(status==0)
				{
					fprintf(stderr, "Error in writeInode()\n");
					return 0;
				}	/* file inode written */

				free(buffer);
				free(buffer1);
			}
			/*if destination block size is larger, multiple blocks of source are copied into single block*/
			else
			{
				int blocking_factor=(fs[fs_index1].sb.block_size/fs[fs_index].sb.block_size);
				i=0;
				int j=-1;
				buffer=malloc(fs[fs_index].sb.block_size);
				buffer1=malloc(fs[fs_index1].sb.block_size);
				memset(buffer1,0,fs[fs_index1].sb.block_size);
				while(inode.datablocks[i]!=-1)
				{
					j++;
					memset(buffer,0,fs[fs_index].sb.block_size);
					offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
					/* smaller source block is read*/
					status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in readDatablock()\n");
						return 0;
					}
					/*source block copied into proper position of destination block*/
					memcpy(buffer1+(j*fs[fs_index].sb.block_size), buffer, fs[fs_index].sb.block_size);
					/*destination block is filled now*/
					if(j==(blocking_factor-1))
					{
						/*finding free datablock index*/
						free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
						debug("free_data_block_index: ",free_data_block_index);
						if(free_data_block_index==-1)
						{
							fprintf(stderr, "Error while copying file. No datablocks found\n");
							return 0;
						}
						else
						{
							offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
							debug("offset :",offset);
							/*writing the datablock*/
							status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
							if(status==0)
							{
								fprintf(stderr, "Error in writeDatablock()\n");
								return 0;
							}

							fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
							fs[fs_index1].sb.free_data_block_count--;
							file_inode.data_block_count++;
							file_inode.datablocks[datablock_index++]=free_data_block_index;
							printInode(file_inode);

						}
						j=-1;
						memset(buffer1,0,fs[fs_index1].sb.block_size);

					}


					i++;
				}
				/*if last block is partially filled*/
				if(j!=-1)
				{
					free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
					debug("free_data_block_index: ",free_data_block_index);
					if(free_data_block_index==-1)
					{
						fprintf(stderr, "Error while copying file. No datablocks found\n");
						return 0;
					}
					else
					{
						offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
						debug("offset :",offset);
						status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
						if(status==0)
						{
							fprintf(stderr, "Error in writeDatablock()\n");
							return 0;
						}

						fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
						fs[fs_index1].sb.free_data_block_count--;
						file_inode.data_block_count++;
						file_inode.datablocks[datablock_index++]=free_data_block_index;
						printInode(file_inode);
					}
				}

				/*Finally inode of the file is written*/

				offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
				status=writeInode(fs[fs_index1].fd, offset, file_inode);
				if(status==0)
				{
					fprintf(stderr, "Error in writeInode()\n");
					return 0;
				}	/* file inode written */

				free(buffer);
				free(buffer1);

			}
		}
		/*copying files from filesystem to osfile*/
		else
		{
			fd1=open(subcommands[2], O_CREAT|O_RDWR, S_IRWXU);
			/*searching for the drive letter in mounted filesystems*/
			c=subcommands[1][0];
			fs_index=search_drive(c);
			if(fs_index==-1)
			{
				fprintf(stderr, "Filesystem not yet mounted. Please mount it first.\n");
				return 1;
			}
			src++;
			int inode_index=search_file(fs_index, src, &inode);
			if(inode_index==0)
			{
				fprintf(stderr, "Error in copy_file()\n");
				return 0;
			}
			if(inode_index==-1)
			{
				fprintf(stderr, "Source file does not exist.\n");
				return 1;				
			}
			debug("found",0);
			printInode(inode);
			status=ftruncate(fd1, ((inode.data_block_count+1)*fs[fs_index].sb.block_size));
			if(status==-1)
			{
				perror("ftruncate() error: ");
				return 0;
			}
			lseek(fd1,0, SEEK_SET);
			buffer=malloc(fs[fs_index].sb.block_size);
			for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
			{
				/*finding which datablocks are filled*/
				if(inode.datablocks[i]!=-1)
				{
					offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
					memset(buffer, 0, fs[fs_index].sb.block_size);
					/*reading the datablock*/
					status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in readDatablock()\n");
						return 0;
					}
					/*writing into the osfile*/
					status=write(fd1, buffer, fs[fs_index].sb.block_size);
					if(status!=fs[fs_index].sb.block_size)
					{
						fprintf(stderr, "Error occured while copying file. Exiting...\n");
						return 0;						
					}
				}
			}
			fprintf(stderr, "File successfully copied.\n");
			close(fd1);
		}
	}
	/*copying files from osfile to filesystem*/
	else
	{
		/*the file information to be stored in datablock of root inode is prepared*/
		src=(char*)strchr(subcommands[2],':');
		if(src==NULL)
		{
			fprintf(stderr, "Wrong format of input for destination file.\n");
			return 0;
		}
		src++;
		debug(src,0);	
		/*searching for the drive letter in mounted filesystems*/
		c=subcommands[2][0];
		fs_index=search_drive(c);
		debug("fs_index:",fs_index);
		if(fs_index==-1)
		{
			fprintf(stderr, "Filesystem not yet mounted. Please mount it first.\n");
			return 1;
		}
		Inode temp;
		/*searching if destination filename is already present*/
		int inode_index=search_file(fs_index, src, &temp);
		if(inode_index==0)
		{
			fprintf(stderr, "Error in copy_file()\n");
			return 0;
		}
		if(inode_index!=-1)
		{
			/*deleting the file, to simulate overwrite*/
			debug("1",0);
			char* temp_filename[]={"", subcommands[2]};
			status=delete_file(temp_filename);
			if(status==0)
			{
				fprintf(stderr, "Error in copy_file()\n");
				return 0;
			}
		}
		debug("2",0);
		memset(file1.file_name, '\0', MAX_FILE_NAME);
		strcpy(file1.file_name,src);

		debug(file1.file_name,0);
		/*the osfile is opened to be read later on and to get it's size*/
		fd1=open(subcommands[1],O_RDWR);
		fstat(fd1,&stat_buf);

		/*This is for the inode of the file to be written later on*/
		free_inode_index=find_free_index(fs[fs_index].sb.inode_bitmap, MAX_IBM_SIZE);
		if(free_inode_index==-1)
		{
			fprintf(stderr, "No free inodes left.\n");
			return 0;
		}
		/*This is for the data block of the root inode containing information about the file*/
		free_data_block_index=find_free_index(fs[fs_index].sb.data_block_bitmap, MAX_DBM_SIZE);
		if(free_data_block_index==-1)
		{
			fprintf(stderr, "No free data blocks left.\n");
			return 0;
		}
		/*Modifying data block of root inode*/		

		file1.inode_no=free_inode_index;	//this is the inode for the file
		file1.size=stat_buf.st_size;	//this is the size of the file


		buffer=malloc(fs[fs_index].sb.block_size);
		memset(buffer,0,fs[fs_index].sb.block_size);
		memcpy(buffer, &file1, sizeof(file1));
		offset=fs[fs_index].sb.data_block_start_location+(free_data_block_index*fs[fs_index].sb.block_size);

		debug("Datablock offset: ",offset);

		/* Finally datablock of root inode is written*/
		status=writeDatablock(fs[fs_index].fd, offset, buffer, fs[fs_index].sb.block_size);
		if(status==0)
		{
			fprintf(stderr, "Error in writeDatablock()\n");
			return 0;
		}

		debug("datablock of root written",0);
		fs[fs_index].sb.free_data_block_count--;
		fs[fs_index].sb.data_block_bitmap[free_data_block_index]=set;
		printSuperblock(fs[fs_index].sb);

		/*Modifying root inode*/
		fs[fs_index].root_inode.size +=fs[fs_index].sb.block_size;
		for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
		{
			if(fs[fs_index].root_inode.datablocks[i]==-1)
			{
				fs[fs_index].root_inode.datablocks[i]=free_data_block_index;
				fs[fs_index].root_inode.data_block_count++;
				break;
			}
		}
		/* root inode modified */

		fs[fs_index].sb.free_inode_count--;
		fs[fs_index].sb.inode_bitmap[free_inode_index]=set;	

		/*Now preparing the inode for the file to be created*/
		inode.type='f';	//new inode to be created
		inode.size=stat_buf.st_size;	//size of the file
		strcpy(inode.file_name, src);	//name of the file
		inode.data_block_count=0;
		/*Initializing inode datablocks*/
		for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
			inode.datablocks[i]=-1;

		memset(buffer,0,fs[fs_index].sb.block_size);

		i=0;

		/*Finding data blocks one by one and writing in them*/

		while(read(fd1, buffer, fs[fs_index].sb.block_size)>0)
		{
			free_data_block_index=find_free_index(fs[fs_index].sb.data_block_bitmap, MAX_DBM_SIZE);
			debug("free_data_block_index: ",free_data_block_index);
			if(free_data_block_index==-1)
			{
				fprintf(stderr, "Error while copying file. No datablocks found\n");
				return 0;
			}
			else
			{
				offset=fs[fs_index].sb.data_block_start_location+(free_data_block_index*fs[fs_index].sb.block_size);
				debug("offset :",offset);
				status=writeDatablock(fs[fs_index].fd, offset, buffer, fs[fs_index].sb.block_size);
				if(status==0)
				{
					fprintf(stderr, "Error in writeDatablock()\n");
					return 0;
				}

				fs[fs_index].sb.data_block_bitmap[free_data_block_index]=set;
				fs[fs_index].sb.free_data_block_count--;
				inode.data_block_count++;
				inode.datablocks[i++]=free_data_block_index;
				printInode(inode);

			}
		}

		/*Finally inode of the file is written*/

		offset=fs[fs_index].sb.inode_start_location+(free_inode_index*fs[fs_index].sb.inode_size);
		status=writeInode(fs[fs_index].fd, offset, inode);
		if(status==0)
		{
			fprintf(stderr, "Error in writeInode()\n");
			return 0;
		}	/* file inode written */
		close(fd1);
		
	}
	return 1;
}

/*function to show contents of a drive*/
int show_filesystem(char** subcommands)
{
	int i=0,index, offset,status;
	char c;
	void* buffer;
	c=subcommands[1][0];
	index=search_drive(c);

	if(index==-1)
	{
		fprintf(stderr, "Filesystem not yet mounted. Please mount it first.\n");
		return 1;
	}

	buffer=malloc(fs[index].sb.block_size);
	fprintf(stderr, "\n------------%c Drive---------------\n",c);
	for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
	{
		/*checking datablocks of root inode, which contain info about all files in it*/
		if(fs[index].root_inode.datablocks[i]!=-1)
		{
			FileInfo file;
			memset(buffer,0,fs[index].sb.block_size);
			offset=fs[index].sb.data_block_start_location+(fs[index].root_inode.datablocks[i]*fs[index].sb.block_size);
			status=readDatablock(fs[index].fd, offset , buffer, fs[index].sb.block_size);
			if(status==0)
			{
				fprintf(stderr, "Error in readDatablock()\n");
				return 0;
			}
			memcpy(&file, buffer, sizeof(file));
			fprintf(stderr, "%s\t\t %ld\n",file.file_name,file.size);

		}
	}
	//printSuperblock(fs[index].sb);
	//printInode(fs[index].root_inode);
	printf("\n");

	return 1;
}

/*deleting files*/
int delete_file(char** subcommands)
{
	int i,fs_index,inode_index,status,offset;
	char c;
	Inode file_inode;
	void* buffer;
	c=subcommands[1][0];
	fs_index=search_drive(c);

	if(fs_index==-1)
	{
		fprintf(stderr, "Filesystem not yet mounted. Please mount it first.\n");
		return 1;
	}
	subcommands[1]+=2;
	debug(subcommands[1],0);
	inode_index=search_file(fs_index, subcommands[1], &file_inode);
	if(inode_index==0)
	{
		fprintf(stderr, "Error in delete_file()\n");
		return 0;
	}
	if(inode_index==-1)
	{
		return -1;
	}
	debug("File found. File inode:",0);
	printInode(file_inode);

	/* clearing the superblock inode bitmap and datablocks bitmap */
	fs[fs_index].sb.inode_bitmap[inode_index]=clear;
	fs[fs_index].sb.free_inode_count++;
	
	for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
	{
		if(file_inode.datablocks[i]!=-1)
		{
			debug("datablock :",file_inode.datablocks[i]);
			fs[fs_index].sb.data_block_bitmap[file_inode.datablocks[i]]=clear;
			fs[fs_index].sb.free_data_block_count++;			
		}
	}
	/* managing the root_inode */
	fs[fs_index].root_inode.size-=fs[fs_index].sb.block_size;	//size decreased by block size
	fs[fs_index].root_inode.data_block_count--;	//datablock count decreased by 1
	buffer=malloc(fs[fs_index].sb.block_size);

	for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
	{
		if(fs[fs_index].root_inode.datablocks[i]!=-1)
		{
			FileInfo file;
			memset(buffer,0,fs[fs_index].sb.block_size);
			offset=fs[fs_index].sb.data_block_start_location+(fs[fs_index].root_inode.datablocks[i]*fs[fs_index].sb.block_size);
			status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
			if(status==0)
			{
				fprintf(stderr, "Error in readDatablock()\n");
				return 0;
			}
			memcpy(&file, buffer, sizeof(file));
			/*checking the index of this datablock to find whether it has the filename or not*/
			if(strcasecmp(file.file_name, subcommands[1])==0)
			{
				debug("found file datablock",0);
				fs[fs_index].sb.data_block_bitmap[fs[fs_index].root_inode.datablocks[i]]=clear;	//clearing the corresponding bit in superblock 
				fs[fs_index].root_inode.datablocks[i]=-1;	//removing index from datablocks of root inode
				fs[fs_index].sb.free_data_block_count++;	//free data block count of superblock increased by 1
				break;
			}

		}
	}

	return 1;
}
/*move file implemented same as copy file, except that in the end, source file is deleted*/
int move_file(char** subcommands)
{
	Inode inode,file_inode,root_inode;
	FileInfo file1;
	int i,fd1,flag,fs_index,inode_index,offset,no_of_datablocks,status,free_inode_index,free_data_block_index;
	void *buffer;
	char c,c1;
	char *src, *src1;
	c=subcommands[1][0];
	c1=subcommands[2][0];
	src=(subcommands[1]+2);
	src1=(subcommands[2]+2);	
	/*if source and destination drive are same, simply rename*/
	if(c==c1)
	{
		debug(src,0);
		debug(src1,0);
		fs_index=search_drive(c);

		if(fs_index==-1)
		{
			fprintf(stderr, "Filesystem not yet mounted. Please mount it first.\n");
			return 1;
		}
		inode_index=search_file(fs_index, src, &file_inode);
		if(inode_index==0)
		{
			fprintf(stderr, "Error in move_file()\n");
			return 0;
		}
		if(inode_index==-1)
		{
			fprintf(stderr, "Source file not found.\n");
			return 1;
		}
		/*changing the filename in particular inode*/
		memset(file_inode.file_name,'\0', MAX_FILE_NAME);
		strcpy(file_inode.file_name, src1);
		offset=fs[fs_index].sb.inode_start_location+(inode_index*fs[fs_index].sb.inode_size);
		/*writing inode back*/
		status=writeInode(fs[fs_index].fd, offset, file_inode);
		if(status==0)
		{
			fprintf(stderr, "Error in writeInode()\n");
			return 0;
		}
		buffer=malloc(fs[fs_index].sb.block_size);
		for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
		{
			/*checking the datablocks of root_inode to change filename their also*/
			if(fs[fs_index].root_inode.datablocks[i]!=-1)
			{
				FileInfo file;
				memset(buffer,0,fs[fs_index].sb.block_size);
				offset=fs[fs_index].sb.data_block_start_location+(fs[fs_index].root_inode.datablocks[i]*fs[fs_index].sb.block_size);
				status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
				if(status==0)
				{
					fprintf(stderr, "Error in readDatablock()\n");
					return 0;
				}
				memcpy(&file, buffer, sizeof(file));
				if(strcasecmp(file.file_name, src)==0)
				{
					debug("found file datablock. renaming...",0);
					memset(file.file_name,'\0',MAX_FILE_NAME);
					strcpy(file.file_name,src1);	//filename copied
					memset(buffer,0,fs[fs_index].sb.block_size);
					memcpy(buffer, &file, sizeof(file));
					status=writeDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in readDatablock()\n");
						return 0;
					}		
					break;
				}

			}
		}

		free(buffer);
	}
	else
	{
		/*same as copy_file() function*/
		int fs_index1,inode_index,inode_index1;
		void *buffer1;
		Inode inode1;
		fs_index=search_drive(c);
		if(fs_index==-1)
		{
			fprintf(stderr, "Filesystem 1 not yet mounted. Please mount it first.\n");
			return 1;
		}
		fs_index1=search_drive(c1);
		if(fs_index1==-1)
		{
			fprintf(stderr, "Filesystem 2 not yet mounted. Please mount it first.\n");
			return 1;
		}
		inode_index=search_file(fs_index, src, &inode);
		if(inode_index==0)
		{
			fprintf(stderr, "Error in move_file()\n");
			return 0;
		}
		if(inode_index==-1)
		{
			fprintf(stderr, "Source file does not exist.\n");
			return 1;				
		}
		inode_index1=search_file(fs_index1, src1, &inode1);
		if(inode_index1==0)
		{
			fprintf(stderr, "Error in move_file()\n");
			return 0;
		}
		if(inode_index1!=-1)
		{
			debug("1",0);
			char* temp_filename[]={"", subcommands[2]};
			status=delete_file(temp_filename);
			if(status==0)
			{
				fprintf(stderr, "Error in move_file()\n");
				return 0;
			}
		}
		debug("ok",0);
		memset(file1.file_name, '\0', MAX_FILE_NAME);
		strcpy(file1.file_name,src1);

		debug(file1.file_name,0);

		/*This is for the inode of the file to be written later on*/
		free_inode_index=find_free_index(fs[fs_index1].sb.inode_bitmap, MAX_IBM_SIZE);
		if(free_inode_index==-1)
		{
			fprintf(stderr, "No free inodes left.\n");
			return 0;
		}
		/*This is for the data block of the root inode containing information about the file*/
		free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
		if(free_data_block_index==-1)
		{
			fprintf(stderr, "No free data blocks left.\n");
			return 0;
		}
		/*Modifying data block of root inode*/		

		file1.inode_no=free_inode_index;	//this is the inode for the file
		file1.size=inode.size;	//this is the size of the file


		buffer=malloc(fs[fs_index1].sb.block_size);
		memset(buffer,0,fs[fs_index1].sb.block_size);
		memcpy(buffer, &file1, sizeof(file1));
		offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);

		debug("Datablock offset: ",offset);

		/* Finally datablock of root inode is written*/
		status=writeDatablock(fs[fs_index1].fd, offset, buffer, fs[fs_index1].sb.block_size);
		if(status==0)
		{
			fprintf(stderr, "Error in writeDatablock()\n");
			return 0;
		}
		free(buffer);

		debug("datablock of root written",0);
		fs[fs_index1].sb.free_data_block_count--;
		fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
		printSuperblock(fs[fs_index1].sb);

		/*Modifying root inode*/
		fs[fs_index1].root_inode.size +=fs[fs_index1].sb.block_size;
		for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
		{
			if(fs[fs_index1].root_inode.datablocks[i]==-1)
			{
				fs[fs_index1].root_inode.datablocks[i]=free_data_block_index;
				fs[fs_index1].root_inode.data_block_count++;
				break;
			}
		}
		/* root inode modified */

		fs[fs_index1].sb.free_inode_count--;
		fs[fs_index1].sb.inode_bitmap[free_inode_index]=set;	
		Inode file_inode;
		/*Now preparing the inode for the file to be created*/
		file_inode.type='f';	//new inode to be created
		file_inode.size=inode.size;	//size of the file
		strcpy(file_inode.file_name, src1);	//name of the file
		file_inode.data_block_count=0;
		/*Initializing inode datablocks*/
		for(i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
			file_inode.datablocks[i]=-1;

		int datablock_index=0;

		if(fs[fs_index].sb.block_size==fs[fs_index1].sb.block_size)
		{
			buffer=malloc(fs[fs_index1].sb.block_size);
			i=0;
			/*Finding data blocks one by one and writing in them*/

			while(inode.datablocks[i]!=-1)
			{
				memset(buffer,0,fs[fs_index].sb.block_size);
				offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
				status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
				if(status==0)
				{
					fprintf(stderr, "Error in readDatablock()\n");
					return 0;
				}

				free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
				debug("free_data_block_index: ",free_data_block_index);
				if(free_data_block_index==-1)
				{
					fprintf(stderr, "Error while copying file. No datablocks found\n");
					return 0;
				}
				else
				{
					offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
					debug("offset :",offset);
					status=writeDatablock(fs[fs_index1].fd, offset, buffer, fs[fs_index1].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in writeDatablock()\n");
						return 0;
					}

					fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
					fs[fs_index1].sb.free_data_block_count--;
					file_inode.data_block_count++;
					file_inode.datablocks[datablock_index++]=free_data_block_index;
					printInode(file_inode);

				}
				i++;
			}
			/*Finally inode of the file is written*/

			offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
			status=writeInode(fs[fs_index1].fd, offset, file_inode);
			if(status==0)
			{
				fprintf(stderr, "Error in writeInode()\n");
				return 0;
			}	/* file inode written */

			free(buffer);	

		}
		else if(fs[fs_index].sb.block_size>fs[fs_index1].sb.block_size)
		{
			int blocking_factor=(fs[fs_index].sb.block_size/fs[fs_index1].sb.block_size);
			i=0;
			buffer=malloc(fs[fs_index].sb.block_size);
			buffer1=malloc(fs[fs_index1].sb.block_size);

			while(inode.datablocks[i]!=-1)
			{
				memset(buffer,0,fs[fs_index].sb.block_size);
				offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
				status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
				if(status==0)
				{
					fprintf(stderr, "Error in readDatablock()\n");
					return 0;
				}

				for(int j=0;j<blocking_factor;j++)
				{
					memset(buffer1,0,fs[fs_index1].sb.block_size);
					memcpy(buffer1, (buffer+(j*fs[fs_index1].sb.block_size)), fs[fs_index1].sb.block_size);
					free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
					debug("free_data_block_index: ",free_data_block_index);
					if(free_data_block_index==-1)
					{
						fprintf(stderr, "Error while moving file. No datablocks found\n");
						return 0;
					}
					else
					{
						offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
						debug("offset :",offset);
						status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
						if(status==0)
						{
							fprintf(stderr, "Error in writeDatablock()\n");
							return 0;
						}

						fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
						fs[fs_index1].sb.free_data_block_count--;
						file_inode.data_block_count++;
						file_inode.datablocks[datablock_index++]=free_data_block_index;
						printInode(file_inode);

					}						
				}
				i++;					

			}

			/*Finally inode of the file is written*/

			offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
			status=writeInode(fs[fs_index1].fd, offset, file_inode);
			if(status==0)
			{
				fprintf(stderr, "Error in writeInode()\n");
				return 0;
			}	/* file inode written */

			free(buffer);
			free(buffer1);
		}
		else
		{
			int blocking_factor=(fs[fs_index1].sb.block_size/fs[fs_index].sb.block_size);
			i=0;
			int j=-1;
			buffer=malloc(fs[fs_index].sb.block_size);
			buffer1=malloc(fs[fs_index1].sb.block_size);
			memset(buffer1,0,fs[fs_index1].sb.block_size);
			while(inode.datablocks[i]!=-1)
			{
				j++;
				memset(buffer,0,fs[fs_index].sb.block_size);
				offset=fs[fs_index].sb.data_block_start_location+(inode.datablocks[i]*fs[fs_index].sb.block_size);
				status=readDatablock(fs[fs_index].fd, offset , buffer, fs[fs_index].sb.block_size);
				if(status==0)
				{
					fprintf(stderr, "Error in readDatablock()\n");
					return 0;
				}
				memcpy(buffer1+(j*fs[fs_index].sb.block_size), buffer, fs[fs_index].sb.block_size);
				if(j==(blocking_factor-1))
				{
					free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
					debug("free_data_block_index: ",free_data_block_index);
					if(free_data_block_index==-1)
					{
						fprintf(stderr, "Error while moving file. No datablocks found\n");
						return 0;
					}
					else
					{
						offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
						debug("offset :",offset);
						status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
						if(status==0)
						{
							fprintf(stderr, "Error in writeDatablock()\n");
							return 0;
						}

						fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
						fs[fs_index1].sb.free_data_block_count--;
						file_inode.data_block_count++;
						file_inode.datablocks[datablock_index++]=free_data_block_index;
						printInode(file_inode);

					}
					j=-1;
					memset(buffer1,0,fs[fs_index1].sb.block_size);

				}


				i++;
			}
			if(j!=-1)
			{
				free_data_block_index=find_free_index(fs[fs_index1].sb.data_block_bitmap, MAX_DBM_SIZE);
				debug("free_data_block_index: ",free_data_block_index);
				if(free_data_block_index==-1)
				{
					fprintf(stderr, "Error while moving file. No datablocks found\n");
					return 0;
				}
				else
				{
					offset=fs[fs_index1].sb.data_block_start_location+(free_data_block_index*fs[fs_index1].sb.block_size);
					debug("offset :",offset);
					status=writeDatablock(fs[fs_index1].fd, offset, buffer1, fs[fs_index1].sb.block_size);
					if(status==0)
					{
						fprintf(stderr, "Error in writeDatablock()\n");
						return 0;
					}

					fs[fs_index1].sb.data_block_bitmap[free_data_block_index]=set;
					fs[fs_index1].sb.free_data_block_count--;
					file_inode.data_block_count++;
					file_inode.datablocks[datablock_index++]=free_data_block_index;
					printInode(file_inode);
				}
			}

			/*Finally inode of the file is written*/

			offset=fs[fs_index1].sb.inode_start_location+(free_inode_index*fs[fs_index1].sb.inode_size);
			status=writeInode(fs[fs_index1].fd, offset, file_inode);
			if(status==0)
			{
				fprintf(stderr, "Error in writeInode()\n");
				return 0;
			}	/* file inode written */

			free(buffer);
			free(buffer1);

		}
		/*finally, source file is deleted*/
		status=delete_file(subcommands);
		if(status==0)
		{
			fprintf(stderr, "Error in move_file()\n");
			return 0;	
		}
	}

	return 1;
}

/*checking whether the drive letter has been mounted or not*/
int search_drive(char drive)
{
	int i,fs_index=-1;
	for(i=0;i<fscount;i++)
	{
		if(fs[i].fs_name==drive)
		{
			fs_index=i;
			break;
		}
	}

	return fs_index;	
}

/*searching the file in fs_index filesystem*/
int search_file(int fs_index, char* filename, Inode *inode)
{
	/*searching for the file in the existing inodes*/
	int i,offset,status;
	for(i=1;i<MAX_IBM_SIZE;i++)	//since 0th inode is root inode
	{
		if(fs[fs_index].sb.inode_bitmap[i]==set)
		{
			debug("search_file",0);
			offset=fs[fs_index].sb.inode_start_location+(i*fs[fs_index].sb.inode_size);
			status=readInode(fs[fs_index].fd,offset,inode);	//particular inode is stored
			if(status==0)
			{
				fprintf(stderr, "Error in readInode() in search_file()\n");
				return 0;
			}
			if(strcasecmp(inode[0].file_name,filename)==0)	//file found
			{
				break;
			}				
		}
	}

	if(i==MAX_IBM_SIZE)
		return -1;	//file not found
	else
		return i;	//index of inode is returned
}

/*to write back superblock into filesystem*/
int writeSuperblock(int fd, Superblock sb)
{
	int status;
	lseek(fd,0, SEEK_SET);
	status=write(fd, &sb, SUPERBLOCK_SIZE);
	if(status==-1)
	{
		perror("write() error: ");
		return 0;
	}
	return 1;
}

/*to read superblock from filesystem*/
int readSuperblock(int fd, Superblock* sb)
{
	int status;
	lseek(fd,0, SEEK_SET);
	status=read(fd, sb, SUPERBLOCK_SIZE);
	if(status==-1)
	{
		perror("read() error: ");
		return 0;
	}
	return 1;
}

/*to print superblock of filesystem*/
void printSuperblock(Superblock sb)
{
  printf("\n-------Superblock----------\n");
  printf("Size of filesystem : %ld\n",sb.fs_size);
  printf("Block Size : %d\n",sb.block_size);
  printf("Root INode Number=%d\n",sb.root_inode_no);
  printf("Inode Start Location=%d\n",sb.inode_start_location);
  printf("Data Block Start Location=%d\n",sb.data_block_start_location);
  printf("Inode Size=%d\n",sb.inode_size);
  printf("Total Inode Count=%d\n",sb.inode_count);
  printf("Total Data Block Count=%d\n",sb.data_block_count);
  printf("Free Inode Count=%d\n",sb.free_inode_count);
  printf("Free Data Block Count=%d\n",sb.free_data_block_count);
  
  printf("Inode Bitmap \n ");
  for(int i=0;i<MAX_IBM_SIZE;i++)
  {
    printf("%d ",sb.inode_bitmap[i]);
  }
  printf("\nDatablock Bitmap \n ");
  for(int i=0;i<(MAX_DBM_SIZE/100);i++)
  {
    printf("%d ",sb.data_block_bitmap[i]);
  } 

  printf("\n");

}

/*to write back inodes into filesystem*/
int writeInode(int fd, int offset, Inode inode)
{
	int status;
	lseek(fd,offset, SEEK_SET);
	status=write(fd, &inode, INODE_SIZE);
	if(status==-1)
	{
		perror("write() error: ");
		return 0;
	}
	return 1;
}

/*to read inodes from filesystem*/
int readInode(int fd, int offset, Inode* inode)
{
	int status;
	lseek(fd,offset, SEEK_SET);
	status=read(fd, inode, INODE_SIZE);
	if(status==-1)
	{
		perror("read() error: ");
		return 0;
	}
	return 1;
}

/*printing details of an inode*/
void printInode(Inode inode)
{
	printf("\nInode:\n");
	printf("\nFile Type=%c\n",inode.type);
	printf("File Size=%ld\n",inode.size);
	printf("Data Block Count=%d\n",inode.data_block_count);
	printf("Data  blocks\n");
	for(int i=0;i<MAX_DATA_BLOCKS_PER_INODE;i++)
	printf("%d ",inode.datablocks[i]);
	printf("\n");

}

/*writing block by block in filesystem*/
int writeDatablock(int fd, int offset, void* buffer, int data_block_size)
{
	int status;
	lseek(fd,offset, SEEK_SET);
	status=write(fd, buffer, data_block_size);
	if(status==-1)
	{
		perror("write() error: ");
		return 0;
	}
	return 1;
}

/*reading block by block from filesystem*/
int readDatablock(int fd, int offset, void* buffer, int data_block_size)
{
	int status;
	lseek(fd,offset, SEEK_SET);
	status=read(fd, buffer, data_block_size);
	if(status==-1)
	{
		perror("read() error: ");
		return 0;
	}
	return 1;
}

/*finding free index from bitmap passed to it*/
int find_free_index(unsigned char bitmap[], int size)
{
	int i;
	for(i=0;i<size;i++)
	{
		if(bitmap[i]==clear)
			return i;
	}
	return -1;
}

/*printing the details of mounted  filesystem*/
void printFS(FS_list fs)
{
 
  printf("\nFilename=%s",fs.file_name);
  printf("\nFile System Name=%c",fs.fs_name);
  printSuperblock(fs.sb);
  printInode(fs.root_inode);

}



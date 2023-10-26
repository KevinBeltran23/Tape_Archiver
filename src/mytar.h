#include<sys/stat.h>
#include<string.h>
#include<stdint.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<dirent.h>
#include<fcntl.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<utime.h>
#include<math.h>
#include<netinet/in.h>
#define BUFFER_SIZE 512
#define MAX_NAME 256 
#define MAX_VERBOSE_LIST 312 
#define PERMISSION_SIZE 10
#define OWNER_OFFSET 11
#define SIZE_OFFSET 29
#define SIZE_LENGTH 8
#define OCTAL_LENGTH 12
#define MTIME_OFFSET 38
#define MTIME_LENGTH 16
#define NAME_OFFSET 55
#define TIME_LENGTH 16
#define USERNAME_SIZE 32
#define NAME_SIZE 100
#define PREFIX_SIZE 155
#define PREFIX_INDEX 148
#define OCTAL_MAX 07777777
#define OWNERGROUP_WIDTH 16
#define BASE_OCTAL 8
#define SPECIALHEX_SEPARATOR 4

typedef struct __attribute__ ((__packed__)) header{
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
}header;

void createListing(char *filepath);
void createHeader(char *filepath, char *wbuffer);
void create(char *, int, int);
void copyfile(int, int);
void extraction(int fdarc, int v);
void extractionNamed(int fdarc, int v , char **argv, int argc);
void getName(char*, header *);
void getVerbose(char *, header *);
void listAll(int, int);
void listNamed(int, int, char **, int);
int isEndTar(char *, int);



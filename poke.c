/* For the size of the file. */
#include <sys/stat.h>
/* This contains the mmap calls. */
#include <sys/mman.h> 
/* These are for error printing. */
#include <errno.h>
#include <string.h>
#include <stdarg.h>
/* This is for open. */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
/* For exit. */
#include <stdlib.h>
/* For the final part of the example. */
#include <ctype.h>


/* "check" checks "test" and prints an error and exits if it is
   true. */
static void
check (int test, const char * message, ...){
    if (test) {
        va_list args;
        va_start (args, message);
        vfprintf (stderr, message, args);
        va_end (args);
        fprintf (stderr, "\n");
        exit (EXIT_FAILURE);
    }
}

char charmap[0xFF];

void printPokeString(const char* str,const char * buff,char *map){
	printf("%s", str);
	int i = 0;
	while(buff[i] != 0x50){
		printf("%c",map[(unsigned char)buff[i]]);
		i++;
	}
	printf("\n");
}

char pokeStabe(char stabe, char *map){
	int i = 0;
	for (i = 0; i <= 0xff; i++){
		if (map[i] == stabe){
			return i;
		}
	}
	return 0xFF;
}

void pokeString(char *map, int offset, char *charmap,const char* str){
	int i = 0;
	while(str[i] != 0){
		map[offset+i] = pokeStabe(str[i],charmap);
		i++;
	}
	map[offset+i] = 0x50;
}

void pokePocket(char *map){
	int offset = 0x25C9;
	int num = map[offset];
	offset++;
	int i;
	printf("There are %i items in players pocket\n", num);
	for (i = 0;i < num; i++){
		printf("Type: %02hhx   count: %02hhx\n",map[offset+i*2],map[offset+i*2+1]);
	}
}

void pokePoecketAdd(char *map, char id, char count){
	int offset = 0x25C9;
	int num = map[offset];
	map[offset]++;
	offset = offset+1+num*2;
	map[offset] = id;
	map[++offset] = count;
	map[++offset] = 0xFF;
}

void pokeTeam(char *map){
	int off = 0x2F2C;
	int offset = 0x2F2C;
	int num = map[offset];
	int i;
	printf("There are %i Pokemons in players team\n", num);
	for (i = 0;i < num; i++){
		printf("Type: %02hhx\n",map[offset+i+1]);
	}
	offset = offset+0x008;
	for (i = 0; i < num; i++){
		printf("Index-ID:\t %02hhx\n", map[offset+i*44]);
		printf("Current HP:\t %hu\n", map[offset+i*44+2]);
		map[offset+i*44+2] = 0xff;
		printf("Level:\t\t %hhx\n",map[offset+i*44+1+3]);
		printf("\n");
	}

	offset = off+0x0110;
	for (i = 0;i < num; i++){
		printPokeString("OT: ",&map[offset+i*11],charmap);
	}
}

unsigned char calcChksum(const char *map){
	int i = 0;
	unsigned char cks = 0;
	for(i = 0x2598; i < 0x3523; i++){
		cks = cks+map[i];
	}
	cks = cks^0xFF;
	return cks;
}

int main (int argc, char *argv[]){
	memset(charmap,'0',0x7F);
	memcpy(&charmap[0x80],"ABCDEFGHIJKLMNOPQRSTUVWXYZ():;[]abcdefghijklmnopqrstuvwxyz~~~~~~",0x3F);
	if(argc < 2) return -1;
    /* The file descriptor. */
    int fd;
    /* Information about the file. */
    struct stat s;
    int status;
    size_t size;
    /* The file name to open. */
    char * file_name = argv[1];
    /* The memory-mapped thing itself. */
    char * mapped;
    int i;

    /* Open the file for reading. */
    fd = open (argv[1], O_RDWR);
    check (fd < 0, "open %s failed: %s", file_name, strerror (errno));

    /* Get the size of the file. */
    status = fstat (fd, & s);
    check (status < 0, "stat %s failed: %s", file_name, strerror (errno));
    size = s.st_size;

    /* Memory-map the file. */
    mapped = mmap (0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    check (mapped == MAP_FAILED, "mmap %s failed: %s", file_name, strerror (errno));

    printf("Size: 0x%x\n", size);
    if (size != 0x8000){
    	printf("Not a valid savefile\n");
    	return -1;
    }
	printPokeString("Trainer name: ",&mapped[0x2598],charmap);
	pokePocket(mapped);
	pokeTeam(mapped);
	//pokePoecketAdd(mapped,0x17,0x01); // would add a item
	//pokePoecketAdd(mapped,0x07,0x01); // would add a item


	pokeString(mapped,0x2598,charmap,"Heinrich");
	mapped[0x2602] = 0xFF; // Badges

    unsigned char cks = calcChksum(mapped);
    printf("Checksum is: %hhx\n", cks);
    mapped[0x3523] = cks;
    if (msync(mapped, size, MS_SYNC) == -1){
        perror("Could not sync the file to disk");
    }
    munmap(mapped,size);
    close(fd);
    return 0;
}

#include "mytar.h"

int main(int argc, char *argv[]){
    int fdarc = 1;
    char character;
    char *file = malloc(MAX_NAME);
    int c = 0, t = 0, x = 0, v = 0, f = 0, S = 0, i = 0;
    char buffer[BUFFER_SIZE];
    
    while((character = argv[1][i++])){
        if(character == 'c'){
            c = 1;
        }
        else if(character == 't'){
            t = 1;
        }
        else if(character == 'x'){
            x = 1;
        }
        else if(character == 'f'){
            f = 1;
        }
        else if(character == 'S'){
            S = 1;
        }
        else if(character == 'v'){
            v = 1;
        }
        else{
            fprintf(stderr, "unrecognized character");
            exit(EXIT_FAILURE);
        }
    }
    if(c == 0 && t == 0 && x == 0){
        fprintf(stderr, "no c or t or x");
        exit(EXIT_FAILURE);
    }
    /* check mode */
    if(c){
        if(f && !argv[3]){
            fprintf(stderr, "no filename");
            exit(EXIT_FAILURE);
        }
        if(!argv[3] || !argv[2]){
            fprintf(stderr, "no filename");
            exit(EXIT_FAILURE);
        }
        if((fdarc = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 
                            S_IRWXU|S_IRWXG|S_IRWXO)) == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
        for(i = 3; argv[i] != NULL; i++){
            if(strlen(argv[i]) <= MAX_NAME){
                memset(file, 0, MAX_NAME);
                memcpy(file, argv[i], MAX_NAME);
                create(file, fdarc, v);
            }
        }
        /*add two null blocks*/
        memset(buffer, 0, BUFFER_SIZE);
        write(fdarc, buffer, BUFFER_SIZE);
        memset(buffer, 0, BUFFER_SIZE);
        write(fdarc, buffer, BUFFER_SIZE);
    }
    else if(x){
        if(f && !argv[2]){
            fprintf(stderr, "no archive file");
            exit(EXIT_FAILURE);
        }
        if((fdarc = open(argv[2], O_RDONLY)) == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
        if(argv[3]){
            extractionNamed(fdarc, v, argv, argc);
        }
        else{
            extraction(fdarc, v);
        }
    }
    else if(t){
        if(!argv[2]){
            fprintf(stderr, "no filename");
            exit(EXIT_FAILURE);
        }
        if((fdarc = open(argv[2], O_RDONLY)) == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
        if(argv[3]) {
            listNamed(fdarc, v, argv, argc);
        }
        else {
            listAll(fdarc, v);
        }
    }
    free(file);
    close(fdarc);
    return 0;
}

void copyfile(int src, int dest){
    int bytesRead;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    while((bytesRead = read(src, buffer, BUFFER_SIZE)) > 0){
        if((write(dest, buffer, BUFFER_SIZE)) == -1){
            /*something went wrong*/
            perror("write");
            exit(EXIT_FAILURE);
        }
    memset(buffer, 0, BUFFER_SIZE);
    }
}

void create(char *filepath, int fdarc, int v){
    DIR *dir;
    struct stat stats;
    struct dirent *entry;
    int fdfile;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if(lstat(filepath, &stats) == -1){
        /*skip paths that aren't readable*/
        return;
    }
    if(S_ISDIR(stats.st_mode)){
        memset(buffer, 0, BUFFER_SIZE);
        createHeader(filepath, buffer);
        if(v){
            printf("%s\n", filepath);
        }
        if((write(fdarc, buffer, BUFFER_SIZE)) == -1){
            perror("write");
            exit(EXIT_FAILURE);
        }
        if(!(dir = opendir(filepath))){
            perror("opendir");
            exit(EXIT_FAILURE);
        }
        while((entry = readdir(dir))){
            if(strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")){
                filepath = strcat(filepath, "/");
                filepath = strcat(filepath, entry->d_name);
                create(filepath, fdarc, v);
                filepath[strlen(filepath) - strlen(entry->d_name) - 1] = '\0';
            }
        }
        closedir(dir);
    }
    else if (S_ISREG(stats.st_mode) || S_ISLNK(stats.st_mode)){
        memset(buffer, 0, BUFFER_SIZE);
        createHeader(filepath, buffer);
        if(v){
            printf("%s\n", filepath);
        }
        if((write(fdarc, buffer, BUFFER_SIZE)) == -1){
            perror("write");
            exit(EXIT_FAILURE);
        }
        /*check*/
        fdfile = open(filepath, O_RDONLY);
        copyfile(fdfile, fdarc);
        close(fdfile);
    }
    else{
        /*not a valid filetype*/
        return;
    }
}
void checkCksum(header *head){
    int i;
    unsigned int sum = 0;
    int chksum;
    for(i = 0; i < sizeof(struct header); i++){
        if((i >= PREFIX_INDEX) && (i <= PREFIX_SIZE)){
            /*do nothing*/
        }
        else{
            unsigned char *byte = ((unsigned char*)head + i);
            sum += *byte;
        }
    }
    sum += MAX_NAME;
    chksum = (unsigned int)strtol(head -> chksum, NULL, BASE_OCTAL);
    if(sum != chksum){
        fprintf(stderr, "invalid checksum");
        exit(EXIT_FAILURE);
    }
}

void createHeader(char *filepath, char *wbuffer){
    struct stat stats;
    struct passwd *pw;
    struct group *grp;
    struct header *head;
    char lname[NAME_SIZE], dir[MAX_NAME + 1];
    int i, len, netint, slash, prefixlen, namelen;
    unsigned int sum = 0;
    
    if(!(head = malloc(sizeof(struct header)))){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(head, 0, sizeof(struct header));
    memset(wbuffer, 0, BUFFER_SIZE);

    if(lstat(filepath, &stats)){
        perror("stat");
        exit(EXIT_FAILURE);
    }
    
    pw = getpwuid(stats.st_uid);
    grp = getgrgid(stats.st_gid);
    
    memset(dir, 0, MAX_NAME + 1);
    sprintf(dir, "%s", filepath);
    len = strlen(dir);

    if(S_ISDIR(stats.st_mode)){
        if(len > 0 && dir[len - 1] != '/'){
            dir[len] = '/';
            dir[len + 1] = '\0';
            len++;
        } 
    }
    /*name and prefix*/
    if(len > NAME_SIZE){
        slash = 0;
        for(i = 0; i < len; i++){
            if(dir[i] == '/'){
                slash = i;
                if(len - slash < NAME_SIZE){
                    break;
                }
            }
        }
        if(slash >= 0){
            prefixlen = slash + 1;
            namelen = len - prefixlen;

            if(prefixlen > PREFIX_SIZE){
                fprintf(stderr, "prefix too long");
                exit(EXIT_FAILURE);
            }
            if(namelen > NAME_SIZE){
                fprintf(stderr, "name too long");
                exit(EXIT_FAILURE);
            }
            if(namelen + prefixlen > MAX_NAME){
                fprintf(stderr, "path too long");
                exit(EXIT_FAILURE);
            }
            
            memset(head -> prefix, 0, PREFIX_SIZE);
            memset(head -> name, 0, NAME_SIZE);

            memcpy(head -> prefix, dir, prefixlen);
            memcpy(head -> name, dir + prefixlen, namelen);

            if(head -> prefix[strlen(head -> prefix) - 1] == '/'){
                head -> prefix[strlen(head -> prefix) - 1] = '\0';
            }
        }
    }
    else{
        memset(head -> name, 0, NAME_SIZE);
        memcpy(head -> name, dir, NAME_SIZE);  
    }
    /*mode*/
    snprintf(head -> mode, SIZE_LENGTH, "%07o", (unsigned int)stats.st_mode);
    /*uid*/
    if(stats.st_uid > OCTAL_MAX){
        netint = htonl((uint32_t)stats.st_uid);
        head -> uid[0] = (unsigned char)0x80;
        *(int*)(head -> uid + SPECIALHEX_SEPARATOR) = netint;
    }
    else{
        memset(head -> uid, '0', sizeof(head -> uid));
        snprintf(head -> uid, SIZE_LENGTH, "%07o", (unsigned int)stats.st_uid);
    }
    /*gid*/
    if(stats.st_gid > OCTAL_MAX){
        netint = htonl((uint32_t)stats.st_gid);
        head -> gid[0] = (unsigned char)0x80;
        *(int*)(head -> uid + SPECIALHEX_SEPARATOR) = netint;
    }
    else{
        memset(head -> gid, '0', sizeof(head -> gid));
        snprintf(head -> gid, SIZE_LENGTH, "%07o", (unsigned int)stats.st_gid);
    }
    /*size*/
    memset(head -> size, '0', sizeof(head -> size) - 1);
    if(S_ISREG(stats.st_mode)){
        snprintf(head -> size, 12, "%011o", (unsigned int)stats.st_size);
    }
    /*mtime*/
    snprintf(head -> mtime, 12, "%011o", (unsigned int)stats.st_mtime);
    /*get typeflag*/
    if(S_ISREG(stats.st_mode)){
        sprintf(head -> typeflag, "0");
    }
    else if(S_ISDIR(stats.st_mode)){
        sprintf(head -> typeflag, "5");
        readlink(filepath, head -> linkname, sizeof(head -> linkname) - 1);
    }
    else if(S_ISLNK(stats.st_mode)){
        sprintf(head -> typeflag, "2");
    }
    /*linkname*/
    if(S_ISLNK(stats.st_mode)){
        readlink(filepath, lname, sizeof(lname));
        snprintf(head -> linkname, NAME_SIZE, "%s", lname);
        if(strlen(lname) >= NAME_SIZE){
            head -> linkname[NAME_SIZE - 1] = lname[NAME_SIZE - 1];
        }
    }
    /*magic*/
    sprintf(head -> magic, "ustar");
    /*version*/
    sprintf(head -> version, "00");
    /*uname*/
    snprintf(head -> uname, USERNAME_SIZE, "%s", pw -> pw_name);
    /*gname*/
    snprintf(head -> gname, USERNAME_SIZE, "%s", grp -> gr_name);

    /*now find chksum*/
    for(i = 0; i < sizeof(struct header); i++){
        unsigned char *byte = ((unsigned char*)head + i);
        sum += *byte;
    }
    sum += MAX_NAME; 
    memset(head -> chksum, 0, BASE_OCTAL);
    snprintf(head -> chksum, SIZE_LENGTH, "%07o", sum);
    
    /*write to the buffer*/
    for(i = 0; i < sizeof(struct header); i++){
        char byte = *((char*)head + i);
        wbuffer[i] = byte;
    }
    free(head);
}

/*Extract files named in command link*/
void extractionNamed(int fdarc, int v, char *argv[], int numArgs){
    struct header *head;
    struct utimbuf times;
    char buffer[BUFFER_SIZE];
    char path[MAX_NAME], arg[MAX_NAME], fullname[MAX_NAME];
    int i, fd, octal, bytes_read, total_bytes, mtime, write_bytes;
    int flag = 0;
    
    memset(buffer, 0, BUFFER_SIZE);
    bytes_read = read(fdarc, buffer, BUFFER_SIZE);

    head = (header*)buffer;
    if(isEndTar(buffer, fdarc)){
        return;
    }
    checkCksum(head);
    getName(path, head); 

    /*command argument is in the path || path is in command argument*/
    for(i = 3; i < numArgs; i++){
        memset(arg, 0, MAX_NAME);
        if(!memcmp(argv[i], path, strlen(argv[i]))){
            memcpy(arg, argv[i], strlen(argv[i]));
            flag = 1;
            break;
        }
        if(!memcmp(path, argv[i], strlen(path))){
            memcpy(arg, argv[i], strlen(argv[i]));
            flag = 1;
            break;
        }
    }
    /*is regular file*/
    if(*head -> typeflag == '0' || *head -> typeflag == '\0'){
        if(flag){
            if((fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 
                S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH|S_IRGRP|S_IWGRP)) == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            if(v){
                getName(fullname, head);
                printf("%s\n", fullname);
            }
            /*Give everyone execute permissions if anyone has execute set*/
            octal = strtol(head -> mode, NULL, BASE_OCTAL);
            if((octal & S_IXOTH) || (octal & S_IXUSR) || (octal & S_IXGRP)){
                if(chmod(path, S_IRWXU|S_IRWXG|S_IRWXO) == 1){
                    perror("chmod");
                    exit(EXIT_FAILURE);
                }   
            }
            /*getting mtime*/
            mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
        }
        /*extracting file contents*/
        octal = strtol(head -> size, NULL, BASE_OCTAL);
        total_bytes = 0;
        memset(buffer, 0, BUFFER_SIZE);

        while(total_bytes < (int)octal && (bytes_read = read(fdarc, buffer, 
                                                        BUFFER_SIZE)) > 0){
            if(flag){
                write_bytes = (total_bytes + bytes_read <= 
                (int)octal ? bytes_read : (int)octal - total_bytes);

                write(fd, buffer, write_bytes);
            }
            total_bytes += bytes_read;
        }
        if(flag){
            if(close(fd) == -1){
                perror("close");
                exit(EXIT_FAILURE);
            }

            /*restoring mtime*/
            times.actime = time(NULL);
            times.modtime = (int)mtime;
            utime(path, &times);
        }
    }   
    /*is symbolic link*/
    else if(*head -> typeflag == '2'){
        if(flag){
            if((symlink(head -> linkname, path)) == -1){
                printf("%s: File does not exist\n", path);
            }
            else{
                if(v){
                    printf("%s\n", path);
                }       
                printf("%s: File exists\n", path);
        
                /*restoring mtime*/
                mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
                times.actime = time(NULL);
                times.modtime = (int)mtime;
                utime(path, &times);
            }
        }
    }
    /*is directory*/
    else if(*head -> typeflag == '5'){
        if(flag){
            if(mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO) == -1){
                perror("mkdir");
                exit(EXIT_FAILURE);
            }   
            if(v){
                getName(fullname, head);
                if(strlen(fullname) >= strlen(arg)){
                    printf("%s\n", fullname);
                }
            }
            /*restoring mtime*/
            mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
            times.actime = time(NULL);
            times.modtime = (int)mtime;
            utime(path, &times);
        }
    }
    else {
        fprintf(stderr, "typeflag not supported");
        exit(EXIT_FAILURE);
    }
    extractionNamed(fdarc, v, argv, numArgs);
}

/*Extract all files*/
void extraction(int fdarc, int v){
    struct header *head;
    struct utimbuf times;
    char buffer[BUFFER_SIZE], path[MAX_NAME];
    int fd, octal, bytes_read, total_bytes, mtime, write_bytes;

    memset(buffer, 0, BUFFER_SIZE);
    bytes_read = read(fdarc, buffer, BUFFER_SIZE);

    head = (header*)buffer;
    if(isEndTar(buffer, fdarc)){
        return;
    }
    checkCksum(head);
    getName(path, head);

    /*is regular file*/
    if(*head -> typeflag == '0' 
        || *head -> typeflag == '\0'){
        if((fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 
            S_IRUSR|S_IWUSR|S_IROTH|S_IWOTH|S_IRGRP|S_IWGRP)) == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
        if(v){
        printf("%s\n", path);
        }
        
        /*Give everyone execute permissions if anyone has execute set*/
        octal = strtol(head -> mode, NULL, BASE_OCTAL);
        if((octal & S_IXOTH) || (octal & S_IXUSR) || (octal & S_IXGRP)){
            if(chmod(path, S_IRWXU|S_IRWXG|S_IRWXO) == 1){
                perror("chmod");
                exit(EXIT_FAILURE);
            }   
        }

        /*getting mtime*/
        mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
    
        /*extracting file contents*/
        octal = strtol(head -> size, NULL, BASE_OCTAL);
        total_bytes = 0;
        memset(buffer, 0, BUFFER_SIZE);
        while(total_bytes < (int)octal && (bytes_read = read(fdarc, buffer, 
                                                        BUFFER_SIZE)) > 0){
            write_bytes = (total_bytes + bytes_read <= 
            (int)octal ? bytes_read : (int)octal - total_bytes);

            write(fd, buffer, write_bytes);
            total_bytes += bytes_read;
        }
        if(close(fd) == -1){
            perror("close");
            exit(EXIT_FAILURE);
        }
        
        /*restoring mtime*/
        times.actime = time(NULL);
        times.modtime = (int)mtime;
        utime(path, &times);
    }
    /*is symbolic link*/
    else if(*head -> typeflag == '2'){
        if((symlink(head -> linkname, path)) == -1){
            printf("%s: File does not exist\n", path);
        }
        else{
            if(v){
                printf("%s\n", path);
            }       
            printf("%s: File exists\n", path);
        
            /*restoring mtime*/
            mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
            times.actime = time(NULL);
            times.modtime = (int)mtime;
            utime(path, &times);
        }
    }
    /*is directory*/
    else if(*head -> typeflag == '5'){
        if(mkdir(path, S_IRWXU|S_IRWXG|S_IRWXO) == -1){
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
        if(v){
            printf("%s\n", path);
        }
        
        /*restoring mtime*/
        mtime = strtol(head -> mtime, NULL, BASE_OCTAL);
        times.actime = time(NULL);
        times.modtime = (int)mtime;
        utime(path, &times);
    }
    else {
        fprintf(stderr, "typeflag not supported");
        exit(EXIT_FAILURE);
    }
    extraction(fdarc, v);
}

/*list all files*/
void listAll(int fdtar, int v){
    char buffer[BUFFER_SIZE];
    char name[MAX_NAME + 1];
    header *head;
    int bytesRead;
    double blocks;
    int nameoffset = 0;

    /*get headers*/
    memset(buffer, 0, BUFFER_SIZE);
    while((bytesRead = read(fdtar, buffer, BUFFER_SIZE)) > 0){
        if(isEndTar(buffer, fdtar)){
            return;
        }
        else{
            head = (header*)buffer;
            checkCksum(head);

            if(strlen(head -> uname) + 1 + strlen(head -> gname) > 17){
                nameoffset = strlen(head -> uname) + strlen(head->gname) - 16;
            }
            char verb[MAX_VERBOSE_LIST + nameoffset];

            if(v){
                getVerbose(verb, head);
                printf("%s\n", verb);
            }
            else {
                getName(name, head);
                printf("%s\n", name);
            }
            blocks = ((int)strtol(head -> size, NULL, BASE_OCTAL) + 
                                BUFFER_SIZE - 1) / BUFFER_SIZE;
            lseek(fdtar, (off_t)blocks * BUFFER_SIZE, SEEK_CUR);
        }
    }
}

/*list files named in command line*/
void listNamed(int fdtar, int v, char *args[], int numArgs){
    char buffer[BUFFER_SIZE];
    char name[MAX_NAME + 1];
    header *head;
    int bytesRead;
    double blocks;
    int nameoffset = 0, flag, i;

    /*get headers*/
    memset(buffer, 0, BUFFER_SIZE);
    while((bytesRead = read(fdtar, buffer, BUFFER_SIZE)) > 0){
        flag = 0;
        if(isEndTar(buffer, fdtar)){
            return;
        }
        else{
            head = (header*)buffer;
            checkCksum(head);
            getName(name, head);

            /*Check if file was name in command line args*/
            for(i = 3; i < numArgs; i++){
                if(!memcmp(args[i], name, strlen(args[i]))) {
                    flag = 1;
                    break;
                }
            }
            /*File is a command line arg*/
            if(flag) {
                if(strlen(head -> uname) + 1 + strlen(head -> gname) > 17){
                    nameoffset = strlen(head->uname) + strlen(head->gname) - 16;
                }
                char verb[MAX_VERBOSE_LIST + nameoffset];

                if(v){
                    getVerbose(verb, head);
                    printf("%s\n", verb);
                }
                else {
                    getName(name, head);
                    printf("%s\n", name);
                }
            }
            blocks = ((int)strtol(head -> size, NULL, BASE_OCTAL) + 
                            BUFFER_SIZE - 1) / BUFFER_SIZE;
            lseek(fdtar, (off_t)blocks * BUFFER_SIZE, SEEK_CUR);
        }
    }
}

/*Put full filepath into path*/
void getName(char *path, header *head){
    char name[NAME_SIZE + 1];
    char prefix[PREFIX_SIZE + 1];

    memset(path, 0, MAX_NAME);
    memset(prefix, 0, PREFIX_SIZE + 1);
    memset(name, 0, NAME_SIZE + 1);

    memcpy(prefix, head -> prefix, PREFIX_SIZE);
    memcpy(name, head -> name, NAME_SIZE);

    if(strlen(prefix)){
        sprintf(path, "%s/%s", prefix, name);
    }
    else{
        sprintf(path, "%s", name);
    }
}

/*Do verbose listing*/
void getVerbose(char *verb, header *head){
    char name[MAX_NAME];
    char octalstr[OCTAL_LENGTH + 1];
    char intstr[SIZE_LENGTH];
    char timestr[TIME_LENGTH + 1];
    int padding, nameoverflow = 0;
    time_t mtime;
    struct tm *timeInfo;
    int permissions;

    memset(verb, '-', PERMISSION_SIZE);
    if(strlen(head -> uname) + 1 + strlen(head -> gname) > 17){
        nameoverflow = strlen(head -> uname) + strlen(head -> gname) - 16;
    }

    /*permissions*/
    if(*head -> typeflag == '5'){
        *verb = 'd';
    }
    else if(*head -> typeflag == '2'){
        *verb = '1';
    }
    permissions = (int)strtol(head -> mode, NULL, BASE_OCTAL);
    if(permissions & S_IRUSR){
        verb[1] = 'r';
    }
    if(permissions & S_IWUSR){
        verb[2] = 'w';
    }
    if(permissions & S_IXUSR){
        verb[3] = 'x';
    }
    if(permissions & S_IRGRP){
        verb[4] = 'r';
    }
    if(permissions & S_IWGRP){
        verb[5] = 'w';
    }
    if(permissions & S_IXGRP){
        verb[6] = 'x';
    }
    if(permissions & S_IROTH){
        verb[7] = 'r';
    }
    if(permissions & S_IWOTH){
        verb[8] = 'w';
    }
    if(permissions & S_IXOTH){
        verb[9] = 'x';
    }

    /*uname and gname */
    memset(verb + PERMISSION_SIZE, ' ', NAME_OFFSET - PERMISSION_SIZE);
    strcpy(verb + OWNER_OFFSET, head -> uname);
    verb[OWNER_OFFSET + strlen(head -> uname)] = '/';
    strcpy(verb + OWNER_OFFSET + strlen(head -> uname) + 1, head -> gname);
    verb[SIZE_OFFSET + nameoverflow - 1] = ' ';
    if(strlen(head->uname) + 1 + strlen(head->gname) < 17) {
        memset(verb + OWNER_OFFSET + strlen(head->uname) + 1 + 
        strlen(head->gname), ' ', 17 - (strlen(head->uname + 1 + 
        strlen(head->gname))));
    }

    /*file size*/
    memcpy(octalstr, head -> size, OCTAL_LENGTH);
    octalstr[OCTAL_LENGTH] = '\0';
    sprintf(intstr, "%ld", strtol(octalstr, NULL, 8));
    padding = SIZE_LENGTH - strlen(intstr);
    if(padding > 0) {
        sprintf(verb+SIZE_OFFSET+nameoverflow, "%*s%s", padding, "", intstr);
    }
    else {
        sprintf(verb + SIZE_OFFSET + nameoverflow, "%s", intstr);
    }

    /*mtime*/
    memcpy(timestr, head -> mtime, OCTAL_LENGTH);
    mtime = (time_t)strtol(timestr, NULL, BASE_OCTAL);
    timeInfo = localtime(&mtime);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M", timeInfo);
    memcpy(verb + MTIME_OFFSET + nameoverflow, timestr, TIME_LENGTH);
    verb[SIZE_OFFSET + SIZE_LENGTH + nameoverflow] = ' ';
    verb[MTIME_OFFSET + MTIME_LENGTH + nameoverflow] = ' ';

    /* file name */
    getName(name, head);
    strcpy(verb + NAME_OFFSET + nameoverflow, name);
}

/*Check if Tar archive has ended*/
int isEndTar(char *buffer, int fdarc){
    int i;
    for(i = 0; i < BUFFER_SIZE; i++){
        if(buffer[i] != '\0'){
            return 0;
        }
    }
    read(fdarc, buffer, BUFFER_SIZE);
    for(i = 0; i < BUFFER_SIZE; i++){
        if(buffer[i] != '\0'){
            lseek(fdarc, -BUFFER_SIZE, SEEK_CUR);
            return 0;
        }
    }
    if(close(fdarc) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    read(fdarc, buffer, BUFFER_SIZE);
    for(i = 0; i < BUFFER_SIZE; i++){
        if(buffer[i] != '\0'){
            lseek(fdarc, -BUFFER_SIZE, SEEK_CUR);
            return 0;
        }
    }
    if(close(fdarc) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    return 1;
}




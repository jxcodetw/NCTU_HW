#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>

#define NUM_SYM 15
#define MAX_SYM_LEN 20
#define MAX_FILE_IN_FOLDER 200
#define BUF_LEN 1000

char sym[NUM_SYM][MAX_SYM_LEN] = {
	"cat", "cd", "chmod", "echo", "exit", "find", "help", "id", "mkdir", "pwd", "rm", "rmdir", "stat", "touch", "umask"
};
typedef enum {
	CAT = 0, CD, CHMOD, ECHO, EXIT, FIND, HELP, ID, MKDIR, PWD, RM, RMDIR, STAT, TOUCH, UMASK, BAD
} TOKEN;

char line[BUF_LEN], buf[BUF_LEN], cwd[BUF_LEN];
size_t tk_top = 0, tk_end;
struct dirent *ep[MAX_FILE_IN_FOLDER]; // not need to free

// ---- helper functions ----

char* next_str() {
	int idx = 0;
	// eat space
	while(line[tk_top] == ' ') {
		tk_top++;
	}

	while(line[tk_top] != ' ' && line[tk_top] != '\n') {
		buf[idx++] = line[tk_top++];
	}
	buf[idx] = '\0';
	return buf;
}

TOKEN next_token() {
	char *cmd = next_str();
	for(int i=0;i<NUM_SYM;++i) {
		if (strcmp(cmd, sym[i]) == 0) {
			return i;
		}
	}
	return BAD;
}

mode_t parse_mode(char *mode_str) {
	mode_t mode = 0;
	for(int i=0;i<strlen(mode_str);++i) {
		mode <<= 3;
		mode |= (mode_str[i]-'0');
	}
	return mode;
}

void print_mode_rwx(mode_t mode) {
	printf((S_ISDIR(mode)) ? "d" : "-");
	printf((mode & S_IRUSR) ? "r" : "-");
	printf((mode & S_IWUSR) ? "w" : "-");
	printf((mode & S_ISUID) ? "s" : ((mode & S_IXUSR) ? "x" : "-"));
	printf((mode & S_IRGRP) ? "r" : "-");
	printf((mode & S_IWGRP) ? "w" : "-");
	printf((mode & S_ISGID) ? "s" : ((mode & S_IXGRP) ? "x" : "-"));
	printf((mode & S_IROTH) ? "r" : "-");
	printf((mode & S_IWOTH) ? "w" : "-");
	printf((mode & S_ISVTX) ? "t" : ((mode & S_IXOTH) ? "x" : "-"));
}

void find_detail(char *dirpath, char *filename) {
	struct stat st;
	size_t path_len = strlen(dirpath)+strlen(filename);

	// concate full path
	char *path = malloc(path_len);
	strncpy(path, dirpath, path_len);
	strncat(path, filename, path_len);
	if (stat(path, &st) != 0) {
		perror("find stat() error");
	}

	print_mode_rwx(st.st_mode);

	const char* units[] = {"B", "K", "M", "G", "T"};
	int i_unit = 0;
	double size = (float)st.st_size;
	while (size > 1024) {
		size /= 1024;
		i_unit++;
	}
	printf(" %5.1lf %s %s\n", size, units[i_unit], filename);
}

int find_compare(const void *a, const void *b)
{
	return strcmp((*(struct dirent **)a)->d_name, (*(struct dirent **)b)->d_name);
}

// ----- implement commands -----

void builtin_cat() {
	char *filename = next_str();
	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("cat fopen() error");
		return;
	}
	char c;
	while((c = getc(fp)) != EOF) {
		putchar(c);
	}
	fclose(fp);
}

void builtin_cd() {
	char *path = next_str();
	int ret = chdir(path);
	if (ret != 0) {
		puts("cd chdir() error");
	}
}

void builtin_chmod() {
	mode_t mode = parse_mode(next_str());
	char *path = next_str();
	if (chmod(path, mode) != 0) {
		perror("chmod chmod() error");
	}
}

void builtin_echo() {
	char *msg = strdup(next_str());
	char *filename = next_str();
	if (filename[0] == '\0') {
		puts(msg);
	} else {
		FILE *fp = fopen(filename, "a");
		if (fp == NULL) {
			perror("echo fopen() error");
			return;
		}

		for(int i=0;i<strlen(msg);++i) {
			fputc(msg[i], fp);
		}
		fputc('\n', fp);
		fclose(fp);
	}
	free(msg);
}

void builtin_find() {
	char *path = next_str();
	if (path[0] == '\0') {
		path[0] = '.';
		path[1] = '\0';
	}
	DIR *dp = opendir(path);
	if (dp == NULL) {
		perror("find opendir() error");
		return;
	}
	int idx = 0;
	errno = 0;
	while(ep[idx] = readdir(dp)) {
		if (errno != 0) {
			perror("find readdir() error");
			closedir(dp);
			return;
		}
		idx++;
	}
	// sort filenames
	qsort(&ep, idx, sizeof(struct dirent*), find_compare);

	// check path ends with /
	size_t path_len = strlen(path);
	if (path[path_len-1] != '/') {
		path[path_len] = '/';
		path[path_len+1] = '\0';
	}

	for(int i=0;i<idx;++i) {
		find_detail(path, ep[i]->d_name);
	}
	// if you use ep[i] it cannot be closed
	// here we are safe to close
	closedir(dp);
}

void builtin_help() {
	static char *help_msg =
"cat {file}:              Display content of {file}.\n"
"cd {dir}:                Switch current working directory to {dir}.\n"
"chmod {mode} {file/dir}: Change the mode (permission) of a file or directory.\n"
"                         {mode} is an octal number.\n"
"                         Please do not follow symbolc links.\n"
"echo {str} [filename]:   Display {str}. If [filename] is given,\n"
"                         open [filename] and append {str} to the file.\n"
"exit:                    Leave the shell.\n"
"find [dir]:              List files/dirs in the current working directory\n"
"                         or [dir] if it is given.\n"
"                         Minimum outputs contatin file type, size, and name.\n"
"help:                    Display help message.\n"
"id:                      Show current euid and egid.\n"
"mkdir {dir}:             Create a new directory {dir}.\n"
"pwd:                     Print the current working directory.\n"
"rm {file}:               Remove a file.\n"
"rmdir {dir}:             Remove an empty directory.\n"
"stat {file/dir}:         Display detailed information of the given file/dir.\n"
"touch {file}:            Create {file} if it does not exist,\n"
"                         or update its access and modification timestamp.\n"
"umask {mode}:            Change the umask of the current session.\n";
	puts(help_msg);
}

void builtin_id() {
	uid_t uid = geteuid();
	gid_t gid = getegid();

#ifdef RESOLVE_USER_NAME
	struct passwd *p = getpwuid(uid);
	struct group *g = getgrgid(gid);
	printf("euid=%d(%s) egid=%d(%s)\n", uid, p->pw_name, gid, g->gr_name);
#else
	printf("euid=%d egid=%d\n", uid, gid);
#endif
}

void builtin_mkdir() {
	char *path = next_str();
	if (mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) != 0) {
		perror("mkdir mkdir() error");
	}
}

void builtin_pwd() {
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n", cwd);
	} else {
		perror("pwd getcwd() error");
	}
}

void builtin_rm() {
	char *path = next_str();
	if (unlink(path) != 0) {
		perror("rm unlink() error");
	}
}

void builtin_rmdir() {
	char *path = next_str();
	if (rmdir(path) != 0) {
		perror("rmdir rmdir() error");
	}
}

void print_mode_oct(mode_t mode) {
	mode = mode & 0x00000FFF;
	unsigned char digit[4] = {0};
	int idx = 0;
	while(mode != 0) {
		digit[idx++] = mode % 8;
		mode /= 8;
	}
	for(int i=3;i>=0;--i) {
		printf("%c", digit[i] + '0');
	}
}

void builtin_stat() {
	char *path = next_str();
	struct stat st;
	if (stat(path, &st) != 0) {
		perror("stat stat() error");
	}

	printf("  File: \'%s\'\n", path);
	printf("  Size: %ld\t\t Blocks: %ld\t\tIO Block: %ld\t%s\n", st.st_size, st.st_blocks, st.st_blksize, (S_ISDIR(st.st_mode)) ? "directory" : "regular file");
	printf("Device: %xh/%lud\t Inode: %lu\tLinks: %lu\n", (int)st.st_dev, st.st_dev, st.st_ino, st.st_nlink);
	printf("Access: (");
	print_mode_oct(st.st_mode);printf("/");print_mode_rwx(st.st_mode);
#ifdef RESOLVE_USER_NAME
	struct passwd *p = getpwuid(st.st_uid);
	struct group *g = getgrgid(st.st_gid);
	printf(") Uid: (%d/%s)\tGid: (%d/%s)\n", st.st_uid, p->pw_name, st.st_gid, g->gr_name);
#else
	printf(") Uid: (%d)\t\tGid: (%d)\n", st.st_uid, st.st_gid);
#endif
	printf("Access: %s", ctime(&(st.st_atime)));
	printf("Modify: %s", ctime(&(st.st_mtime)));
	printf("Change: %s", ctime(&(st.st_ctime)));
	printf(" Birth: -\n");
}

void builtin_touch() {
	char *path = next_str();
	// create empty file
	FILE *fp = fopen(path, "a");
	if (fp == NULL) {
		perror("touch fopen() error");
	}
	fclose(fp);
	// set current time
	struct utimbuf _utimbuf;
	time_t rawtime;
	time(&rawtime);
	_utimbuf.actime = rawtime;
	_utimbuf.modtime = rawtime;
	if (utime(path, &_utimbuf) != 0) {
		perror("touch utime() error");
	}
}

void builtin_umask() {
	mode_t mode = parse_mode(next_str());
	umask(mode); // always success
}

char* prompt() {
	printf("$ ");
	char* ret = fgets(line, 1000, stdin);
	size_t len = strlen(line);

	// handle f**king telnet
	if (line[len-2] == '\r') {
		line[len-2] = '\n';
		line[len-1] = '\0';
	}
	return ret;
}

int main(int argc, char *argv[]) {
	setlinebuf(stdin);
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc != 3) {
		printf("Usage: %s euid egid\n", argv[0]);
		return -1;
	}

	uid_t euid;
	gid_t egid;
	if (sscanf(argv[1], "%d", &euid) == EOF && errno != 0) {
		perror("parse euid sscanf() error");
		return -1;
	}
	if (sscanf(argv[2], "%d", &egid) == EOF && errno != 0) {
		perror("parse egid sscanf() error");
		return -1;
	}
#ifdef DEBUG
	printf("DEBUG: %d, %d\n", euid, egid);
#endif
	if (setegid(egid) != 0) {
		perror("main setegid() error");
		return -1;
	}

	if (seteuid(euid) != 0) {
		perror("main seteuid() error");
		return -1;
	}


	while(prompt()) {
		tk_end = strlen(line);
		tk_top = 0;
		TOKEN token = next_token();
		switch(token) {
			case CAT:   builtin_cat();   break;
			case CD:    builtin_cd();    break;
			case CHMOD: builtin_chmod(); break;
			case ECHO:  builtin_echo();  break;
			case EXIT:  puts("bye~");    return 0;
			case FIND:  builtin_find();  break;
			case HELP:  builtin_help();  break;
			case ID:    builtin_id();    break;
			case MKDIR: builtin_mkdir(); break;
			case PWD:   builtin_pwd();   break;
			case RM:    builtin_rm();    break;
			case RMDIR: builtin_rmdir(); break;
			case STAT:  builtin_stat();  break;
			case TOUCH: builtin_touch(); break;
			case UMASK: builtin_umask(); break;
			default:
				puts("what???");
		}
	}
	return 0;
}

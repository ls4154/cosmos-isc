/*
 * read SSD data from host
 * host_lba_reader <FILE> <LBA> <BYTES>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SECT_SIZE 4096
char *fname;
int lba;
int size;
char buf[65536];

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <FILE> <LBA> <BYTES>\n", argv[0]);
		return 1;
	}
	fname = argv[1];
	lba = atoi(argv[2]);
	size = atoi(argv[3]);

	int fd_output = 1;
	if (strcmp("-", fname) != 0)
		fd_output = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd_output == -1) {
		perror("output open");
		return 1;
	}

	int fd_nvme = open("/dev/nvme0n1", O_RDONLY);
	if (fd_nvme == -1) {
		perror("nvme open");
		return 1;
	}
	lseek(fd_nvme, lba * SECT_SIZE, SEEK_SET);

	int rd = 0, wr, wtemp;
	int rtotal = 0;
	while (rtotal < size) {
		rd = read(fd_nvme, buf, min(sizeof(buf), size - rtotal));
		if (rd == 0)
			break;
		if (rd < 0) {
			perror("read");
			return 1;
		}
		wr = 0;
		while (wr < rd) {
			wtemp = write(fd_output, buf + wr, rd - wr);
			if (wtemp < 0) {
				perror("write");
				return 1;
			}
			wr += wtemp;
		}
		rtotal += wr;
	}
	fprintf(stderr, "%d bytes read\n", rtotal);
	close(fd_output);
	close(fd_nvme);
	return 0;
}

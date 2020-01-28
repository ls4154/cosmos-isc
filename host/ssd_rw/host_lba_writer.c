/*
 * write data to SSD from host
 * host_lba_reader <FILE> <LBA> <BYTES>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SECT_SIZE 4096
char *fname;
int lba;
char buf[65536];

int main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: %s <FILE> <LBA>\n", argv[0]);
		return 1;
	}
	fname = argv[1];
	lba = atoi(argv[2]);

	int fd_input = open(fname, O_RDONLY);
	if (fd_input == -1) {
		perror("input open");
		return 1;
	}
	int fd_nvme = open("/dev/nvme0n1", O_RDWR);
	if (fd_nvme == -1) {
		perror("nvme open");
		return 1;
	}
	lseek(fd_nvme, lba * SECT_SIZE, SEEK_SET);

	int rd, wr, wtemp;
	int wtotal = 0;
	while (1) {
		rd = read(fd_input, buf, sizeof(buf));
		if (rd == 0)
			break;
		if (rd < 0) {
			perror("read");
			return 1;
		}
		wr = 0;
		while (wr < rd) {
			wtemp = write(fd_nvme, buf + wr, rd - wr);
			if (wtemp < 0) {
				perror("write");
				return 1;
			}
			wr += wtemp;
		}
		wtotal += wr;
	}
	printf("%d byted write\n", wtotal);
	return 0;
}

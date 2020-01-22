#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <isc_ssd.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))

char *FNAME;
int LBA;
int SIZE; // file size in bytes
char buf[65536];

const int WRITE_CHUNK = 1048576;

int main(int argc, char **argv)
{
	isc_init_shared();

	if (argc < 3) {
		fprintf(stderr, "usage: %s <FILE> <LBA>\n", argv[0]);
		return 1;
	}
	FNAME = argv[1];
	LBA = atoi(argv[2]);

	int fd = open(FNAME, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	SIZE = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	fprintf(stderr, "lba: %d\n", LBA);
	fprintf(stderr, "size : %d bytes\n", SIZE);

	int left = SIZE;
	int lba = LBA;
	int write_size = MIN(WRITE_CHUNK, left);

	if (read(fd, isc_shared[0].payload_write, write_size) < write_size) {
		perror("read");
		return 1;
	}
	isc_write_request(0, lba, write_size);

	for (int i = 0; left > 0; i++) {
		int idx = i % 2;

		while (!isc_check_done(idx));

		left -= write_size;
		lba += to_sect(write_size);
		write_size = MIN(WRITE_CHUNK, left);

		if (left <= 0)
			goto print;

		if (read(fd, isc_shared[!idx].payload_write, write_size) < write_size) {
			perror("read");
			return 1;
		}
		isc_write_request(!idx, lba, write_size);
print:
		;
	}
	return 0;
}

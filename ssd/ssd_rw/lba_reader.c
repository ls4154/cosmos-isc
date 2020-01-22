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
int fd = STDOUT_FILENO;

const int READ_CHUNK = 1048576;

static void process(char *payload, int payload_len)
{
	if (write(fd, payload, payload_len) < payload_len) {
		perror("write");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	isc_init_shared();

	if (argc < 4) {
		fprintf(stderr, "usage: %s <FILE> <LBA> <BYTES>\n", argv[0]);
		return 1;
	}
	FNAME = argv[1];
	if (strcmp(FNAME, "-")) {
		fd = open(FNAME, O_RDWR | O_CREAT | O_TRUNC, 0644);
		if (fd < 0) {
			perror("open");
			return 1;
		}
	}
	LBA = atoi(argv[2]);
	SIZE = atoi(argv[3]);

	fprintf(stderr, "lba: %d\n", LBA);
	fprintf(stderr, "size : %d bytes\n", SIZE);

	int left = SIZE;
	int lba = LBA;
	int read_size = MIN(READ_CHUNK, left);

	memset(isc_shared[0].payload, 0, sizeof(isc_shared[0].payload));
	isc_read_request(0, lba, read_size);

	for (int i = 0, j; left > 0; i++) {
		int idx = i % 2;

		while (!isc_check_done(idx));

		int cur_read_size = read_size;

		left -= read_size;
		lba += toSect(read_size);
		read_size = MIN(READ_CHUNK, left);

		if (left <= 0)
			goto print;

		memset(isc_shared[!idx].payload, 0, sizeof(isc_shared[!idx].payload));
		isc_read_request(!idx, lba, read_size);
print:
		for (j = 0; j < toSect(cur_read_size)-1; j++)
			process(isc_payload(idx, j), SECTSIZE);
		if (cur_read_size % SECTSIZE > 0)
			process(isc_payload(idx, j), cur_read_size % SECTSIZE);
		else
			process(isc_payload(idx, j), SECTSIZE);
	}
	return 0;
}

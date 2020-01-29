#include <cstdio>
#include <string>
#include <cstdlib>
#include <map>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

static int READ_CHUNK = 1048576; // 1 MiB
static int FILESIZE, LBA, LBA_RESULT;

static void process(char *payload, int payload_len);
static void print_result(void);

char nvme_dev[64] = "/dev/nvme0n1";
int nvme_fd;

char *buf;

#define SECTSIZE 4096
static inline int to_sect(int bytes)
{
		return (bytes + SECTSIZE - 1) / SECTSIZE;
}

int main(int argc, char **argv)
{
	if (argc < 4) {
		fprintf(stderr, "usage: %s <FILESIZE> <LBA> <LBA_RESULT>\n", argv[0]);
		fprintf(stderr, "use LBA_RESULT 0 to print stdout\n");
		return 1;
	}
	FILESIZE = atoi(argv[1]);
	LBA = atoi(argv[2]);
	LBA_RESULT = atoi(argv[3]);

	fprintf(stderr, "file size: %d\n", FILESIZE);
	fprintf(stderr, "lba: %d\n", LBA);
	fprintf(stderr, "lba result: %d\n", LBA_RESULT);

	nvme_fd = open(nvme_dev, O_RDWR);
	if (nvme_fd < 0) {
		perror("open nvme");
		return 1;
	}

	buf = (char *)malloc(FILESIZE + 10);
	if (buf == NULL) {
		perror("malloc");
		return 1;
	}

	lseek(nvme_fd, LBA, SEEK_SET);

	for (int total = 0; total < FILESIZE; )
		total += read(nvme_fd, buf, FILESIZE - total);

	process(buf, FILESIZE);

	free(buf);
	print_result();
}

static map<string, int> mp;
static inline bool delim(char ch)
{
	return ch == ' ' || ch == '\n' || ch == '.';
}
static inline void put_word(char *word)
{
	if (mp.count(word)) mp[word]++;
	else mp.insert({word, 1});
}
static void process(char *payload, int payload_len)
{
	static char last[64];
	static int last_len = 0;

	int l = 0, r = 0;

	if (last_len) {
		while (!delim(payload[l]))
			last[last_len++] = payload[l++];
		last[last_len] = '\0';
		put_word(last);
		last_len = 0;
		l++;
		r = l;
	}
	while (1) {
		if (r >= payload_len) {
			if (l != r) {
				last_len = 0;
				while (l < r)
					last[last_len++] = payload[l++];
				last[last_len] = '\0';
			}
			break;
		}
		if (delim(payload[r])) {
			if (l == r) {
				r++;
				l++;
				continue;
			}
			payload[r] = '\0';
			put_word(&payload[l]);
			r++;
			l = r;
			continue;
		}
		r++;
	}
}

static void print_result(void)
{
	int wr = 0;
	if (LBA_RESULT == 0) {
		for (auto &it : mp)
			wr += printf("%s %d\n", it.first.c_str(), it.second);
	} else {
		char buf[256];
		lseek(nvme_fd, LBA_RESULT, SEEK_SET);
		for (auto &it : mp) {
			int w = sprintf(buf, "%s %d\n", it.first.c_str(), it.second);
			write(nvme_fd, buf, w); // TODO: check retval
			wr += w;
		}
	}

	fprintf(stderr, "result %d bytes\n", wr);
}

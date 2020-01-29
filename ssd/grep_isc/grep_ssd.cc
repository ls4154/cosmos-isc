#include <cstdio>
#include <cstring>
#include <map>
#include <isc_ssd.h>
using namespace std;

static int READ_CHUNK = 1048576; // 1 MiB
static int FILESIZE, LBA, LBA_RESULT;
char *TARGET;
int *res;
int resl;

static void process(char *payload, int payload_len);
static void print_result(void);

int main(int argc, char **argv)
{
	isc_init_shared();

	if (argc < 5) {
		fprintf(stderr, "usage: %s <LBA> <FILESIZE> <LBA_RESULT> <WORD>\n", argv[0]);
		fprintf(stderr, "use LBA_RESULT 0 to print stdout\n");
		return 1;
	}
	LBA = atoi(argv[1]);
	FILESIZE = atoi(argv[2]);
	LBA_RESULT = atoi(argv[3]);
	TARGET = argv[4];

	res = (char *)malloc(1048576);
	if (res == NULL) {
		perror("malloc");
		return 1;
	}

	fprintf(stderr, "file size: %d\n", FILESIZE);
	fprintf(stderr, "lba: %d\n", LBA);
	fprintf(stderr, "lba result: %d\n", LBA_RESULT);

	int left = FILESIZE;
	int lba = LBA;
	int read_size = min(READ_CHUNK, left);

	isc_read_request(0, lba, read_size);

	for (int i = 0, j; left > 0; i++) {
		int idx = i % 2;

		while (!isc_check_done(idx));

		int cur_read_size = read_size;

		left -= read_size;
		lba += to_sect(read_size);
		read_size = min(READ_CHUNK, left);

		if (left <= 0)
			goto word_count;

		isc_read_request(!idx, lba, read_size);
word_count:

		for (j = 0; j < to_sect(cur_read_size)-1; j++)
			process(isc_payload(idx, j), SECTSIZE);
		if (cur_read_size % SECTSIZE == 0)
			process(isc_payload(idx, j), SECTSIZE);
		else
			process(isc_payload(idx, j), cur_read_size % SECTSIZE);
	}
	print_result();
}

static inline bool delim(char ch)
{
	return ch == '\n';
}
static int ll;
static int check_str(char *s, int offset)
{
	if(strstr(s, TARGET) != NULL)
		res[resl++] = offset;
}
static void process(char *payload, int payload_len)
{
	static char last[400];
	static int last_len = 0;
	static int last_off;

	int l = 0, r = 0;

	if (last_len) {
		while (!delim(payload[l]))
			last[last_len++] = payload[l++];
		last[last_len] = '\0';
		check_str(last, last_off);
		last_len = 0;
		l++;
		r = l;
	}
	while (1) {
		if (r >= payload_len) {
			if (l != r) {
				last_len = 0;
				last_off = ll + l;
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
			check_str(&payload[l], ll + l);
			r++;
			l = r;
			continue;
		}
		r++;
	}
	ll += payload_len;
}
static void print_result(void)
{
	int wr = 0;
	if (LBA_RESULT == 0) {
		for (auto &it : mp)
			wr += printf("%s %d\n", it.first.c_str(), it.second);
	} else {
		char *payload = isc_shared[0].payload_write;
		for (auto &it : mp)
			wr += sprintf(payload + wr, "%s %d\n", it.first.c_str(), it.second);
		// TODO: chunk output
		isc_write_request(0, LBA_RESULT, wr);
		while (!isc_check_done(0));
	}

	fprintf(stderr, "result %d bytes\n", wr);
}

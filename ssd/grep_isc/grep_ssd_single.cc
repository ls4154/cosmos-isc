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

	if (argc < 4) {
		fprintf(stderr, "usage: %s <LBA> <FILESIZE> <LBA_RESULT>\n", argv[0]);
		fprintf(stderr, "use LBA_RESULT 0 to print stdout\n");
		return 1;
	}
	LBA = atoi(argv[1]);
	FILESIZE = atoi(argv[2]);
	LBA_RESULT = atoi(argv[3]);

	dprint("file size: %d\n", FILESIZE);
	dprint("lba: %d\n", LBA);
	dprint("lba result: %d\n", LBA_RESULT);

	int left = FILESIZE;
	int lba = LBA;
	int read_size = min(READ_CHUNK, left);

	for (int i = 0, j; left > 0; i++) {
		isc_read_request(0, lba, read_size);

		while (!isc_check_done(0));

		int cur_read_size = read_size;

		left -= read_size;
		lba += to_sect(read_size);
		read_size = min(READ_CHUNK, left);

		if (left <= 0)
			goto word_count;

word_count:

		for (j = 0; j < to_sect(cur_read_size)-1; j++)
			process(isc_payload(0, j), SECTSIZE);
		if (cur_read_size % SECTSIZE == 0)
			process(isc_payload(0, j), SECTSIZE);
		else
			process(isc_payload(0, j), cur_read_size % SECTSIZE);
	}
	print_result();
}

static map<string, int> mp;
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
	if (LBA_RESULT == 0) {
		for (int i = 0; i < resl; i++)
			printf("%d ", res[i]);
		puts("");
	} else {
		char *payload = isc_shared[0].payload_write;
		memcpy(payload, res, resl * sizeof(int));
		puts("");
		// TODO: chunk output
		isc_write_request(0, LBA_RESULT, wr);
		while (!isc_check_done(0));
	}
	dprint("result size:%d\n", resl * sizeof(int));
}

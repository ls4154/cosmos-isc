#include <cstdio>
#include <string>
#include <map>
#include <isc_ssd.h>
using namespace std;

static int READ_CHUNK = 1048576; // 1 MiB
static int FILESIZE, LBA, LBA_RESULT;

static void process(char *payload, int payload_len);
static void print_result(void);

int main(int argc, char **argv)
{
	isc_init_shared();

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
		char *payload = isc_shared[0].payload_write;
		for (auto &it : mp)
			wr += sprintf(payload + wr, "%s %d\n", it.first.c_str(), it.second);
		// TODO: chunk output
		isc_write_request(0, LBA_RESULT, wr);
		while (!isc_check_done(0));
	}

	fprintf(stderr, "result %d bytes\n", wr);
}

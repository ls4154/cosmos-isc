#ifndef FTL_H
#define FTL_H

void ftl_init(void);
void ftl_read(unsigned int tag, unsigned int nlb, unsigned int start_lba);
void ftl_write(unsigned int tag, unsigned int nlb, unsigned int start_lba);
void ftl_run(void);

#endif /* FTL_H */

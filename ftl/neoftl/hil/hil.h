#ifndef HIL_H
#define HIL_H

extern unsigned int storageCapacity_L;

void hil_init(void);
void hil_run(void);

void hil_set_storage_blocks(unsigned int nblock);
unsigned int hil_get_storage_blocks(void);

void hil_read_block(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);
void hil_write_block(unsigned int nCmdSlotTag, unsigned int nStartLBA, unsigned int nLBACount);

#endif

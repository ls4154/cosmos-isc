#ifndef NVME_IDENTIFY_H
#define NVME_IDENTIFY_H

#define PCI_VENDOR_ID			0x1EDC
#define PCI_SUBSYSTEM_VENDOR_ID		0x1EDC
#define SERIAL_NUMBER			"SSDD515T"
#define MODEL_NUMBER			"Cosmos+ OpenSSD"
#define FIRMWARE_REVISION		"TYPE0005"

void identify_controller(unsigned int pBuffer);

void identify_namespace(unsigned int pBuffer);


#endif /* NVME_IDENTIFY_H */

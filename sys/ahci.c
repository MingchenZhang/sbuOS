#include <sys/ahci.h>
#include <sys/misc.h>
#include <sys/kprintf.h>
#include <sys/config.h>

#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define ATA_CMD_READ_DMA_EX     0x25
#define ATA_CMD_WRITE_DMA_EX     0x35
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define ATA_DEV_ERR 0x01

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define PORT_SCTL_DET 0b1111
#define PORT_SCTL_DET_RESET 0b0001
#define PORT_SCTL_IPM 0b111100000000
#define PORT_SCTL_DISABLE_PARTIAL_SLUMBER 0b001100000000

typedef struct disk_entry {
	hba_cmd_header_t cmds[32]; __attribute__((aligned(1024)))
	hba_fis_t fis; __attribute__((aligned(256)))
	char exist;
	char port_num;
	uint32_t dev_type;
	hba_port_t* port_p;
	hba_cmd_tbl_t cmdtbl[32];
} disk_entry_t;

disk_entry_t* disks;

static void start_cmd(hba_port_t *port) {
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PxCMD_CR);
 
	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= 0b1000;
	port->cmd |= HBA_PxCMD_ST; 
}
 
// Stop command engine
static void stop_cmd(hba_port_t *port) {
	// Clear ST (bit0)
	port->cmd &= ~HBA_PxCMD_ST;
	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;
	// Wait until FR (bit14), CR (bit15) are cleared
	while(1){
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

#define AHCI_BASE 0x400000
/*
static void init_port(){
	for(int i=0; disks[i].exist; i++){
		volatile hba_port_t* port = disks[i].port_p;
		stop_cmd(port);
		// hard reset port
		port->sctl |=  PORT_SCTL_DET_RESET;
		// wait 1 sec
		volatile uint64_t start_time;
		start_time = pic_tick_count + 50;
		while(start_time >= pic_tick_count); // race condition warning
		port->sctl &= ~PORT_SCTL_DET;
		while((port->ssts & 0xf) > 0);
		port->serr_rwc = 0;
		// set power state to active
		uint32_t state;
		state = port->cmd & ~(0xF0000000);
		port->cmd = state | 0x10000000;
		// Transitions to both Partial and Slumber states disabled
		state = port->sctl & ~PORT_SCTL_IPM;
		port->sctl = state | PORT_SCTL_DISABLE_PARTIAL_SLUMBER;
		// may also set POD, SUD and FRE bits in the CMD register
		port->cmd |= 0b1110;
		// wait 1 sec
		start_time = pic_tick_count + PIT_FREQUENCY;
		while(start_time >= pic_tick_count); // race condition warning
		// init FIS
		port->fb = (uint64_t)&disks[i].fis;
		// init command list
		port->clb = (uint64_t)&disks[i].cmds;
		// port->serr_rwc = 1; // label: m
		// port->is_rwc   = 0; // label: m
		// port->ie       = 1; // label: m
		// start zeroing memory
		memset(&disks[i].fis, 0, 256);
		memset(&disks[i].cmds, 0, 1024); // zero the 32 of the command header
		hba_cmd_header_t* cmdheader = disks[i].cmds;
		for (int j=0; j<32; j++){ // for each command header
			cmdheader[j].prdtl = PRDT_ENTRIES_PER_COMMAND_TABLE;
			// PRDT_ENTRIES_PER_COMMAND_TABLE prdt entries per command table
			cmdheader[j].ctba = (uint64_t)&disks[i].cmdtbl[j];
			memset((void*)cmdheader[i].ctba, 0, sizeof(hba_cmd_tbl_t));
		}
		start_cmd(port);
		kprintf("ssts=%x \n", port->ssts);
		// port->is_rwc = 0; // label: m
		// port->ie = 0xffffffff; // label: m
	}
	// debug_wait();
}
*/
static void init_port(){
	for(int i=0; disks[i].exist; i++){
		volatile hba_port_t* port = disks[i].port_p;
		int portno = disks[i].port_num;
		stop_cmd(port);
		
		// hard reset port
		port->sctl |=  (port->sctl & ~PORT_SCTL_DET) | 0x1;
		// wait 1 sec
		volatile uint64_t start_time;
		start_time = pic_tick_count + 50;
		while(start_time >= pic_tick_count); // race condition warning
		port->sctl &= ~PORT_SCTL_DET;
		while((port->ssts & 0xf) > 0);
		port->serr_rwc = -1;
		// set power state to active
		uint32_t state;
		state = port->cmd & ~(0xF0000000);
		port->cmd = state | 0x10000000;
		// Transitions to both Partial and Slumber states disabled
		state = port->sctl & ~PORT_SCTL_IPM;
		port->sctl = state | PORT_SCTL_DISABLE_PARTIAL_SLUMBER;
		// may also set POD, SUD and FRE bits in the CMD register
		port->cmd |= 0b110;
		// init FIS
		port->fb = AHCI_BASE + (32<<10) + (portno<<8);
		// init command list
		port->clb = AHCI_BASE + (portno<<10);
		// port->serr_rwc = 1; // label: m
		// port->is_rwc   = 0; // label: m
		// port->ie       = 1; // label: m
		// start zeroing memory
		memset((void*)port->fb, 0, 256);
		memset((void*)port->clb, 0, 1024); // zero the 32 of the command header
		hba_cmd_header_t* cmdheader = (void*)port->clb;
		for (int j=0; j<32; j++){ // for each command header
			cmdheader[j].prdtl = 8;
			// 8 prdt entries per command table
			cmdheader[j].ctba = AHCI_BASE + (40<<10) + (portno<<13) + (i<<8);
			memset((void*)cmdheader[j].ctba, 0, 256);
		}
		start_cmd(port);
		kprintf("SERR = %x\n", port->serr_rwc);
		// port->is_rwc = 0; // label: m
		// port->ie = 0xffffffff; // label: m
	}
	// debug_wait();
}


void init_ahci(void* map_addr){
	kprintf("disk_entry_t size:%d\n", sizeof(disk_entry_t));
	kprintf("hba_mem_t size:%d\n", sizeof(hba_mem_t));
	disks = (void*)0x60000; // TODO: remove this terrible hack, please
	volatile hba_mem_t* hba_mem_p = map_addr;
	hba_mem_p->ghc |= 0x80000000;
	for(uint64_t i=0; i<sizeof(disk_entry_t)*8; i++){
		((uint8_t*)disks)[i] = 0;
	}
	// memset(disks, 0, sizeof(disk_entry_t)*8);
	int disk_entry_i = 0;
	kprintf("pi: %x\n", hba_mem_p->pi);
	for(int i=0; i<32; i++){
		if(hba_mem_p->pi & (1 << i)){ // if port is implemented
			// kprintf("port implemented\n");
			kprintf(".");
			hba_port_t* port_p = &hba_mem_p->ports[i];
			// uint32_t ssts = port_p->ssts;
			// uint8_t ipm = (ssts >> 8) & 0x0F;
			// uint8_t det = ssts & 0x0F;
			// if(ipm != HBA_PORT_IPM_ACTIVE)
				// continue;
			// if(det != HBA_PORT_DET_PRESENT)
				// continue;
			switch(port_p->sig){
			case SATA_SIG_ATA:
				disks[disk_entry_i].exist = 1;
				disks[disk_entry_i].dev_type = AHCI_DEV_SATA;
				disks[disk_entry_i].port_p = port_p;
				disks[disk_entry_i].port_num = i;
				disk_entry_i++;
				kprintf("sata drive found\n");
				break;
			case SATA_SIG_ATAPI:
				disks[disk_entry_i].exist = 1;
				disks[disk_entry_i].dev_type = AHCI_DEV_SATAPI;
				disks[disk_entry_i].port_p = port_p;
				disks[disk_entry_i].port_num = i;
				disk_entry_i++;
				kprintf("satapi drive found\n");
				break;
			case SATA_SIG_SEMB:
				kprintf("Enclosure management bridge found\n");
				break;
			case SATA_SIG_PM:
				kprintf("Port multiplier found\n");
				break;
			default:
				break;
			}
		}
	}
	
	// reset HBA
	// hba_mem_p->ghc |= 0b1;
	// while(hba_mem_p->ghc & 0b1);
	init_port();
	kprintf("AHCI initialized\n");
}

// sector_count is the number of 512 byte sector
static int ahci_read(hba_port_t* port, uint32_t startl, uint32_t starth, uint32_t sector_count, void* buffer){
	port->is_rwc = -1;
	// port->cmd |= 0b1110;
	// find available slot
	uint32_t slot = (port->sact | port->ci);
	int i=0;
	for(; i<32; i++){
		if(!(slot&1)) break;
		slot >>= 1;
	}
	if(i == 32) {
		kprintf("ERROR: AHCI no available command slot\n");
		return 0;
	}
	slot = i;
	hba_cmd_header_t* cmdheader = (hba_cmd_header_t*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);
	cmdheader->w = 0;
	// cmdheader->c = 1; // label: m
	cmdheader->prdtl = 1;
	
	hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t));
	
	// for (i=0; i<cmdheader->prdtl-1; i++) {
		// cmdtbl->prdt_entry[i].dba = (uint64_t)buffer;
		// cmdtbl->prdt_entry[i].dbc = 8*1024;	// 8K bytes, 16 sectors
		// cmdtbl->prdt_entry[i].i = 1;
		// buffer += 8*1024;	// 8K bytes
		// sector_count -= 16;	// 16 sectors
	// }
	// Last entry
	cmdtbl->prdt_entry[0].dba = (uint64_t)buffer;
	cmdtbl->prdt_entry[0].dbc = sector_count * 512; // 512 bytes per sector
	cmdtbl->prdt_entry[0].i = 1;
	
	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->count = (uint16_t)sector_count;
	
	kprintf("ssts = %x\n", port->ssts);
	uint64_t spin = 0;
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ))){
		if(port->tfd & ATA_DEV_ERR) {
			kprintf("ERROR: task file error\n");
			kprintf("tfd = %x\n", port->tfd);
			return 0;
		}
		if(spin++ > 10000000) {
			kprintf("ERROR: port hung\n");
			kprintf("tfd = %x\n", port->tfd);
			kprintf("ssts = %x\n", port->ssts);
			kprintf("serr = %x\n", port->serr_rwc);
			return 0;
		}
	} // wait for busy port
		
	
	port->ci = 1<<slot;
	
	kprintf("1.3 ");
	kprintf("ssts=%x ", port->ssts);
	while (1){
		if ((port->ci & (1<<slot)) == 0) break;
		if (port->is_rwc & HBA_PxIS_TFES) {	// Task file error
			kprintf("ERROR: read disk error\n");
			return 0;
		}
	}
	
	// Check again
	if (port->is_rwc & HBA_PxIS_TFES) {
			kprintf("ERROR: read disk error\n");
		return 0;
	}
 
	return 1;
}

// sector_count is the number of 512 byte sector
static int ahci_write(hba_port_t* port, uint32_t startl, uint32_t starth, uint32_t sector_count, void* buffer){
	port->is_rwc = -1;
	// port->cmd |= 0b1110;
	// find available slot
	uint32_t slot = (port->sact | port->ci);
	int i=0;
	for(; i<32; i++){
		if(!(slot&1)) break;
		slot >>= 1;
	}
	if(i == 32) {
		kprintf("ERROR: AHCI no available command slot\n");
		return 0;
	}
	slot = i;
	hba_cmd_header_t* cmdheader = (hba_cmd_header_t*)port->clb;
	cmdheader += slot;
	cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);
	cmdheader->w = 1;
	// cmdheader->c = 1; // label: m
	cmdheader->prdtl = 1;
	
	hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
	memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t));
	
	// for (i=0; i<cmdheader->prdtl-1; i++) {
		// cmdtbl->prdt_entry[i].dba = (uint64_t)buffer;
		// cmdtbl->prdt_entry[i].dbc = 8*1024;	// 8K bytes, 16 sectors
		// cmdtbl->prdt_entry[i].i = 1;
		// buffer += 8*1024;	// 8K bytes
		// sector_count -= 16;	// 16 sectors
	// }
	// Last entry
	cmdtbl->prdt_entry[0].dba = (uint64_t)buffer;
	cmdtbl->prdt_entry[0].dbc = sector_count * 512 -1;	// 512 bytes per sector
	cmdtbl->prdt_entry[0].i = 1;
	
	fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_WRITE_DMA_EX;
 
	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl>>8);
	cmdfis->lba2 = (uint8_t)(startl>>16);
	cmdfis->device = 1<<6;	// LBA mode
 
	cmdfis->lba3 = (uint8_t)(startl>>24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth>>8);
 
	cmdfis->count = (uint16_t)sector_count;
	
	// kprintf("SERR = %x\n", port->serr_rwc);
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ))) // wait for busy port
		if(port->tfd & ATA_DEV_ERR) {
			kprintf("ERROR: task file error\n");
			kprintf("tfd = %x\n", port->tfd);
			return 0;
		}
	
	port->ci = 1<<slot;
	
	kprintf("1.3 ");
	while (1){
		if ((port->ci & (1<<slot)) == 0) break;
		if (port->is_rwc & HBA_PxIS_TFES) {	// Task file error
			kprintf("ERROR: write disk error\n");
			return 0;
		}
	}
	
	// Check again
	if (port->is_rwc & HBA_PxIS_TFES) {
			kprintf("ERROR: write disk error\n");
		return 0;
	}
 
	return 1;
}

int test_read(int sector_count, int disk_index){
	if (!disks) {
		kprintf("ERROR: disk not initialized\n");
		return 0;
	}
	uint64_t bytes = sector_count*512;
	uint8_t buffer[bytes] __attribute__((aligned(4096)));
	// uint8_t* buffer = (void*)0x100000000;
	kprintf("TEST_WRITE_READ: buffer is at %p\n", buffer);
	// find a test subject
	int port_i=0;
	for(; port_i<8; port_i++){
		if(disks[port_i].exist) disk_index--;
		if(disk_index == -1) break;
	}
	if(port_i == 8){
		kprintf("ERROR: no disk to test\n");
		return 0;
	}
	kprintf("1 ");
	if(!ahci_read(disks[port_i].port_p, 0, 0, sector_count, buffer)){
		kprintf("TEST_WRITE_READ: write failed\n");
		return 0;
	}
	return 1;
}

int test_write_read(int sector_count, int disk_index){
	if (!disks) {
		kprintf("ERROR: disk not initialized\n");
		return 0;
	}
	uint64_t bytes = sector_count*512;
	uint8_t buffer[bytes] __attribute__((aligned(1024)));
	// uint8_t* buffer = (void*)0x100000000;
	kprintf("TEST_WRITE_READ: buffer is at %p\n", buffer);
	// find a test subject
	int port_i=0;
	for(; port_i<8; port_i++){
		if(disks[port_i].exist) disk_index--;
		if(disk_index == -1) break;
	}
	if(port_i == 8){
		kprintf("ERROR: no disk to test\n");
		return 0;
	}
	kprintf("1 ");
	for(uint64_t i=0; i<bytes; i++){
		buffer[i] = 0b10101010;
	}
	if(!ahci_write(disks[port_i].port_p, 100, 0, sector_count, buffer)){
		kprintf("TEST_WRITE_READ: write failed\n");
		return 0;
	}
	kprintf("2 ");
	for(uint64_t i=0; i<bytes; i++){
		buffer[i] = 0;
	}
	kprintf("3 ");
	if(!ahci_read(disks[port_i].port_p, 100, 0, sector_count, buffer)){
		kprintf("TEST_WRITE_READ: read failed\n");
		return 0;
	}
	kprintf("4 ");
	for(uint64_t i=0; i<bytes; i++){
		if(buffer[i] != 0b10101010) return 0;
		if(i%1024==0) kprintf(".");
	}
	kprintf("5 ");
	return 1;
}

int test_wp3(){
	if (!disks) {
		kprintf("ERROR: disk not initialized\n");
		return 0;
	}
	// find a test subject
	int port_i=0;
	for(; port_i<8; port_i++){
		if(disks[port_i].exist) break;
	}
	if(port_i == 8){
		kprintf("ERROR: no disk to test\n");
		return 0;
	}
	kprintf("TEST_WP3: will test on %d\n", port_i);
	
	uint8_t buffer[4096];
	for(int i=0; i<100; i++){
		for(int j=0; j<4096; j++){
			buffer[j] = (uint8_t)i;
		}
		if(!ahci_write(disks[port_i].port_p, 8*i, 0, 8, buffer)){
			kprintf("TEST_WP3: write failed\n");
			return 0;
		}
	}
	kprintf("TEST_WP3: write finished\n");
	for(int i=0; i<100; i++){
		if(!ahci_read(disks[port_i].port_p, 8*i, 0, 8, buffer)){
			kprintf("TEST_WP3: read failed\n");
			return 0;
		}
		for(int j=0; j<4096; j++){
			if(buffer[j] != (uint8_t)i) {
				kprintf("TEST_WP3: read mismatch\n");
				return 0;
			}
		}
	}
	return 1;
}


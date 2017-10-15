#include <sys/misc.h>
#include <sys/kprintf.h>
#include <sys/defs.h>
#include <sys/ahci.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

// hba_mem_t test_hba_mem __attribute__((aligned(4096)));

struct config_space_address_struct {
	uint32_t zero:      2;
	uint32_t reg_num:   6;
	uint32_t func_num:  3;
	uint32_t dev_num:   5;
	uint32_t bus_num:   8;
	uint32_t reserve:   7;
	uint32_t enable:    1;
} __attribute__((packed)); 

union config_space_address{
	uint32_t address;
	struct config_space_address_struct struc;
};

uint16_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset){
	union config_space_address address;
	address.address = 0;
	address.struc.reg_num = offset >> 2;
	address.struc.func_num = func;
	address.struc.dev_num = dev;
	address.struc.bus_num = bus;
	address.struc.enable = 1;
	
	asm_outl(PCI_CONFIG_ADDRESS, address.address);
	return (uint16_t)((asm_inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
}

void pci_config_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data){
	union config_space_address address;
	address.address = 0;
	address.struc.reg_num = offset >> 2;
	address.struc.func_num = func;
	address.struc.dev_num = dev;
	address.struc.bus_num = bus;
	address.struc.enable = 1;
	
	asm_outl(PCI_CONFIG_ADDRESS, address.address);
	// return (uint16_t)((asm_inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
	asm_outl(PCI_CONFIG_DATA, data);
}

static int pci_is_valid_dev(uint8_t bus, uint8_t slot) {
	return pci_config_read(bus, slot, 0, 0) != 0xFFFF;
}

void test_set_ahci_hba_routine(uint16_t bus, uint8_t device, uint8_t func){ // TODO: remove this terrible hack, please
	hba_mem_t* hba_mem = (void*)(0xA2000);
	// hba_mem_t* hba_mem = (void*)(&test_hba_mem);
	memset(hba_mem, 0, sizeof(hba_mem_t));
	uint32_t set_to = (uint32_t)((uint64_t)hba_mem % 0x40000000);
	kprintf("TEST: setting AHCI HBA address to %p\n", set_to);
	pci_config_write32(bus, device, func, 0x24, set_to);
}

static void* find_ahci_HBA(uint8_t bus, uint8_t dev, uint8_t func){
	test_set_ahci_hba_routine(bus, dev, func); // TODO: remove this
	uint16_t bar5_low = pci_config_read(bus, dev, func, 0x24);
	uint16_t bar5_high = pci_config_read(bus, dev, func, 0x26);
	// uint16_t bar4_low = pci_config_read(bus, dev, func, 0x20);
	// uint16_t bar4_high = pci_config_read(bus, dev, func, 0x22);
	void* hba_mem = (void*)(((((uint64_t)bar5_high<<16)|bar5_low) & 0xFFFFFFE0));
	kprintf("found HBA memory space at %p\n", hba_mem);
	// kprintf("HBA should be at %x\n", &test_hba_mem); // TODO: remove this
	// kprintf("bar[4] %x\n", (((uint64_t)bar4_high<<16)|bar4_low));
	return hba_mem;
}

void init_pci(){
	for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint8_t device = 0; device < 32; device++) {
            if(!pci_is_valid_dev(bus, device)) continue;
			uint16_t class_high = pci_config_read(bus, device, 0, 0x0A);
			uint16_t class_low = pci_config_read(bus, device, 0, 0x08);
			uint8_t header = pci_config_read(bus, device, 0, 0x0E) & 0xFF;
			// kprintf("pci device found at %d, %d, h: %x. c: %x, s: %x, p: %x\n", bus, device, header, (class_high>>8), (class_high & 0xFF), (class_low>>8));
			if((class_high == 0x0106) && (class_low>>8 == 0x01)){
				kprintf("AHCI controller detected. \n");
				void* hba_mem = find_ahci_HBA(bus, device, 0);
				init_ahci((void*)(hba_mem));
			}else if(header & 0x80){ // multi-functional device
				for(uint8_t func=0; func< 8; func++){
					if(pci_config_read(bus, device, func, 0) == 0xFFFF) continue;
					class_high = pci_config_read(bus, device, func, 0x0A);
					class_low = pci_config_read(bus, device, func, 0x08);
					header = pci_config_read(bus, device, func, 0x0E) & 0xFF;
					// kprintf("pci device found at %d, %d, h: %x. c: %x, s: %x, p: %x\n", bus, device, header, (class_high>>8), (class_high & 0xFF), (class_low>>8));
					if((class_high == 0x0106) && (class_low>>8 == 0x01)){
						kprintf("AHCI controller detected. \n");
						void* hba_mem = find_ahci_HBA(bus, device, func);
						init_ahci((void*)(hba_mem));
					}
				}
			}
        }
    }
}

void print_pci() {
	for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint8_t device = 0; device < 32; device++) {
            if(!pci_is_valid_dev(bus, device)) continue;
			uint16_t deviceID = pci_config_read(bus, device, 0, 0x02);
			uint16_t vendorID = pci_config_read(bus, device, 0, 0x00);
			uint16_t header = pci_config_read(bus, device, 0, 0x0E) & 0xFF;
			uint16_t class_high = pci_config_read(bus, device, 0, 0x0A);
			uint16_t class_low = pci_config_read(bus, device, 0, 0x08);
			kprintf("pci device found at %d, %d. deviceID: %x, vendorID: %x, header: %x\n"
				"class code: %x, subclass: %x, progIF: %x\n", 
				bus, device, deviceID, vendorID, header, (class_high>>8), (class_high & 0xFF), (class_low>>8));
        }
    }
	kprintf("pci bus printed\n");
}
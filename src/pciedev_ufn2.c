//
// file:        pciedev_ufn2.c
// created on:  2021 Jun 25
// purpose:     functions in this file will be nice to export to upciedev
//

#include "pciedev_ufn2.h"
#include "pciedev_ufn.h"
#include <linux/module.h>


loff_t    pciedev_llseek_exp(struct file *filp, loff_t off, int frm)
{
    ssize_t       retval         = 0;
    int             minor          = 0;
    int             d_num          = 0;

    struct pciedev_dev       *dev = filp->private_data;
    minor = dev->dev_minor;
    d_num = dev->dev_num;

    filp->f_pos = (loff_t)PCIED_FPOS;
    if(!dev->dev_sts){
        retval = -EFAULT;
        return retval;
    }
    return (loff_t)PCIED_FPOS;
}
EXPORT_SYMBOL(pciedev_llseek_exp);


int pciedev_device_init_exp(pciedev_dev* a_pciedev_p, const pciedev_cdev* a_pciedev_cdev_p,
                            const struct file_operations* a_pciedev_fops,int a_brd_num)
{
    int devno;
    int result;

    //pciedev_cdev_p->pciedev_dev_m[i]->parent_dev = pciedev_cdev_p;
    a_pciedev_p->dev_minor  = (a_pciedev_cdev_p->PCIEDEV_MINOR + a_brd_num);
    devno = MKDEV(a_pciedev_cdev_p->PCIEDEV_MAJOR, a_pciedev_p->dev_minor);
    a_pciedev_p->dev_num    = devno;
    cdev_init(&(a_pciedev_p->cdev), a_pciedev_fops);
    a_pciedev_p->cdev.owner = THIS_MODULE;
    a_pciedev_p->cdev.ops = a_pciedev_fops;
    result = cdev_add(&(a_pciedev_p->cdev), devno, 1);
    if (result){
        printk(KERN_NOTICE "Error %d adding devno%d num%d\n", result, devno, a_brd_num);
        return 1;
    }
    INIT_LIST_HEAD(&(a_pciedev_p->prj_info_list.prj_list));
    INIT_LIST_HEAD(&(a_pciedev_p->module_info_list.module_list));
    INIT_LIST_HEAD(&(a_pciedev_p->dev_file_list.node_file_list));
    //mutex_init(&(pciedev_cdev_p->pciedev_dev_m[i]->dev_mut));
    InitCritRegionLock(&(a_pciedev_p->dev_mut), _DEFAULT_TIMEOUT_);
    a_pciedev_p->dev_sts                   = 0;
    a_pciedev_p->dev_file_ref            = 0;
    a_pciedev_p->irq_mode                = 0;
    a_pciedev_p->msi                         = 0;
    a_pciedev_p->dev_dma_64mask   = 0;
    a_pciedev_p->pciedev_all_mems  = 0;
    a_pciedev_p->brd_num                = a_brd_num;
    a_pciedev_p->binded                   = 0;
    a_pciedev_p->dev_file_list.file_cnt = 0;
    a_pciedev_p->null_dev                   = 0;
    printk(KERN_ALERT "INIT ADD PARENT BASE\n");
    a_pciedev_p->parent_base_dev     = &base_upciedev_dev;
    //a_pciedev_p->parent_base_dev     = p_base_upciedev_dev;

    if(a_brd_num == PCIEDEV_NR_DEVS){
        a_pciedev_p->binded        = 1;
        a_pciedev_p->null_dev      = 1;
    }

    return result;
}
EXPORT_SYMBOL(pciedev_device_init_exp);


void pciedev_device_clean_exp(pciedev_dev* a_pciedev_p)
{
    cdev_del(&a_pciedev_p->cdev);
}
EXPORT_SYMBOL(pciedev_device_clean_exp);


int upciedev_driver_init_exp(pciedev_cdev* a_pciedev_cdev_p, const struct file_operations* a_pciedev_fops, const char* a_dev_name)
{
    int   result;
    dev_t devt;
    char* pcEndPtr;

    printk(KERN_ALERT "############UPCIEDEV_INIT MODULE  NAME %s\n", a_dev_name);

    a_pciedev_cdev_p->PCIEDEV_MAJOR             = 47;
    a_pciedev_cdev_p->PCIEDEV_MINOR             = 0;
    a_pciedev_cdev_p->pciedevModuleNum         = 0;
    a_pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = 1;
    a_pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = 1;
    a_pciedev_cdev_p->UPCIEDEV_VER_MAJ        = 1;
    a_pciedev_cdev_p->UPCIEDEV_VER_MIN        = 1;

    result = alloc_chrdev_region(&devt, a_pciedev_cdev_p->PCIEDEV_MINOR, (PCIEDEV_NR_DEVS + 1), a_dev_name);
    a_pciedev_cdev_p->PCIEDEV_MAJOR = MAJOR(devt);
    /* Populate sysfs entries */
    a_pciedev_cdev_p->pciedev_class = class_create(a_pciedev_fops->owner, a_dev_name);
    /*Get module driver version information*/
    a_pciedev_cdev_p->UPCIEDEV_VER_MAJ = simple_strtol(THIS_MODULE->version, &pcEndPtr, 10);
    a_pciedev_cdev_p->UPCIEDEV_VER_MIN = simple_strtol(THIS_MODULE->version + 2, &pcEndPtr, 10);
    printk(KERN_ALERT "&&&&&UPCIEDEV_INIT CALLED; UPCIEDEV MODULE VERSION %i.%i\n",
            a_pciedev_cdev_p->UPCIEDEV_VER_MAJ, a_pciedev_cdev_p->UPCIEDEV_VER_MIN);

    a_pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ = simple_strtol(a_pciedev_fops->owner->version, &pcEndPtr, 10);
    a_pciedev_cdev_p->PCIEDEV_DRV_VER_MIN = simple_strtol(a_pciedev_fops->owner->version + 2, &pcEndPtr, 10);
    printk(KERN_ALERT "&&&&&UPCIEDEV_INIT CALLED; THIS MODULE VERSION %i.%i\n",
        a_pciedev_cdev_p->PCIEDEV_DRV_VER_MAJ, a_pciedev_cdev_p->PCIEDEV_DRV_VER_MIN);

    return result; /* succeed */
}
EXPORT_SYMBOL(upciedev_driver_init_exp);


void upciedev_driver_clean_exp(const pciedev_cdev* a_pciedev_cdev_p)
{
    dev_t  devno ;

    printk(KERN_ALERT "UPCIEDEV_CLEANUP_MODULE CALLED\n");

    devno = MKDEV(a_pciedev_cdev_p->PCIEDEV_MAJOR, a_pciedev_cdev_p->PCIEDEV_MINOR);
    unregister_chrdev_region(devno, (PCIEDEV_NR_DEVS + 1));
    class_destroy(a_pciedev_cdev_p->pciedev_class);
}
EXPORT_SYMBOL(upciedev_driver_clean_exp);



/*///////////////////////////////*/

int    pciedev_probe_of_single_device_exp(struct pci_dev* a_dev, pciedev_dev* a_pciedev, const char* a_dev_name, int* a_slot_num_p)
{

    int    m_brdNum   = 0;
    int     err       = 0;
    int     tmp_info = 0;
    int     i, nToAdd,  k;

    u16 vendor_id;
    u16 device_id;
    u8  revision;
    u8  irq_line;
    u8  irq_pin;
    u32 res_start;
    u32 res_end;
    u32 res_flag;
    int pcie_cap;
    int pcie_parent_cap;
    u32 tmp_slot_cap     = 0;
    int tmp_slot_num     = 0;
    int tmp_dev_num      = 0;
    int tmp_bus_func     = 0;
    int tmp_parent_num    = 0;

     int cur_mask = 0;
    u8  dev_payload;
    u32 tmp_payload_size = 0;

    u16 subvendor_id;
    u16 subdevice_id;
    u16 class_code;
    u32 tmp_devfn;
    u32 busNumber;
    u32 devNumber;
    u32 funcNumber;

    u32 ptmp_devfn;
    u32 pbusNumber;
    u32 pdevNumber;
    u32 pfuncNumber;

    char f_name[64];
    char prc_entr[64];

    int    tmp_msi_num = 0;

    if(a_pciedev->binded){
        printk(KERN_WARNING "Device in the slot %d already binded\n",(int)a_pciedev->slot_num);
        return -1;
    }

    printk(KERN_ALERT "############PCIEDEV_PROBE THIS IS U_FUNCTION NAME %s\n", dev_name);

    /*************************BOARD DEV INIT******************************************************/
    printk(KERN_WARNING "PCIEDEV_PROBE CALLED\n");

    printk(KERN_ALERT "PCIEDEV_PROBE  PARENT NUM IS %d\n", tmp_parent_num);

    /*setup device*/
    printk(KERN_ALERT "PCIEDEV_PROBE : BOARD NUM %i\n", m_brdNum);

    if ((err= pci_enable_device(a_dev)))
        return err;
    err = pci_request_regions(a_dev, a_dev_name);
    if (err ){
        pci_disable_device(a_dev);
        pci_set_drvdata(a_dev, NULL);
        a_pciedev->binded = 0;
        return err;
    }
    pci_set_master(dev);

    tmp_devfn  = (u32)dev->devfn;
    busNumber  = (u32)dev->bus->number;
    devNumber  = (u32)PCI_SLOT(tmp_devfn);
    funcNumber = (u32)PCI_FUNC(tmp_devfn);
    tmp_bus_func = ((busNumber & 0xFF)<<8) + (devNumber & 0xFF);
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X, BUS_FUNC %x\n",
                                      tmp_devfn, busNumber, devNumber, funcNumber, tmp_bus_func);

    ptmp_devfn  = (u32)dev->bus->self->devfn;
    pbusNumber  = (u32)dev->bus->self->bus->number;
    pdevNumber  = (u32)PCI_SLOT(tmp_devfn);
    pfuncNumber = (u32)PCI_FUNC(tmp_devfn);
    printk(KERN_ALERT "PCIEDEV_PROBE:DEVFN %X, BUS_NUM %X, DEV_NUM %X, FUNC_NUM %X\n",
                                      ptmp_devfn, pbusNumber, pdevNumber, pfuncNumber);

    pcie_cap = pci_find_capability (dev->bus->self, PCI_CAP_ID_EXP);
    printk(KERN_INFO "PCIEDEV_PROBE: PCIE SWITCH CAP address %X\n",pcie_cap);

    pci_read_config_dword(dev->bus->self, (pcie_cap +PCI_EXP_SLTCAP), &tmp_slot_cap);
    tmp_slot_num = (tmp_slot_cap >> 19);
    tmp_dev_num  = tmp_slot_num;
    printk(KERN_ALERT "PCIEDEV_PROBE:SLOT NUM %d DEV NUM%d SLOT_CAP %X\n",tmp_slot_num,tmp_dev_num,tmp_slot_cap);
    if(tmp_parent_num == 1){
        pcie_parent_cap =pci_find_capability (dev->bus->parent->self, PCI_CAP_ID_EXP);
        pci_read_config_dword(dev->bus->parent->self, (pcie_parent_cap +PCI_EXP_SLTCAP), &tmp_slot_cap);
        tmp_slot_num = (tmp_slot_cap >> 19);
        tmp_dev_num  = tmp_slot_num;
        printk(KERN_ALERT "PCIEDEV_PROBE:PARENTSLOT NUM %d DEV NUM%d SLOT_CAP %X\n",tmp_slot_num,tmp_dev_num,tmp_slot_cap);
    }
    if(tmp_parent_num == 2){
        tmp_slot_num = 13;
        tmp_dev_num  = tmp_slot_num;
        printk(KERN_ALERT "PCIEDEV_PROBE:NTB DEVICE PARENTSLOT NUM %d DEV NUM%d \n",tmp_slot_num,tmp_dev_num);
    }

    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->swap         = 0;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->slot_num  = tmp_slot_num;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->bus_func  = tmp_bus_func;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->pciedev_pci_dev = dev;

    dev_set_drvdata(&(dev->dev), ( pciedev_cdev_p->pciedev_dev_m[m_brdNum]));

    pcie_cap = pci_find_capability (dev, PCI_CAP_ID_EXP);
    printk(KERN_ALERT "DAMC_PROBE: PCIE CAP address %X\n",pcie_cap);
    pci_read_config_byte(dev, (pcie_cap +PCI_EXP_DEVCAP), &dev_payload);
    dev_payload &=0x0003;
    printk(KERN_ALERT "DAMC_PROBE: DEVICE CAP  %X\n",dev_payload);

    switch(dev_payload){
        case 0:
                   tmp_payload_size = 128;
                   break;
        case 1:
                   tmp_payload_size = 256;
                   break;
        case 2:
                   tmp_payload_size = 512;
                   break;
        case 3:
                   tmp_payload_size = 1024;
                   break;
        case 4:
                   tmp_payload_size = 2048;
                   break;
        case 5:
                   tmp_payload_size = 4096;
                   break;
    }
    printk(KERN_ALERT "DAMC: DEVICE PAYLOAD  %d\n",tmp_payload_size);


    if (!(cur_mask = pci_set_dma_mask(dev, DMA_BIT_MASK(64))) &&
        !(cur_mask = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64)))) {
             pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_dma_64mask = 1;
            printk(KERN_ALERT "CURRENT 64MASK %i\n", cur_mask);
    } else {
            if ((err = pci_set_dma_mask(dev, DMA_BIT_MASK(32))) &&
                (err = pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32)))) {
                    printk(KERN_ALERT "No usable DMA configuration\n");
            }else{
             pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_dma_64mask = 0;
            printk(KERN_ALERT "CURRENT 32MASK %i\n", cur_mask);
            }
    }

    pci_read_config_word(dev, PCI_VENDOR_ID,   &vendor_id);
    pci_read_config_word(dev, PCI_DEVICE_ID,   &device_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID,   &subvendor_id);
    pci_read_config_word(dev, PCI_SUBSYSTEM_ID,   &subdevice_id);
    pci_read_config_word(dev, PCI_CLASS_DEVICE,   &class_code);
    pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
    pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq_line);
    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq_pin);

    printk(KERN_INFO "PCIEDEV_PROBE: VENDOR_ID  %X\n",vendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_DEVICE_ID  %X\n",device_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_VENDOR_ID  %X\n",subvendor_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_SUBSYSTEM_ID  %X\n",subdevice_id);
    printk(KERN_INFO "PCIEDEV_PROBE: PCI_CLASS_DEVICE  %X\n",class_code);

     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->vendor_id      = vendor_id;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->device_id      = device_id;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->subvendor_id   = subvendor_id;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->subdevice_id   = subdevice_id;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->class_code     = class_code;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->revision       = revision;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_line       = irq_line;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_pin        = irq_pin;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->scratch_bar    = 0;
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->scratch_offset = 0;

   /*****Set Up Base Tables*/
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].dev_stst       = 1;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].slot_num      = tmp_slot_num;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].slot_bus        =busNumber;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].slot_device   = devNumber;

    /*******SETUP BARs******/
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->pciedev_all_mems = 0;
    for (i = 0, nToAdd = 1; i < NUMBER_OF_BARS; ++i, nToAdd *= 2)
        {
            res_start = pci_resource_start(dev, i);
            res_end = pci_resource_end(dev, i);
            res_flag = pci_resource_flags(dev, i);
            pciedev_cdev_p->pciedev_dev_m[m_brdNum]->mem_base[i] = res_start;
            pciedev_cdev_p->pciedev_dev_m[m_brdNum]->mem_base_end[i]   = res_end;
            pciedev_cdev_p->pciedev_dev_m[m_brdNum]->mem_base_flag[i]  = res_flag;
            if(res_start){
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->memmory_base[i] = pci_iomap(dev, i, (res_end - res_start));
        printk(KERN_INFO "PCIEDEV_PROBE: mem_region %d address %X  SIZE %X FLAG %X MMAPED %p\n", i,res_start, (res_end - res_start),
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->mem_base_flag[i],
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->memmory_base[i]);

        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->rw_off[i] = (res_end - res_start);
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->pciedev_all_mems += nToAdd;

        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[i].res_start = res_start;
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[i].res_end = res_end;
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[tmp_slot_num].bars[i].res_flag = res_flag;
            } else{
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->memmory_base[i] = 0;
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->rw_off[i]       = 0;
        printk(KERN_INFO "PCIEDEV: NO BASE%i address\n", i);
            }
        }

    for(i = 0; i < NUMBER_OF_SLOTS + 1; ++i){
        printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: *********SLOT %i DEV_NUM %i\n",
            pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].slot_num, i);
        for(k = 0; k < NUMBER_OF_BARS; ++ k){
            printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i BAR %i START %X\n",
                i, k, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].bars[k].res_start);
            printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i BAR %i STOP %X\n",
                i, k, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].bars[k].res_start);
            printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i BAR %i FLAG %X\n",
                i, k, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].bars[k].res_start);
        }
        printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i BUS %X\n",
            i, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].slot_bus);
        printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i DEVICE %X\n",
            i, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].slot_device);
        printk(KERN_NOTICE "PROBE_PHYS_ADDRESS: DEV_NUM %i DEV_STS %i\n",
            i, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->parent_base_dev->dev_phys_addresses[i].dev_stst);
    }

    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->enbl_irq_num = 0;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->device_irq_num = 0;
    /******GET BRD INFO******/
    tmp_info = pciedev_get_brdinfo(pciedev_cdev_p->pciedev_dev_m[m_brdNum]);
    if(tmp_info == 1){
        printk(KERN_ALERT "$$$$$$$$$$$$$PROBE THIS  IS DESY BOARD %i\n", tmp_info);
        tmp_info = pciedev_get_prjinfo(pciedev_cdev_p->pciedev_dev_m[m_brdNum]);
        printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  NUMBER OF PRJs %i\n", tmp_info);
    }else{
        if(tmp_info == 2){
            printk(KERN_ALERT "$$$$$$$$$$$$$PROBE THIS  IS SHAPI BOARD %i\n", tmp_info);
            tmp_info = pciedev_get_shapi_module_info(pciedev_cdev_p->pciedev_dev_m[m_brdNum]);
            printk(KERN_ALERT "$$$$$$$$$$$$$PROBE  NUMBER OF MODULES %i\n", tmp_info);
        }
    }


    /*******PREPARE INTERRUPTS******/

    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_flag = IRQF_SHARED ;
    #ifdef CONFIG_PCI_MSI
   tmp_msi_num = pci_msi_vec_count(dev);
   printk(KERN_ALERT "MSI COUNT %i\n", tmp_msi_num);



/*
   static inline int
pci_alloc_irq_vectors(struct pci_dev *dev, unsigned int min_vecs,
              unsigned int max_vecs, unsigned int flags)
{
    return pci_alloc_irq_vectors_affinity(dev, min_vecs, max_vecs, flags,
                          NULL);
}

int pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec);
static inline int pci_enable_msi_exact(struct pci_dev *dev, int nvec)
{
    int rc = pci_enable_msi_range(dev, nvec, nvec);
    if (rc < 0)
        return rc;
    return 0;
}

*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
    if(tmp_msi_num >0)pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_flag = PCI_IRQ_ALL_TYPES ;
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi_num = pci_alloc_irq_vectors(dev, 1, tmp_msi_num, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_flag);
#else
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi_num = pci_enable_msi_range(dev, 1, tmp_msi_num);
#endif
//pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi_num = pci_enable_msi_range(dev, 1, tmp_msi_num);

   //if (pci_enable_msi(dev) == 0) {
   if (pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi_num >= 0) {
             pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi = 1;
             pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_flag &= ~IRQF_SHARED;
            printk(KERN_ALERT "MSI ENABLED IRQ_NUM %i\n", pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi_num);
      printk(KERN_ALERT "MSI ENABLED MSI OFFSET 0x%X\n", dev->msi_cap);
      printk(KERN_ALERT "MSI ENABLED ON PCI_DEV %i\n", dev->msi_enabled);
    } else {
             pciedev_cdev_p->pciedev_dev_m[m_brdNum]->msi = 0;
            printk(KERN_ALERT "MSI NOT SUPPORTED\n");
    }
    #endif
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->pci_dev_irq = dev->irq;
     printk(KERN_ALERT "MSI ENABLED DEV->IRQ %i\n", dev->irq);
     pciedev_cdev_p->pciedev_dev_m[m_brdNum]->irq_mode = 0;

    /* Send uvents to udev, so it'll create /dev nodes */
    if( pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_sts){
        pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_sts   = 0;
        device_destroy(pciedev_cdev_p->pciedev_class,  MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR,
                                                  pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum));
    }
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_sts   = 1;
    sprintf(pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(f_name, "%ss%d", dev_name, tmp_slot_num);
    sprintf(prc_entr, "%ss%d", dev_name, tmp_slot_num);
    printk(KERN_INFO "PCIEDEV_PROBE:  CREAT DEVICE MAJOR %i MINOR %i F_NAME %s DEV_NAME %s\n",
               pciedev_cdev_p->PCIEDEV_MAJOR, (pciedev_cdev_p->PCIEDEV_MINOR + m_brdNum), f_name, dev_name);

    device_create(pciedev_cdev_p->pciedev_class, NULL, MKDEV(pciedev_cdev_p->PCIEDEV_MAJOR,  pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_minor),
                            & pciedev_cdev_p->pciedev_dev_m[m_brdNum]->pciedev_pci_dev->dev, f_name, pciedev_cdev_p->pciedev_dev_m[m_brdNum]->dev_name);


    register_upciedev_proc(tmp_slot_num, dev_name, pciedev_cdev_p->pciedev_dev_m[m_brdNum], pciedev_cdev_p);
    pciedev_cdev_p->pciedev_dev_m[m_brdNum]->register_size = RW_D32;

    pciedev_cdev_p->pciedevModuleNum ++;
    return 0;
}
EXPORT_SYMBOL(pciedev_probe_exp);

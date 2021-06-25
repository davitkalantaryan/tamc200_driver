/*
 *	File: timer_drv_main.c
 *
 *	Created on: Oct 15, 2013, Modified 2015 Oct. 5
 *	Author: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *
 */

#define	TAMC200_NR_DEVS		16

#define TAMC200_DEVNAME		"tamc200"       /* name of device */
#define TAMC200_DRV_NAME	"tamc200"       /* name of device */


#define	IP_TIMER_WHOLE_BUFFER_SIZE_ALL	1024
//#define	_ALLOC_MEMORY_(_a_flag_)		kzalloc(IP_TIMER_WHOLE_BUFFER_SIZE_ALL,(_a_flag_))
//#define	_DEALLOC_MEMORY_(_a_memory_)	kfree((_a_memory_))
#define	_ALLOC_MEMORY_(_a_flag_)		(void*)get_zeroed_page((_a_flag_))
#define	_DEALLOC_MEMORY_(_a_memory_)	free_page((unsigned long int)(_a_memory_))


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <pciedev_ufn.h>
#include "pciedev_ufn2.h"
#include <debug_functions.h>
#include "tamc200_io.h"
#include "linux_version_dependence.h"

#ifdef __INTELISENSE__
#include "../../include/used_for_driver_intelisense.h"
#endif

MODULE_AUTHOR("Davit Kalantaryan");
MODULE_DESCRIPTION("Driver for TEWS TAMC200 IP carrier");
MODULE_VERSION("1.1.0");
MODULE_LICENSE("Dual BSD/GPL");

#define TAMC200_VENDOR_ID    0x10B5	/* TEWS vendor ID */
#define TAMC200_DEVICE_ID    0x9030	/* IP carrier board device ID */
#define TAMC200_SUBVENDOR_ID 0x1498	/* TEWS vendor ID */
#define TAMC200_SUBDEVICE_ID 0x80C8	/* IP carrier board device ID */


struct STamc200{
    struct pciedev_dev  dev;
    void*				sharedAddress;
    int					deviceIrqAddress;
    int                 numberOfIRQs;
    enum EIpCarrierType carrierType[TAMC200_NR_CARRIERS];
    u32                 event_num;
    u32                 isIrqActive : 1;
    u32                 u32remainingBits : 31;
    wait_queue_head_t	waitIRQ;
};

static long           tamc200_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int            tamc200_mmap(struct file *a_filp, struct vm_area_struct *a_vma);
static int  __devinit tamc200_probe(struct pci_dev *a_dev, const struct pci_device_id *id);
static void __devexit tamc200_remove(struct pci_dev *dev);

static struct pciedev_cdev s_tamc200_cdev={0,};

static const struct pci_device_id s_tamc200_ids[]  = {
    { TAMC200_VENDOR_ID, TAMC200_DEVICE_ID,TAMC200_SUBVENDOR_ID, TAMC200_SUBDEVICE_ID, 0, 0, 0},
    { 0, }
};
MODULE_DEVICE_TABLE(pci, esdadio_ids);

static const struct file_operations s_tamc200FileOps = {
    .owner                    =  THIS_MODULE,
    .read                     =  pciedev_read_exp,
    .write                    =  pciedev_write_exp,
    .unlocked_ioctl           =  tamc200_ioctl,
    .open                     =  pciedev_open_exp,
    .release                  =  pciedev_release_exp,
    .mmap                     =  tamc200_mmap,
    .llseek                   =  pciedev_llseek_exp
};

static struct pci_driver s_tamc200_driver = {
    .name       = TAMC200_DRV_NAME,
    .id_table   = s_tamc200_ids,
    .probe      = tamc200_probe,
    .remove    = __devexit_p(tamc200_remove),
};

static void tamc200_vma_open  (struct vm_area_struct *a_vma) { (void)a_vma; }
static void tamc200_vma_close (struct vm_area_struct *a_vma) { (void)a_vma; }


static const struct vm_operations_struct s_tamc200_mmap_vm_ops =
{
    .open = tamc200_vma_open,
    .close = tamc200_vma_close,
    //.fault = daq_vma_fault,	// Finally page fault exception should be used to do
    // page size changing really dynamic
};


static int tamc200_mmap(struct file *a_filp, struct vm_area_struct *a_vma)
{
    struct pciedev_dev*		dev = a_filp->private_data;
    struct STamc200*		pTamc200 = dev->parent;
    unsigned long sizeFrUser = a_vma->vm_end - a_vma->vm_start;

    if(sizeFrUser == (unsigned long)SIZE_FOR_INTR_SHMEM){
        unsigned long sizeOrig = IP_TIMER_WHOLE_BUFFER_SIZE_ALL;
        unsigned int size = sizeFrUser>sizeOrig ? sizeOrig : sizeFrUser;

        ALERTCT("\n");

        if (!pTamc200->sharedAddress){
            ERRCT("device is not registered for interrupts\n");
            return -ENODEV;
        }

        if (remap_pfn_range(a_vma, a_vma->vm_start, virt_to_phys((void *)pTamc200->sharedAddress) >> PAGE_SHIFT, size, a_vma->vm_page_prot) < 0){
            ERRCT("remap_pfn_range failed\n");
            return -EIO;
        }

        a_vma->vm_private_data = pTamc200;
        a_vma->vm_ops = &s_tamc200_mmap_vm_ops;
        tamc200_vma_open(a_vma);

        return 0;
    }

    return pciedev_remap_mmap_exp(a_filp,a_vma);
}


struct STamc200       s_vTamc200_dev[TAMC200_NR_DEVS];	/* allocated in iptimer_init_module */
static const int      s_ips_irq_vec[3] = { 0xFC, 0xFD, 0xFE };

/*
 * The top-half interrupt handler.
 */
static irqreturn_t tamc200_interrupt(INTR_ARGS(int a_irq, void *a_dev_id, struct pt_regs* a_regs))
{
    struct STamc200 * pTamc200 = a_dev_id;
    int* pnBufferIndex = (int*)pTamc200->sharedAddress;
    int nNextBufferIndex ;
    char* deviceBar2Address = (char*)pTamc200->dev.memmory_base[2];
    char* ip_base_addres = (char*)pTamc200->dev.memmory_base[3] + pTamc200->deviceIrqAddress;
    struct STimeTelegram* pTimeTelegram ;
    struct timeval	aTv;
    u16 uEvLow;
    u16 uEvHigh;

    (void)a_irq;
    UNWARN_INTR_LAST_ARG(a_regs)

    do_gettimeofday(&aTv);

    uEvLow = ioread16(deviceBar2Address + 0xC);
    smp_rmb();
    if (uEvLow == 0){ return IRQ_NONE; }

    uEvLow = ioread16(ip_base_addres + 0x40);//?
    smp_rmb();
    if (pTamc200->dev.swap){ uEvLow=UPCIEDEV_SWAPS(uEvLow); }

    uEvHigh = ioread16(ip_base_addres + 0x42);//?
    smp_rmb();
    if (pTamc200->dev.swap){ uEvHigh=UPCIEDEV_SWAPS(uEvHigh); }

    pTamc200->event_num = (long)uEvHigh;
    pTamc200->event_num <<= 16;
    pTamc200->event_num |= (long)uEvLow;

    nNextBufferIndex = pTamc200->event_num % IP_TIMER_RING_BUFFERS_COUNT;
    pTimeTelegram = (struct STimeTelegram*)((char*)pTamc200->sharedAddress + _OFFSET_TO_SPECIFIC_BUFFER_(nNextBufferIndex));

    pTimeTelegram->gen_event = (int)pTamc200->event_num;
    pTimeTelegram->seconds = aTv.tv_sec;
    pTimeTelegram->useconds = aTv.tv_usec;

    *pnBufferIndex = nNextBufferIndex;
    ++pTamc200->numberOfIRQs;
    wake_up(&pTamc200->waitIRQ);

    iowrite16(0xFFFF, deviceBar2Address + 0xC); //?
    smp_wmb();

    iowrite16(0xFFFF, ip_base_addres + 0x3A); //?
    smp_wmb();

    return IRQ_HANDLED;
}


static void DisableAllInterrupts(struct STamc200* a_pTamc200)
{
    ALERTCT("\n");

    if (a_pTamc200->isIrqActive){
        free_irq(a_pTamc200->dev.pci_dev_irq, a_pTamc200);
        _DEALLOC_MEMORY_(a_pTamc200->sharedAddress);
        a_pTamc200->isIrqActive = 0;
    }
    a_pTamc200->dev.irq_type = 0;
}



static int EnableInterrupt(struct STamc200* a_pTamc200, int a_IpModule)
{
    char*	deviceBar2Address = (char*)(a_pTamc200->dev.memmory_base[2]);
    char*	deviceBar3Address = (char*)(a_pTamc200->dev.memmory_base[3]);
    char*	ip_base_addres = deviceBar3Address + 0x100 * a_IpModule;
    int nReturn = 0;
    u16 tmp_slot_cntrl = 0;

    ALERTCT("\n");

    a_IpModule = a_IpModule > 2 ? 2 : (a_IpModule < 0 ? 0 : a_IpModule);

    if (!a_pTamc200->isIrqActive){
        ALERTCT(KERN_ALERT "IRQ FOR IP MODULE %i ENABLED\n", a_IpModule);
        tmp_slot_cntrl |= (1 & 0x3) << 6;
        tmp_slot_cntrl |= 0x0020;

        a_pTamc200->sharedAddress = _ALLOC_MEMORY_(GFP_KERNEL);
        if (!a_pTamc200->sharedAddress){
            ERRCT("No memory!\n");
            return -1;
        }

        *((int*)a_pTamc200->sharedAddress) = IP_TIMER_RING_BUFFERS_COUNT-1;
        a_pTamc200->deviceIrqAddress = a_IpModule * 0x100;
        init_waitqueue_head(&a_pTamc200->waitIRQ);
        nReturn = request_irq(a_pTamc200->dev.pci_dev_irq, &tamc200_interrupt, IRQF_SHARED , TAMC200_DRV_NAME, a_pTamc200);
        if (nReturn){
            ERRCT("Unable to activate interrupt for pin %d\n", a_pTamc200->dev.pci_dev_irq);
            return nReturn;
        }

        a_pTamc200->isIrqActive = 1;
        a_pTamc200->numberOfIRQs = 0;
        ALERTCT("\n");
        a_pTamc200->dev.irq_type = 2;
    }

    //printk(KERN_ALERT "TAMC200_PROBE:  SLOT %i CNTRL %X\n", k, tmp_slot_cntrl);
    iowrite16(tmp_slot_cntrl, deviceBar2Address + 0x2 * (a_IpModule + 1));
    smp_wmb();
    iowrite16(s_ips_irq_vec[a_IpModule], (ip_base_addres + 0x2E));
    smp_wmb();

    return 0;
}


static void __devexit tamc200_remove(struct pci_dev* a_dev)
{
    pciedev_dev* pciedevdev = dev_get_drvdata(&(a_dev->dev));
    if(pciedevdev && pciedevdev->dev_sts){
        struct STamc200*		pTamc200 = container_of(pciedevdev, struct STamc200, dev);
        char* deviceBar2Address = (char*)pTamc200->dev.memmory_base[2];
        char* deviceBar3Address = (char*)pTamc200->dev.memmory_base[3];
        char* ip_base_addres;
        int   k;
        int   tmp_slot_num;

        ALERTCT( "SLOT %d BOARD %d\n", pciedevdev->slot_num, pciedevdev->brd_num);

        /*DISABLING INTERRUPTS ON THE MODULES*/
        iowrite16(0x0000, deviceBar2Address + 0x2);
        iowrite16(0x0000, deviceBar2Address + 0x4);
        iowrite16(0x0000, deviceBar2Address + 0x6);
        smp_wmb();

        DisableAllInterrupts(pTamc200);

        for(k = 0; k < TAMC200_NR_CARRIERS; ++k){
            switch(pTamc200->carrierType[k]){
            case IpCarrierDelayGate:{
                ip_base_addres = (char*)deviceBar3Address + 0x100*k;
                iowrite16(0x0000, (ip_base_addres + 0x2A));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x2A));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x20));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x22));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x24));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x26));
                smp_wmb();
                iowrite16(0x0000, (ip_base_addres + 0x28));
                smp_wmb();
                iowrite16(0xFFFF, (ip_base_addres + 0x2C));
                smp_wmb();
            }break;
            default:
                break;
            }  // switch(pTamc200->carrierType[k]){
        } // for(k = 0; k < TAMC200_NR_SLOTS; ++k)

        memset(pTamc200, 0, sizeof(struct STamc200));
        pciedev_remove_exp(a_dev,&s_tamc200_cdev, TAMC200_DEVNAME, &tmp_slot_num);
    }
}


static int __devinit tamc200_probe(struct pci_dev* a_dev, const struct pci_device_id* a_id)
{
    int tmp_brd_num;
    //int result = pciedev_probe_exp(a_dev,a_id,&s_tamc200FileOps,&s_tamc200_cdev,TAMC200_DEVNAME,&tmp_brd_num);
    int result = pciedev_probe_exp(a_dev,a_id,NULL,&s_tamc200_cdev,TAMC200_DEVNAME,&tmp_brd_num);

    char*	deviceBar2Address;
    char*	deviceBar3Address;
    char*  ip_base_addres;
    int nReturn;
    int k;
    int brdNum = a_dev->brd_num % TAMC200_NR_DEVS;
    u32 tmp_module_id;
    u16 tmp_data_16;

    ALERTCT("\n");


    if (unlikely(s_vTamc200_dev[brdNum].dev)) return -1; // Already in use
    s_vTamc200_dev[brdNum].dev = a_dev;

    nReturn = Mtcagen_GainAccess_exp(a_dev, 0, NULL, &s_tamc200FileOps, DRV_NAME, "%ss%d", DEVNAME, a_dev->brd_num);
    if (nReturn)
    {
        ERRCT("nReturn = %d\n", nReturn);
        s_vTamc200_dev[brdNum].dev = NULL;
        return nReturn;
    }
    a_dev->parent = &s_vTamc200_dev[brdNum];

    deviceBar2Address = (char*)s_vTamc200_dev[brdNum].dev->memmory_base[2];
    deviceBar3Address = (char*)s_vTamc200_dev[brdNum].dev->memmory_base[3];


    for (k = 0; k < TAMC200_NR_SLOTS; k++)
    {
        //if(tamc200_dev[tmp_slot_num].ip_s[k].ip_on)
        {
            ALERTCT("TAMC200_PROBE:  SLOT %i ENABLED\n", k);

            ip_base_addres = deviceBar3Address + 0x100 * k;

            tmp_data_16 = ioread16(ip_base_addres + 0x80);
            smp_rmb();
            tmp_module_id = tmp_data_16 << 24;

            tmp_data_16 = ioread16(ip_base_addres + 0x82);
            smp_rmb();
            tmp_module_id += (tmp_data_16 << 16);

            tmp_data_16 = ioread16(ip_base_addres + 0x84);
            smp_rmb();
            tmp_module_id += tmp_data_16 << 8;

            tmp_data_16 = ioread16(ip_base_addres + 0x86);
            smp_rmb();
            tmp_module_id += tmp_data_16;

            ALERTCT("TAMC200_PROBE: MODULE ID %X\n", tmp_module_id);
        }
    } // for(k = 0; k < TAMC200_NR_SLOTS; k++)

    return 0;
}



static void tamc200_vma_open(struct vm_area_struct *a_vma){}
static void tamc200_vma_close(struct vm_area_struct *a_vma){}


static struct vm_operations_struct tamc200_mmap_vm_ops =
{
    .open = tamc200_vma_open,
    .close = tamc200_vma_close,
    //.fault = daq_vma_fault,	// Finally page fault exception should be used to do
    // page size changing really dynamoc
};


static long  tamc200_ioctl(struct file *a_filp, unsigned int a_cmd, unsigned long a_arg)
{
    struct pciedev_dev*		dev = a_filp->private_data;
    struct STamc200*		pTamc200 = dev->parent;
    struct SIrqWaiterStruct*	psIRQ = &pTamc200->irqWaiterStruct;
    long nReturn = 0;
    long lnNextNumberOfIRQDone = pTamc200->irqWaiterStruct.numberOfIRQs + 1;
    u64						ulnJiffiesTmOut;
    int32_t nUserValue;

    DEBUGNEW("\n");

    if (unlikely(!dev->dev_sts))
    {
        WARNCT("device has been taken out!\n");
        return -ENODEV;
    }

    //if (_IOC_TYPE(cmd) != TIMING_IOC)    return -ENOTTY;
    //if (_IOC_NR(cmd) > PCIE_GEN_IOC_MAXNR) return -ENOTTY;
    //if (mutex_lock_interruptible(&dev->m_PcieGen.m_dev_mut))return -ERESTARTSYS;

    switch (a_cmd)
    {
    case IP_TIMER_TEST1:
        DEBUGNEW("IP_TIMER_TEST1\n");
        return s_nOtherDeviceInterrupt;
        break;

    case GEN_REQUEST_IRQ1_FOR_DEV: case GEN_UNREQUEST_IRQ_FOR_DEV: break; // These are not allowed to do

    case IP_TIMER_TEST_TIMING:
#ifdef DEBUG_TIMING
        copy_to_user((struct STimingTest*)a_arg, &s_TimingTest, sizeof(struct STimingTest));
#endif
        break;

    case IP_TIMER_ACTIVATE_INTERRUPT: case GEN_REQUEST_IRQ2_FOR_DEV:
        DEBUGNEW("IP_TIMER_ACTIVATE_INTERRUPT\n");
        if (psIRQ->isIrqActive) return 0;
        if (copy_from_user(&nUserValue, (int32_t*)a_arg, sizeof(int32_t)))
        {
            nReturn = -EFAULT;
            goto returnPoint;
        }
        nReturn = EnableInterrupt(pTamc200, nUserValue);
        break;

    case IP_TIMER_WAIT_FOR_EVENT_INF:
        DEBUGNEW("IP_TIMER_WAIT_FOR_EVENT_INF\n");
        if (unlikely(!psIRQ->isIrqActive)) return -1;
        nReturn = wait_event_interruptible(psIRQ->waitIRQ, lnNextNumberOfIRQDone <= psIRQ->numberOfIRQs);
        break;

    case IP_TIMER_WAIT_FOR_EVENT_TIMEOUT:
        DEBUGNEW("IP_TIMER_WAIT_FOR_EVENT_TIMEOUT\n");
        if (unlikely(!psIRQ->isIrqActive)) return -1;
        if (copy_from_user(&nUserValue, (int32_t*)a_arg, sizeof(int32_t)))
        {
            nReturn = -EFAULT;
            goto returnPoint;
        }
        ulnJiffiesTmOut = msecs_to_jiffies(nUserValue);
        nReturn = wait_event_interruptible_timeout(psIRQ->waitIRQ, lnNextNumberOfIRQDone <= psIRQ->numberOfIRQs, ulnJiffiesTmOut);
        break;

    default:
        DEBUGNEW("default\n");
        return mtcagen_ioctl_exp(a_filp, a_cmd, a_arg);
    }

returnPoint:

    return nReturn;
}


void __exit tamc200_cleanup_module(void)
{
    ALERTCT("\n");
    removeIDfromMainDriverByParam(&s_DevParam);
}

static int __init tamc200_init_module(void)
{
    int result;
    ALERTCT("\n");
    memset(s_vTamc200_dev, 0, sizeof(s_vTamc200_dev));
    result = upciedev_driver_init_exp(&s_tamc200_cdev,&s_tamc200FileOps,TAMC200_DEVNAME);
    pci_register_driver(&s_tamc200_driver);

    printk(KERN_ALERT "!!!!!!!!!!!!!!! __USER_CS=%d(0x%x), __USER_DS=%d(0x%x), __KERNEL_CS=%d(%d), __KERNEL_DS=%d(0x%x)\n",
        (int)__USER_CS, (int)__USER_CS, (int)__USER_DS, (int)__USER_DS, (int)__KERNEL_CS, (int)__KERNEL_CS, (int)__KERNEL_DS, (int)__KERNEL_DS);

    ALERTCT("============= version 10:, result=%d\n",result);

    return result; /* succeed when result==0*/
}

module_init(tamc200_init_module);
module_exit(tamc200_cleanup_module);

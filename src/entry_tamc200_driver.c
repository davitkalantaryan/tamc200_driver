/**
 *Copyright 2015-  DESY (Deutsches Elektronen-Synchrotron, www.desy.de)
 *
 *This file is part of TAMC200 driver.
 *
 *TAMC200 is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *TAMC200 is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with TAMC200.  If not, see <http://www.gnu.org/licenses/>.
 **/

/*
 *	File: timer_drv_main.c
 *
 *	Created on: Oct 15, 2013, Modified 2015 Oct. 5
 *  Created by: Davit Kalantaryan
 *	Authors (maintainers):
 *      Davit Kalantaryan (davit.kalantaryan@desy.de)
 *      Ludwig Petrosyan  (ludwig.petrosyan@desy.de)
 *
 *
 */

#define	TAMC200_NR_DEVS		(PCIEDEV_NR_DEVS + 1)

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
#include <linux/spinlock.h>
#include <pciedev_ufn.h>
#include "pciedev_ufn2.h"
#include <debug_functions.h>
#include "tamc200_io.h"
#include "linux_version_dependence.h"

#ifdef __INTELISENSE__
#include "../../include/used_for_driver_intelisense.h"
#endif

MODULE_AUTHOR("Davit Kalantaryan, Ludwig Petrosyan");
MODULE_DESCRIPTION("Driver for TEWS TAMC200 IP carrier");
MODULE_VERSION("1.1.0");
MODULE_LICENSE("Dual BSD/GPL");

#define TAMC200_VENDOR_ID    0x10B5	/* TEWS vendor ID */
#define TAMC200_DEVICE_ID    0x9030	/* IP carrier board device ID */
#define TAMC200_SUBVENDOR_ID 0x1498	/* TEWS vendor ID */
#define TAMC200_SUBDEVICE_ID 0x80C8	/* IP carrier board device ID */

struct SIntrReport{
    int                 numberOfIRQs;
    wait_queue_head_t	waitIRQ;
};

struct STamc200{
    pciedev_dev*        dev_p;
    void*				sharedAddresses[TAMC200_NR_CARRIERS];
    //int					deviceIrqAddresses[TAMC200_NR_CARRIERS];  // a_pTamc200->deviceIrqAddresses = a_IpModule * 0x100;
    enum EIpCarrierType carrierType[TAMC200_NR_CARRIERS];
    struct SIntrReport  intrData[TAMC200_NR_CARRIERS];
    spinlock_t          intrLocks[TAMC200_NR_CARRIERS];
    u32                 isIrqActive[TAMC200_NR_CARRIERS];
    u32                 numberIrqActive : 4;  // keeps count of carrier modules, requested for interrupt
    u32                 u32remainingBits : 28;
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
MODULE_DEVICE_TABLE(pci, s_tamc200_ids);

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

struct STamc200       s_vTamc200_dev[TAMC200_NR_DEVS];	/* allocated in iptimer_init_module */
static const int      s_ips_irq_vec[3] = { 0xFC, 0xFD, 0xFE };

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
    int tmp_bar_num = a_vma->vm_pgoff;
    if(tmp_bar_num>=NUMBER_OF_BARS){
        int nCarrier = tmp_bar_num-NUMBER_OF_BARS;
        if(nCarrier<TAMC200_NR_CARRIERS){
            struct pciedev_dev*		dev = a_filp->private_data;
            struct STamc200*		pTamc200 = dev->parent;
            unsigned long sizeFrUser = a_vma->vm_end - a_vma->vm_start;
            unsigned long sizeOrig = IP_TIMER_WHOLE_BUFFER_SIZE_ALL;
            unsigned int size = sizeFrUser>sizeOrig ? sizeOrig : sizeFrUser;

            if (!pTamc200->sharedAddresses[nCarrier]){
                ERRCT("device is not registered for interrupts\n");
                return -ENODEV;
            }

            if (remap_pfn_range(a_vma, a_vma->vm_start, virt_to_phys((void *)pTamc200->sharedAddresses[nCarrier])>>PAGE_SHIFT,size,a_vma->vm_page_prot)<0){
                ERRCT("remap_pfn_range failed\n");
                return -EIO;
            }

            a_vma->vm_private_data = pTamc200;
            a_vma->vm_ops = &s_tamc200_mmap_vm_ops;
            tamc200_vma_open(a_vma);

            return 0;
        }
    }

    return pciedev_remap_mmap_exp(a_filp,a_vma);
}



/*
 * The top-half interrupt handler.
 */
static irqreturn_t tamc200_interrupt(INTR_ARGS(int a_irq, void *a_dev_id, struct pt_regs* a_regs))
{
    // see documentation "tamc200_user_manual.pdf", page 23
    static const u16 sintrMasks[TAMC200_NR_CARRIERS]={0x03,0x0c,0x30};
    struct STamc200 * pTamc200 = a_dev_id;
    int* pnBufferIndex;
    int nNextBufferIndex ;
    char* deviceBar2Address = (char*)pTamc200->dev_p->memmory_base[2];
    char* deviceBar3Address = (char*)pTamc200->dev_p->memmory_base[3];
    char* ip_base_addres;
    struct STimeTelegram* pTimeTelegram ;
    struct timeval	aTv;
    int anyCarrierInterrupted = 0;
    int cr;
    u16 ipStatusReg;
    u16 uEvLow;
    u16 uEvHigh;
    u32 event_num;

    (void)a_irq;
    UNWARN_INTR_LAST_ARG(a_regs)

    do_gettimeofday(&aTv);

    // see documentation "tamc200_user_manual.pdf", page 23
    ipStatusReg = ioread16(deviceBar2Address + 0xC);
    smp_rmb();

    for(cr=0;cr<TAMC200_NR_CARRIERS;++cr){
        spin_lock(&(pTamc200->intrLocks[cr]));
        if((ipStatusReg&sintrMasks[cr])&&pTamc200->isIrqActive[cr]){
            anyCarrierInterrupted = 1;
            ip_base_addres = deviceBar3Address + cr*0x100; // a_IpModule * 0x100
            pnBufferIndex = (int*)pTamc200->sharedAddresses[cr];

            switch(pTamc200->carrierType[cr]){
            case IpCarrierDelayGate:
                uEvLow = ioread16(ip_base_addres + 0x40);
                smp_rmb();
                if (pTamc200->dev_p->swap){ uEvLow=UPCIEDEV_SWAPS(uEvLow); }

                uEvHigh = ioread16(ip_base_addres + 0x42);
                smp_rmb();
                if (pTamc200->dev_p->swap){ uEvHigh=UPCIEDEV_SWAPS(uEvHigh); }

                event_num = (long)uEvHigh;
                event_num <<= 16;
                event_num |= (long)uEvLow;

                nNextBufferIndex = event_num % IP_TIMER_RING_BUFFERS_COUNT;
                pTimeTelegram = (struct STimeTelegram*)((char*)pTamc200->sharedAddresses[cr] + _OFFSET_TO_SPECIFIC_BUFFER_(nNextBufferIndex));

                pTimeTelegram->gen_event = (int)event_num;
                pTimeTelegram->seconds = aTv.tv_sec;
                pTimeTelegram->useconds = aTv.tv_usec;

                iowrite16(0xFFFF, ip_base_addres + 0x3A);
                smp_wmb();
                break;
            default:
                nNextBufferIndex = 0;
                break;
            }

            *pnBufferIndex = nNextBufferIndex;
            ++pTamc200->intrData[cr].numberOfIRQs;
            wake_up(&pTamc200->intrData[cr].waitIRQ);

            iowrite16(0xFFFF, deviceBar2Address + 0xC); // reset interrupt (see documentation "tamc200_user_manual.pdf", page 24 )
            smp_wmb();
        }  // if(ipStatusReg&sintrMasks[cr]){
        spin_unlock(&(pTamc200->intrLocks[cr]));
    }  // for(cr=0;cr<TAMC200_NR_CARRIERS;++cr){

    return anyCarrierInterrupted?IRQ_HANDLED:IRQ_NONE;
}


static void DisableInterrupt(struct STamc200* a_pTamc200, int a_ipModule)
{
    const int ipModule = a_ipModule % TAMC200_NR_CARRIERS;
    ALERTCT("\n");

    spin_lock(&(a_pTamc200->intrLocks[ipModule]));
	ALERTCT("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); // todo: delete this
    if (a_pTamc200->isIrqActive[ipModule]){
        a_pTamc200->isIrqActive[ipModule] = 0;
        spin_unlock(&(a_pTamc200->intrLocks[ipModule]));
        _DEALLOC_MEMORY_(a_pTamc200->sharedAddresses[ipModule]);
        a_pTamc200->sharedAddresses[ipModule] = NULL;
    }
    else{
        spin_unlock(&(a_pTamc200->intrLocks[ipModule]));
    }

    if((--(a_pTamc200->numberIrqActive))==0){
        free_irq(a_pTamc200->dev_p->pci_dev_irq, a_pTamc200);
        a_pTamc200->dev_p->irq_type = 0;
    }
}



static int EnableInterrupt(struct STamc200* a_pTamc200, int a_ipModule)
{
    const int ipModule = a_ipModule % TAMC200_NR_CARRIERS;
    ALERTCT("\n");

    if(!a_pTamc200->isIrqActive[ipModule]){
        char*	deviceBar2Address = (char*)(a_pTamc200->dev_p->memmory_base[2]);
        char*	deviceBar3Address = (char*)(a_pTamc200->dev_p->memmory_base[3]);
        char*	ip_base_addres = deviceBar3Address + 0x100 * ipModule;
        int nReturn = 0;
        u16 tmp_slot_cntrl = 0;

        tmp_slot_cntrl |= (1 & 0x3) << 6;
        tmp_slot_cntrl |= 0x0020;

        a_pTamc200->sharedAddresses[ipModule] = _ALLOC_MEMORY_(GFP_KERNEL);
        if (!a_pTamc200->sharedAddresses[ipModule]){
            ERRCT("No memory!\n");
            return -ENOMEM;
        }

        *((int*)a_pTamc200->sharedAddresses[ipModule]) = IP_TIMER_RING_BUFFERS_COUNT-1;
        init_waitqueue_head(&a_pTamc200->intrData[ipModule].waitIRQ);
        if((a_pTamc200->numberIrqActive++)==0){
            nReturn = request_irq(a_pTamc200->dev_p->pci_dev_irq, &tamc200_interrupt, IRQF_SHARED , TAMC200_DRV_NAME, a_pTamc200);
            if (nReturn){
                a_pTamc200->numberIrqActive = 2;
                DisableInterrupt(a_pTamc200,ipModule);
                a_pTamc200->numberIrqActive = 0;
                ERRCT("Unable to activate interrupt for pin %d\n", a_pTamc200->dev_p->pci_dev_irq);
                return nReturn;
            }
            a_pTamc200->dev_p->irq_type = 2;
        }

        a_pTamc200->intrData[ipModule].numberOfIRQs =0;
        a_pTamc200->isIrqActive[ipModule] = 1;

        iowrite16(tmp_slot_cntrl, deviceBar2Address + 0x2 * (ipModule + 1));
        smp_wmb();
        iowrite16(s_ips_irq_vec[ipModule], (ip_base_addres + 0x2E));
        smp_wmb();
    }

    return 0;
}


static void __devexit tamc200_remove(struct pci_dev* a_dev)
{
    pciedev_dev* pciedevdev = dev_get_drvdata(&(a_dev->dev));
    if(pciedevdev && pciedevdev->dev_sts){
        //struct STamc200*		pTamc200 = container_of(pciedevdev, struct STamc200, dev);
        //parent
        struct STamc200*		pTamc200 = pciedevdev->parent;
        char* deviceBar2Address = (char*)pciedevdev->memmory_base[2];
        char* deviceBar3Address = (char*)pciedevdev->memmory_base[3];
        char* ip_base_addres;
        int   cr;
        //int   tmp_slot_num;

		ALERTCT("!!!!!!!!!!!!!!!!!!!!!!!!!!! pTamc200=%p\n",pTamc200); // todo: delete this
        ALERTCT( "SLOT %d BOARD %d\n", pciedevdev->slot_num, pciedevdev->brd_num);

        /*DISABLING INTERRUPTS ON THE MODULES*/
        iowrite16(0x0000, deviceBar2Address + 0x2);
        iowrite16(0x0000, deviceBar2Address + 0x4);
        iowrite16(0x0000, deviceBar2Address + 0x6);
        smp_wmb();

        for(cr = 0; cr < TAMC200_NR_CARRIERS; ++cr){
            DisableInterrupt(pTamc200,cr);
            switch(pTamc200->carrierType[cr]){
            case IpCarrierDelayGate:{
                ip_base_addres = (char*)deviceBar3Address + cr*0x100;
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
        pciedev_remove_single_device_exp(a_dev,&s_tamc200_cdev, TAMC200_DEVNAME);
    }
}


static int __devinit tamc200_probe(struct pci_dev* a_dev, const struct pci_device_id* a_id)
{
    char*	deviceBar2Address;
    char*	deviceBar3Address;
    char*  ip_base_addres;
    int tmp_brd_num;
    int result;
    int cr;
    u32 tmp_module_id;
    u16 tmp_data_16;
    pciedev_dev* dev_p = kzalloc(sizeof(pciedev_dev),GFP_KERNEL);
    if(!dev_p){
        return -ENOMEM;
    }
    (void)a_id;
    result = pciedev_probe_of_single_device_exp(a_dev,dev_p,TAMC200_DEVNAME,&s_tamc200_cdev,&s_tamc200FileOps);
    if(result){
        kfree(dev_p);
        ERRCT("pciedev_probe_of_single_device_exp failed\n");
        return result;
    }
    tmp_brd_num = dev_p->brd_num;
    if(s_vTamc200_dev[tmp_brd_num].dev_p){
        kfree(dev_p);
        ERRCT("board number %d is already in use\n",tmp_brd_num);
        return -EBUSY;
    }

    s_vTamc200_dev[tmp_brd_num].dev_p = dev_p;
	dev_p->parent = &(s_vTamc200_dev[tmp_brd_num]);
    deviceBar2Address = (char*)dev_p->memmory_base[2];
    deviceBar3Address = (char*)dev_p->memmory_base[3];
	
    for (cr = 0; cr < TAMC200_NR_CARRIERS; ++cr){
        //if(tamc200_dev[tmp_slot_num].ip_s[k].ip_on)
        {
            ALERTCT("TAMC200_PROBE:  CARRIER %i ENABLED\n", cr);

            ip_base_addres = deviceBar3Address + 0x100 * cr;

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
    } // for (cr = 0; cr < TAMC200_NR_CARRIERS; ++cr){

    return 0;
}


static long  tamc200_ioctl(struct file *a_filp, unsigned int a_cmd, unsigned long a_arg)
{
    struct pciedev_dev*		dev = a_filp->private_data;
    struct STamc200*		pTamc200 = dev->parent;
    long nReturn = 0;
    long    lnNextNumberOfIRQDone;
    u64		ulnJiffiesTmOut;
    int32_t nCarrierModule;

    DEBUGNEW("\n");

    if (unlikely(!dev->dev_sts)){
        WARNCT("device has been taken out!\n");
        return -ENODEV;
    }

    //if (_IOC_TYPE(cmd) != TIMING_IOC)    return -ENOTTY;
    //if (_IOC_NR(cmd) > PCIE_GEN_IOC_MAXNR) return -ENOTTY;
    //if (mutex_lock_interruptible(&dev->m_PcieGen.m_dev_mut))return -ERESTARTSYS;

    switch (a_cmd)
    {
    case IP_TIMER_ACTIVATE_INTERRUPT: /*case GEN_REQUEST_IRQ2_FOR_DEV:*/
        DEBUGNEW("IP_TIMER_ACTIVATE_INTERRUPT\n");
        if (copy_from_user(&nCarrierModule, (int32_t*)a_arg, sizeof(int32_t))){
            nReturn = -EFAULT;
            goto returnPoint;
        }
        nCarrierModule %= TAMC200_NR_CARRIERS;
        if (pTamc200->isIrqActive[nCarrierModule]) return 0;
        nReturn = EnableInterrupt(pTamc200, nCarrierModule);
        break;

    case IP_TIMER_WAIT_FOR_EVENT_INF:
        DEBUGNEW("IP_TIMER_WAIT_FOR_EVENT_INF\n");
        if (copy_from_user(&nCarrierModule, (int32_t*)a_arg, sizeof(int32_t))){
            nReturn = -EFAULT;
            goto returnPoint;
        }
        nCarrierModule %= TAMC200_NR_CARRIERS;
        if (unlikely(!pTamc200->isIrqActive[nCarrierModule])) return -1;
        lnNextNumberOfIRQDone = pTamc200->intrData[nCarrierModule].numberOfIRQs;
        nReturn = wait_event_interruptible(pTamc200->intrData[nCarrierModule].waitIRQ, lnNextNumberOfIRQDone <= pTamc200->intrData[nCarrierModule].numberOfIRQs);
        break;

    case IP_TIMER_WAIT_FOR_EVENT_TIMEOUT:{
        SWaitInterruptTimeout aWaiterStr;
        DEBUGNEW("IP_TIMER_WAIT_FOR_EVENT_TIMEOUT\n");
        if (copy_from_user(&aWaiterStr, (SWaitInterruptTimeout*)a_arg, sizeof(SWaitInterruptTimeout))){
            nReturn = -EFAULT;
            goto returnPoint;
        }
        nCarrierModule = aWaiterStr.carrierNumber % TAMC200_NR_CARRIERS;
        if (unlikely(!pTamc200->isIrqActive[nCarrierModule])) return -1;
        lnNextNumberOfIRQDone = pTamc200->intrData[nCarrierModule].numberOfIRQs;
        ulnJiffiesTmOut = msecs_to_jiffies(aWaiterStr.miliseconds);
        nReturn = wait_event_interruptible_timeout(pTamc200->intrData[nCarrierModule].waitIRQ,
                                                   lnNextNumberOfIRQDone <= pTamc200->intrData[nCarrierModule].numberOfIRQs, ulnJiffiesTmOut);
    }break;

    default:
        DEBUGNEW("default\n");
        return pciedev_ioctl_exp(a_filp, &a_cmd, &a_arg,&s_tamc200_cdev);
    }

returnPoint:

    return nReturn;
}


static void __exit tamc200_cleanup_module(void)
{
    ALERTCT("\n");
    pci_unregister_driver(&s_tamc200_driver);
    upciedev_driver_clean_exp(&s_tamc200_cdev);
}


static int __init tamc200_init_module(void)
{
    int i, cr;
    int result;

    ALERTCT("\n");
    result = upciedev_driver_init_exp(&s_tamc200_cdev,&s_tamc200FileOps,TAMC200_DEVNAME);

    if(result){
        ERRCT("Unable to init driver\n");
        return result;
    }

    memset(s_vTamc200_dev, 0, sizeof(s_vTamc200_dev));
    for(i=0;i<TAMC200_NR_DEVS;++i){
        for(cr=0;cr<TAMC200_NR_CARRIERS;++cr){
            spin_lock_init(&(s_vTamc200_dev[i].intrLocks[cr]));
        }
    }

    pci_register_driver(&s_tamc200_driver);

    printk(KERN_ALERT "!!!!!!!!!!!!!!! __USER_CS=%d(0x%x), __USER_DS=%d(0x%x), __KERNEL_CS=%d(%d), __KERNEL_DS=%d(0x%x)\n",
        (int)__USER_CS, (int)__USER_CS, (int)__USER_DS, (int)__USER_DS, (int)__KERNEL_CS, (int)__KERNEL_CS, (int)__KERNEL_DS, (int)__KERNEL_DS);

    ALERTCT("============= version 10:, result=%d\n",result);

    return result; /* succeed when result==0*/
}

module_init(tamc200_init_module);
module_exit(tamc200_cleanup_module);

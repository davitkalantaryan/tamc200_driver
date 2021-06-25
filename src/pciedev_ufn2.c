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

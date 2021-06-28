

#ifndef PCIEDEV_UFN2_H
#define PCIEDEV_UFN2_H

#include "pciedev_ufn.h"
#include <linux/types.h>
#include <linux/fs.h>

loff_t pciedev_llseek_exp(struct file *filp, loff_t off, int frm);
void   upciedev_driver_clean_exp(const pciedev_cdev* a_pciedev_cdev_p);
int    upciedev_driver_init_exp(pciedev_cdev* a_pciedev_cdev_p, const struct file_operations* a_pciedev_fops, const char* a_dev_name);
int    pciedev_remove_single_device_exp(struct pci_dev* a_dev, const pciedev_cdev* a_pciedev_cdev_m, const char* a_dev_name);
int    pciedev_probe_of_single_device_exp(struct pci_dev* a_dev, pciedev_dev* a_pciedev, const char* a_dev_name,
                                          struct pciedev_cdev* a_pciedev_cdev_p, const struct file_operations* a_pciedev_fops);


#endif  // #ifndef  PCIEDEV_UFN2_H

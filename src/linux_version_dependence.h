/*
 *	File: version_dependence.h
 *
 *	Created on: Apr 11, 2014
 *	Author: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *
 *
 */
#ifndef __version_dependence_h__
#define __version_dependence_h__


#include <linux/version.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#if LINUX_VERSION_CODE < 0x20613 // irq_handler_t has changed in 2.6.19
#define INTR_ARGS(_irq,_dev_id,_regs)   _irq,_dev_id,_regs
#define UNWARN_INTR_LAST_ARG(_la)       (void)_la;
#else
#define INTR_ARGS(_irq,_dev_id,_regs)   _irq,_dev_id
#define UNWARN_INTR_LAST_ARG(_la)
#endif


#ifdef __CDT_PARSER__
#define __init
#define __exit
#define __user
// add to includes: /var/oe/develop/oe-9g45/openembedded-core/build/tmp-eglibc/work/stamp9g45-angstrom-linux-gnueabi/linux-3.0-r1/linux-3.0/include
// add to includes: /var/oe/develop/oe-9g45/openembedded-core/build/tmp-eglibc/work/stamp9g45-angstrom-linux-gnueabi/linux-3.0-r1/linux-3.0/arch/arm/mach-at91/include
// add to includes: /var/oe/develop/oe-9g45/openembedded-core/build/tmp-eglibc/work/stamp9g45-angstrom-linux-gnueabi/linux-3.0-r1/linux-3.0/arch/arm/include
#endif

#if LINUX_VERSION_CODE < 132632 
typedef void type_work;
#define vers_work_get_device(work_arg,type,member) (work_arg)
#define	vers_INIT_WORK(work_ptr_,func_ptr_,data_)	INIT_WORK((work_ptr_), (func_ptr_), (data_))
#else
typedef struct work_struct type_work;
#define vers_work_get_device container_of
#define	vers_INIT_WORK(work_ptr_,func_ptr_,data_)	INIT_WORK((work_ptr_), (func_ptr_))
#endif



#if		(LINUX_VERSION_CODE>=100)	&&(LINUX_VERSION_CODE<197895)	// kernel 2.6.18


static inline struct proc_dir_entry *vers_create_proc_entry(
	const char* a_name, umode_t a_mode, void* a_pData,
	int(*a_procinfo)(char*, char**, off_t, int, int*, void*)
	)
{
	struct proc_dir_entry * pRet = create_proc_entry(a_name, a_mode, 0);
	pRet->read_proc = a_procinfo;
	pRet->data = a_pData;

	return pRet;
}


#elif	(LINUX_VERSION_CODE>=197895)	&&(LINUX_VERSION_CODE<199945)	// kernel 3.5.0


static inline struct proc_dir_entry *vers_create_proc_entry(
	const char* a_name, umode_t a_mode, void* a_pData,
	int(*a_procinfo)(char*, char**, off_t, int, int*, void*)
	)
{
	struct proc_dir_entry * pRet = create_proc_entry(a_name, a_mode, 0);
	pRet->read_proc = a_procinfo;
	pRet->data = a_pData;

	return pRet;
}



#else																	// kernel 3.13.0


#define		__devinitdata
#define		__devinit
#define		__devexit
#define		__devexit_p(x)		x



static inline struct proc_dir_entry *vers_create_proc_entry(
    const char* a_name, umode_t a_mode, void* a_pData,
    int(*a_procinfo)(char*, char**, off_t, int, int*, void*)
    )
{
    struct proc_dir_entry * pRet = proc_create_data(a_name, a_mode, NULL, NULL, a_pData);
    //pRet->read_proc = a_procinfo;
    //pRet->data = a_pData;
    (void)a_procinfo;
    return pRet;
}

#endif



#endif  /* #ifndef __version_dependence_h__ */

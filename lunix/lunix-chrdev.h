/*
 * lunix-chrdev.h
 *
 * Definition file for the
 * Lunix:TNG character device
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

#ifndef _LUNIX_CHRDEV_H
#define _LUNIX_CHRDEV_H

/*
 * Lunix:TNG character device
 */
#define LUNIX_CHRDEV_MAJOR 60   /* Reserved for local / experimental use */
#define LUNIX_CHRDEV_BUFSZ 20   /* Buffer size used to hold textual info */

/* Compile-time parameters */

#ifdef __KERNEL__ 

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "lunix.h"

/*
 * Private state for an open character device node
 */
struct lunix_chrdev_state_struct {
	enum lunix_msr_enum type;
	struct lunix_sensor_struct *sensor; // 1 sensor!

	/* A buffer used to hold cached textual info */
	int buf_lim;
	unsigned char buf_data[LUNIX_CHRDEV_BUFSZ];
	uint32_t buf_timestamp;

	struct semaphore lock; //for multiple processes reading from the same character device

	/*
	 * Fixme: Any mode settings? e.g. blocking vs. non-blocking
	 */
	enum lunix_data_mode data_mode;
};

/*
 * Function prototypes
 */
int lunix_chrdev_init(void);
void lunix_chrdev_destroy(void);

#endif /* __KERNEL__ */

#include <linux/ioctl.h>

/*
 * Definition of ioctl commands
 */
#define LUNIX_IOC_MAGIC     LUNIX_CHRDEV_MAJOR
#define LUNIX_IOC_RAW_DATA		_IOR(LUNIX_IOC_MAGIC, 0, int *)
#define LUNIX_IOC_COOKED_DATA	_IOR(LUNIX_IOC_MAGIC, 1, int *)

#define LUNIX_IOC_MAXNR 1

#endif /* _LUNIX_H */

/*--------------- MMAP ------------------*/

void lunix_vma_open(struct vm_area_struct *vma);
void lunix_vma_close(struct vm_area_struct *vma);

/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * < team 60 >
 *
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

// We included this for the semaphores - LDD3 page 128
//#include <asm/semaphore.h>

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

/*
 * Just a quick [unlocked] check to see if the cached
 * chrdev state needs to be updated from sensor measurements.
 */
/*
 * Declare a prototype so we can define the "unused" attribute and keep
 * the compiler happy. This function is not yet used, because this helpcode
 * is a stub.
 */
static int __attribute__((unused)) lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *);
static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{
	debug("Are there new data?");

	struct lunix_sensor_struct *sensor;

	WARN_ON ( !(sensor = state->sensor));
	/* ? */
	uint32_t sensor_last_update = sensor->msr_data[state->type]->last_update;

	int ret = (sensor_last_update > state->buf_timestamp);

	debug("The buffer timestamp is %d, and sensor timestamp is %d, ret = %d\n", (int)state->buf_timestamp, (int)sensor_last_update, ret); 
	//WARN_ON (sensor_last_update < state->buf_timestamp);
	return ret; //(sensor_last_update > state->buf_timestamp);
	/* The following return is bogus, just for the stub to compile */
	//return 0; /* ? */
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	debug("Updating state");

	struct lunix_sensor_struct __attribute__((unused)) *sensor;		// this is used to avoid warnings if this pointer remains unused
	WARN_ON ( !(sensor = state->sensor));

	// For spin_lock_irqsave() function
	spinlock_t *sensor_lock = &sensor->lock;
	unsigned long flags;
	uint32_t timestamp;
	uint16_t value;
	
	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */
	/* ? */
	int new_data_present = lunix_chrdev_state_needs_refresh(state);
	if(!new_data_present) return -EAGAIN; 		// See LDD3, p. 170 MAYBE 0
	debug("There were new data present!\n");
	/* Why use spinlocks? See LDD3, p. 119 */
	
	/*
	 * Any new data available?
	 */
	// If there are data, take care of the sensor race condition between line discipline and driver and then read the data.
	debug("Entering spinlock\n");
	spin_lock_irqsave(sensor_lock, flags);
	value = sensor->msr_data[state->type]->values[0];
	timestamp = sensor->msr_data[state->type]->last_update;
	spin_unlock_irqrestore(sensor_lock, flags);
	debug("Exited spinlock\n");
	/* ? */

	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore
	 */
	
	// There are some ready functions to use (uint16_to_light() etc..) but we use look-up tables instead because they
	// are performing floating point operations and we lack a floating point unit.

	long new_meas, new_meas_integer, new_meas_decimal;
	switch(state->type) {
	case BATT:
		new_meas = lookup_voltage[value];
		break;
	case TEMP:
		new_meas = lookup_temperature[value];
		break;
	case LIGHT:
		new_meas = lookup_light[value];
		break;
	default:
		WARN_ON(1);
		break;
	}

	debug("Obtained value: %ld\n", new_meas);
	
	new_meas_integer = new_meas/1000;
	new_meas_decimal = new_meas < 0 ? -new_meas%1000 : new_meas%1000;

	// LDD3 page 130
	debug("Entering mutex\n");
	if(down_interruptible(&state->lock)){
		printk(KERN_ERR "Mutex error!\n");
		return -ERESTARTSYS;
	}
	
	switch(state->data_mode) {
		case COOKED:
			state->buf_lim = snprintf(state->buf_data, LUNIX_CHRDEV_BUFSZ, "%ld.%ld\n", new_meas_integer, new_meas_decimal); // :( no snprintk
		break;

		case RAW:
			state->buf_lim = snprintf(state->buf_data, LUNIX_CHRDEV_BUFSZ, "%d\n", value); // :( no snprintk
		break;

	}
	state->buf_timestamp = timestamp;
	up(&state->lock);
	debug("Exited mutex\n");
	// There is a possibility that more than one processes call update function at the same time. One of them will stop in the sensor lock
	// after the first reading, the one that waited reads the same data. Then it updates its buffer with the same value as the first one
	// (or with potentially new measurements). We consider this right.
	debug("Our sensor is a %s and its value is %ld.%ld", type(state->type), new_meas_integer, new_meas_decimal);	// We defined type macro in lunix.h
	/* ? */

	debug("leaving\n");
	return 0;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

// LDD3 page 76

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	/* Declarations */
	/* ? */
	int ret;
	unsigned int minor_num;
	int s_type, s_id;

	debug("entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;

	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */
	
	/* Allocate a new Lunix character device private state structure */
	/* ? */

	//We don't use the following, since we want a new state per open call.
	//struct lunix_chrdev_state_struct *private_state = container_of(inode->i_cdev, struct lunix_chrdev_state_struct, cdev); ?

	//https://archive.kernel.org/oldlinux/htmldocs/kernel-api/API-kmalloc.html
	struct lunix_chrdev_state_struct *private_state = (struct lunix_chrdev_state_struct *) kzalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL);
	filp->private_data = private_state; 	// See LDD3 pg. 76
	
	// LDD3 page 196
	if(!private_state) {
		ret = -ENOMEM;
		printk(KERN_ERR "Failed allocation");
 		goto out;
 	}
	
	//Perform access control and make sure file struct is configured correctly

	if(!(filp->f_mode & FMODE_READ)){
		printk(KERN_ERR "User doesn't have read permission");
		goto out;
	}

	WARN_ON(filp->f_pos != 0);

	//BUG_ON(filp->f_op != lunix_chrdev_fops);

	//Initialize the private state

	minor_num = iminor(inode);
	s_type = minor_num & ((1<<3)-1); 	// 3 LSB
	s_id = minor_num >> 3;		// sensor number

	private_state->type = s_type;
	private_state->sensor = &(lunix_sensors[s_id]);
	private_state->buf_lim = 0;
	private_state->buf_data[0] = '\0';
	private_state->buf_timestamp = ktime_get_real_seconds(); //https://docs.kernel.org/core-api/timekeeping.html
	sema_init(&private_state->lock, 1);
	private_state->data_mode = COOKED;

out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
	/* ? */
	kfree(filp->private_data);
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk(KERN_DEBUG "The argument of ioctl is: %lu\n", arg);

	int ret;
	struct lunix_chrdev_state_struct *state = filp->private_data;
	printk(KERN_DEBUG "The command of ioctl is: %u\n", cmd);

	
	if (_IOC_TYPE(cmd) != LUNIX_IOC_MAGIC) {
		ret = -ENOTTY;
		goto out;
	}

	if (_IOC_NR(cmd) > LUNIX_IOC_MAXNR) {
		ret = -ENOTTY;
		goto out;
	}

	switch(cmd) {
		case LUNIX_IOC_RAW_DATA:
			printk(KERN_DEBUG "I RECEIVED THE COMMAND CORRECTLY");

			if(down_interruptible(&state->lock)){
				return -ERESTARTSYS;
			}
			
			state->data_mode = RAW;

			up(&state->lock);
			break;
		break;

		case LUNIX_IOC_COOKED_DATA:
			printk(KERN_DEBUG "I RECEIVED THE COMMAND CORRECTLY");

			if(down_interruptible(&state->lock)){
				return -ERESTARTSYS;
			}
			
			state->data_mode = COOKED;

			up(&state->lock);
			break;
		break;

		default:
			ret = -ENOTTY;
			goto out;
		break;
	}


out:
	return ret;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{
	ssize_t ret;

	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;

	state = filp->private_data;
	WARN_ON(!state);

	sensor = state->sensor;
	WARN_ON(!sensor);

	/* Lock? */
	//NOT NOW... State_update and state_refresh lock themselves when necessary
	//We can't lock the same semaphore twice...
	//We will lock after checking for updates
	// if(down_interruptible(&state->lock)) {
	//    ret = -ERESTARTSYS; //LDD3 page 130
	//    goto out;
	// }

	/*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e. we need to report
	 * on a "fresh" measurement, do so
	 */

	if (*f_pos == 0) {
		while (lunix_chrdev_state_update(state) == -EAGAIN) {
			debug("I am in while (lunix_chrdev_state_update(state) == -EAGAIN) loop!\n");
			/* ? */
			/* The process needs to sleep */
			/* See LDD3, page 153 for a hint */
			up(&state->lock); //release the lock

			// if(filp->f_flags & O_NONBLOCK){ //can't block
			// 	ret = -EAGAIN;
			// 	goto out;
			// }

			if(wait_event_interruptible(sensor->wq, lunix_chrdev_state_needs_refresh(state))) {
				ret = -ERESTARTSYS;
				goto out;
			}

			debug("Finished waiting for lunix_chrdev_state_needs_refresh(state)!\n");

			if(down_interruptible(&state->lock)) {
				ret = -ERESTARTSYS;
				goto out;
			}
		}
	}

	debug("EXITED WHILE LOOP!\n");

	//data is there
	
	//Now we lock...
	if(down_interruptible(&state->lock)) {
	   ret = -ERESTARTSYS; //LDD3 page 130
	   goto out;
	}

	/* End of file */
	/* ? */
	int eof = state->buf_lim;

	/* Determine the number of cached bytes to copy to userspace */
	/* ? */
	cnt = (eof - *f_pos <= cnt) ? (eof - *f_pos) : cnt;

	//Read data
	if(copy_to_user(usrbuf, state->buf_data + *f_pos, cnt)) {
		ret = -EFAULT;
		goto out_with_lock;
	}

	debug("Copied data to user!\n");

	*f_pos += cnt;
	ret = cnt;

	/* Auto-rewind on EOF mode? */
	/* ? */
	if(*f_pos == eof) {
		*f_pos = 0;
	}
	/*
	 * The next two lines  are just meant to suppress a compiler warning
	 * for the "unused" out: label, and for the uninitialized "ret" value.
	 * It's true, this helpcode is a stub, and doesn't use them properly.
	 * Remove them when you've started working on this code.
	 */
	 //ret = -ENODEV;
	 //goto out;

out_with_lock:
	up (&state->lock);
out:
	return ret;
}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops = 
{
	.owner          = THIS_MODULE,
	.open           = lunix_chrdev_open,
	.release        = lunix_chrdev_release,
	.read           = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap           = lunix_chrdev_mmap,
	.llseek			= no_llseek 			// See LDD3 pg. 191
};

int lunix_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;

	debug("initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	/* ? */
	/* register_chrdev_region? */
	ret = register_chrdev_region(dev_no, lunix_minor_cnt, "lunix-tng");		// LDD3 page 63

	if (ret < 0) {
		printk(KERN_ERR "failed to register region, ret = %d\n", ret);
		goto out;
	}
	/* ? */
	ret = cdev_add(&lunix_chrdev_cdev, dev_no, lunix_minor_cnt);		// LDD3 page 74
	/* cdev_add? */
	if (ret < 0) {
		printk(KERN_ERR "failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;

	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}

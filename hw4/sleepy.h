/* sleepy.h */

#ifndef SLEEPY_H_1727_INCLUDED
#define SLEEPY_H_1727_INCLUDED

/* Number of devices to create (default: sleepy0 and sleepy1) */
#ifndef SLEEPY_NDEVICES
#define SLEEPY_NDEVICES 10
#endif

/* The structure to represent 'sleepy' devices. 
 *  data - data buffer;
 *  buffer_size - size of the data buffer;
 *  block_size - maximum number of bytes that can be read or written 
 *    in one call;
 *  sleepy_mutex - a mutex to protect the fields of this structure;
 *  cdev - ñharacter device structure.
 *  wq - a wait queue for sleeping devices to be put on.
 *  wait_count - the number of devices allowed to be sleeping.
 */
struct sleepy_dev {
  unsigned char *data;
  struct mutex sleepy_mutex; 
  struct cdev cdev;
  wait_queue_head_t wq;
  volatile int wait_count;
};
#endif /* SLEEPY_H_1727_INCLUDED */

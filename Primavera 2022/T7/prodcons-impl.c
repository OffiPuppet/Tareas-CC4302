/* Implemente aqui el driver para /dev/prodcons */

/* Necessary includes for device drivers */

#include <linux/init.h>

/* #include <linux/config.h> */

#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of prodcons.c functions */

static int prodcons_open(struct inode *inode, struct file *filp);
static int prodcons_release(struct inode *inode, struct file *filp);
static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int prodcons_init(void);
void prodcons_exit(void);

/* Structure that declares the usual file */
/* access functions */

struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

/* Declaration of the init and exit functions */

module_init(prodcons_init);
module_exit(prodcons_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int prodcons_major = 61;     /* Major number */

/* Buffer to store data */

#define MAX_SIZE 8192
static char *prodcons_buffer;
static ssize_t curr_size;
static int readers, writing, global_count;

/* El mutex y la condicion para prodcons */

static KMutex mutex;
static KCondition cond;

int prodcons_init(void) {
  int rc;

  /* Registering device */
  rc = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
  if (rc < 0) {
    printk("<1>prodcons: cannot obtain major number %d\n", prodcons_major);
    return rc;
  }

  readers = global_count = 0;
  writing = FALSE;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating prodcons_buffer */
  prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (prodcons_buffer == NULL) {
    prodcons_exit();
    return -ENOMEM;
  }
  memset(prodcons_buffer, 0, MAX_SIZE);
  printk("<1>Inserting prodcons module\n");
  return 0;
}

void prodcons_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(prodcons_major, "prodcons");

  /* Freeing buffer prodcons */
  if (prodcons_buffer) {
    kfree(prodcons_buffer);
  }
  printk("<1>Removing prodcons module\n");
}

static int prodcons_open(struct inode *inode, struct file *filp) {
  char *mode=   filp->f_mode & FMODE_WRITE ? "write" :
                filp->f_mode & FMODE_READ ? "read" :
                "unknown";
  printk("<1>open %p for %s\n", filp, mode);
  return 0;
}

static int prodcons_release(struct inode *inode, struct file *filp) {
  printk("<1>release %p\n", filp);
  return 0;
}

ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    //Inspirado en el archivo syncread-impl.c
    ssize_t rc;
    m_lock(&mutex);
    while (curr_size <= *f_pos && writing) {
            /* si el lector esta en el final del archivo pero hay un proceso
             * escribiendo todavia en el archivo, el lector espera.
             */
            if (c_wait(&cond, &mutex)) {
                printk("<1>read interrupted global_count=%d\n", global_count);
                rc= -EINTR;
                goto epilog;
            }
    }
    ssize_t volatile_size = curr_size - *f_pos;
    if(count > volatile_size)
        count = volatile_size;
        
    printk("<1>prodconsread global_count=%d\n", global_count);
    
    /* Transfiriendo datos hacia el espacio del usuario */
    if (copy_to_user(buf, prodcons_buffer + *f_pos, count)!=0) {
        /* el valor de buf es una direccion invalida */
        rc= -EFAULT;
        goto epilog;
    }
    *f_pos += count;
    rc = count;
    global_count = 0;

epilog:
    m_unlock(&mutex);
    return rc;
}

ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    //Inspirado en el archivo syncread-impl.c
    ssize_t rc;
    loff_t last;
    m_lock(&mutex);
    last = *f_pos + count;
    if(last > MAX_SIZE){
        loff_t volatile_count = last-MAX_SIZE;
        count -= volatile_count;
    /* Transfiriendo datos desde el espacio del usuario */
    if (copy_from_user(prodcons_buffer + 0, buf, count)!=0) {
        /* el valor de buf es una direccion invalida */
        printk("<1>prodconsread BAD WRITING");
        rc = -EFAULT;
        goto epilog;
    }
    
    *f_pos += count;
    curr_size = *f_pos;
    rc = global_count = count;
    printk("<1>prodconswrite global_count=%d", global_count);
    c_signal(&cond);
    
epilog:
  m_unlock(&mutex);
  return rc;
}

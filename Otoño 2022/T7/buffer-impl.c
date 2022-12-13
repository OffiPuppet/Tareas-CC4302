/* Implemente aqui el driver para /dev/buffer */
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

// buffer functions
int buffer_open(struct inode *inode, struct file *filp);
int buffer_release(struct inode *inode, struct file *filp);
ssize_t buffer_read(struct file *filp, char *buf, size_t ucount, loff_t *f_pos);
ssize_t buffer_write(struct file *filp, const char *buf, size_t ucount, loff_t *f_pos);
void buffer_exit(void);
int buffer_init(void);

struct file_operations buffer_fops = {
    read: buffer_read,
    write: buffer_write,
    open: buffer_open,
    release: buffer_release
};

// Variables globales
int buffer_major = 61;
int in, out, read_space, write_space;
int size[3];
int read = 1;
// cada fila tiene 80 bytes como maximo
#define MAX_SIZE 80
static char *buffer[3];

static KMutex mutex;
static KCondition cond;

// init & exit functions
module_init(buffer_init);
module_exit(buffer_exit);

int buffer_init(void) {
    int rc;
    rc = register_chrdev(buffer_major, "buffer", &buffer_fops);
    if (rc < 0) {
        printk("buffer_init error");
        return rc;
    }
    in = out = read_space = write_space = 0;
    size[0] = 0;
    size[1] = 0;
    size[2] = 0;
    buffer[0] = kmalloc(MAX_SIZE, GFP_KERNEL);
    buffer[1] = kmalloc(MAX_SIZE, GFP_KERNEL);
    buffer[2] = kmalloc(MAX_SIZE, GFP_KERNEL);
    
    m_init(&mutex);
    c_init(&cond);

    if (buffer == NULL) {
        buffer_exit();
        return -ENOMEM;
    }

    memset(buffer[0], 0, MAX_SIZE);
    memset(buffer[1], 0, MAX_SIZE);
    memset(buffer[2], 0, MAX_SIZE);

    return 0;
}

void buffer_exit(void) {
    unregister_chrdev(buffer_major, "buffer");
    if (buffer != NULL){
        kfree(buffer[0]);
        kfree(buffer[1]);
        kfree(buffer[2]);
    } 
}

int buffer_open(struct inode *inode, struct file *filp) {
    char *mode = filp->f_mode & FMODE_WRITE ? "write" :
                filp->f_mode & FMODE_READ ? "read" :
                "unknown";
    printk("<1>open%p for %s\n", filp, mode);
    return 0;
}

int buffer_release(struct inode *inode, struct file *filp) {
    printk("<1>release %p\n", filp);
    return 0;
}

ssize_t buffer_read(struct file *filp, char *buf, size_t ucount, loff_t *f_pos) {
    int count = ucount;
    int k;
    printk("<1>read %p %d\n", filp, count);
    m_lock(&mutex);

    if (read == 0) {
        read = 1;
        c_broadcast(&cond);
        m_unlock(&mutex);
        return 0;
    }
    // El buffer está vacío, el lector espera
    while (size[read_space] == 0) {
        if (c_wait(&cond, &mutex)) {
            printk("<1>read interrupted\n");
            count = -EINTR;
            goto epilog;
        }
    }
    if (count > size[read_space]) {
        count = size[read_space];
    }
    for (k = 0; k<count; k++) {
        if (copy_to_user(buf + k, buffer[read_space] + out, 1) != 0) {
            count = -EFAULT;
            goto epilog;
        }
        printk("<1>read byte %c (%d) from %d\n", buffer[read_space][out], buffer[read_space][out], out);
        out = (out+1) % MAX_SIZE;
    }
    // Liberamos del buffer el texto leido e indicamos la siguiente fila de lectura del buffer
    read = out = 0;
    size[read_space] = 0;
    memset(buffer[read_space], 0, MAX_SIZE);
    read_space = (read_space + 1)% 3;

epilog:
        c_broadcast(&cond);
        m_unlock(&mutex);
        return count;
}

ssize_t buffer_write(struct file *filp, const char *buf, size_t ucount, loff_t *f_pos) {
    int count = ucount;
    int k;

    printk("<1>write %p %d\n", filp, count);
    m_lock(&mutex);
    
    // El espacio del buffer esta ocupado
    while (size[write_space] != 0) {
        if (c_wait(&cond, &mutex)) {
            printk("<1>write interrupted\n");
            count = -EINTR;
            goto epilog;
        }
    }
    for (k= 0; k<count; k++) {
        if (copy_from_user(buffer[write_space] + in, buf + k, 1) != 0) {
            count = -EFAULT;
            goto epilog;
        }
        printk("<1>write byte %c (%d) at %d\n", buffer[write_space][in], buffer[write_space][in], in);
        in = (in+1) % MAX_SIZE;
        size[write_space]++;
        c_broadcast(&cond);
    }
    in = 0;
    write_space = (write_space + 1) % 3;

epilog:
        m_unlock (&mutex);
        return count;
}









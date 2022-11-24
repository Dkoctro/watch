#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/proc_fs.h> 
#include <linux/sched.h> 
#include <linux/uaccess.h> 
#include <linux/version.h> 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) 
#include <linux/minmax.h> 
#endif 
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) 
#define HAVE_PROC_OPS 
#endif 
 
#define PROCFS_MAX_SIZE 2048UL 
#define PROCFS_ENTRY_FILENAME "thread_info" 
 
static struct proc_dir_entry *our_proc_file; 
static char procfs_buffer[PROCFS_MAX_SIZE]; 
static unsigned long procfs_buffer_size = 0; 
 
static ssize_t procfs_read(struct file *filp, char __user *buffer, 
                           size_t length, loff_t *offset) 
{ 
    if (*offset || procfs_buffer_size == 0) { 
        pr_debug("procfs_read: END\n"); 
        *offset = 0; 
        return 0; 
    } 
    
    if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size)) 
        return -EFAULT; 
    *offset += procfs_buffer_size; 
 
    pr_debug("procfs_read: read %lu bytes\n", procfs_buffer_size);
    return procfs_buffer_size; 
} 
static ssize_t procfs_write(struct file *file, const char __user *buffer, 
                            size_t len, loff_t *off) 
{ 
    pr_info("write success\n");
    char tmp[2000];
    if (copy_from_user(tmp, buffer, len)) 
        return -EFAULT;

    long thread_id = 0;
    
    //sscanf(tmp, "%ld", &thread_id);
    
    struct task_struct *task = get_current();
    int temp = 0;
    temp = sprintf(procfs_buffer+procfs_buffer_size, "ThreadID:%d Time:%lld(ms) context switch times:%d\n", task->pid, (task->utime/100000), (int)(task->nvcsw + task->nivcsw));
    procfs_buffer_size += temp;

    *off += procfs_buffer_size; 
    pr_info("write %s success\n", procfs_buffer);
    pr_info("procfs_write: write %lu bytes\n", procfs_buffer_size); 
    return procfs_buffer_size;
} 
static int procfs_open(struct inode *inode, struct file *file) 
{ 
    try_module_get(THIS_MODULE); 
    return 0; 
} 
static int procfs_close(struct inode *inode, struct file *file) 
{ 
    module_put(THIS_MODULE); 
    return 0; 
} 
 
#ifdef HAVE_PROC_OPS 
static struct proc_ops file_ops_4_our_proc_file = { 
    .proc_read = procfs_read, 
    .proc_write = procfs_write, 
    .proc_open = procfs_open, 
    .proc_release = procfs_close, 
}; 
#else 
static const struct file_operations file_ops_4_our_proc_file = { 
    .read = procfs_read, 
    .write = procfs_write, 
    .open = procfs_open, 
    .release = procfs_close, 
}; 
#endif 
 
static int __init procfs3_init(void) 
{ 
    our_proc_file = proc_create(PROCFS_ENTRY_FILENAME, 0666, NULL, 
                                &file_ops_4_our_proc_file); 
    if (our_proc_file == NULL) { 
        remove_proc_entry(PROCFS_ENTRY_FILENAME, NULL); 
        pr_info("Error: Could not initialize /proc/%s\n", 
                 PROCFS_ENTRY_FILENAME); 
        return -ENOMEM; 
    } 
    proc_set_size(our_proc_file, 80); 
    proc_set_user(our_proc_file, GLOBAL_ROOT_UID, GLOBAL_ROOT_GID); 
 
    pr_info("/proc/%s created\n", PROCFS_ENTRY_FILENAME); 
    return 0; 
} 
 
static void __exit procfs3_exit(void) 
{ 
    remove_proc_entry(PROCFS_ENTRY_FILENAME, NULL); 
    pr_info("/proc/%s removed\n", PROCFS_ENTRY_FILENAME); 
} 
 
module_init(procfs3_init); 
module_exit(procfs3_exit); 
 
MODULE_LICENSE("GPL");
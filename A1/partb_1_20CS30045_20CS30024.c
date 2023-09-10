#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rushil Venkateswar and Jay Kumar Thakur");
MODULE_DESCRIPTION("LKM for a deque (double-ended queue)");
MODULE_VERSION("0.1");

#define PROCFS_NAME "partb_1_20CS30045_20CS30024"
#define PROCFS_MAX_SIZE 1024

enum proc_state {
    PROC_FILE_OPEN,
    PROC_READ_VALUE,
};

// node in deque
typedef struct node {
    int val;
    struct node *prev, *next;
} node;

typedef struct deque {
    int capacity; // max size
    int curr_size; // current size
    struct node *head, *tail;
} deque;

// Linked list of processes
typedef struct process_node {
    pid_t pid;
    enum proc_state state;
    struct deque *dq;
    struct process_node *next;
} process_node;

// Global variables
static struct proc_dir_entry *proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE];
static size_t procfs_buffer_size = 0;
static struct process_node *process_list = NULL;

DEFINE_MUTEX(mutex);

// function prototypes

static int init_deque(deque *dq, int sz);
static int insert_deque(deque *dq, int val);
static int read_deque(deque *dq);
static void free_deque(deque *dq);

static struct process_node *find_process(pid_t pid);
static struct process_node *insert_process(pid_t pid);
static void delete_process_list(void);
static int delete_process(pid_t pid);

static int procfile_open(struct inode *inode, struct file *file);
static int procfile_close(struct inode *inode, struct file *file);
static ssize_t handle_read(struct process_node *curr);
static ssize_t procfile_read(struct file *filep, char __user *buffer, size_t length, loff_t *offset);
static ssize_t handle_write(struct process_node *curr);
static ssize_t procfile_write(struct file *filep, const char __user *buffer, size_t length, loff_t *offset);

static const struct proc_ops proc_fops = {
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_write = procfile_write,
    .proc_release = procfile_close,
};

// Module initialization
int init_module(void) {
    pr_info("LKM for %s loaded\n", PROCFS_NAME);

    proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_fops);
    if (proc_file == NULL) {
        pr_alert("Error: could not create proc file\n");
        return -ENOENT;
    }
    pr_info("/proc/%s created\n", PROCFS_NAME);
    return 0;
}

// Module cleanup
void cleanup_module(void) {
    delete_process_list();
    remove_proc_entry(PROCFS_NAME, NULL);
    pr_info("/proc/%s removed\n", PROCFS_NAME);
    pr_info("LKM for %s unloaded\n", PROCFS_NAME);
}


// initialize the deque
static int init_deque(deque *dq, int sz) {
    if (sz < 1 || sz > 100) {
        pr_alert("Error: deque size invalid\n");
        return -EINVAL;
    }

    dq->capacity = sz;
    dq->curr_size = 0;
    dq->head = dq->tail = NULL;

    return 0;
}

// insert into deque
static int insert_deque(deque *dq, int val) {
    node *temp;

    if (dq->capacity == dq->curr_size) {
        pr_alert("Error: deque is full\n");
        return -EACCES;
    }

    temp = (node *) kmalloc(sizeof(node), GFP_KERNEL);
    temp->prev = temp->next = NULL;
    temp->val = val;

    if (dq->curr_size == 0) {
        dq->head = dq->tail = temp;
    }
    else {
        if (val & 1) {
            temp->next = dq->head;
            dq->head->prev = temp;
            dq->head = temp;
        }
        else {
            dq->tail->next = temp;
            temp->prev = dq->tail;
            dq->tail = temp;
        }
    }
    (dq->curr_size)++;

    return sizeof(int);
}

// read from head of deque
static int read_deque(deque *dq) {
    node *temp;
    int val;

    if (dq->curr_size == 0) {
        pr_alert("Error: deque is empty\n");
        return -EACCES;
    }

    temp = dq->head;
    val = temp->val;

    if (dq->curr_size == 1) {
        dq->head = dq->tail = NULL;
    }
    else {
        dq->head = dq->head->next;
        dq->head->prev = NULL;
    }

    kfree(temp);
    (dq->curr_size)--;

    return val;
}

static void free_deque(deque *dq) {
    node *ptr, *prev;

    if (dq == NULL) {
        return;
    }

    ptr = dq->head;
    while (ptr != NULL) {
        prev = ptr;
        ptr = ptr->next;
        kfree(prev);
    }

    kfree(dq);
}




// Find the process node with the given pid
static struct process_node *find_process(pid_t pid) {
    struct process_node *curr = process_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

// Insert a process node with the given pid
static struct process_node *insert_process(pid_t pid) {
    struct process_node *node = kmalloc(sizeof(struct process_node), GFP_KERNEL);
    if (node == NULL) {
        return node;
    }
    node->pid = pid;
    node->state = PROC_FILE_OPEN;
    node->dq = kmalloc(sizeof(deque), GFP_KERNEL);
    node->next = process_list;
    process_list = node;
    return node;
}

// Free the memory allocated to all process nodes
static void delete_process_list(void) {
    struct process_node *curr = process_list;
    while (curr != NULL) {
        struct process_node *prev = curr;
        curr = curr->next;

        free_deque(prev->dq);
        kfree(prev);
    }
}

// Delete a process node with the given pid
static int delete_process(pid_t pid) {
    struct process_node *prev = NULL;
    struct process_node *curr = process_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            if (prev == NULL) {
                process_list = curr->next;
            } else {
                prev->next = curr->next;
            }

            free_deque(curr->dq);
            kfree(curr);

            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -EACCES;
}

// Open, close, read and write handlers for proc file

// Open handler for proc file
static int procfile_open(struct inode *inode, struct file *file) {
    pid_t pid;
    int ret;
    struct process_node *curr;

    mutex_lock(&mutex);

    pid = current->pid;
    pr_info("procfile_open() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        curr = insert_process(pid);
        if (curr == NULL) {
            pr_alert("Error: could not allocate memory for process node\n");
            ret = -ENOMEM;
        } else {
            pr_info("Process %d has been added to the process list\n", pid);
        }
    } else {
        pr_alert("Error: process %d has the proc file already open\n", pid);
        ret = -EACCES;
    }

    mutex_unlock(&mutex);
    return ret;
}

// Close handler for proc file
static int procfile_close(struct inode *inode, struct file *file) {
    pid_t pid;
    int ret;
    struct process_node *curr;

    mutex_lock(&mutex);

    pid = current->pid;
    pr_info("procfile_close() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        pr_alert("Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        delete_process(pid);
        pr_info("Process %d has been removed from the process list\n", pid);
    }

    mutex_unlock(&mutex);
    return ret;
}

// Helper function to handle reads
static ssize_t handle_read(struct process_node *curr) {
    int val;
    if (curr->state == PROC_FILE_OPEN) {
        pr_alert("Error: process %d has not yet written anything to the proc file\n", curr->pid);
        return -EACCES;
    }
    // curr->dq cannot be NULL if the control comes here
    if (curr->dq->curr_size == 0) {
        pr_alert("Error: deque is empty\n");
        return -EACCES;
    }
    val = read_deque(curr->dq);
    strncpy(procfs_buffer, (const char *)&val, sizeof(int));
    procfs_buffer[sizeof(int)] = '\0';
    procfs_buffer_size = sizeof(int);
    return procfs_buffer_size;
}

// Read handler for proc file
static ssize_t procfile_read(struct file *filep, char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int ret;
    struct process_node *curr;
    
    mutex_lock(&mutex);

    pid = current->pid;
    pr_info("procfile_read() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        pr_alert("Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        procfs_buffer_size = min(length, (size_t)PROCFS_MAX_SIZE);
        ret = handle_read(curr);
        if (ret >= 0) {
            if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size) != 0) {
                pr_alert("Error: could not copy data to user space\n");
                ret = -EACCES;
            } else {
                ret = procfs_buffer_size;
            }
        }
    }
    mutex_unlock(&mutex);
    return ret;
}

// Helper function to handle writes
static ssize_t handle_write(struct process_node *curr) {
    size_t capacity;
    int val;

    if (curr->state == PROC_FILE_OPEN) {
        if (procfs_buffer_size > 1ul) {
            pr_alert("Error: Buffer size for capacity must be 1 byte\n");
            return -EINVAL;
        }
        capacity = (size_t)procfs_buffer[0];
        if (capacity < 1 || capacity > 100) {
            pr_alert("Error: Capacity must be between 1 and 100\n");
            return -EINVAL;
        }
        if (init_deque(curr->dq, capacity) < 0) {
            pr_alert("Error: deque initialization failed\n");
            return -ENOMEM;
        }
        pr_info("Deque with capacity %zu has been intialized for process %d\n", capacity, curr->pid);
        curr->state = PROC_READ_VALUE;
    } else if (curr->state == PROC_READ_VALUE) {
        if (procfs_buffer_size > sizeof(int)) {
            pr_alert("Error: Buffer size for value must be 4 bytes\n");
            return -EINVAL;
        }
        if (curr->dq->curr_size == curr->dq->capacity) {
            pr_alert("Error: deque is full\n");
            return -EACCES;
        }
        val = *((int *)procfs_buffer);
        if (insert_deque(curr->dq, val) < 0) {
            pr_alert("Error: Unable to insert value in deque\n");
            return -EACCES;
        }
        pr_info("Value %d has been written to the proc file for process %d\n", val, curr->pid);
    }
    return procfs_buffer_size;
}

// Write handler for proc file
static ssize_t procfile_write(struct file *filep, const char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int ret;
    struct process_node *curr;
    
    mutex_lock(&mutex);

    pid = current->pid;
    pr_info("procfile_write() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        pr_alert("Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        if (buffer == NULL || length == 0) {
            pr_alert("Error: empty write\n");
            ret = -EINVAL;
        } else {
            procfs_buffer_size = min(length, (size_t)PROCFS_MAX_SIZE);
            if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
                pr_alert("Error: could not copy from user\n");
                ret = -EFAULT;
            } else {
                ret = handle_write(curr);
            }
        }
    }
    mutex_unlock(&mutex);
    return ret;
}

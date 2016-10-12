#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

/* for io r/w and cpu time counting */
#include <linux/task_io_accounting_ops.h>
#include <linux/time.h>

/* for string comparison */
#include <linux/string.h>

// #include <stdlib.h>


#define FILE_NAME "procmon"
#define FILE_NAME_SORTING "procmon_sorting" // procmon_sorting name
#define BUF_SIZE 512
#define MAX_PNAME 20 // max length of process name
#define MAX_PROC_NUM 200

char mybuf[BUF_SIZE];

/* structure of process info entry for sorting */
typedef struct {
	char pname[MAX_PNAME];
	int pid;
	unsigned long vm;
	long rss;
	unsigned long long r;
	unsigned long long w;
} procmon_entry_t;

/* for sorting */
enum sort {
	PID,
	VIRT,
	RSS,
	IO
};

int sort = PID; // set default value as PID

/* An auxiliary function for selection sort. return max or min index according to sorting criterion */
int max_(procmon_entry_t *procmon_entry, int start_idx, int entry_num) {
	int i = start_idx;
	int max_idx = i;

	switch (sort) {
		case PID:
			for (i=start_idx; i<entry_num; i++) {
				if (procmon_entry[i].pid < procmon_entry[max_idx].pid)
					max_idx = i;
			}
			break;
		case VIRT:
			for (i=start_idx; i<entry_num; i++) {
				if (procmon_entry[i].vm > procmon_entry[max_idx].vm)
					max_idx = i;
			}
			break;
		case RSS:
			for (i=start_idx; i<entry_num; i++) {
				if (procmon_entry[i].rss > procmon_entry[max_idx].rss)
					max_idx = i;
			}
			break;
		case IO:
			for (i=start_idx; i<entry_num; i++) {
				if (procmon_entry[i].r + procmon_entry[i].w > procmon_entry[max_idx].r + procmon_entry[max_idx].w)
					max_idx = i;
			}
			break;
		default:
			for (i=start_idx; i<entry_num; i++) {
				if (procmon_entry[i].pid > procmon_entry[max_idx].pid)
					max_idx = i;
			}
			break;
	}
	return max_idx;
}


/** sorting function
*** input
procmon_entry_t *procmon_entry: array of procmon_entry
int entry_num: number of procmon_entrys
int sort_criterion: sorting criterion */
void selection_sort(procmon_entry_t *procmon_entry, int entry_num) {
	int i, j;
	procmon_entry_t temp_entry;

	for (i=0; i<entry_num; i++) {
		j = max_(procmon_entry, i, entry_num);
		temp_entry = procmon_entry[j];
		procmon_entry[j] = procmon_entry[i];
		procmon_entry[i] = temp_entry;
	}
}

/* count virtual memory size of a task */
unsigned long virtual_memory(struct task_struct *task) {
	unsigned long vm = 0;

	if(task->active_mm != NULL)
		vm = (task->active_mm->total_vm) * 4; // for converting to KB unit, multiply by 4.
	else
		vm = 0;

	return vm;
}	

long rs_size(struct task_struct *task) {
	atomic_long_t rss_file;
	atomic_long_t rss_anon;
	long total_rss;

	if(task->active_mm != NULL) {
		rss_file = task->active_mm->rss_stat.count[MM_FILEPAGES];
		rss_anon = task->active_mm->rss_stat.count[MM_ANONPAGES];
		total_rss = (atomic_long_read(&rss_file) + atomic_long_read(&rss_anon)) * 4; // for converting to KB unit, multiply by 4.
	}
	else
		total_rss = 0;

	return total_rss;
}

unsigned long long disk_read(struct task_struct *task) {
	unsigned long long r;
	struct task_struct *task_ = task;

	task_io_accounting_add(&task->ioac, &task->signal->ioac);
	while_each_thread(task, task_)
		task_io_accounting_add(&task->ioac, &task_->ioac);

	r = task->ioac.read_bytes / 1024; // to convert KB unit

	return r;
}

unsigned long long disk_write(struct task_struct *task) {
	unsigned long long w;
	struct task_struct *task_ = task;

	task_io_accounting_add(&task->ioac, &task->signal->ioac);
	while_each_thread(task, task_)
		task_io_accounting_add(&task->ioac, &task_->ioac);

	w = task->ioac.write_bytes / 1024; // to convert KB unit

	return w;
}


static int procmon_proc_show(struct seq_file *m, void *v)
{
	struct task_struct *task;
	unsigned long vm;
	long rss;
	unsigned long long r, w;

	/* process information array for sorting */
	procmon_entry_t *procmon_entrys = (procmon_entry_t*) kmalloc(sizeof(procmon_entry_t), ZONE_NORMAL);
	int entry_num = -1; // this value stands for the number of process
	int i; // for iteration

	seq_printf(m, "======================== Process Monitoring Manager for EE516 by YPKOO ======================== \n");
	seq_printf(m, "%-6s\t%-20s%8s\t%8s\t%8s\t%8s\t%8s\n", "PID", "ProcessName", "VIRT(KB)", "RSS Mem(KB)", "DiskRead(KB)", "DiskWrite(KB)", "Total I/O(KB)");

	/* Store each process information into procmon_entrys array */
	for_each_process(task) {
		entry_num++; // increments entry_num first. Since it was set as -1 in declaration, it becomes 0 in the first loop and ends with the exact process number - 1.
		
		/* Increase the size of procmon_entrys array according to current number of entry_num */
		procmon_entrys = (procmon_entry_t*) krealloc(procmon_entrys, sizeof(procmon_entry_t) * (entry_num + 1), ZONE_NORMAL);

		task_lock(task);
		vm = virtual_memory(task);
		rss = rs_size(task);
		r = disk_read(task);
		w = disk_write(task);
		task_unlock(task);

		/* Store each information into procmon_entrys array */
		procmon_entrys[entry_num].pid = task->pid;
		strncpy(procmon_entrys[entry_num].pname, task->comm, strlen(task->comm) + 1);
		procmon_entrys[entry_num].vm = vm;
		procmon_entrys[entry_num].rss = rss;
		procmon_entrys[entry_num].r = r;
		procmon_entrys[entry_num].w = w;
	}

	/* sort the procmon_entrys array according to sorting criterion 
	   since the entry_num is the actual process number - 1, we should + 1*/
	selection_sort(procmon_entrys, entry_num + 1);

	/* print the result */
	for (i=0; i<entry_num; i++) {
		seq_printf(m, "%-6d\t%-20s%8lu\t%8ld\t%8llu\t%8llu\t%8llu\n", procmon_entrys[i].pid, procmon_entrys[i].pname, procmon_entrys[i].vm, procmon_entrys[i].rss, procmon_entrys[i].r, procmon_entrys[i].w, procmon_entrys[i].r + procmon_entrys[i].w);
	}

	/* free memory */
	kfree(procmon_entrys);

	return 0;
}

/* result for cat /proc/procmon_sorting. This shows the current sorting criterion. */
static int procmon_sorting_proc_show(struct seq_file *m, void *v)
{
	switch(sort) {
		case PID: seq_printf(m, "[PID]\tVIRT\tRSS\tI/O\n"); break;
		case VIRT: seq_printf(m, "PID\t[VIRT]\tRSS\tI/O\n"); break;
		case RSS: seq_printf(m, "PID\tVIRT\t[RSS]\tI/O\n"); break;
		case IO: seq_printf(m, "PID\tVIRT\tRSS\t[I/O]\n"); break;

		default: seq_printf(m, "[PID]\tVIRT\tRSS\tI/O\n"); break;
	}
	return 0;
}

static int procmon_proc_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "proc called open\n");
	return single_open(file, procmon_proc_show, NULL);
}

/* procmon_sorting open function */
static int procmon_sorting_proc_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "proc sorting called open\n");
	return single_open(file, procmon_sorting_proc_show, NULL);
}


static ssize_t procmon_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	memset(mybuf, 0, sizeof(mybuf));

	if (count > BUF_SIZE) {
		count = BUF_SIZE;
	}

	printk(KERN_INFO "proc write : %s\n", mybuf); 
	return (ssize_t)count;
}

/* procmon_sorting write function */
static ssize_t procmon_sorting_proc_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{

	memset(mybuf, 0, sizeof(mybuf));

	if (count > BUF_SIZE) {
		count = BUF_SIZE;
	}

	if (copy_from_user(mybuf, buf, count)) {
		return -EFAULT;
	}

	/* set sort value according to write input. If input has wrong value, set the value as the default value, PID */
	if (strncmp(buf, "pid", 3) == 0) {
		sort = PID;
	}
	else if(strncmp(buf, "virt", 4) == 0) {
		sort = VIRT;
	}
	else if (strncmp(buf, "rss", 3) == 0) {
		sort = RSS;
	}
	else if (strncmp(buf, "io", 2) == 0) {
		sort = IO;
	}
	else {
		sort = PID;
	}


	printk(KERN_INFO "proc sorting write : %s\n", mybuf); 
	return (ssize_t)count;
}




struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = procmon_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = procmon_proc_write,
	.release = single_release,
};

/* file operations for procmon_sorting */
struct file_operations fops_sorting = {
	.owner = THIS_MODULE,
	.open = procmon_sorting_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = procmon_sorting_proc_write,
	.release = single_release,
};


static int __init init_procmon (void)
{
	
	struct proc_dir_entry *procmon_proc;

	/* proc_dir_entry for sorting */
	struct proc_dir_entry *procmon_sorting;


	procmon_proc = proc_create(FILE_NAME, 644, NULL, &fops);
	if (!procmon_proc) {
		printk(KERN_ERR "==Cannot create procmon proc entry \n");
		return -1;
	}
	printk(KERN_INFO "== init procmon\n");

	/* create procmon_sorting proc */
	procmon_sorting = proc_create(FILE_NAME_SORTING, 644, NULL, &fops_sorting);
	if (!procmon_sorting) {
		printk(KERN_ERR "==Cannot create procmon_sorting proc entry \n");
		return -1;
	}
	
	printk(KERN_INFO "== init procmon_sorting\n");
	return 0;
}

static void __exit exit_procmon(void)
{
	remove_proc_entry(FILE_NAME, NULL);
	printk(KERN_INFO "== exit procmon\n");

	/* remove procmon_sorting proc */
	remove_proc_entry(FILE_NAME_SORTING, NULL);
	printk(KERN_INFO "== exit procmon_sorting\n");
}



module_init(init_procmon);
module_exit(exit_procmon);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dong-Jae Shin");
MODULE_DESCRIPTION("EE516 Project2 Process Monitoring Module");

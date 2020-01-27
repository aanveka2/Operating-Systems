#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/rtc.h>


MODULE_LICENSE("GPL");

static struct miscdevice linepipe;

static int N;

static struct semaphore full;
static struct semaphore empty;

//static struct semaphore read_mutex;
//static struct semaphore write_mutex;
static struct semaphore mutex;


static int char_count = 100; 



static int read_index = 0;
static int write_index = 0;

static int empty_lines; 
module_param(N, int, 0000);               //No of lines from command line

char ** pipe;


static int my_open(struct inode*, struct file*);
static ssize_t my_read(struct file*, char*, size_t, loff_t*);
static ssize_t my_write(struct file*, const char*, size_t, loff_t*);
static int my_release(struct inode*, struct file*);

static struct file_operations linepipe_fops =
{
        .open = &my_open,
        .read = &my_read,
        .write = &my_write,
        .release = &my_release
};

static struct miscdevice linepipe = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "linepipe",
    .fops = &linepipe_fops,
};

int init_module()
{

        int reg;         
        if((reg = misc_register(&linepipe)) < 0)
        {
                printk("linepipe did not register successfully\n");
                return reg;
        }
        printk(KERN_INFO "linepipe Registered successfully\n");
        printk(KERN_INFO "Number of lines in pipe (N): %d\n", N);

        int mem = 0;
        pipe = (char**)kmalloc(N*sizeof(char*), GFP_KERNEL);//The GFP_KERNEL flag specifies a normal kernel allocation.
        while(mem < N)
        {
                pipe[mem] = (char*)kmalloc((char_count+1)*sizeof(char), GFP_KERNEL);
                pipe[char_count] = '\0';
                ++mem;
        }

        //initializing semaphore and mutex
        sema_init(&full, 0); //Counts full buffer slots
        sema_init(&empty, N); //Counts empty buffer slots
        sema_init(&mutex, 1); //Controls access to critical section
        
        empty_lines = N;
        return 0;
}

static int my_open(struct inode* _inode, struct file* _file) //inode points to inode object and file is the pointer to file object.
{
        printk(KERN_INFO "linepipe opened\n");
        return 0;
}

static ssize_t my_read(struct file* _file, char* user_buffer, size_t len, loff_t* offset)//file is the pointer to file object, out is the buffer to hold data, size is length of buffer and off is its offset
{
        int index = 0;
        
        printk(KERN_INFO " Reading from the pipe\n");

        //To acquire lock on read mutex and performing down operation 
	// Decreament full count	
        if(down_interruptible(&full)< 0)
		{			
			printk(KERN_ALERT "Exit by user");
			return -1;
		}
        // Enter critical region 
        if(down_interruptible(&mutex)< 0)
		{
			printk(KERN_ALERT "Exit by user");
			return -1;
		}
		
	
        read_index = read_index % N;
        while(index < len)
        {
                if(empty_lines >= N)
                {
                        break;
                } 
                
                if(copy_from_user(&pipe[write_index][index], &user_buffer[index], 1))
                {
                        printk(KERN_ALERT "Error in copying from kernel space to user space");  
                        return -1;
                }
                index++;
        }
        ++read_index;
        ++empty_lines;
        up(&mutex);
        up(&empty);
        return index;
}

static ssize_t my_write(struct file* _file, const char* user_buffer, size_t len, loff_t* offset)//file is the pointer to file object, out is the buffer to hold data, size is length of buffer and off is its offset
{
        int index = 0;
        
		 printk(KERN_INFO " Writing to pipe \n");
        //To acquire lock on write mutex and performing down operation 
	// Decreament empty count 
        if(down_interruptible(&empty)< 0)
		{
			printk(KERN_ALERT "Exit by user");	
			return -1;
		}
        // Enter critical region 
        if(down_interruptible(&mutex)<0)
		{
			printk(KERN_ALERT "Exit by user");		
			return -1;
		}
		
        write_index = write_index % N;
        while(index < len)
        {
                if(empty_lines <= 0)
                {
                        break;
                }
                
                if(copy_from_user(&pipe[write_index][index], &user_buffer[index], 1))
                {
                        printk(KERN_ALERT "Error in copying from User space to kernel space");  
                        return -1;
                }
                index++;
        }
        ++write_index;
        --empty_lines;
        up(&mutex);
        up(&full);
        return index;
}

static int my_release(struct inode* _inode, struct file* _file)
{
        printk(KERN_INFO "linepipe closed\n");
        return 0;
}

void cleanup_module()
{
        /*freeing memory*/
        int i = 0;
        while (i < N)
        {
                kfree(pipe[i]);
                i++;
        }
        misc_deregister(&linepipe);
        printk(KERN_INFO "linepipe unregistered!\n");
}
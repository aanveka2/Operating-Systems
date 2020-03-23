#include <linux/init.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <asm/pgtable.h>
#include <linux/mm_types.h>


MODULE_LICENSE("GPL");

static int process_id = 1;
module_param(process_id, int, S_IRUGO);


// called when module is installed
int __init init_module()
{
	printk(KERN_ALERT "mymodule: Hello World!\n");

    struct pid *pid;
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned long   pfn;

    struct task_struct *pid_struct;
    struct mm_struct *pid_mm_struct;


    pid = find_get_pid (process_id);
    pid_struct =  pid_task (pid, PIDTYPE_PID);
    pid_mm_struct =pid_struct->mm;
    struct vm_area_struct *vma;
    unsigned long vaddr;
    for(vma =pid_mm_struct-> mmap; vma; vma =vma->vm_next)
    {
        for(vaddr =vma->vm_start ; vaddr <vma->vm_end; vaddr++)
        {
            pgd =pgd_offset(pid_mm_struct, vaddr);
            if(pgd_present(*pgd))
            {
                p4d =p4d_offset(pgd, vaddr);
                if(p4d_present(*p4d))
                {
                    pud =pud_offset(p4d, vaddr);
                    if(pud_present(*pud))
                    {
                        pmd = pmd_offset(pud, vaddr);
                        if(pmd_present(*pmd))
                        {
                            pte= pte_offset_map(pmd,vaddr);
                            if(!pte_none(*pte))
                            {
                                pfn = pte_pfn(*pte);
                                printk(KERN_ALERT "Page Frame Number: %lx", pfn);
                            }
                        }
                    }
                }
            }

        }



    }
    return 0;
}



// called when module is removed
void __exit cleanup_module()
{
	printk(KERN_ALERT "mymodule: Goodbye, cruel world!!\n");
}


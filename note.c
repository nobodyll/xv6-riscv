void start()
{
    // 每个cpu在调用entry.s，完成初始化cpu stack之后    // 进入start()

    /*
    **
    **  1. Set M Previous Privilege mode to Supervisor, for mret.
    **  2. set M Exception Program Counter to main, for mret.
    **  3. delegate all interrupts and exceptions to supervisor mode.
    **  4. configure Physical Memory Protection to give supervisor mode
    **     access to all of physical memory.
    **  5. ask for click interrupts.
    **  6. keep each CPU's hartid in its tp register, for cpuid().
    **  7. asm volatile("mret") switch to supervisor mode and jump to main().
    **
    */
}

void main()
{

    /*
    ** 1. 如果是cpuid == 0的cpu,那么需要进行一些初始化工作    ** 2. 所有的cpu都需要开启cpu的分页机制，设置stvec为kernelvec,
    **    然后打开设备中断    ** 3. 所有cpu调用scheduler进行进程调度    **
    */
}

void scheduler()
{
    /*
    **
    ** Per-CPU process scheduler.
    ** Each CPU calls scheduler() after setting itself up.
    ** Scheduler never returns.  It loops, doing:
    **  - choose a process to run.
    **  - swtch to start running that process.
    **  - eventually that process transfers control
    **    via swtch back to the scheduler.
    **
    */
    struct proc *p;
    struct cpu *c = mycpu();

    c->proc = 0;
    for (;;)
    {
        // Avoid deadlock by ensuring that devices can interrupt.
        // 有可能所有的proc都不是RUNNABLE,所以需要打开中断，使得一些进程可以        // 满足条件进入RUNNABLE状态,否则所有的进程有可能死锁.
        intr_on();

        for (p = proc; p < &proc[NPROC]; p++)
        {
            acquire(&p->lock);
            if (p->state == RUNNABLE)
            {
                // Switch to chosen process.  It is the process's job
                // to release its lock and then reacquire it
                // before jumping back to us.
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);

                // Process is done running for now.
                // It should have changed its p->state before coming back.
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}

void swtch()
{
    /*
    **
    ** Context switch
    **  void swtch(struct context *old, struct context *new);
    ** Save current registers in old. Load from new.
    **
    */

    /*
    ** swtch是一个函数调用吗？ 应该不是    ** 他是一个上下文切换函数.
    ** 上下文切换对于线程(scheduler/process)来说应该是透明的,
    ** 进程和调度器应该察觉不到，好像没有发生一样.

    ** 上下文切换的原理    ** 1. 保存原先的状态(ra寄存器,sp寄存器,callee-saved寄存器)
    ** 2. 切换堆栈, 调用ret指令跳转到一个新的执行流.
    */

    // 调度器中调用swtch是把内核调度器线程切换到某个RUNNABLE状态的进程    // sched中调用swtch是把RUNNABLE状态的进程切换到调度器线程
    // 进程第一次swtch，是在scheduler中，swtch切换到forkret
    // forkret() {
    //     release(&myproc()->lock);
    //     usertrapret();
    // }

    // 进程第二次swtch,是从yield中切换到调度器scheduler线程
    // 进程第三次swtch,会从swtch切换到yield中返回
    //
}

/*
**
** 中断** 用户空间中断过程
** 1. 空户程序(系统调用或者异常)
** 2. 发生trap,
**    -更改cpu权限,从user mode -> supervisor mode),
**    -cpu关中断，**    -保存返回地址sepc,
**    -将pc寄存器改为stvec(uservec代码).
** 3. 程序跳转到uservec(file:trampoline.S)执行,此时线程堆栈还是**    用户程序的堆栈,页表用的和是用户程序的页表** 4. uservec需要保存程序的状态**    -保存所有寄存器的状态到TRAPFRAME
**    -加载程序的内核堆栈**    -开启内核页表** 5. 跳转执行usertrap()
**    -设置stvec为kernelvec, 现在处于内核态,现在发生中断由kernelvec处理** 6. p->trapframe->epc = r_sepc();保存user program counter
** 7. 根据发生中断的原因进程具体处理**    -syscall()
**    -device inter
**    -timer interrupt
** 8. 调用usertrapret(),这时可能发生了yield，在此cpu上执行的进程已经不再是原来的进程** 9. usertrapret()
**    -关中断**    -设置stvec寄存器为uservec
**    -保存kernel_sp,kernel_trap,kernel_hartid到进程的trapframe
**    -设置S Previous Privilege mode to User.
**    -设置sepc为p->trapframe->ecp
** 10. 调用userret(user_pagetable)
**    -restore all reg from TrapFrame
** 11 sret 返回用户空间

*/

/*
 * 分析程序的堆栈切换*
 * 1. 最开始entry.s为每一个cpu设置自己的堆栈,该堆栈在系统的数据段,是全局数据* 2. scheduler->swtch 切换到进程的上下文,此时堆栈切换为进程的内核堆栈p->kstack
 * 3. usertrapret->uerret 切换到用户程序堆栈p->trapframe->sp
 * 4. 发生trap, 由用户堆栈p->trapfarme->sp切换到内核堆栈p->kstack
 * 5. 如果是timer interrupt,发生调度sched->swtch切换到调度器线程使用cpu的堆栈* 6. 然后重复2-5
 *
 */

/*
 * kernel trap过程*
 * 1. 用户程序程序 trap
 * 2. stvec <- kernelvec
 * 3. syscall之前会重新打开中断 (//TODO 为什么在处理syscall的时候打开中断)
 * 4. 这时在内核态发生中断,
 * 5. hardware do
 *    -关中断*    -save sepc, scause, status
 *    -pc <- kernelvec
 * 6. 执行kernelvec代码*    -使用p->kstack保存中断前的状态(所有的寄存器)
 * 7. call kerneltrap
 *    -save sepc,sstatus,scause to local variable(p->kstack)
 *    - restore trap registers (sepc,sstatus)
 * 8. sret (return to whatever we were doing in the kenrel.)
 * 9. 继续执行用户trap
 *
 *
 */

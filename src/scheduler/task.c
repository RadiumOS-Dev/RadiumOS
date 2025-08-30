#include "task.h"
#include "../utility/utility.h"
#include "../terminal/terminal.h"
#include "../errors/error.h"
#include "../io/io.h"

// ----- GDT / TSS -----

GDTEntry gdt_entries[NUM_GDT_ENTRIES];
GDTPointer gdt_pointer;
TSS tss;

int set_gdt_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    if (num >= NUM_GDT_ENTRIES) {
        memory_error("GDT entry setup", "0x001");
        return 0;
    }
    
    gdt_entries[num].base_low    = base & 0xFFFF;
    gdt_entries[num].base_mid    = base >> 16 & 0xFF;
    gdt_entries[num].base_high   = base >> 24 & 0xFF;
    gdt_entries[num].limit_low   = limit & 0xFFFF;
    gdt_entries[num].granularity = (flags & 0xF0) | (limit >> 16 & 0xF);
    gdt_entries[num].access      = access;
    
    return 1;
}

int setup_gdt() {
    gdt_pointer.limit = NUM_GDT_ENTRIES * 8 - 1;
    gdt_pointer.base = (uint32_t) &gdt_entries;

    memset((uint8_t*) &tss, 0, sizeof(tss));
    tss.ss0 = GDT_KERNEL_DATA;

    if (!set_gdt_entry(0, 0, 0, 0, 0)) return 0;                // 0x00: null
    if (!set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xC0)) return 0; // 0x08: kernel mode text
    if (!set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xC0)) return 0; // 0x10: kernel mode data
    if (!set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xC0)) return 0; // 0x18: user mode code segment
    if (!set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xC0)) return 0; // 0x20: user mode data segment
    if (!set_gdt_entry(5, (uint32_t) &tss, sizeof(tss), 0x89, 0x40)) return 0; // 0x28: tss

    load_gdt((uint32_t) &gdt_pointer);
    asm("ltr %%ax" :: "a"((uint16_t) GDT_TSS));
    
    return 1;
}

IDTEntry idt[256] __attribute__((aligned(16)));
IDTPointer idt_pointer;

int set_idt_entry(uint8_t vector, void* isr, uint8_t attributes) {
    if (!isr) {
        system_error("IDT entry setup", "0x003");
        return 0;
    }
    
    idt[vector].isr_low    = (uint32_t) isr & 0xFFFF;
    idt[vector].segment_selector = GDT_KERNEL_CODE;
    idt[vector].reserved   = 0;
    idt[vector].attributes = attributes;
    idt[vector].isr_high   = (uint32_t) isr >> 16;
    
    return 1;
}

int remap_pic() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
    
    return 1;
}

extern void* isr_redirect_table[];
extern void isr128();

int setup_interrupts() {
    if (!remap_pic()) {
        hardware_fault("PIC", "0x002");
        return 0;
    }

    // clear IDT
    memset((uint8_t*) &idt, 0, sizeof(IDTEntry) * 256);

    for (int i = 0; i < 48; i++) {
        if (!isr_redirect_table[i]) {
            system_error("ISR table initialization", "0x004");
            continue;
        }
        if (!set_idt_entry(i, isr_redirect_table[i], 0x8E)) {
            return 0;
        }
    }
    
    if (!set_idt_entry(0x80, isr128, 0xEE)) {
        return 0;
    }

    idt_pointer.limit = sizeof(IDTEntry) * 256 - 1;
    idt_pointer.base  = (uint32_t) &idt;
    asm("lidt %0" :: "m"(idt_pointer));
    
    return 1;
}

void handle_interrupt(TrapFrame regs) {
    if (regs.interrupt > 255) {
        system_error("Invalid interrupt vector", "0x005");
        return;
    }
    
    *(VGA_MEMORY + 80 + regs.interrupt) = 0xF100 | 'G';

    if (regs.interrupt >= 32 && regs.interrupt <= 47) {
        if (regs.interrupt >= 40) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);

        if (regs.interrupt == 32) {
            schedule();
        }
    }
 
    if (regs.interrupt == 0x80) {
        if (regs.eax >= 80 * 25) {
            memory_error("Syscall memory access", "0x006");
            return;
        }
        *(VGA_MEMORY + 80 * 2 + regs.eax) = 0xB000 | 'S';
    }
}

int setup_pit(uint32_t frequency) {
    if (frequency == 0) {
        hardware_fault("PIT", "0x007");
        return 0;
    }
    
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    uint8_t l = (uint8_t) (divisor & 0xFF);
    uint8_t h = (uint8_t) (divisor >> 8 & 0xFF);
    outb(0x40, l);
    outb(0x40, h);
    
    return 1;
}

Task tasks[MAX_TASKS];
int num_tasks;
Task* current_task;

// Stack allocation functions
uint32_t allocate_kernel_stack() {
    static uint32_t stack_base = 0x200000;
    static uint32_t stack_size = 0x4000;
    uint32_t allocated_stack = stack_base;
    stack_base += stack_size;
    return allocated_stack + stack_size;
}

uint32_t allocate_user_stack() {
    static uint32_t user_stack_base = 0x400000;
    static uint32_t user_stack_size = 0x4000;
    uint32_t allocated_stack = user_stack_base;
    user_stack_base += user_stack_size;
    return allocated_stack + user_stack_size;
}

int create_task(uint32_t id, uint32_t eip, uint32_t user_stack, uint32_t kernel_stack, bool is_kernel_task) {
    if (id >= MAX_TASKS) {
        task_error("Task creation", "0x008");
        return 0;
    }
    
    if (num_tasks >= MAX_TASKS) {
        task_error("Task creation", "0x009");
        return 0;
    }
    
    if (!eip) {
        memory_error("Invalid task entry point", "0x00A");
        return 0;
    }
    
    if (tasks[id].id != 0 && id != 0) {
        task_error("Task already exists", "0x00B");
        return 0;
    }

    // Allocate stacks if not provided
    if (kernel_stack == 0) {
        kernel_stack = allocate_kernel_stack();
        if (kernel_stack == 0) {
            memory_error("Kernel stack allocation failed", "0x00C");
            return 0;
        }
    }
    
    if (!is_kernel_task && user_stack == 0) {
        user_stack = allocate_user_stack();
        if (user_stack == 0) {
            memory_error("User stack allocation failed", "0x00D");
            return 0;
        }
    }

    num_tasks++;

    if (!is_kernel_task) {
        // we can pass things to the task by pushing to its user stack
        // with cdecl, this will pass it as arguments
        user_stack -= 4;
        *(uint32_t*) user_stack = id; // first arg to task func
        user_stack -= 4;
        *(uint32_t*) user_stack = 0; // task func return address, shouldnt be used
    }

    uint32_t code_selector = is_kernel_task ? GDT_KERNEL_CODE : (GDT_USER_CODE | RPL_USER);
    uint32_t data_selector = is_kernel_task ? GDT_KERNEL_DATA : (GDT_USER_DATA | RPL_USER);

    uint8_t* kesp = (uint8_t*) kernel_stack;

    // we need to set up the initial kernel stack for this task
    // this stack will be loaded next time switch_context gets
    // called for this task
    // once switch_context switches esp to this stack, the ret
    // instruction will pop off a return value, so we redirect it
    // to new_task_setup to init registers and exit the interrupt
    kesp -= sizeof(NewTaskKernelStack);
    NewTaskKernelStack* stack = (NewTaskKernelStack*) kesp;
    stack->ebp = stack->edi = stack->esi = stack->ebx = 0;
    stack->switch_context_return_addr = (uint32_t) new_task_setup;
    stack->data_selector = data_selector;
    stack->eip = eip;
    stack->cs = code_selector;
    stack->eflags = 0x200; // enable interrupts

    // Fixed: Use appropriate stack pointer based on task type
    if (!is_kernel_task) {
        stack->usermode_esp = user_stack; 
        stack->usermode_ss = data_selector;
    } else {
        stack->usermode_esp = (uint32_t) kesp;  // Cast to uint32_t
        stack->usermode_ss = data_selector;
    }

    tasks[id].kesp_bottom = kernel_stack;
    tasks[id].kesp = (uint32_t) kesp;
    tasks[id].id = id;
    tasks[id].is_active = true;  // Add this line
    
    return 1;  // Return success
}

int setup_tasks() {
    memset((uint8_t*) tasks, 0, sizeof(Task) * MAX_TASKS);

    num_tasks = 1;
    current_task = &tasks[0];
    current_task->id = 0;
    current_task->is_active = true;
    current_task->kesp_bottom = 0x100000;
    current_task->kesp = 0x100000 - 0x1000;
    
    if (!current_task) {
        kernel_panic("Task initialization failed", "0x00F");
        return 0;
    }
    
    return 1;
}

void schedule() {
    if (num_tasks == 0) {
        system_error("Scheduler", "0x010");
        return;
    }
    
    if (!current_task) {
        kernel_panic("Current task is null", "0x011");
        return;
    }
    
    int next_id = (current_task->id + 1) % num_tasks;
    int attempts = 0;
    
    while (attempts < num_tasks) {
        Task* next = &tasks[next_id];
        
        if (next->is_active && next->kesp_bottom != 0 && next->kesp != 0) {
            if (next->kesp < next->kesp_bottom && 
                (next->kesp_bottom - next->kesp) >= sizeof(NewTaskKernelStack)) {
                
                Task* old = current_task;
                current_task = next;
                tss.esp0 = next->kesp_bottom;
                switch_context(old, next);
                return;
            }
        }
        
        next_id = (next_id + 1) % num_tasks;
        attempts++;
    }
    
    task_error("No valid tasks to schedule", "0x013");
}

void cleanup_task(uint32_t task_id) {
    if (task_id >= MAX_TASKS) {
        task_error("Task cleanup", "0x014");
        return;
    }
    
    if (task_id == 0) {
        kernel_panic("Cannot cleanup kernel task", "0x015");
        return;
    }
    
    tasks[task_id].is_active = false;
    memset(&tasks[task_id], 0, sizeof(Task));
    
    if (current_task && current_task->id == task_id) {
        schedule();
    }
}

bool validate_task_memory(uint32_t task_id) {
    if (task_id >= MAX_TASKS) {
        return false;
    }
    
    Task* task = &tasks[task_id];
    
    if (!task->is_active) {
        return false;
    }
    
    if (!task->kesp || !task->kesp_bottom) {
        memory_error("Task memory validation", "0x016");
        return false;
    }
    
    if (task->kesp >= task->kesp_bottom) {
        memory_error("Task stack corruption", "0x017");
        return false;
    }
    
    if ((task->kesp_bottom - task->kesp) < sizeof(NewTaskKernelStack)) {
        memory_error("Insufficient stack space", "0x018");
        return false;
    }
    
    return true;
}

Task* get_next_valid_task() {
    int start_id = current_task ? current_task->id : 0;
    int next_id = (start_id + 1) % num_tasks;
    int attempts = 0;
    
    while (attempts < num_tasks) {
        if (tasks[next_id].is_active && validate_task_memory(next_id)) {
            return &tasks[next_id];
        }
        next_id = (next_id + 1) % num_tasks;
        attempts++;
    }
    
    return &tasks[0];
}

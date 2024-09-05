#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
//	준용 추가
#include "file.h"
#include "filesys/filesys.h"

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

//	준용 추가
void exit(int status)
{
	thread_current()->exitStatus = status;
	thread_exit();
}

void fork(const char *thread_name, struct intr_frame *f)
{
	//	thread_create 를 쓰거나, 비슷하게 작성하면 될 듯.
}

bool create(const char *file, unsigned initial_size)
{
	return filesys_create(file, (off_t)initial_size);
}

bool remove(const char *file)
{
	return filesys_remove(file);
}

bool isFileOpened(int fd)
{
	if (thread_current()->descriptors[fd] == NULL)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int open(const char *file)
{
	struct thread *th = thread_current();
	if (th->nextDescriptor >= (1 << 12))
	{
		return -1;
	}
	else
	{
		int desc = th->nextDescriptor;
		th->descriptors[desc] = filesys_open(file);
		th->nextDescriptor++;
		return desc;
	}
}

int filesize(int fd)
{
	if (isFileOpened(fd))
	{
		return file_length(thread_current()->descriptors[fd]);
	}
	else
	{
		return -1;
	}
}

int read(int fd, void *buffer, unsigned size)
{
	if (isFileOpened(fd)) {
		
	} else {
		return -1;
	}
}

int write(int fd, const void *buffer, unsigned size)
{
	if (fd == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}
	else if (isFileOpened(fd))
	{
		struct file *target = thread_current()->descriptors[fd];
		return file_write(target, buffer, (off_t)size);
	}
	return -1;
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f)
{
	// TODO: Your implementation goes here.
	//	systemcall 번호 - rax
	//	인자 - rdi, rsi, rdx, r10, r8, r9
	switch (f->R.rax)
	{
	case SYS_HALT:
		power_off();
		break;
	case SYS_EXIT:
		exit(f->R.rdi);
		break;
	case SYS_FORK:
		fork(f->R.rdi, f);
		break;
	case SYS_EXEC:
		break;
	case SYS_WAIT:
		break;
	case SYS_CREATE:
		f->R.rax = create(f->R.rdi, f->R.rsi);
		break;
	case SYS_REMOVE:
		f->R.rax = remove(f->R.rdi);
		break;
	case SYS_OPEN:
		f->R.rax = open(f->R.rdi);
		break;
	case SYS_FILESIZE:
		f->R.rax = filesize(f->R.rdi);
		break;
	case SYS_READ:
		f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_WRITE:
		f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_SEEK:
		break;
	case SYS_TELL:
		break;
	case SYS_CLOSE:
		break;
	}
}

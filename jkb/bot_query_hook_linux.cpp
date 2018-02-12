//
// JK_Botti - be more human!
//
// bot_query_hook_linux.cpp
//

#ifndef _WIN32

#ifndef __USE_GNU
#define __USE_GNU
#endif

#include <sys/mman.h>
#include <pthread.h>

#include <memory.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "bot_query_hook.h"

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr) (((addr)+PAGE_SIZE-1)&PAGE_MASK)

//
// Linux work, based on my "Linux code for dynamic linkents" from metamod-p
//

// 0xe9 is opcode for our function forwarder
#define JMP_SIZE 1

//pointer size on x86-32: 4 bytes
#define PTR_SIZE sizeof(void*)
	
//opcode + sizeof pointer
#define BYTES_SIZE (JMP_SIZE + PTR_SIZE)

typedef ssize_t (*sendto_func)(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

static bool is_sendto_hook_setup = false;

//pointer to original sendto
static sendto_func sendto_original;

//contains jmp to replacement_sendto @sendto_original
static unsigned char sendto_new_bytes[BYTES_SIZE];

//contains original bytes of sendto
static unsigned char sendto_old_bytes[BYTES_SIZE];

//Mutex for our protection
static pthread_mutex_t mutex_replacement_sendto = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

//constructs new jmp forwarder
static void construct_jmp_instruction(void *x, void *place, void *target)
{
	((unsigned char *)x)[0] = 0xe9;
	*(unsigned long *)((char *)x + 1) = ((unsigned long)target) - (((unsigned long)place) + 5);
}

//restores old sendto
inline void restore_original_sendto(void)
{
	//Copy old sendto bytes back
	memcpy((void*)sendto_original, sendto_old_bytes, BYTES_SIZE);
}

//resets new sendto
inline void reset_sendto_hook(void)
{
	//Copy new sendto bytes back
	memcpy((void*)sendto_original, sendto_new_bytes, BYTES_SIZE);
}

// Replacement sendto function
static ssize_t __replacement_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	return sendto_hook(socket, message, length, flags, dest_addr, dest_len);
}

//
ssize_t call_original_sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	/* Emulate sendto using sendmsg.. this is faster than restoring/replacing sendto() code. */
	/* Also, internally libc sendto() is wrapper around sendmsg(). */
	struct iovec iov = {0,};
	iov.iov_base = (void*)message;
	iov.iov_len = length;
	struct msghdr msg = {0,};
	msg.msg_name = (void*)dest_addr;
	msg.msg_namelen = dest_len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	return sendmsg(socket, &msg, flags);
}

//
bool hook_sendto_function(void)
{
	if(is_sendto_hook_setup)
		return(true);
	
	is_sendto_hook_setup = false;
	
	//metamod-p p31 parses elf structures, we find function easier&better way:
	void * sym_ptr = (void*)&sendto;
	while(*(unsigned short*)sym_ptr == 0x25ff) {
		sym_ptr = **(void***)((char *)sym_ptr + 2);
	}
	
	sendto_original = (sendto_func)sym_ptr;
	
	//Backup old bytes of "sendto" function
	memcpy(sendto_old_bytes, (void*)sendto_original, BYTES_SIZE);
	
	//Construct new bytes: "jmp offset[replacement_sendto] @ sendto_original"
	construct_jmp_instruction((void*)&sendto_new_bytes[0], (void*)sendto_original, (void*)&__replacement_sendto);
	
	//Check if bytes overlap page border.	
	unsigned long start_of_page = PAGE_ALIGN((long)sendto_original) - PAGE_SIZE;
	unsigned long size_of_pages = 0;
	
	if((unsigned long)sendto_original + BYTES_SIZE > PAGE_ALIGN((unsigned long)sendto_original))
	{
		//bytes are located on two pages
		size_of_pages = PAGE_SIZE*2;
	}
	else
	{
		//bytes are located entirely on one page.
		size_of_pages = PAGE_SIZE;
	}
	
	//Remove PROT_READ restriction
	if(mprotect((void*)start_of_page, size_of_pages, PROT_READ|PROT_WRITE|PROT_EXEC))
	{
		UTIL_ConsolePrintf("Couldn't initialize sendto hook, mprotect failed: %i.  Exiting...\n", errno);
		return(false);
	}
	
	//Write our own jmp-forwarder on "sendto"
	reset_sendto_hook();
	
	is_sendto_hook_setup = true;
	
	//done
	return(true);
}

//
bool unhook_sendto_function(void)
{
	if(!is_sendto_hook_setup)
		return(true);
	
	//Lock before modifing original sendto
	pthread_mutex_lock(&mutex_replacement_sendto);
	
	//reset sendto hook
	restore_original_sendto();
	
	//unlock
	pthread_mutex_unlock(&mutex_replacement_sendto);
	
	is_sendto_hook_setup = false;
	
	return(true);
}

#endif /*!_WIN32*/

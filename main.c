#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mailbox.h"

#define GPU_QPUS 1

#define GPU_MEM_FLG 0xC	//cached
//#define GPU_MEM_FLG 0x4	//direct
#define GPU_MEM_MAP 0x0	//cached
//#define GPU_MEM_MAP 0x20000000	//direct

unsigned int program[] = {
#include "c2f.qasm.bin.hex"
};

#define as_gpu_address(x) (unsigned) gpu_pointer + ((void *)x - arm_pointer)

int main(int argc, char *argv[])
{
	int i;
	int mb = mbox_open();

	if (argc != 2) {
		fprintf(stderr, "Specify a Celcius temperature to convert to Ferenheit\n");
		return 1;
	}

	/* Enable QPU for execution */
	int qpu_enabled = !qpu_enable(mb, 1);
	if (!qpu_enabled) {
		fprintf(stderr, "Unable to enable QPU. Check your firmware is latest.");
		goto cleanup;
	}

	/* Allocate GPU memory and map it into ARM address space */
	unsigned size = 4096;
	unsigned align = 4096;
	unsigned handle = mem_alloc(mb, size, align, GPU_MEM_FLG);
	if (!handle) {
		printf("Failed to allocate GPU memory.");
		goto cleanup;
	}
	void *gpu_pointer = (void *) mem_lock(mb, handle);
	void *arm_pointer = mapmem((unsigned) gpu_pointer + GPU_MEM_MAP, size);

	/* Fill result buffer with 0x55 */
	memset(arm_pointer, 0, size);

	((unsigned *) arm_pointer)[512 / sizeof(unsigned)] = atoi(argv[1]);

	unsigned *p = (unsigned *) arm_pointer;

	/*
	   +---------------+ <----+
	   |  QPU Code     |      |
	   |  ...          |      |
	   +---------------+ <--+ |
	   |  Uniforms     |    | |
	   +---------------+    | |
	   |  QPU0 Uniform -----+ |
	   |  QPU0 Start   -------+
	   +---------------+
	   ...
	   +---------------+ +512
	   | Src for VPM   |
	   | ...           |
	   +---------------+
	   ...
	   +---------------+ +1024
	   | Dest for VPM  |
	   | ...           |
	   +---------------+
	 */

	/* Copy QPU program into GPU memory */
	unsigned *qpu_code = p;
	memcpy(p, program, sizeof program);
	p += (sizeof program) / (sizeof program[0]);

	/* Build Uniforms */
	unsigned *qpu_uniform = p;
	for (i = 0; i < GPU_QPUS; ++i) {
		*p++ = 1;
		*p++ = (unsigned) (gpu_pointer + 512 + i * 16 * 6 * 4);
		*p++ = (unsigned) (gpu_pointer + 1024 + i * 16 * 6 * 4);
	}

	/* Build QPU Launch messages */
	unsigned *qpu_msg = p;
	for (i = 0; i < GPU_QPUS; ++i) {
		*p++ = as_gpu_address(qpu_uniform + i * 4 * 3);
		*p++ = as_gpu_address(qpu_code);
	}

#ifdef DEBUG
	// Test buffer
	printf("before:");
	for (i=0; i<size/4; i++) {
		if ((i%8)==0) printf("\n%08x:", gpu_pointer+i*4);
		printf(" %08x", ((unsigned *)arm_pointer)[i]);
	}
	printf("\n");
#endif /* DEBUG */

	/* Launch QPU program and block till its done */
	unsigned r = execute_qpu(mb, GPU_QPUS, as_gpu_address(qpu_msg), 1, 3000);
#ifdef DEBUG
	printf("qpu exec %p returns %d\n", gpu_pointer + ((void *)qpu_msg - arm_pointer), r);
#endif /* DEBUG */

#ifdef DEBUG
	// Test buffer
	printf("after:");
	for (i=0; i<(int)size/4; i++) {
		if ((i%8)==0) printf("\n%p:", gpu_pointer+i*4);
		printf(" %08x", ((unsigned *)arm_pointer)[i]);
	}
	printf("\n");
#endif /* DEBUG */

#ifdef DEBUG
	printf("In Celsius: %d\nIn Ferenheit: %d\n", ((unsigned*)arm_pointer)[512/sizeof(unsigned)], ((unsigned*)arm_pointer)[1024/sizeof(unsigned)]);
#endif /* DEBUG */
	printf("%d\n", ((unsigned *) arm_pointer)[1024 / sizeof(unsigned)]);

	cleanup:
	/* Release GPU memory */
	if (arm_pointer) {
		unmapmem(arm_pointer, size);
	}
	if (handle) {
		mem_unlock(mb, handle);
		mem_free(mb, handle);
	}

	/* Release QPU */
	if (qpu_enabled) {
		qpu_enable(mb, 0);
	}

	/* Release mailbox */
	mbox_close(mb);

	return 0;
}

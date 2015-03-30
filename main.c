#define _POSIX_C_SOURCE 202112L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "mailbox/mailbox.h"
#include "mailbox/xmailbox.h"
#include "v3d/v3d.h"
#include "v3d/v3d_utils.h"

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
	int mb = xmbox_open();
	uint32_t *v3d_p;

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
	v3d_init();
	v3d_utils_init();
	v3d_p=mapmem_cpu(v3d_peripheral_addr(), V3D_LENGTH);

	/* Allocate GPU memory and map it into ARM address space */
	unsigned size = 4096;
	unsigned align = 4096;

	unsigned handle = xmem_alloc(mb, size, align, GPU_MEM_FLG);
	unsigned gpu_pointer = xmem_lock(mb, handle);
	void *arm_pointer = mapmem_cpu((unsigned) gpu_pointer + GPU_MEM_MAP, size);
#ifdef DEBUG
	printf("gpu_pointer=%p arm_pointer=%p\n", gpu_pointer, arm_pointer);
#endif /* DEBUG */

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
	v3d_reset_SRQCS(v3d_p);
	if(v3d_read(v3d_p, V3D_QPURQCC) != 0){
		fprintf(stderr, "error: program count is not zero even after resetting\n");
		printf("%d\n", v3d_read(v3d_p, V3D_QPURQCC));
		exit(EXIT_FAILURE);
	}
	v3d_write(v3d_p, V3D_QPURQUL, 1024);
	v3d_write(v3d_p, V3D_QPURQUA, as_gpu_address(qpu_uniform));
	v3d_write(v3d_p, V3D_QPURQPC, as_gpu_address(qpu_code));
	while(v3d_read(v3d_p, V3D_QPURQCC) == 0)
		;

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
	printf("Re:%d We:%d\n", v3d_read(v3d_p, V3D_VPMERR), v3d_read(v3d_p, V3D_VPMEWR));
	printf("In Celsius: %d\nIn Ferenheit: %d\n", ((unsigned*)arm_pointer)[512/sizeof(unsigned)], ((unsigned*)arm_pointer)[1024/sizeof(unsigned)]);
#endif /* DEBUG */
	printf("%d\n", ((unsigned *) arm_pointer)[1024 / sizeof(unsigned)]);

	cleanup:
	/* Release GPU memory */
	if (arm_pointer) {
		unmapmem_cpu(arm_pointer, size);
	}
	xmem_unlock(mb, handle);
	xmem_free(mb, gpu_pointer);

	unmapmem_cpu(v3d_p, V3D_LENGTH);
	v3d_utils_finalize();
	v3d_finalize();

	/* Release QPU */
	if (qpu_enabled) {
		qpu_enable(mb, 0);
	}

	/* Release mailbox */
	xmbox_close(mb);

	return 0;
}

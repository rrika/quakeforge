#ifndef __QF_Vulkan_command_h
#define __QF_Vulkan_command_h

typedef struct qfv_cmdpool_s {
	struct qfv_device_s *device;
	VkCommandPool cmdpool;
} qfv_cmdpool_t;

typedef struct qfv_cmdbuffer_s {
	struct qfv_device_s *device;
	VkCommandPool cmdpool;
	VkCommandBuffer buffer;
} qfv_cmdbuffer_t;

typedef struct qfv_cmdbufferset_s {
	struct qfv_device_s *device;
	VkCommandBuffer *buffers;
	int         numBuffers;
} qfv_cmdbufferset_t;

typedef struct qfv_semaphore_s {
	struct qfv_device_s *device;
	VkSemaphore semaphore;
} qfv_semaphore_t;

typedef struct qfv_semaphoreset_s {
	struct qfv_device_s *device;
	VkSemaphore *semaphores;
	VkPipelineStageFlags *stages;
	int         numSemaphores;
} qfv_semaphoreset_t;

typedef struct qfv_fence_s {
	struct qfv_device_s *device;
	VkFence     fence;
} qfv_fence_t;

typedef struct qfv_fenceset_s {
	struct qfv_device_s *device;
	VkFence    *fences;
	int         numFences;
} qfv_fenceset_t;

struct qfv_queue_s;
qfv_cmdpool_t *QFV_CreateCommandPool (struct qfv_device_s *device,
									  uint32_t queueFamily,
									  int transient, int reset);
int QFV_ResetCommandPool (qfv_cmdpool_t *pool, int release);
void QFV_DestroyCommandPool (qfv_cmdpool_t *pool);
qfv_cmdbuffer_t *QFV_AllocateCommandBuffers (qfv_cmdpool_t *pool,
											 int secondary, int count);
qfv_cmdbufferset_t *QFV_CreateCommandBufferSet (qfv_cmdbuffer_t **buffers,
												int numBuffers);
void QFV_FreeCommandBuffers (qfv_cmdbuffer_t *buffer, int count);
// NOTE: does not destroy buffers
void QFV_DestroyCommandBufferSet (qfv_cmdbufferset_t *buffers);
int QFV_BeginCommandBuffer (qfv_cmdbuffer_t *buffer, int oneTime,
							int rpContinue, int simultaneous,
							VkCommandBufferInheritanceInfo *inheritanceInfo);
int QFV_EndCommandBuffer (qfv_cmdbuffer_t *buffer);
int QFV_ResetCommandBuffer (qfv_cmdbuffer_t *buffer, int release);

qfv_semaphore_t *QFV_CreateSemaphore (struct qfv_device_s *device);
qfv_semaphoreset_t *QFV_CreateSemaphoreSet (qfv_semaphore_t **semaphores,
											int numSemaphores);
void QFV_DestroySemaphore (qfv_semaphore_t *semaphore);
// NOTE: does not destroy semaphores
void QFV_DestroySemaphoreSet (qfv_semaphoreset_t *semaphores);
qfv_fence_t *QFV_CreateFence (struct qfv_device_s *device, int signaled);
qfv_fenceset_t *QFV_CreateFenceSet (qfv_fence_t **fences, int numFences);
void QFV_DestroyFence (qfv_fence_t *fence);
// NOTE: does not destroy fences
void QFV_DestroyFenceSet (qfv_fenceset_t *fences);
int QFV_WaitForFences (qfv_fenceset_t *fences, int all, uint64_t timeout);
int QFV_ResetFences (qfv_fenceset_t *fences);
int QFV_QueueSubmit (struct qfv_queue_s *queue,
					 qfv_semaphoreset_t *waitSemaphores,
					 qfv_cmdbufferset_t *buffers,
					 qfv_semaphoreset_t *signalSemaphores, qfv_fence_t *fence);
int QFV_QueueWaitIdle (struct qfv_queue_s *queue);


#endif//__QF_Vulkan_command_h

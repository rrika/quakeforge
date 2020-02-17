#ifndef __QF_Vulkan_buffer_h
#define __QF_Vulkan_buffer_h

#include "QF/darray.h"

typedef struct qfv_buffertransition_s {
	VkBuffer    buffer;
	VkAccessFlags srcAccess;
	VkAccessFlags dstAccess;
	uint32_t    srcQueueFamily;
	uint32_t    dstQueueFamily;
	VkDeviceSize offset;
	VkDeviceSize size;
} qfv_buffertransition_t;

typedef struct qfv_buffertransitionset_s
	DARRAY_TYPE (qfv_buffertransition_t) qfv_buffertransitionset_t;
typedef struct qfv_bufferbarrierset_s
	DARRAY_TYPE (VkBufferMemoryBarrier) qfv_bufferbarrierset_t;

struct qfv_device_s;
VkBuffer QFV_CreateBuffer (struct qfv_device_s *device,
						   VkDeviceSize size,
						   VkBufferUsageFlags usage);

VkDeviceMemory QFV_AllocBufferMemory (struct qfv_device_s *device,
									  VkBuffer buffer,
									  VkMemoryPropertyFlags properties,
									  VkDeviceSize size, VkDeviceSize offset);

int QFV_BindBufferMemory (struct qfv_device_s *device,
						  VkBuffer buffer, VkDeviceMemory object,
						  VkDeviceSize offset);

qfv_bufferbarrierset_t *
QFV_CreateBufferTransitions (qfv_buffertransition_t *transitions,
							 int numTransitions);

VkBufferView QFV_CreateBufferView (struct qfv_device_s *device,
								   VkBuffer buffer, VkFormat format,
								   VkDeviceSize offset, VkDeviceSize size);

#endif//__QF_Vulkan_buffer_h

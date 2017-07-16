/* linux/drivers/media/video/s5p-fimc/fimc_vb2.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Camera Interface (FIMC) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include "fimc-core.h"

#if defined(CONFIG_VIDEOBUF2_SDVMM)
void *fimc_sdvmm_init(struct fimc_dev *fimc)
{
	struct vb2_vcm vb2_vcm;
	struct vb2_cma vb2_cma;
	char cma_name[FIMC_CMA_NAME_SIZE] = {0,};
	struct vb2_drv vb2_drv;

	fimc->vcm_id = VCM_DEV_FIMC0 + fimc->id;

	vb2_vcm.vcm_id = fimc->vcm_id;
	vb2_vcm.size = SZ_64M;

	vb2_cma.dev = &fimc->pdev->dev;
	/* FIXME: need to set type value */
	sprintf(cma_name, "%s%d", FIMC_CMA_NAME, fimc->id);
	vb2_cma.type = cma_name;
	vb2_cma.alignment = SZ_4K;

	vb2_drv.cacheable = false;
	vb2_drv.remap_dva = false;

	return vb2_sdvmm_init(&vb2_vcm, &vb2_cma, &vb2_drv);
}

const struct fimc_vb2 fimc_vb2_sdvmm = {
	.ops		= &vb2_sdvmm_memops,
	.init		= fimc_sdvmm_init,
	.cleanup	= vb2_sdvmm_cleanup,
	.plane_addr	= vb2_sdvmm_plane_dvaddr,
	.resume		= vb2_sdvmm_resume,
	.suspend	= vb2_sdvmm_suspend,
	.cache_flush	= vb2_sdvmm_cache_flush,
	.set_cacheable	= vb2_sdvmm_set_cacheable,
};

#elif defined(CONFIG_VIDEOBUF2_CMA_PHYS)
void *fimc_cma_init(struct fimc_dev *fimc)
{
	return vb2_cma_phys_init(&fimc->pdev->dev, NULL, 0, false);
}

int fimc_cma_resume(void *alloc_ctx)
{
	return 1;
}
void fimc_cma_suspend(void *alloc_ctx){}
void fimc_cma_set_cacheable(void *alloc_ctx, bool cacheable){}

int fimc_cma_cache_flush(struct vb2_buffer *vb, u32 plane_no)
{
	return 0;
}

const struct fimc_vb2 fimc_vb2_cma = {
	.ops		= &vb2_cma_phys_memops,
	.init		= fimc_cma_init,
	.cleanup	= vb2_cma_phys_cleanup,
	.plane_addr	= vb2_cma_phys_plane_paddr,
	.resume		= fimc_cma_resume,
	.suspend	= fimc_cma_suspend,
	.cache_flush	= fimc_cma_cache_flush,
	.set_cacheable	= fimc_cma_set_cacheable,
};

#elif defined(CONFIG_VIDEOBUF2_ION)
void *fimc_ion_init(struct fimc_dev *fimc)
{
	return vb2_ion_create_context(&fimc->pdev->dev, SZ_4K,
			VB2ION_CTX_IOMMU | VB2ION_CTX_UNCACHED | VB2ION_CTX_VMCONTIG);
}

static unsigned long fimc_vb2_plane_addr(struct vb2_buffer *vb, u32 plane_no)
{
	void *cookie = vb2_plane_cookie(vb, plane_no);
	dma_addr_t dva = 0;

	WARN_ON(vb2_ion_dma_address(cookie, &dva) != 0);

	return dva;
}

const struct fimc_vb2 fimc_vb2_ion = {
	.ops		= &vb2_ion_memops,
	.init		= fimc_ion_init,
	.cleanup	= vb2_ion_destroy_context,
	.plane_addr	= fimc_vb2_plane_addr,
	.resume		= vb2_ion_attach_iommu,
	.suspend	= vb2_ion_detach_iommu,
	.cache_flush	= vb2_ion_cache_flush,
	.set_cacheable	= vb2_ion_set_cached,
};
#endif

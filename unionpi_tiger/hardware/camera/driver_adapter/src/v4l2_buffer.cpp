/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dlfcn.h>
#include <unistd.h>
#include "v4l2_buffer.h"

namespace OHOS::Camera {
#define OUTPUT_V4L2_PIX_FMT V4L2_PIX_FMT_NV21

static uint32_t pixelFormatV4l2ToGe2d(uint32_t v4l2PixelFmt)
{
    uint32_t ge2dPixelFmt;

    switch (v4l2PixelFmt) {
        case V4L2_PIX_FMT_RGBA32:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGBA_8888;
            break;
        case V4L2_PIX_FMT_RGBX32:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGBX_8888;
            break;
        case V4L2_PIX_FMT_RGB24:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGB_888;
            break;
        case V4L2_PIX_FMT_BGRA32:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_BGRA_8888;
            break;
        case V4L2_PIX_FMT_YVU420:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YV12;
            break;
        case V4L2_PIX_FMT_GREY:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_Y8;
            break;
        case V4L2_PIX_FMT_NV16:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_422_SP;
            break;
        case V4L2_PIX_FMT_NV21:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCrCb_420_SP;
            break;
        case V4L2_PIX_FMT_UYVY:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_422_UYVY;
            break;
        case V4L2_PIX_FMT_BGR24:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_BGR_888;
            break;
        case V4L2_PIX_FMT_NV12:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
        default:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_INVALID;
            break;
    }

    return ge2dPixelFmt;
}

int HosV4L2Buffers::doBlit(aml_ge2d_t *ge2d, Ge2dCanvasInfo & srcInfo, Ge2dCanvasInfo & dstInfo)
{
    int ret;
    aml_ge2d_info_t *pge2dinfo = &ge2d->ge2dinfo;
    uint32_t srcCanvasW, srcCanvasH;
    uint32_t srcFmt;
    int srcDmaFd;
    uint32_t dstCanvasW, dstCanvasH;
    uint32_t dstFmt;
    int dstDmaFd;

    srcCanvasW = srcInfo.width;
    srcCanvasH = srcInfo.height;
    srcFmt = srcInfo.format;
    srcDmaFd = srcInfo.dmaFd;

    dstCanvasW = dstInfo.width;
    dstCanvasH = dstInfo.height;
    dstFmt = dstInfo.format;
    dstDmaFd = dstInfo.dmaFd;

    if (!ge2d || !srcCanvasW || !srcCanvasH || !dstCanvasW || !dstCanvasH || srcDmaFd<0 || dstDmaFd<0) {
        CAMERA_LOGE("Invalid param.");
        return -1;
    }

    pge2dinfo->offset = 0;
    pge2dinfo->ge2d_op = GE2D_OP_STRETCHBLIT;
    pge2dinfo->blend_mode = GE2D_BLEND_MODE_NONE;

    pge2dinfo->src_info[0].plane_number = 1;
    pge2dinfo->src_info[0].layer_mode = GE2D_LAYER_MODE_INVALID;
    pge2dinfo->src_info[0].plane_alpha = 0xff;
    pge2dinfo->src_info[0].memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->src_info[1].memtype = GE2D_CANVAS_TYPE_INVALID;
    pge2dinfo->src_info[0].mem_alloc_type = GE2D_MEM_DMABUF;
    pge2dinfo->src_info[1].mem_alloc_type = GE2D_MEM_DMABUF;
    pge2dinfo->src_info[0].shared_fd[0] = srcDmaFd;
    pge2dinfo->src_info[0].canvas_w = srcCanvasW;
    pge2dinfo->src_info[0].canvas_h = srcCanvasH;
    pge2dinfo->src_info[0].format = srcFmt;
    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = srcCanvasW;
    pge2dinfo->src_info[0].rect.h = srcCanvasH;

    pge2dinfo->dst_info.plane_number = 1;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->dst_info.mem_alloc_type = GE2D_MEM_DMABUF;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC;
    pge2dinfo->dst_info.shared_fd[0] = dstDmaFd;
    pge2dinfo->dst_info.canvas_w = dstCanvasW;
    pge2dinfo->dst_info.canvas_h = dstCanvasH;
    pge2dinfo->dst_info.format = dstFmt;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = dstCanvasW;
    pge2dinfo->dst_info.rect.h = dstCanvasH;

    int (*prtGe2dProcess)(aml_ge2d_info_t *) = (int (*)(aml_ge2d_info_t *))ge2dProcess;
    ret = prtGe2dProcess(pge2dinfo);
    if (ret) {
        CAMERA_LOGE("ge2dProcess() failed. ret=%{public}d", ret);
    }

    return ret;
}

static inline uint64_t getTickMs()
{
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec / 1000000ULL + ts.tv_sec * 1000ULL;
}

HosV4L2Buffers::HosV4L2Buffers(enum v4l2_memory memType, enum v4l2_buf_type bufferType)
    : memoryType_(memType), bufferType_(bufferType)
{
    CAMERA_LOGD("HosV4L2Buffers::HosV4L2Buffers enter");
#if defined(__aarch64__)
    static const char* libge2d = "/vendor/lib64/libge2d.z.so";
#else
    static const char* libge2d = "/vendor/lib/libge2d.z.so";
#endif
    int ret;

    ge2d_ = calloc(sizeof(aml_ge2d_t), 1);
    if (!ge2d_) {
        CAMERA_LOGE("No enough memory for ge2d ctx");
        return;
    }

    if (ge2dHandle == NULL) {
        ge2dHandle = dlopen(libge2d, RTLD_NOW | RTLD_GLOBAL);
        if (ge2dHandle == NULL) {
            CAMERA_LOGE("open lib libge2d so fail, reason:%{public}s", dlerror());
            return;
        }

        ge2dInit = dlsym(ge2dHandle, "aml_ge2d_init");
        ge2dProcess = dlsym(ge2dHandle, "aml_ge2d_process");
        ge2dExit = dlsym(ge2dHandle, "aml_ge2d_exit");
        if (ge2dInit == NULL || ge2dProcess == NULL || ge2dExit == NULL) {
            CAMERA_LOGE("lib libge2d so func not found!");
            dlclose(&ge2dHandle);
            return;
        }
    }

    int (*ptrGe2dInit)(aml_ge2d_t *) = (int (*)(aml_ge2d_t *))ge2dInit;
    if (ptrGe2dInit((aml_ge2d_t*)ge2d_)) {
        free(ge2d_);
        ge2d_ = NULL;
        CAMERA_LOGE("aml_ge2d_init() failed.");
        return;
    }

    CAMERA_LOGD("HosV4L2Buffers::HosV4L2Buffers exit");
}

HosV4L2Buffers::~HosV4L2Buffers()
{
    if (ge2d_) {
        void (*ptrGe2dExit)(aml_ge2d_t *) = (void (*)(aml_ge2d_t *))ge2dExit;
        ptrGe2dExit((aml_ge2d_t*)ge2d_);
        CAMERA_LOGD("ge2dExit()");
        free(ge2d_);
    }
}

RetCode HosV4L2Buffers::V4L2ReqBuffers(int fd, int unsigned buffCont)
{
    struct v4l2_requestbuffers req = {};

    CAMERA_LOGD("V4L2ReqBuffers buffCont %d\n", buffCont);

    req.count = buffCont;
    req.type = bufferType_;
    req.memory = memoryType_;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        CAMERA_LOGE("does not support memory mapping %s\n", strerror(errno));
        return RC_ERROR;
    }

    if (req.count != buffCont) {
        CAMERA_LOGE("error Insufficient buffer memory on \n");

        req.count = 0;
        req.type = bufferType_;
        req.memory = memoryType_;
        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            CAMERA_LOGE("V4L2ReqBuffers does not release buffer	%s\n", strerror(errno));
            return RC_ERROR;
        }

        return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2QueueBuffer(int fd, const std::shared_ptr<FrameSpec>& frameSpec)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};

    if (frameSpec == nullptr) {
        CAMERA_LOGE("V4L2QueueBuffer: frameSpec is NULL\n");
        return RC_ERROR;
    }

    buf.index = (uint32_t)frameSpec->buffer_->GetIndex();
    buf.type = bufferType_;
    buf.memory = memoryType_;

    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes = planes;
        buf.m.planes[0].length = frameSpec->buffer_->GetSize();
        buf.m.planes[0].m.userptr = (unsigned long)frameSpec->buffer_->GetVirAddress();
        buf.length = 1;

        CAMERA_LOGD("++++++++++++ V4L2QueueBuffer buf.index = %{public}d, buf.length = \
            %{public}d, buf.m.userptr = %{public}p\n", \
            buf.index, buf.m.planes[0].length, (void*)buf.m.planes[0].m.userptr);
    } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        buf.length = frameSpec->buffer_->GetSize();
        buf.m.userptr = (unsigned long)frameSpec->buffer_->GetVirAddress();

        CAMERA_LOGD("++++++++++++ V4L2QueueBuffer buf.index = %{public}d, buf.length = \
            %{public}d, buf.m.userptr = %{public}p\n", \
            buf.index, buf.length, (void*)buf.m.userptr);
    }

    std::lock_guard<std::mutex> l(bufferLock_);
    int rc = ioctl(fd, VIDIOC_QBUF, &buf);
    if (rc < 0) {
        CAMERA_LOGE("ioctl VIDIOC_QBUF failed: %s\n", strerror(errno));
        return RC_ERROR;
    }

    auto itr = queueBuffers_.find(fd);
    if (itr != queueBuffers_.end()) {
        itr->second[buf.index] = frameSpec;
        CAMERA_LOGD("insert frameMap fd = %{public}d buf.index = %{public}d\n", fd, buf.index);
    } else {
        FrameMap frameMap;
        frameMap.insert(std::make_pair(buf.index, frameSpec));
        queueBuffers_.insert(std::make_pair(fd, frameMap));
        CAMERA_LOGD("insert fd = %{public}d buf.index = %{public}d\n", fd, buf.index);
    }

    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2DequeueBuffer(int fd)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};

    buf.type = bufferType_;
    buf.memory = memoryType_;

    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        buf.m.planes = planes;
        buf.length = 1;
    }
    int rc = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (rc < 0) {
        CAMERA_LOGE("ioctl VIDIOC_DQBUF failed: %s\n", strerror(errno));
        return RC_ERROR;
    }

    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        CAMERA_LOGD("---------------- V4L2DequeueBuffer index = %{public}d buf.m.ptr = %{public}p len = %{public}d\n",
            buf.index, (void*)buf.m.planes[0].m.userptr, buf.m.planes[0].length);
    } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        CAMERA_LOGD("---------------- V4L2DequeueBuffer index = %{public}d buf.m.ptr = %{public}p len = %{public}d\n",
            buf.index, (void*)buf.m.userptr, buf.length);
    }

    std::lock_guard<std::mutex> l(bufferLock_);

    auto IterMap = queueBuffers_.find(fd);
    if (IterMap == queueBuffers_.end()) {
        CAMERA_LOGE("std::map queueBuffers_ no fd\n");
        return RC_ERROR;
    }
    auto& bufferMap = IterMap->second;

    auto Iter = bufferMap.find(buf.index);
    if (Iter == bufferMap.end()) {
        CAMERA_LOGE("V4L2DequeueBuffer buf.index == %{public}d is not find in FrameMap\n", buf.index);
        return RC_ERROR;
    }

    if (dequeueBuffer_ == nullptr) {
        CAMERA_LOGE("V4L2DequeueBuffer buf.index == %{public}d no callback\n", buf.index);
        bufferMap.erase(Iter);
        return RC_ERROR;
    }

    if (memoryType_ == V4L2_MEMORY_MMAP) {
        BlitForMMAP(fd, buf.index, Iter->second->buffer_);
    }

    // callback to up
    dequeueBuffer_(Iter->second);

    bufferMap.erase(Iter);

    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2AllocBuffer(int fd, const std::shared_ptr<FrameSpec>& frameSpec)
{
    struct v4l2_buffer buf = {};
    struct v4l2_plane planes[1] = {};
    CAMERA_LOGD("V4L2AllocBuffer\n");

    if (frameSpec == nullptr) {
        CAMERA_LOGE("V4L2AllocBuffer frameSpec is NULL\n");
        return RC_ERROR;
    }

    switch (memoryType_) {
        case V4L2_MEMORY_MMAP:
            break;
        case V4L2_MEMORY_USERPTR:
            buf.type = bufferType_;
            buf.memory = memoryType_;
            buf.index = (uint32_t)frameSpec->buffer_->GetIndex();

            if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                buf.m.planes = planes;
                buf.length = 1;
            }
            CAMERA_LOGD("V4L2_MEMORY_USERPTR Print the cnt: %{public}d\n", buf.index);

            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                CAMERA_LOGE("error: ioctl VIDIOC_QUERYBUF failed: %{public}s\n", strerror(errno));
                return RC_ERROR;
            }

            CAMERA_LOGD("buf.length = %{public}d frameSpec->buffer_->GetSize() = %{public}d\n", buf.length,
                        frameSpec->buffer_->GetSize());

            if (buf.length > frameSpec->buffer_->GetSize()) {
                CAMERA_LOGE("ERROR:user buff < V4L2 buf.length\n");
                return RC_ERROR;
            }

            break;
        case V4L2_MEMORY_OVERLAY:
            // to do something
            break;

        case V4L2_MEMORY_DMABUF:
            // to do something
            break;

        default:
            CAMERA_LOGE("It can not be happening - incorrect memory type\n");
            return RC_ERROR;
    }

    return RC_OK;
}

RetCode HosV4L2Buffers::V4L2ReleaseBuffers(int fd)
{
    CAMERA_LOGE("HosV4L2Buffers::V4L2ReleaseBuffers\n");

    std::lock_guard<std::mutex> l(bufferLock_);
    queueBuffers_.erase(fd);

    return V4L2ReqBuffers(fd, 0);
}

void HosV4L2Buffers::SetCallback(BufCallback cb)
{
    CAMERA_LOGD("HosV4L2Buffers::SetCallback OK.");
    dequeueBuffer_ = cb;
}

RetCode HosV4L2Buffers::Flush(int fd)
{
    CAMERA_LOGD("HosV4L2Buffers::Flush enter\n");
    std::lock_guard<std::mutex> l(bufferLock_);

    if (dequeueBuffer_ == nullptr) {
        CAMERA_LOGE("HosV4L2Buffers::Flush  dequeueBuffer_ == nullptr");
        return RC_ERROR;
    }

    auto IterMap = queueBuffers_.find(fd);
    if (IterMap == queueBuffers_.end()) {
        CAMERA_LOGE("HosV4L2Buffers::Flush std::map queueBuffers_ no fd");
        return RC_ERROR;
    }
    auto &bufferMap = IterMap->second;

    for (auto &it : bufferMap) {
        std::shared_ptr<FrameSpec> frameSpec = it.second;
        CAMERA_LOGD("HosV4L2Buffers::Flush throw up buffer begin, buffpool=%{public}d",
                    (int32_t)frameSpec->bufferPoolId_);
        frameSpec->buffer_->SetBufferStatus(CAMERA_BUFFER_STATUS_INVALID);
        dequeueBuffer_(frameSpec);
        CAMERA_LOGD("HosV4L2Buffers::Flush throw up buffer end");
    }

    bufferMap.clear();

    CAMERA_LOGD("HosV4L2Buffers::Flush exit\n");

    return RC_OK;
}

RetCode HosV4L2Buffers::BlitForMMAP(int fd, uint32_t fromIndex, std::shared_ptr<IBuffer> toBuffer)
{
    int ret;
    struct v4l2_format fmt = {};
    struct v4l2_exportbuffer expbuf = {};
    int32_t dstDma = toBuffer->GetFileDescriptor();
    uint32_t dstWidth = toBuffer->GetWidth();
    uint32_t dstHeight = toBuffer->GetHeight();
    int32_t srcDma = -1;
    uint32_t srcWidth = 0;
    uint32_t srcHeight = 0;
    uint32_t srcFmt;
    uint32_t dstFmt;
    uint64_t tickBegin = getTickMs();
    Ge2dCanvasInfo srcInfo;
    Ge2dCanvasInfo dstInfo;

    // Get src format
    fmt.type = bufferType_;
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        CAMERA_LOGE("error: ioctl VIDIOC_G_FMT failed: %s\n", strerror(errno));
        return RC_ERROR;
    }

    if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        srcWidth = fmt.fmt.pix_mp.width;
        srcHeight = fmt.fmt.pix_mp.height;
        srcFmt = pixelFormatV4l2ToGe2d(fmt.fmt.pix_mp.pixelformat);
    } else if (bufferType_ == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
        srcWidth = fmt.fmt.pix.width;
        srcHeight = fmt.fmt.pix.height;
        srcFmt = pixelFormatV4l2ToGe2d(fmt.fmt.pix.pixelformat);
    }

    dstFmt = pixelFormatV4l2ToGe2d(OUTPUT_V4L2_PIX_FMT);
    if (!srcFmt || !dstFmt) {
        CAMERA_LOGE("Error: Invalid srcFmt or dstFmt: %{public}d, %{public}d", srcFmt, dstFmt);
        return RC_ERROR;
    }

    // Export dma fd
    expbuf.type = bufferType_;
    expbuf.index = fromIndex;
    ret = ioctl(fd, VIDIOC_EXPBUF, &expbuf);
    if (ret < 0) {
        CAMERA_LOGE("error: ioctl VIDIOC_EXPBUF failed: ret = %{public}d, %{public}s\n", ret, strerror(errno));
        return RC_ERROR;
    }
    srcDma = expbuf.fd;

    // DO blit
    srcInfo.width = srcWidth;
    srcInfo.height = srcHeight;
    srcInfo.format = srcFmt;
    srcInfo.dmaFd = srcDma;
    dstInfo.width = dstWidth;
    dstInfo.height = dstHeight;
    dstInfo.format = dstFmt;
    dstInfo.dmaFd = dstDma;
    ret = doBlit((aml_ge2d_t*)ge2d_, srcInfo, dstInfo);

    close(srcDma);

    CAMERA_LOGD("fromIndex=%{public}d, blit ret=%{public}d, use_time=%{public}llums", \
                fromIndex, ret, getTickMs()-tickBegin);

    CAMERA_LOGD("Format=%{public}d, Size=%{public}d, EncodeType=%{public}d", \
                toBuffer->GetFormat(), toBuffer->GetSize(), toBuffer->GetEncodeType());

    return ret ? RC_ERROR : RC_OK;
}
} // namespace OHOS::Camera

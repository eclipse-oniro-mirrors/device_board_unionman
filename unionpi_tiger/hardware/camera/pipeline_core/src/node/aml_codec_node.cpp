/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <sys/mman.h>

#include "securec.h"
#include "camera.h"
#include "aml_codec_node.h"

#include "vpcodec_1_0.h"
#include "aml_ge2d.h"
#include "ge2d_dmabuf.h"

namespace OHOS::Camera {
#define ENCODER_FRAMERATE (30)
#define ENCODER_BITRATE (2000000)
#define ENCODER_GOP (20)

uint32_t AMLCodecNode::previewWidth_ = 0;
uint32_t AMLCodecNode::previewHeight_ = 0;
using Ge2dCanvasInfo = struct _Ge2dCanvasInfo {
    uint32_t width;
    uint32_t height;
    uint32_t format;
    int dmaFd;
};

static uint32_t pixelFormatOHOSToGe2d(uint32_t bufferFormat)
{
    uint32_t ge2dPixelFmt;

    switch (bufferFormat) {
        case CAMERA_FORMAT_RGBA_8888:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGBA_8888;
            break;
        case CAMERA_FORMAT_RGBX_8888:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGBX_8888;
            break;
        case CAMERA_FORMAT_RGB_888:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_RGB_888;
            break;
        case CAMERA_FORMAT_BGRA_8888:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_BGRA_8888;
            break;
        case CAMERA_FORMAT_YCRCB_420_P:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YV12;
            break;
        case CAMERA_FORMAT_YCBCR_422_SP:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_422_SP;
            break;
        case CAMERA_FORMAT_YCRCB_420_SP:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCrCb_420_SP;
            break;
        case CAMERA_FORMAT_UYVY_422_PKG:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_422_UYVY;
            break;
        case CAMERA_FORMAT_YCBCR_420_SP:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_YCbCr_420_SP_NV12;
            break;
        default:
            ge2dPixelFmt = GE2D_PIXEL_FORMAT_INVALID;
            break;
    }

    return ge2dPixelFmt;
}

static int doBlit(aml_ge2d_t *ge2d, Ge2dCanvasInfo & srcInfo, Ge2dCanvasInfo & dstInfo)
{
    aml_ge2d_info_t *pge2dinfo = &ge2d->ge2dinfo;
    int ret;

    if (!ge2d || !srcInfo.width || !srcInfo.height || !dstInfo.width || \
        !dstInfo.height || srcInfo.dmaFd < 0 || dstInfo.dmaFd < 0) {
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
    pge2dinfo->src_info[0].shared_fd[0] = srcInfo.dmaFd;
    pge2dinfo->src_info[0].canvas_w = srcInfo.width;
    pge2dinfo->src_info[0].canvas_h = srcInfo.height;
    pge2dinfo->src_info[0].format = srcInfo.format;
    pge2dinfo->src_info[0].rect.x = 0;
    pge2dinfo->src_info[0].rect.y = 0;
    pge2dinfo->src_info[0].rect.w = srcInfo.width;
    pge2dinfo->src_info[0].rect.h = srcInfo.height;

    pge2dinfo->dst_info.plane_number = 1;
    pge2dinfo->dst_info.rotation = GE2D_ROTATION_0;
    pge2dinfo->dst_info.mem_alloc_type = GE2D_MEM_DMABUF;
    pge2dinfo->dst_info.memtype = GE2D_CANVAS_ALLOC ;
    pge2dinfo->dst_info.shared_fd[0] = dstInfo.dmaFd;
    pge2dinfo->dst_info.canvas_w = dstInfo.width;
    pge2dinfo->dst_info.canvas_h = dstInfo.height;
    pge2dinfo->dst_info.format = dstInfo.format;
    pge2dinfo->dst_info.rect.x = 0;
    pge2dinfo->dst_info.rect.y = 0;
    pge2dinfo->dst_info.rect.w = dstInfo.width;
    pge2dinfo->dst_info.rect.h = dstInfo.height;

    ret = aml_ge2d_process(pge2dinfo);
    if (ret) {
        CAMERA_LOGE("aml_ge2d_process() failed. ret=%{bublic}d", ret);
    }

    return ret;
}

static inline uint64_t getTickMs()
{
    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec / 1000000ULL + ts.tv_sec * 1000ULL;
}

AMLCodecNode::AMLCodecNode(const std::string& name, const std::string& type) : NodeBase(name, type)
{
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(), type_.c_str());
    ge2d_ = calloc(sizeof(aml_ge2d_t), 1);
    if (!ge2d_) {
        CAMERA_LOGE("No enough memory for ge2d ctx");
        return;
    }
    
    if (aml_ge2d_init((aml_ge2d_t*)ge2d_)) {
        free(ge2d_);
        ge2d_ = nullptr;
        CAMERA_LOGE("aml_ge2d_init() failed.");
        return;
    }
}

AMLCodecNode::~AMLCodecNode()
{
    CAMERA_LOGI("~AMLCodecNode Node exit.");
    if (ge2d_) {
        aml_ge2d_exit((aml_ge2d_t*)ge2d_);
        CAMERA_LOGD("aml_ge2d_exit()");
        free(ge2d_);
    }
}

RetCode AMLCodecNode::Start(const int32_t streamId)
{
    CAMERA_LOGI("AMLCodecNode::Start streamId = %{public}d\n", streamId);
    vencFrameCnt_ = 0;
    return RC_OK;
}

RetCode AMLCodecNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("AMLCodecNode::Stop streamId = %{public}d\n", streamId);

    if (h264Enc_ >= 0) {
        vl_video_encoder_destory(h264Enc_);
        h264Enc_ = -1;
    }
    
    return RC_OK;
}

RetCode AMLCodecNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("AMLCodecNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

void AMLCodecNode::encodeJpegToMemory(unsigned char* image, int width, int height, \
    size_t* jpegSize, unsigned char** jpegBuf)
{
    struct jpeg_compress_struct cInfo;
    struct jpeg_error_mgr jErr;
    JSAMPROW row_pointer[1];
    int row_stride = 0;
    constexpr uint32_t colorMap = 3;
    constexpr uint32_t compressionRatio = 100;
    constexpr uint32_t pixelsThick = 3;

    cInfo.err = jpeg_std_error(&jErr);

    jpeg_create_compress(&cInfo);
    cInfo.image_width = width;
    cInfo.image_height = height;
    cInfo.input_components = colorMap;
    cInfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cInfo);
    jpeg_set_quality(&cInfo, compressionRatio, TRUE);
    jpeg_mem_dest(&cInfo, jpegBuf, jpegSize);
    jpeg_start_compress(&cInfo, TRUE);

    row_stride = width;
    while (cInfo.next_scanline < cInfo.image_height) {
        row_pointer[0] = &image[cInfo.next_scanline * row_stride * pixelsThick];
        jpeg_write_scanlines(&cInfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cInfo);
    jpeg_destroy_compress(&cInfo);
}

void AMLCodecNode::EncodeForPreview(std::shared_ptr<IBuffer>& buffer)
{
    aml_ge2d_t *ge2d = (aml_ge2d_t *)ge2d_;
    int ret, dmaFd = 0;
    uint8_t *vaddrTmp = nullptr;
    uint32_t dstFmt;
    uint64_t tickBegin = getTickMs();
    Ge2dCanvasInfo srcInfo;
    Ge2dCanvasInfo dstInfo;

    if (buffer == nullptr) {
        CAMERA_LOGE("Error: buffer == nullptr");
        return;
    }

    if (!ge2d) {
        CAMERA_LOGE("Error: ge2d == nullptr");
        return;
    }

    dstFmt = pixelFormatOHOSToGe2d((uint32_t)buffer->GetFormat());
    if (!dstFmt) {
        CAMERA_LOGE("Error: Unsuported dstFmt: %{public}u", buffer->GetFormat());
        return;
    }

    if (dstFmt == (uint32_t)GE2D_PIXEL_FORMAT_YCrCb_420_SP) {
        // No need. Skip
        return;
    }

    dmaFd = dmabuf_alloc(ge2d->ge2dinfo.ge2d_fd, GE2D_BUF_OUTPUT, buffer->GetSize());
    if (dmaFd < 0) {
        CAMERA_LOGE("Error: dmabuf_alloc() failed.");
        return;
    }

    srcInfo.width = buffer->GetWidth();
    srcInfo.height = buffer->GetHeight();
    srcInfo.format = (uint32_t)GE2D_PIXEL_FORMAT_YCrCb_420_SP;
    srcInfo.dmaFd = buffer->GetFileDescriptor();
    dstInfo.width = buffer->GetWidth();
    dstInfo.height = buffer->GetHeight();
    dstInfo.format = dstFmt;
    dstInfo.dmaFd = dmaFd;
    doBlit(ge2d, srcInfo, dstInfo);

    srcInfo.width = buffer->GetWidth();
    srcInfo.height = buffer->GetHeight();
    srcInfo.format = dstFmt;
    srcInfo.dmaFd = dmaFd;
    dstInfo.width = buffer->GetWidth();
    dstInfo.height = buffer->GetHeight();
    dstInfo.format = dstFmt;
    dstInfo.dmaFd = buffer->GetFileDescriptor();
    doBlit(ge2d, srcInfo, dstInfo);

    close(dmaFd);

    CAMERA_LOGD("srcFmt=%{public}d, dstFmt=%{public}d, use_time=%{public}llums", \
        GE2D_PIXEL_FORMAT_YCrCb_420_SP, dstFmt, getTickMs()-tickBegin);
}

void AMLCodecNode::EncodeForJpeg(std::shared_ptr<IBuffer>& buffer)
{
    aml_ge2d_t *ge2d = (aml_ge2d_t *)ge2d_;
    int dmaFd = -1;
    uint8_t *srcBuf = nullptr;
    unsigned char* jBuf = nullptr;
    size_t jpegSize = 0;
    Ge2dCanvasInfo srcInfo;
    Ge2dCanvasInfo dstInfo;

    if (buffer == nullptr) {
        CAMERA_LOGI("AMLCodecNode::EncodeForJpeg buffer == nullptr");
        return;
    }

    dmaFd = dmabuf_alloc(ge2d->ge2dinfo.ge2d_fd, GE2D_BUF_OUTPUT, buffer->GetSize());
    if (dmaFd < 0) {
        CAMERA_LOGE("Error: dmabuf_alloc() failed.");
        return;
    }

    srcInfo.width = previewWidth_;
    srcInfo.height = previewHeight_;
    srcInfo.format = (uint32_t)GE2D_PIXEL_FORMAT_YCrCb_420_SP;
    srcInfo.dmaFd = buffer->GetFileDescriptor();
    dstInfo.width = previewWidth_;
    dstInfo.height = previewHeight_;
    dstInfo.format = (uint32_t)GE2D_PIXEL_FORMAT_RGB_888;
    dstInfo.dmaFd = dmaFd;
    doBlit(ge2d, srcInfo, dstInfo);

    srcBuf = (uint8_t *)mmap(NULL, buffer->GetSize(), PROT_READ | PROT_WRITE, MAP_SHARED, dmaFd, 0);
    if (!srcBuf) {
        CAMERA_LOGE("Error: mmap() failed.");
        close(dmaFd);
        return;
    }

    encodeJpegToMemory(srcBuf, previewWidth_, previewHeight_, &jpegSize, &jBuf);
    if (memcpy_s(buffer->GetVirAddress(), buffer->GetSize(), jBuf, jpegSize) != EOK) {
        CAMERA_LOGE("AMLCodecNode::memcpy_s fail!\n");
    }

    buffer->SetEsFrameSize(jpegSize);
    free(jBuf);

    if (dmaFd >= 0) {
        munmap(srcBuf, buffer->GetSize());
        close(dmaFd);
    }

    CAMERA_LOGE("AMLCodecNode::EncodeForJpeg jpegSize = %{public}d\n", jpegSize);
}

void AMLCodecNode::EncodeForVideo(std::shared_ptr<IBuffer>& buffer)
{
    int datalen = 0;
    unsigned output_size;
    int idr_flag = 0;
    uint64_t tickBegin = getTickMs();
    vl_encoder_param_t encoder_param;
    vl_encode_info_t encode_info;
    const int framerate = 30;
    const int bitrate = 2000000;
    const int gop = 20;

    if (buffer == nullptr) {
        CAMERA_LOGI("buffer == nullptr");
        return;
    }

    CAMERA_LOGD("buffer_size=(%{public}d, %{public}d), preview_size=(%{public}d, %{public}d)", \
        buffer->GetWidth(), buffer->GetHeight(), previewWidth_, previewHeight_);

    if (h264Enc_ < 0) {
        encoder_param.framerate = framerate;
        encoder_param.bitrate = bitrate;
        encoder_param.gop = gop;
        h264Enc_ = (long)vl_video_encoder_init(CODEC_ID_H264, previewWidth_, previewHeight_, \
            encoder_param, IMG_FMT_NV21);
        CAMERA_LOGD("INFO: vl_video_encoder_init(): %{public}s.", (h264Enc_<0)?"FAILED":"SUCCESS");
    }

    if (h264Enc_ < 0) {
        CAMERA_LOGE("Error: Video Encoder not inited yet.");
        return;
    }

    uint8_t *output_buffer = (uint8_t *)malloc(buffer->GetSize());
    if (!output_buffer) {
        CAMERA_LOGE("Error: malloc failed.");
        return;
    }

    encode_info.frame_type = FRAME_TYPE_AUTO;
    encode_info.format = 1; /* NV21 */
    datalen = vl_video_encoder_encode(h264Enc_, encode_info, \
        (unsigned char*)buffer->GetVirAddress(), output_buffer, &idr_flag);
    if (datalen > 0) {
        struct timespec ts = {};

        memcpy_s(buffer->GetVirAddress(), buffer->GetSize(), output_buffer, datalen);
        buffer->SetEsFrameSize(datalen);

        clock_gettime(CLOCK_MONOTONIC, &ts);
        buffer->SetEsTimestamp(ts.tv_nsec + ts.tv_sec * 1000000000ULL);

        buffer->SetEsKeyFrame(idr_flag);

        vencFrameCnt_++;

        CAMERA_LOGD("[%{public}u] datalen=%{public}d, idr=%{public}d, use_time=%{public}llums", \
            vencFrameCnt_, datalen, idr_flag, getTickMs()-tickBegin);
    }

    free(output_buffer);
}

void AMLCodecNode::DeliverBuffer(std::shared_ptr<IBuffer> &buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("AMLCodecNode::DeliverBuffer frameSpec is null");
        return;
    }

    int32_t id = buffer->GetStreamId();
    CAMERA_LOGD("AMLCodecNode::DeliverBuffer ENTER StreamId %{public}d, type: %{public}d",
                id, buffer->GetEncodeType());
    if (buffer->GetBufferStatus() == CAMERA_BUFFER_STATUS_OK) {
        if (buffer->GetEncodeType() == ENCODE_TYPE_JPEG) {
            EncodeForJpeg(buffer);
        } else if (buffer->GetEncodeType() == ENCODE_TYPE_H264 ||
                   buffer->GetEncodeType() == ENCODE_TYPE_H265) {
            EncodeForVideo(buffer);
        } else {
            previewWidth_ = buffer->GetWidth();
            previewHeight_ = buffer->GetHeight();

            EncodeForPreview(buffer);
        }
    }

    outPutPorts_ = GetOutPorts();
    for (auto &it : outPutPorts_) {
        if (it->format_.streamId_ == id) {
            it->DeliverBuffer(buffer);
            CAMERA_LOGI("AMLCodecNode deliver buffer streamid = %{public}d", it->format_.streamId_);
            return;
        }
    }
}

RetCode AMLCodecNode::Capture(const int32_t streamId, const int32_t captureId)
{
    CAMERA_LOGV("AMLCodecNode::Capture streamid = %{public}d captureId = %{public}d", streamId, captureId);
    return RC_OK;
}

RetCode AMLCodecNode::CancelCapture(const int32_t streamId)
{
    CAMERA_LOGI("AMLCodecNode::CancelCapture streamid = %{public}d", streamId);

    return RC_OK;
}

REGISTERNODE(AMLCodecNode, {"AMLCodec"})
} // namespace OHOS::Camera

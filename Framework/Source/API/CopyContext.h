/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once
#include "API/Resource.h"
#include "API/LowLevel/LowLevelContextData.h"

namespace Falcor
{
    class Texture;
    class Buffer;

    class CopyContext : public std::enable_shared_from_this<CopyContext>
    {
    public:
        using SharedPtr = std::shared_ptr<CopyContext>;
        using SharedConstPtr = std::shared_ptr<const CopyContext>;
        virtual ~CopyContext();

        class ReadTextureTask
        {
        public:
            using SharedPtr = std::shared_ptr<ReadTextureTask>;
            static SharedPtr create(CopyContext::SharedPtr pCtx, const Texture* pTexture, uint32_t subresourceIndex);
            std::vector<uint8> getData();
        private:
            ReadTextureTask() = default;
            GpuFence::SharedPtr mpFence;
            Buffer::SharedPtr mpBuffer;
            CopyContext::SharedPtr mpContext;
#ifdef FALCOR_D3D12
            uint32_t mRowCount;
            ResourceFormat mTextureFormat;
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT mFootprint;
#elif defined(FALCOR_VK)
            size_t mDataSize;
#endif
        };

        static SharedPtr create(CommandQueueHandle queue);
        void updateBuffer(const Buffer* pBuffer, const void* pData, size_t offset = 0, size_t numBytes = 0);
        void updateTexture(const Texture* pTexture, const void* pData);
        void updateTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex, const void* pData);
        void updateTextureSubresources(const Texture* pTexture, uint32_t firstSubresource, uint32_t subresourceCount, const void* pData);
        ReadTextureTask::SharedPtr asyncReadTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex);
        std::vector<uint8> readTextureSubresource(const Texture* pTexture, uint32_t subresourceIndex);

        /** Flush the command list. This doesn't reset the command allocator, just submits the commands
            \param[in] wait If true, will block execution until the GPU finished processing the commands
        */
        virtual void flush(bool wait = false);

        /** Check if we have pending commands
        */
        bool hasPendingCommands() const { return mCommandsPending; }

        /** Signal the context that we have pending commands. Useful in case you make raw API calls
        */
        void setPendingCommands(bool commandsPending) { mCommandsPending = commandsPending; }

        /** Insert a resource barrier
            if pViewInfo is nullptr, will transition the entire resource. Otherwise, it will only transition the subresource in the view
        */
        virtual void resourceBarrier(const Resource* pResource, Resource::State newState, const ResourceViewInfo* pViewInfo = nullptr);

        /** Insert a UAV barrier
        */
        virtual void uavBarrier(const Resource* pResource);

        /** Copy an entire resource
        */
        void copyResource(const Resource* pDst, const Resource* pSrc);

        /** Copy a subresource
        */
        void copySubresource(const Texture* pDst, uint32_t dstSubresourceIdx, const Texture* pSrc, uint32_t srcSubresourceIdx);

        /** Copy part of a buffer 
        */
        void copyBufferRegion(const Buffer* pDst, uint64_t dstOffset, const Buffer* pSrc, uint64_t srcOffset, uint64_t numBytes);

        /** Get the low-level context data
        */
        virtual LowLevelContextData::SharedPtr getLowLevelData() const { return mpLowLevelData; }

        /** Override the low-level context data with a user provided object
        */
        void setLowLevelContextData(LowLevelContextData::SharedPtr pLowLevelData) { mpLowLevelData = pLowLevelData; }
    protected:
        void bindDescriptorHeaps();
        CopyContext() = default;
        bool mCommandsPending = false;
        LowLevelContextData::SharedPtr mpLowLevelData;
    };
}

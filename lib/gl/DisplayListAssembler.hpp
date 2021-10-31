// RasteriCEr
// https://github.com/ToNi3141/RasteriCEr
// Copyright (c) 2021 ToNi3141

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef DISPLAYLISTASSEMBLER_HPP
#define DISPLAYLISTASSEMBLER_HPP

#include <stdint.h>
#include "DisplayList.hpp"
#include "Rasterizer.hpp"

template <uint32_t DISPLAY_LIST_SIZE, uint8_t ALIGNMENT>
class DisplayListAssembler {
public:
    using List = DisplayList<DISPLAY_LIST_SIZE, ALIGNMENT>;
    static constexpr uint32_t SET_COLOR_BUFFER_CLEAR_COLOR = 0x0000;
    static constexpr uint32_t SET_DEPTH_BUFFER_CLEAR_DEPTH = 0x0001;
    static constexpr uint32_t SET_CONF_REG1                = 0x0002;
    static constexpr uint32_t SET_CONF_REG2                = 0x0003;
    static constexpr uint32_t SET_TEX_ENV_COLOR            = 0x0004;

    void clearAssembler()
    {
        m_displayList.clear();
        m_streamCommand = nullptr;
        m_wasLastCommandATextureCommand = false;
    }

    bool commit()
    {
        if (openNewStreamSection())
        {
            // Add frame buffer flush command
            SCT *op = m_displayList.template create<SCT>();
            if (op)
            {
                *op = StreamCommand::FRAMEBUFFER_COMMIT | StreamCommand::FRAMEBUFFER_COLOR;
            }

            closeStreamSection();
            return op != nullptr;
        }
        return false;
    }

    bool drawTriangle(const Rasterizer::RasterizedTriangle& triangle)
    {
        if (openNewStreamSection())
        {
            m_wasLastCommandATextureCommand = false;
            return appendStreamCommand(StreamCommand::TRIANGLE_FULL, triangle);
        }
        return false;
    }

    bool updateTexture(const uint32_t addr, std::shared_ptr<const uint16_t> pixels, const uint32_t texSize)
    {
        closeStreamSection();
        bool ret = appendStreamCommand<SCT>(StreamCommand::STORE | texSize, addr);
        void *dest = m_displayList.alloc(texSize);
        if (ret && dest)
        {
            memcpy(dest, pixels.get(), texSize);
            return true;
        }
        return false;
    }

    bool useTexture(const uint32_t texAddr, const uint32_t texSize, const uint32_t texWidth, const uint32_t texHeight)
    {
        (void)texHeight;
        bool ret = false;
        if (openNewStreamSection())
        {
            // Check if the last command was a texture command and not a triangle. If no triangle has to be drawn
            // with the recent texture, then we can just overwrite this texture with the current one and avoiding
            // with that mechanism unnecessary texture loads.
            if (!m_wasLastCommandATextureCommand)
            {
                m_texStreamOp = m_displayList.template create<SCT>();
                if (m_texStreamOp)
                {
                    closeStreamSection();
                    m_texLoad = m_displayList.template create<SCT>();
                    m_texLoadAddr = m_displayList.template create<uint32_t>();
                }
            }
            if (m_texStreamOp && m_texLoad && m_texLoadAddr)
            {
                if (texWidth == 32)
                {
                    *m_texStreamOp = StreamCommand::TEXTURE_STREAM_32x32;
                }
                else if (texWidth == 64)
                {
                    *m_texStreamOp = StreamCommand::TEXTURE_STREAM_64x64;
                }
                else if (texWidth == 128)
                {
                    *m_texStreamOp = StreamCommand::TEXTURE_STREAM_128x128;
                }
                else if (texWidth == 256)
                {
                    *m_texStreamOp = StreamCommand::TEXTURE_STREAM_256x256;
                }
                *m_texLoad = StreamCommand::LOAD | texSize;
                *m_texLoadAddr = texAddr;
                m_wasLastCommandATextureCommand = true;
                ret = true;
            }
            else
            {
                if (!m_wasLastCommandATextureCommand)
                {
                    if (m_texStreamOp)
                    {
                        *m_texStreamOp = StreamCommand::NOP;
                    }
                    if (m_texLoad)
                    {
                        m_displayList.template remove<SCT>();
                    }
                    if (m_texLoadAddr)
                    {
                        m_displayList.template remove<uint32_t>();
                    }
                }
            }
        }

        return ret;
    }

    bool clear(bool colorBuffer, bool depthBuffer)
    {
        if (openNewStreamSection())
        {
            const SCT opColorBuffer = StreamCommand::FRAMEBUFFER_MEMSET | StreamCommand::FRAMEBUFFER_COLOR;
            const SCT opDepthBuffer = StreamCommand::FRAMEBUFFER_MEMSET | StreamCommand::FRAMEBUFFER_DEPTH;

            SCT *op = m_displayList.template create<SCT>();
            if (op)
            {
                if (colorBuffer && depthBuffer)
                {
                    *op = opColorBuffer | opDepthBuffer;
                }
                else if (colorBuffer)
                {
                    *op = opColorBuffer;
                }
                else if (depthBuffer)
                {
                    *op = opDepthBuffer;
                }
                else
                {
                    *op = StreamCommand::NOP;
                }
            }
            return op != nullptr;
        }
        return false;
    }

    template <typename TArg>
    bool writeRegister(uint32_t regIndex, const TArg& regVal)
    {
        if (openNewStreamSection())
        {
            return appendStreamCommand<TArg>(StreamCommand::SET_REG | regIndex, regVal);
        }
        return false;
    }

    const List* getDisplayList() const
    {
        return &m_displayList;
    }

    static constexpr uint32_t uploadCommandSize()
    {
        return List::template sizeOf<SCT>() + List::template sizeOf<uint32_t>();
    }

private:
    struct StreamCommand
    {
        // Anathomy of a command:
        // | 4 bit OP | 28 bit IMM |

        using StreamCommandType = uint32_t;

        // This mask will set the command
        static constexpr StreamCommandType STREAM_COMMAND_OP_MASK = 0xf000'0000;

        // This mask will set the immediate value
        static constexpr StreamCommandType STREAM_COMMAND_IMM_MASK = 0x0fff'ffff;

        // Calculate the triangle size with align overhead.
        static constexpr StreamCommandType TRIANGLE_SIZE_ALIGNED = List::template sizeOf<Rasterizer::RasterizedTriangle>();

        // OPs
        static constexpr StreamCommandType NOP              = 0x0000'0000;
        static constexpr StreamCommandType TEXTURE_STREAM   = 0x1000'0000;
        static constexpr StreamCommandType SET_REG          = 0x2000'0000;
        static constexpr StreamCommandType FRAMEBUFFER_OP   = 0x3000'0000;
        static constexpr StreamCommandType TRIANGLE_STREAM  = 0x4000'0000;

        // Immediate values
        static constexpr StreamCommandType TEXTURE_STREAM_32x32     = TEXTURE_STREAM | 0x0011;
        static constexpr StreamCommandType TEXTURE_STREAM_64x64     = TEXTURE_STREAM | 0x0022;
        static constexpr StreamCommandType TEXTURE_STREAM_128x128   = TEXTURE_STREAM | 0x0044;
        static constexpr StreamCommandType TEXTURE_STREAM_256x256   = TEXTURE_STREAM | 0x0088;

        static constexpr StreamCommandType SET_COLOR_BUFFER_CLEAR_COLOR = SET_REG | 0x0000;
        static constexpr StreamCommandType SET_DEPTH_BUFFER_CLEAR_DEPTH = SET_REG | 0x0001;
        static constexpr StreamCommandType SET_CONF_REG1                = SET_REG | 0x0002;
        static constexpr StreamCommandType SET_CONF_REG2                = SET_REG | 0x0003;
        static constexpr StreamCommandType SET_TEX_ENV_COLOR            = SET_REG | 0x0004;

        static constexpr StreamCommandType FRAMEBUFFER_COMMIT   = FRAMEBUFFER_OP | 0x0001;
        static constexpr StreamCommandType FRAMEBUFFER_MEMSET   = FRAMEBUFFER_OP | 0x0002;
        static constexpr StreamCommandType FRAMEBUFFER_COLOR    = FRAMEBUFFER_OP | 0x0010;
        static constexpr StreamCommandType FRAMEBUFFER_DEPTH    = FRAMEBUFFER_OP | 0x0020;

        static constexpr StreamCommandType TRIANGLE_FULL  = TRIANGLE_STREAM | TRIANGLE_SIZE_ALIGNED;

        static constexpr StreamCommandType STORE     = 0x5000'0000;
        static constexpr StreamCommandType LOAD      = 0x6000'0000;
        static constexpr StreamCommandType MEMSET    = 0x7000'0000;
        static constexpr StreamCommandType STREAM    = 0x8000'0000;
    };
    using SCT = typename StreamCommand::StreamCommandType;

    template <typename TArg, bool CallConstructor = false>
    bool appendStreamCommand(const SCT op, const TArg& arg)
    {
        SCT *opDl = m_displayList.template create<SCT>();
        TArg *argDl = m_displayList.template create<TArg>();

        if (!(opDl && argDl))
        {
            if (opDl)
            {
                m_displayList.template remove<SCT>();
            }

            if (argDl)
            {
                m_displayList.template remove<TArg>();
            }
            // Out of memory error
            return false;
        }

        // This is an optimization. Most of the time, a constructor call is not necessary and will just take a
        // significant amount of CPU time. So, if it is not required, we omit it.
        if constexpr (CallConstructor)
        {
            new (argDl) TArg();
        }

        *opDl = op;
        *argDl = arg;
        return true;
    }

    bool openNewStreamSection()
    {
        if (m_streamCommand == nullptr)
        {
            m_streamCommand = m_displayList.template create<SCT>();
            if (m_streamCommand)
            {
                *m_streamCommand = m_displayList.getSize();
            }
        }
        return m_streamCommand != nullptr;;
    }

    bool closeStreamSection()
    {
        // Note during open, we write the current size of the displaylist into this command.
        // Now we just have to substract from the current display list size the last display list size
        // to know how big our stream section is.
        if (m_streamCommand)
        {
            *m_streamCommand = StreamCommand::STREAM | (m_displayList.getSize() - *m_streamCommand);
            m_streamCommand = nullptr;
            return true;
        }
        return false;
    }

    List m_displayList __attribute__ ((aligned (8)));

    SCT *m_streamCommand{nullptr};

    // Helper variables to optimize the texture loading
    bool m_wasLastCommandATextureCommand{false};
    SCT *m_texStreamOp{nullptr};
    SCT *m_texLoad{nullptr};
    uint32_t *m_texLoadAddr{nullptr};
};


#endif // DISPLAYLISTASSEMBLER_HPP

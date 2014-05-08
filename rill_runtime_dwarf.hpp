//
// Copyright yutopp 2014 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#ifndef RILL_RUNTIME_DWARF_HPP
#define RILL_RUNTIME_DWARF_HPP

#include <cstddef>
#include <cstdint>
#include <climits>
#include <cassert>


namespace rill
{
    namespace runtime
    {
        namespace dwarf
        {
            //
            constexpr std::uint8_t const DW_EH_PE_absptr    = 0x00;

            constexpr std::uint8_t const DW_EH_PE_omit      = 0xff;
            constexpr std::uint8_t const DW_EH_PE_uleb128   = 0x01;
            constexpr std::uint8_t const DW_EH_PE_udata2    = 0x02;
            constexpr std::uint8_t const DW_EH_PE_udata4    = 0x03;
            constexpr std::uint8_t const DW_EH_PE_udata8    = 0x04;
            constexpr std::uint8_t const DW_EH_PE_sleb128   = 0x09;
            constexpr std::uint8_t const DW_EH_PE_sdata2    = 0x0a;
            constexpr std::uint8_t const DW_EH_PE_sdata4    = 0x0b;
            constexpr std::uint8_t const DW_EH_PE_sdata8    = 0x0c;
            constexpr std::uint8_t const DW_EH_PE_signed    = 0x09;

            constexpr std::uint8_t const DW_EH_PE_pcrel     = 0x10;
            constexpr std::uint8_t const DW_EH_PE_textrel   = 0x20;
            constexpr std::uint8_t const DW_EH_PE_datarel   = 0x30;
            constexpr std::uint8_t const DW_EH_PE_funcrel   = 0x40;
            constexpr std::uint8_t const DW_EH_PE_aligned   = 0x50;

            constexpr std::uint8_t const DW_EH_PE_indirect  = 0x80;


            // http://coins-compiler.sourceforge.jp/contributions/CoinsJava/CJData/DWARF2_encoding.html

            template<typename T = std::size_t>
            auto read_uleb128( std::uint8_t const* const addr, T& value )
                -> std::uint8_t const*
            {
                std::size_t bits = 0, i = 0;
                std::uint8_t c;

                value = 0;

                do {
                    c = addr[i];
                    value |= ( c & 0x7f ) << bits;

                    bits += 7;
                    ++i;

                    assert( bits < sizeof( T ) * 8 );
                } while( ( c & 0x80 ) != 0 );

                return addr + i;
            }


            template<typename T = std::size_t>
            auto read_sleb128( std::uint8_t const* const addr, T& value )
                -> std::uint8_t const* const
            {
                std::size_t bits = 0, i = 0;
                std::uint8_t c;

                value = 0;

                do {
                    c = addr[i];
                    value |= ( c & 0x7f ) << bits;

                    bits += 7;
                    ++i;

                    assert( bits < sizeof( T ) * 8 );
                } while( ( c & 0x80 ) != 0 );

                // read signed bit: last byte & 0b01000000(0x40)
                bool const is_nagate = ( *( addr + i - 1 ) & 0x40 ) != 0;

                if ( is_nagate ) {
                    // set MSB
                    value |= ~( 1 << bits ) + 1;
                }

                return addr + i;
            }

#if 0
            struct UT_0
            {
                UT_0()
                {
                    {
                        // echo ".data\n.uleb128 1234\n" > /tmp/test.s && gcc -c /tmp/test.s -o /tmp/test.o && objdump -s -j .data /tmp/test.o
                        std::uint8_t a[] = { 0xd2, 0x09 };

                        std::uint64_t val;
                        read_uleb128( a, val );

                        assert( val == 1234 );
                        std::cout << val << std::endl;


                    }

                    {
                        // echo ".data\n.sleb128 -1234\n" > /tmp/test.s && gcc -c /tmp/test.s -o /tmp/test.o && objdump -s -j .data /tmp/test.o
                        std::uint8_t a[] = { 0xae, 0x76 };

                        std::int64_t val;
                        read_sleb128( a, val );

                        assert( val == -1234 );
                        std::cout << val << std::endl;
                    }
                }

            } ut_0;
#endif


            template<typename T>
            auto read_encoded_value(
                _Unwind_Context* const context,
                unsigned char const encoding,
                unsigned char const* buffer,
                T& result
                )
                -> unsigned char const*
            {
                // check base
                switch( encoding & 0x70 )
                {
                case DW_EH_PE_absptr:
                    // DO nothing
                    break;

                case DW_EH_PE_pcrel:
                    break;

                case DW_EH_PE_datarel:
                    break;

                case DW_EH_PE_textrel:
                    break;

                case DW_EH_PE_funcrel:
                    break;

                case DW_EH_PE_aligned:
                    break;

                default:
                    assert( false );
                }


                //
                // TODO: check endian
                switch( encoding & 0x0f )
                {
                case DW_EH_PE_uleb128:
                    std::cout << "DW_EH_PE_uleb128:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_udata2:
                    std::cout << "DW_EH_PE_udata2:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_udata4:
                    result = *reinterpret_cast<std::uint32_t const*>( buffer );
                    buffer += 4;
                    break;

                case DW_EH_PE_udata8:
                    std::cout << "DW_EH_PE_udata8:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_sleb128:
                    std::cout << "DW_EH_PE_sleb128:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_sdata2:
                    std::cout << "DW_EH_PE_sdata2:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_sdata4:
                    std::cout << "DW_EH_PE_sdata4:" << std::endl;
                    assert( false );
                    break;

                case DW_EH_PE_sdata8:
                    std::cout << std::hex << (encoding & 0x0f ) << std::dec << std::endl;
                    assert( false );

                    break;

                default:
                    assert( false );
                }

                return buffer;
            }


            inline auto encoding_size( std::uint8_t const encoding )
                -> std::size_t
            {
                switch( encoding & 0x0f )
                {
                case DW_EH_PE_udata2:
                case DW_EH_PE_sdata2:
                    return 2;

                case DW_EH_PE_udata4:
                case DW_EH_PE_sdata4:
                    return 4;

                case DW_EH_PE_udata8:
                case DW_EH_PE_sdata8:
                    return 8;

                case DW_EH_PE_uleb128:
                case DW_EH_PE_sleb128:
                    assert( false );
                }

                assert( false );
                return 0;
            }

        } // namespace dwarf
    } // namespace runtime
} // namespace rill

#endif /*RILL_RUNTIME_DWARF_HPP*/

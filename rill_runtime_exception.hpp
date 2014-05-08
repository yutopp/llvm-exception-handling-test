//
// Copyright yutopp 2014 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#ifndef RILL_RUNTIME_EXCEPTION_HPP
#define RILL_RUNTIME_EXCEPTION_HPP

#include <iostream>

#include <cstddef>
#include <cstdint>
#include <climits>
#include <cassert>

//
#include <unwind.h>

#include "rill_runtime_dwarf.hpp"


namespace rill
{
    // type information
    struct type_info
    {
        std::string debug_string;
        int i;
    };


    namespace runtime
    {
        namespace exception
        {
            enum class install_action : std::uint8_t
            {
                e_not_found,
                e_terminate,
                e_cleanup,
                e_catch
            };


            //
            struct exception_object
            {
                //
                _Unwind_Exception unwind_info;

                //
                int thrown_type_info;

                //
                install_action next_install_action;
                std::uint32_t switch_val;
                std::uint8_t const* landing_pad;

                // TODO:
                // storage
                // destructor
            };


            //
            struct lsda_basic_info
            {
                // header of landing pad
                std::uint8_t landing_pad_start_encoding;
                std::uint8_t const* landing_pad_start;

                // header of type table
                std::uint8_t type_table_encoding;
                std::size_t type_table_offset;

                // header of call site
                std::uint8_t call_site_encoding;
                std::size_t call_site_table_length;

                //
                std::uint8_t const* call_site;
                std::uint8_t const* action_table;
                std::uint8_t const* type_table;
            };


            //
            struct lsda_call_site_entry
            {
                std::uint8_t const* landing_pad;
                std::uint8_t const* action_table;
            };

            enum class handling_action
            {
                e_continue,
                e_not_found,
                e_terminate
            };


            //
            enum class clause_type
            {
                e_none,
                e_cleanup,
                e_catch
            };

            struct clause_info
            {
                std::uint64_t clause_index;
            };


            //
            auto parse_lsda_header(
                _Unwind_Context* const context,
                std::uint8_t const* buffer,
                lsda_basic_info& lbi
                )
                -> void;
            //
            auto load_lsda( _Unwind_Context* const context, lsda_basic_info& lbi )
                -> bool;



            //
            auto lsda_table_based_call_site(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                lsda_basic_info const& lbi,
                lsda_call_site_entry& lcse
                )
                -> handling_action;


            //
            auto lsda_read_type_info(
                _Unwind_Context* const context,
                lsda_basic_info const& lbi,
                std::size_t const index
                )
                -> type_info const*;


            //
            inline auto is_force_unwind( _Unwind_Action const& actions )
                -> bool
            {
                return ( actions & _UA_FORCE_UNWIND ) != 0;
            }


            //
            inline auto is_search_phase( _Unwind_Action const& actions )
                -> bool
            {
                return ( actions & _UA_SEARCH_PHASE ) != 0;
            }


            //
            auto lsda_select_handler(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                exception_object const* const ex_obj,
                lsda_basic_info const& lbi,
                lsda_call_site_entry const& lcse,
                clause_info& ci
                )
                -> clause_type;


            //
            auto get_clause_info(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                exception_object const* const ex_obj,
                lsda_basic_info const& lbi,
                lsda_call_site_entry const& lcse,
                clause_info& ci
                )
                -> install_action;


            //
            auto select_clause(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                exception_object const* const ex_obj,
                handling_action const& status,
                lsda_basic_info const& lbi,
                lsda_call_site_entry const& lcse,
                clause_info& ci
                )
                -> install_action;


            //
            inline auto get_rill_exception_object_from_info(
                _Unwind_Exception const* exception_info
                )
                -> exception_object*
            {
                return reinterpret_cast<exception_object*>(
                    reinterpret_cast<std::uintptr_t const>( exception_info ) - offsetof( exception_object, unwind_info )
                    );
            }


            //
            template<std::size_t N>
            inline auto make_exception_class( char const (&name)[N] )
                -> _Unwind_Exception_Class
            {
                // 8 is a size of _Unwind_Exception_Class
                static_assert( N <= 8, "" );
                static_assert( CHAR_BIT == 8, "" );    // optional check

                _Unwind_Exception_Class uec = 0;

                for( std::size_t i=0; i<N; ++i ) {
                    uec |= static_cast<_Unwind_Exception_Class>( name[i] ) << ( 8 * (7 - i) );
                }

                return uec;
            }


            // for x86_64
            inline auto eh_exception_regno()
                -> int
            {
                return 0;
            }
            inline auto eh_selector_regno()
                -> int
            {
                return 1;
            }


            //
            auto install_catch_clause(
                _Unwind_Context* context,
                exception_object const* rill_exception_obj
                )
                -> _Unwind_Reason_Code;


            //
            auto install_finally_clause(
                _Unwind_Context* context,
                exception_object const* rill_exception_obj
                )
                -> _Unwind_Reason_Code;


            //
            //
            //

            // exception class
            inline auto get_exception_class_name()
                -> char const*;
            extern _Unwind_Exception_Class exception_class_value;



            //
            //
            //


            //
            inline auto allocate_exception( std::size_t const size )
                -> exception_object*
            {
                auto eo = new exception_object();

                // TODO: allocate storage holds "size" bytes
                // eo.storage = new ~~~

                return eo;
            }


            //
            inline auto throw_exception(
                exception_object* exception_object,
                int test_index /*TODO: add type info, void(*destructor_for_exception_storage)(void*)*/
                )
                -> _Unwind_Reason_Code
            {
                assert( exception_object != nullptr );

                //
                exception_object->unwind_info.exception_class
                    = exception_class_value;

                exception_object->thrown_type_info = test_index;

                _Unwind_Reason_Code const ret
                    = _Unwind_RaiseException( &exception_object->unwind_info );

                // if reached here, it will be error...
                std::cout << "_Unwind_RaiseException failed with reason code: " << ret << std::endl;
                abort();
            }


            //
            inline auto resume(
                exception_object* exception_object,
                std::uint32_t const selector
                )
                -> void
            {
                std::cout << "resume" << std::endl;

                _Unwind_Resume( &exception_object->unwind_info );
            }


            //
            inline auto free_exception(
                exception_object* exception_object
                )
                -> void
            {
                assert( exception_object != nullptr );

                // TODO: destruct storage in exception_object by the destructor
                //

                delete exception_object;
            }


        } // namespace exception
    } // namespace runtime
} // namespace rill

#endif /*RILL_RUNTIME_EXCEPTION_HPP*/

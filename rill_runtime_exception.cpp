//
// Copyright yutopp 2014 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include <iostream>

#include <cstddef>
#include <cstdint>
#include <climits>
#include <cassert>

#include "rill_runtime_exception.hpp"


namespace rill
{
    namespace runtime
    {
        namespace exception
        {
            //
            auto parse_lsda_header(
                _Unwind_Context* const context,
                std::uint8_t const* buffer,
                lsda_basic_info& lbi
                )
                -> void
            {
                // landing pad
                {
                    // read encoding
                    lbi.landing_pad_start_encoding = *buffer;
                    ++buffer;

                    if ( lbi.landing_pad_start_encoding == dwarf::DW_EH_PE_omit ) {
                        // using default value
                        assert( context != nullptr );

                        lbi.landing_pad_start
                            = reinterpret_cast<std::uint8_t const*>( _Unwind_GetRegionStart( context ) );

                        // RILL_DEBUG( "start: " << lbi.landing_pad_start )

                    } else {
                        assert( false && "not supported" );
                    }
                }


                // type table
                {
                    lbi.type_table_encoding = *buffer;
                    ++buffer;

                    if ( lbi.type_table_encoding == dwarf::DW_EH_PE_omit ) {
                        assert( false );

                    } else {
                        buffer = dwarf::read_uleb128( buffer, lbi.type_table_offset );

                        // calulate the address of type table
                        lbi.type_table = buffer + lbi.type_table_offset;
                    }
                }


                // call site (and action table)
                {
                    lbi.call_site_encoding = *buffer;
                    ++buffer;

                    if ( lbi.call_site_encoding == dwarf::DW_EH_PE_udata4 ) {
                        buffer = dwarf::read_uleb128( buffer, lbi.call_site_table_length );

                        // calulate the address of *action table*
                        lbi.action_table = buffer + lbi.call_site_table_length;

                        // RILL_DEBUG( lbi.call_site_table_length )

                    } else {
                        assert( false );
                    }
                }

                // address of call site
                lbi.call_site = buffer;
            }


            //
            auto load_lsda( _Unwind_Context* const context, lsda_basic_info& lbi )
                -> bool
            {
                // load LSDA(Language Specific Deta Area)
                std::uint8_t const* language_specific_data
                    = static_cast<std::uint8_t const*>( _Unwind_GetLanguageSpecificData( context ) );
                if ( !language_specific_data ) {
                    // RILL_DEBUG( "NO LSDA" )
                    return false;
                }

                // parse
                parse_lsda_header( context, language_specific_data, lbi );

                return true;
            }


            //
            auto lsda_table_based_call_site(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                lsda_basic_info const& lbi,
                lsda_call_site_entry& lcse
                )
                -> handling_action
            {
                std::uintptr_t ip = _Unwind_GetIP( context ) - 1;

                for( std::uint8_t const* cs=lbi.call_site; /**/; /**/ ) {
                    // not found...
                    if ( cs >= lbi.action_table ) {
                        // TODO: dtors in current section...
                        assert( false );
                    }

                    // read data
                    std::size_t call_site_start, call_site_length, landing_pad, action;

                    // table based call site entry
                    cs = dwarf::read_encoded_value( context, lbi.call_site_encoding, cs, call_site_start );
                    cs = dwarf::read_encoded_value( context, lbi.call_site_encoding, cs, call_site_length );
                    cs = dwarf::read_encoded_value( context, lbi.call_site_encoding, cs, landing_pad );
                    cs = dwarf::read_uleb128( cs, action );

                    /* RILL_DEBUG( "block_start_offset: " << call_site_start << std::endl
                       << "block_size: " << call_site_length << std::endl
                       << "landing_pad : " << landing_pad << std::endl
                       << "action: " << action )
                    */


                    if ( ip < reinterpret_cast<std::uintptr_t const>( lbi.landing_pad_start ) + call_site_start ) {
                        // table was not found
                        break;

                    } else if ( ip < reinterpret_cast<std::uintptr_t const>( lbi.landing_pad_start ) + call_site_start + call_site_length ) {
                        // ip is in range of region_start
                        // RILL_DEBUG( "found!" )

                        if ( landing_pad ) {
                            lcse.landing_pad = reinterpret_cast<std::uint8_t*>( reinterpret_cast<std::uintptr_t>( lbi.landing_pad_start ) + landing_pad );
                        }

                        if ( action ) {
                            lcse.action_table = lbi.action_table + action - 1;
                        }

                        return handling_action::e_continue;
                    }
                }


                // not found
                if ( ( actions & _UA_FORCE_UNWIND ) != 0 ) {
                    return handling_action::e_not_found;

                } else {
                    // if handler was not found and action is NOT force_unwind, terminate user program.
                    return handling_action::e_terminate;
                }
            }


            //
            auto lsda_read_type_info(
                _Unwind_Context* const context,
                lsda_basic_info const& lbi,
                std::size_t const index
                )
                -> type_info const*
            {
                std::size_t const offset = dwarf::encoding_size( lbi.type_table_encoding ) * index;

                std::uintptr_t p;
                dwarf::read_encoded_value( context, lbi.type_table_encoding, lbi.type_table - offset, p );

                return reinterpret_cast<type_info*>( p );
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
                -> clause_type
            {
                signed long ar_filter, ar_disp;

                //
                clause_type ct = clause_type::e_none;

                // read action table and update clause_type incrementally
                for( auto ar=lcse.action_table; /**/; /**/ ) {
                    // action table entry
                    ar = dwarf::read_sleb128( ar, reinterpret_cast<std::size_t &>(ar_filter) );
                    dwarf::read_sleb128( ar, reinterpret_cast<std::size_t &>(ar_disp) );

                    ar += ar_disp;

                    //
                    if ( ar_filter == 0 ) {
                        // 0: cleanup
                        //RILL_DEBUG( "FIlter is clean up" )

                        ct = clause_type::e_cleanup;
                        break;

                    } else if ( ar_filter > 0 ) {
                        // positive: catches

                        if ( !is_force_unwind( actions ) ) {
                            type_info const* const catcher_type_info
                                = lsda_read_type_info( context, lbi, ar_filter );

                            // RILL_DEBUG( catcher_type->a )

                            if ( catcher_type_info == nullptr ) {
                                // if catcher_type is null, clause is "catch all"
                                ct = clause_type::e_catch;
                                ci.clause_index = ar_filter;

                            } else {
                                // TODO: implement type match
                                // if matched, update

                                std::cout << "| type_info: " << ex_obj->thrown_type_info << " / " << catcher_type_info->debug_string << std::endl;

                                // testtesttest
                                if ( catcher_type_info->i == ex_obj->thrown_type_info ) {
                                    ct = clause_type::e_catch;
                                    ci.clause_index = ar_filter;

                                    std::cout << "| ! selected clause index: " << ci.clause_index << std::endl;
                                }
                            }
                        }

                        // RILL_DEBUG( "caught" )

                    } else {
                        // negative: exception specification
                        // RILL_DEBUG( "exception specification" )
                        assert( false && "not supported" );
                    }

                    //
                    if ( ar_disp == 0 ) {
                        // finished
                        break;
                    }
                }

                return ct;
            }


            //
            auto get_clause_info(
                _Unwind_Context* const context,
                _Unwind_Action const& actions,
                exception_object const* const ex_obj,
                lsda_basic_info const& lbi,
                lsda_call_site_entry const& lcse,
                clause_info& ci
                )
                -> install_action
            {
                if ( lcse.landing_pad == nullptr ) {
                    // there are no landing pad and catch clause
                    // RILL_DEBUG( "no clean up / no catch clause" )
                    return install_action::e_not_found;

                } else if ( lcse.action_table == nullptr ) {
                    // there is only clean up routine
                    // RILL_DEBUG( "cleanup" )
                    return install_action::e_cleanup;

                } else {
                    // there are some handlers
                    // so, select the target clause

                    auto const ct = lsda_select_handler( context, actions, ex_obj, lbi, lcse, ci );
                    switch( ct ) {
                    case clause_type::e_none:
                        // RILL_DEBUG( "none" )
                        return install_action::e_not_found;

                    case clause_type::e_cleanup:
                        return install_action::e_cleanup;

                    case clause_type::e_catch:
                        return install_action::e_catch;

                    default:
                        assert( false );
                        return install_action::e_terminate;
                    }
                }
            }


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
                -> install_action
            {
                switch( status ) {
                case handling_action::e_continue:
                    return get_clause_info( context, actions, ex_obj, lbi, lcse, ci );

                case handling_action::e_not_found:
                    return install_action::e_not_found;

                case handling_action::e_terminate:
                    return install_action::e_terminate;

                default:
                    assert( false );
                    return install_action::e_terminate;
                }
            }


            //
            auto install_catch_clause(
                _Unwind_Context* context,
                exception_object const* rill_exception_obj
                )
                -> _Unwind_Reason_Code
            {
                // returns { exception_info: i8*, handler_switch_value: i32 }
                _Unwind_SetGR( context, eh_exception_regno(), reinterpret_cast<_Unwind_Word>( rill_exception_obj ) );
                _Unwind_SetGR( context, eh_selector_regno(), rill_exception_obj->switch_val );

                //
                _Unwind_SetIP( context, reinterpret_cast<_Unwind_Ptr>( rill_exception_obj->landing_pad ) );


                return _URC_INSTALL_CONTEXT;
            }


            //
            auto install_finally_clause(
                _Unwind_Context* context,
                exception_object const* rill_exception_obj
                )
                -> _Unwind_Reason_Code
            {
                // returns { exception_info: i8*, handler_switch_value: i32 }
                _Unwind_SetGR( context, eh_exception_regno(), reinterpret_cast<_Unwind_Word>( rill_exception_obj ) );
                _Unwind_SetGR( context, eh_selector_regno(), 0 );

                //
                _Unwind_SetIP( context, reinterpret_cast<_Unwind_Ptr>( rill_exception_obj->landing_pad ) );

                return _URC_INSTALL_CONTEXT;
            }

            //
            // exception class
            char exception_class_name[] = "Rill++";

            inline auto get_exception_class_name()
                -> char const*
            {
                return exception_class_name;
            }

            _Unwind_Exception_Class exception_class_value
                = make_exception_class( exception_class_name );

        } // namespace exception
    } // namespace runtime
} // namespace rill

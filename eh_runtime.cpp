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


extern "C"
{
    // testtesttest
    std::uint32_t choose_exception()
    {
        std::cout << "type number!( 0 or 1 )" << std::endl;
        std::uint32_t i;
        std::cin >> i;

        return i;
    }


    // Global Constatnt
    rill::type_info test_type_val_0 = { "hello_0", 0 };
    rill::type_info test_type_val_1 = { "hello_1", 1 };



    auto _rill_rt_allocate_exception( std::uint64_t const size )
        -> rill::runtime::exception::exception_object*
    {
        return rill::runtime::exception::allocate_exception( size );
    }


    auto _rill_rt_throw_exception(
        rill::runtime::exception::exception_object* exception_object,
        int test_index /*TODO: add type info, void(*destructor_for_exception_storage)(void*)*/
        )
        -> void
    {
        rill::runtime::exception::throw_exception( exception_object, test_index );
    }


    auto _rill_rt_resume(
        rill::runtime::exception::exception_object* exception_object,
        std::uint32_t const selector
        )
        -> void
    {
        rill::runtime::exception::resume( exception_object, selector );
    }


    auto _rill_rt_free_exception(
        rill::runtime::exception::exception_object* exception_object
        )
        -> void
    {
        rill::runtime::exception::free_exception( exception_object );
    }


    // First, this routine is called in Searching action as Phase1.
    // Secondaly, this routine is called as Phase2.
    auto _rill_rt_ex_personality_tb1(
        std::int32_t version,
        _Unwind_Action actions,
        _Unwind_Exception_Class exception_class,
        _Unwind_Exception* exception_info,
        _Unwind_Context* context
        )
        -> _Unwind_Reason_Code
    {
        using namespace rill::runtime::exception;

        //
        if ( version != 1 ) {
            return _URC_FATAL_PHASE1_ERROR;
        }

        //
        if ( exception_class != exception_class_value ) {
            std::cout << "ABABA" << std::endl;
            return _URC_FOREIGN_EXCEPTION_CAUGHT;
        }

        //
        exception_object* rill_exception_obj = get_rill_exception_object_from_info( exception_info );


        //
        if ( is_search_phase( actions ) ) {
            // get basic information of Language Specific Data Area
            lsda_basic_info lbi;
            if ( !load_lsda( context, lbi ) ) {
                std::cout << "Failed LSDA" << std::endl;
                assert( false );
            }

            // Select call site
            lsda_call_site_entry lcse;
            auto const& h_status
                = lsda_table_based_call_site( context, actions, lbi, lcse );

            //
            clause_info ci;
            auto const ia = select_clause(
                context,
                actions,
                rill_exception_obj,
                h_status,
                lbi,
                lcse,
                ci
                );

            //
            rill_exception_obj->next_install_action = ia;
            rill_exception_obj->landing_pad = lcse.landing_pad;

            //
            switch( ia ) {
            case install_action::e_not_found:
                //
                return _URC_CONTINUE_UNWIND;

            case install_action::e_terminate:
                //
                assert( false );

            case install_action::e_cleanup:
                //
                return _URC_CONTINUE_UNWIND;

            case install_action::e_catch:
                //
                rill_exception_obj->switch_val = ci.clause_index;
                return _URC_HANDLER_FOUND;
            }

        } else {
            // phase 2
            switch( rill_exception_obj->next_install_action ) {
            case install_action::e_cleanup:
                return install_finally_clause( context, rill_exception_obj );

            case install_action::e_catch:
                return install_catch_clause( context, rill_exception_obj );

            default:
                assert( false );
            }
        }

        // unreached
        assert( false );
    }

}

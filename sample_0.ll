; type of this is i8**
@test_type_val_0 = external constant i8*
@test_type_val_1 = external constant i8*


; debug strings
@.str = private unnamed_addr constant [12 x i8] c"function f!\00", align 1
@.str1 = private unnamed_addr constant [9 x i8] c"finished\00", align 1
@.str2 = private unnamed_addr constant [14 x i8] c"caught TYPE_0\00", align 1
@.str3 = private unnamed_addr constant [23 x i8] c"failed to catch TYPE_0\00", align 1
@.str4 = private unnamed_addr constant [14 x i8] c"caught TYPE_1\00", align 1
@.str5 = private unnamed_addr constant [17 x i8] c"exception raised\00", align 1


;
define i32 @f() {
entry:
    call i32 @puts( i8* getelementptr inbounds ([12 x i8]* @.str, i32 0, i32 0) )

    ;; sample
    %in = call i32 @choose_exception()

    ;;
    %tmp_e_obj = alloca i8*
    %tmp_e_sel = alloca i32

    ;; allcoate exception object
    ;; argument has no meaning now
    %exception_obj = call i8* @_rill_rt_allocate_exception(i64 0) nounwind

    ;; throw!
    invoke void @_rill_rt_throw_exception(i8* %exception_obj, i32 %in) noreturn
          to label %the_end unwind label %exception_L2

exception_L2:
    ; i8* is pointer to exception object
    ; i32 is selector value
    %ret = landingpad { i8*, i32 } personality i8* bitcast (i32 (...)* @_rill_rt_ex_personality_tb1 to i8*)
          catch i8* bitcast (i8** @test_type_val_0 to i8*)
          catch i8* bitcast (i8** @test_type_val_1 to i8*)

    call i32 @puts( i8* getelementptr inbounds ([17 x i8]* @.str5, i32 0, i32 0) )

    ;
    %tmp0 = extractvalue { i8*, i32 } %ret, 0
    store i8* %tmp0, i8** %tmp_e_obj
    %tmp1 = extractvalue { i8*, i32 } %ret, 1
    store i32 %tmp1, i32* %tmp_e_sel

    br label %catch_L0

;; try to match @test_type_val_0
catch_L0:
    %selected_0 = load i32* %tmp_e_sel
    %target_sel_0 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @test_type_val_0 to i8*)) nounwind
    %is_matched_0 = icmp eq i32 %selected_0, %target_sel_0

    br i1 %is_matched_0, label %matched_to_t0, label %not_matched_to_t0

matched_to_t0:
    call i32 @puts( i8* getelementptr inbounds ([14 x i8]* @.str2, i32 0, i32 0) )

    br label %free_exception_object


;; try to match @test_type_val_1
not_matched_to_t0:
    call i32 @puts( i8* getelementptr inbounds ([23 x i8]* @.str3, i32 0, i32 0) )

    %selected_1 = load i32* %tmp_e_sel
    %target_sel_1 = call i32 @llvm.eh.typeid.for(i8* bitcast (i8** @test_type_val_1 to i8*)) nounwind
    %is_matched_1 = icmp eq i32 %selected_1, %target_sel_1

    br i1 %is_matched_1, label %matched_to_t1, label %not_matched_to_t1

matched_to_t1:
    call i32 @puts( i8* getelementptr inbounds ([14 x i8]* @.str4, i32 0, i32 0) )

    ;; test, try to resume
;    %eobj_obj_1 = load i8** %tmp_e_obj
;    %eobj_sel_1 = load i32* %tmp_e_sel

;    call void @_rill_rt_resume(i8* %eobj_obj_1, i32 %eobj_sel_1)
    br label %free_exception_object
;    unreachable

not_matched_to_t1:
    br label %last

free_exception_object:
    call i32 @puts( i8* getelementptr inbounds ([9 x i8]* @.str1, i32 0, i32 0) )

    %eobj_obj_last = load i8** %tmp_e_obj
    call void @_rill_rt_free_exception(i8* %eobj_obj_last)

    br label %last

last:
    ret i32 0

the_end:
    unreachable
}


define i32 @main() {
entry:
    br label %reigai

reigai:
    call i32 @f()
    ret i32 0
}



declare i8* @_rill_rt_allocate_exception(i64)
declare void @_rill_rt_resume(i8*, i32)
declare void @_rill_rt_free_exception(i8*)
declare void @_rill_rt_throw_exception(i8*, i32)

declare i32 @_rill_rt_ex_personality_tb1(...)


declare i32 @puts(i8*)

declare i32 @choose_exception()

; Function Attrs: nounwind readnone
declare i32 @llvm.eh.typeid.for(i8*) nounwind readnone

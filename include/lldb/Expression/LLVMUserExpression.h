//===-- LLVMUserExpression.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_LLVMUserExpression_h
#define liblldb_LLVMUserExpression_h

// C Includes
// C++ Includes
#include <string>
#include <map>
#include <vector>

// Project includes
#include "lldb/Expression/UserExpression.h"

namespace lldb_private
{

//----------------------------------------------------------------------
/// @class LLVMUserExpression LLVMUserExpression.h "lldb/Expression/LLVMUserExpression.h"
/// @brief Encapsulates a one-time expression for use in lldb.
///
/// LLDB uses expressions for various purposes, notably to call functions
/// and as a backend for the expr command.  LLVMUserExpression is a virtual base
/// class that encapsulates the objects needed to parse and JIT an expression.
/// The actual parsing part will be provided by the specific implementations
/// of LLVMUserExpression - which will be vended through the appropriate TypeSystem.
//----------------------------------------------------------------------
class LLVMUserExpression : public UserExpression
{
  public:
      LLVMUserExpression(ExecutionContextScope &exe_scope, const char *expr, const char *expr_prefix,
                         lldb::LanguageType language, ResultType desired_type,
                         const EvaluateExpressionOptions &options);
      ~LLVMUserExpression() override;

      lldb::ExpressionResults
      Execute(DiagnosticManager &diagnostic_manager, ExecutionContext &exe_ctx,
              const EvaluateExpressionOptions &options, lldb::UserExpressionSP &shared_ptr_to_me,
              lldb::ExpressionVariableSP &result) override;

      bool
      FinalizeJITExecution(DiagnosticManager &diagnostic_manager, ExecutionContext &exe_ctx,
                           lldb::ExpressionVariableSP &result,
                           lldb::addr_t function_stack_bottom = LLDB_INVALID_ADDRESS,
                           lldb::addr_t function_stack_top = LLDB_INVALID_ADDRESS) override;

      bool
      CanInterpret() override
      {
          return m_can_interpret;
    }

    virtual Materializer *
    GetMaterializer() override
    {
        return m_materializer_ap.get();
    }

    //------------------------------------------------------------------
    /// Return the string that the parser should parse.  Must be a full
    /// translation unit.
    //------------------------------------------------------------------
    const char *
    Text() override
    {
        return m_transformed_text.c_str();
    }

    lldb::ModuleSP GetJITModule() override;

protected:
    virtual void
    ScanContext(ExecutionContext &exe_ctx, lldb_private::Error &err) = 0;

    bool
    PrepareToExecuteJITExpression(DiagnosticManager &diagnostic_manager, ExecutionContext &exe_ctx,
                                  lldb::addr_t &struct_address);

    virtual bool
    AddArguments(ExecutionContext &exe_ctx, std::vector<lldb::addr_t> &args, lldb::addr_t struct_address,
                 DiagnosticManager &diagnostic_manager) = 0;

    lldb::addr_t m_stack_frame_bottom; ///< The bottom of the allocated stack frame.
    lldb::addr_t m_stack_frame_top;    ///< The top of the allocated stack frame.

    bool m_allow_cxx;               ///< True if the language allows C++.
    bool m_allow_objc;              ///< True if the language allows Objective-C.
    std::string m_transformed_text; ///< The text of the expression, as send to the parser

    std::shared_ptr<IRExecutionUnit> m_execution_unit_sp; ///< The execution unit the expression is stored in.
    std::unique_ptr<Materializer> m_materializer_ap;      ///< The materializer to use when running the expression.
    lldb::ModuleWP m_jit_module_wp;
    bool m_enforce_valid_object; ///< True if the expression parser should enforce the presence of a valid class pointer
                                 ///in order to generate the expression as a method.
    bool m_in_cplusplus_method;  ///< True if the expression is compiled as a C++ member function (true if it was parsed
                                 ///when exe_ctx was in a C++ method).
    bool m_in_objectivec_method; ///< True if the expression is compiled as an Objective-C method (true if it was parsed
                                 ///when exe_ctx was in an Objective-C method).
    bool m_in_static_method; ///< True if the expression is compiled as a static (or class) method (currently true if it
                             ///was parsed when exe_ctx was in an Objective-C class method).
    bool m_needs_object_ptr; ///< True if "this" or "self" must be looked up and passed in.  False if the expression
                             ///doesn't really use them and they can be NULL.
    bool m_is_swift_class;   /// FIXME: This really should not be in LLVMUserExpression.
    bool m_const_object;     ///< True if "this" is const.
    Target *m_target;        ///< The target for storing persistent data like types and variables.

    bool m_can_interpret; ///< True if the expression could be evaluated statically; false otherwise.
    lldb::addr_t
        m_materialized_address; ///< The address at which the arguments to the expression have been materialized.
    Materializer::DematerializerSP m_dematerializer_sp; ///< The dematerializer.
};

} // namespace lldb_private
#endif

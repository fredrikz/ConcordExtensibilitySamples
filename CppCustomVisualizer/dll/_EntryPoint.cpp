// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// _CppCustomVisualizerService.cpp : Implementation of CCppCustomVisualizerService

#include "stdafx.h"
#include "_EntryPoint.h"
#include "../TargetApp/entity.h"

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::EvaluateVisualizedExpression(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult** ppResultObject
    )
{
    HRESULT hr;

    // This method is called to visualize a FILETIME variable. Its basic job is to create
    // a DkmEvaluationResult object. A DkmEvaluationResult is the data that backs a row in the
    // watch window -- a name, value, and type, a flag indicating if the item can be expanded, and
    // lots of other additional properties.

    Evaluation::DkmPointerValueHome* pPointerValueHome = Evaluation::DkmPointerValueHome::TryCast(pVisualizedExpression->ValueHome());
    if (pPointerValueHome == nullptr)
    {
        // This sample only handles visualizing in-memory FILETIME structures
        return E_NOTIMPL;
    }

    DkmRootVisualizedExpression* pRootVisualizedExpression = DkmRootVisualizedExpression::TryCast(pVisualizedExpression);
    if (pRootVisualizedExpression == nullptr)
    {
        // This sample doesn't provide child evaluation results, so only root expressions are expected
        return E_NOTIMPL;
    }

    // Read the FILETIME value from the target process
    DkmProcess* pTargetProcess = pVisualizedExpression->RuntimeInstance()->Process();
    entity value;
    hr = pTargetProcess->ReadMemory(pPointerValueHome->Address(), DkmReadMemoryFlags::None, &value, sizeof(value), nullptr);
    if (FAILED(hr))
    {
        // If the bytes of the value cannot be read from the target process, just fall back to the default visualization
        return E_NOTIMPL;
    }

    CString strValue;
    strValue = _T("<unable to resolve entity>");

    const UINT64 entity_address = pPointerValueHome->Address();
    CComPtr<DkmEvaluationResult> pEEEvaluationResultOther;
    CString expr;
    expr.Format(L"entity_manager_get_visualizer_data(s_entity_manager,(const "
                L"struct entity*)0x%08x%08x)",
                static_cast<DWORD>(entity_address >> 32),
                static_cast<DWORD>(entity_address));
    hr = EvaluateOtherExpression(pVisualizedExpression, (LPCTSTR)expr,
                                 &pEEEvaluationResultOther);
    if (FAILED(hr)) {
      return hr;
    }

    if ( pEEEvaluationResultOther->TagValue() == DkmEvaluationResult::Tag::SuccessResult ) {
      DkmSuccessEvaluationResult* success = DkmSuccessEvaluationResult::TryCast( pEEEvaluationResultOther );
      DkmString* success_value = success->Value();
      strValue = success_value->Value();
    }

    CComPtr<DkmString> expr_result_other;
    pEEEvaluationResultOther->GetUnderlyingString( &expr_result_other );

    // Format this FILETIME as a string
    CString strEditableValue;

    // If we are formatting a pointer, we want to also show the address of the pointer
    if (pRootVisualizedExpression->Type() != nullptr && wcschr(pRootVisualizedExpression->Type()->Value(), '*') != nullptr)
    {
        // Make the editable value just the pointer string
        UINT64 address = pPointerValueHome->Address();
        if ((pTargetProcess->SystemInformation()->Flags()& DefaultPort::DkmSystemInformationFlags::Is64Bit) != 0)
        {
            strEditableValue.Format(L"0x%08x%08x", static_cast<DWORD>(address >> 32), static_cast<DWORD>(address));
        }
        else
        {
            strEditableValue.Format(L"0x%08x", static_cast<DWORD>(address));
        }

        // Prefix the value with the address
        CString strValueWithAddress;
        strValueWithAddress.Format(L"%s {%s}", static_cast<LPCWSTR>(strEditableValue), static_cast<LPCWSTR>(strValue));
        strValue = strValueWithAddress;
    }

    CComPtr<DkmString> pValue;
    hr = DkmString::Create(DkmSourceString(strValue), &pValue);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<DkmString> pEditableValue;
    hr = DkmString::Create(strEditableValue, &pEditableValue);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<DkmDataAddress> pAddress;
    hr = DkmDataAddress::Create(pVisualizedExpression->RuntimeInstance(), pPointerValueHome->Address(), nullptr, &pAddress);
    if (FAILED(hr))
    {
        return hr;
    }

    DkmEvaluationResultFlags_t resultFlags = DkmEvaluationResultFlags::Expandable;
    if (strEditableValue.IsEmpty())
    {
        // We only allow editting pointers, so mark non-pointers as read-only
        resultFlags |= DkmEvaluationResultFlags::ReadOnly;
    }

    CComPtr<DkmSuccessEvaluationResult> pSuccessEvaluationResult;
    hr = DkmSuccessEvaluationResult::Create(
        pVisualizedExpression->InspectionContext(),
        pVisualizedExpression->StackFrame(),
        pRootVisualizedExpression->Name(),
        pRootVisualizedExpression->FullName(),
        resultFlags,
        pValue,
        pEditableValue,
        pRootVisualizedExpression->Type(),
        DkmEvaluationResultCategory::Class,
        DkmEvaluationResultAccessType::None,
        DkmEvaluationResultStorageType::None,
        DkmEvaluationResultTypeModifierFlags::None,
        pAddress,
        nullptr,
        (DkmReadOnlyCollection<DkmModuleInstance*>*)nullptr,
        // This sample doesn't need to store any state associated with this evaluation result, so we
        // pass `DkmDataItem::Null()` here. A more complicated extension which had associated
        // state such as an extension which took over expansion of evaluation results would likely
        // create an instance of the extension's data item class and pass the instance here.
        // More information: https://github.com/Microsoft/ConcordExtensibilitySamples/wiki/Data-Container-API
        DkmDataItem::Null(),
        &pSuccessEvaluationResult
        );
    if (FAILED(hr))
    {
        return hr;
    }

    *ppResultObject = pSuccessEvaluationResult.Detach();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::UseDefaultEvaluationBehavior(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _Out_ bool* pUseDefaultEvaluationBehavior,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult** ppDefaultEvaluationResult
    )
{
    HRESULT hr;

    // This method is called by the expression evaluator when a visualized expression's children are
    // being expanded, or the value is being set. We just want to delegate this back to the C++ EE.
    // So we need to set `*pUseDefaultEvaluationBehavior` to true and return the evaluation result which would
    // be created if this custom visualizer didn't exist.
    //
    // NOTE: If this custom visualizer supported underlying strings (no DkmEvaluationResultFlags::RawString),
    // this method would also be called when that is requested.

    DkmRootVisualizedExpression* pRootVisualizedExpression = DkmRootVisualizedExpression::TryCast(pVisualizedExpression);
    if (pRootVisualizedExpression == nullptr)
    {
        // This sample doesn't provide child evaluation results, so only root expressions are expected
        return E_NOTIMPL;
    }

    DkmInspectionContext* pParentInspectionContext = pVisualizedExpression->InspectionContext();

    CAutoDkmClosePtr<DkmLanguageExpression> pLanguageExpression;
    hr = DkmLanguageExpression::Create(
        pParentInspectionContext->Language(),
        DkmEvaluationFlags::TreatAsExpression,
        pRootVisualizedExpression->FullName(),
        DkmDataItem::Null(),
        &pLanguageExpression
        );
    if (FAILED(hr))
    {
        return hr;
    }

    // Create a new inspection context with 'DkmEvaluationFlags::ShowValueRaw' set. This is important because
    // the result of the expression is a FILETIME, and we don't want our visualizer to be invoked again. This
    // step would be unnecessary if we were evaluating a different expression that resulted in a type which
    // we didn't visualize.
    CComPtr<DkmInspectionContext> pInspectionContext;
    // If we are running in VS 16 or newer, use this overload...
    hr = DkmInspectionContext::Create(
        pParentInspectionContext->InspectionSession(),
        pParentInspectionContext->RuntimeInstance(),
        pParentInspectionContext->Thread(), pParentInspectionContext->Timeout(),
        DkmEvaluationFlags::TreatAsExpression |
            DkmEvaluationFlags::ShowValueRaw,
        pParentInspectionContext->FuncEvalFlags(),
        pParentInspectionContext->Radix(), pParentInspectionContext->Language(),
        pParentInspectionContext->ReturnValue(),
        (Evaluation::DkmCompiledVisualizationData *)nullptr,
        Evaluation::DkmCompiledVisualizationDataPriority::None,
        pParentInspectionContext->ReturnValues(),
        pParentInspectionContext->SymbolsConnection(), &pInspectionContext);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<DkmEvaluationResult> pEEEvaluationResult;
    hr = pVisualizedExpression->EvaluateExpressionCallback(
        pInspectionContext,
        pLanguageExpression,
        pVisualizedExpression->StackFrame(),
        &pEEEvaluationResult
        );
    if (FAILED(hr))
    {
        return hr;
    }

    *ppDefaultEvaluationResult = pEEEvaluationResult.Detach();
    *pUseDefaultEvaluationBehavior = true;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::EvaluateOtherExpression(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    const wchar_t* expr,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult** ppDefaultEvaluationResult
    )
{
    HRESULT hr;
    CComPtr<DkmString> pValue;
    hr = DkmString::Create(DkmSourceString(expr), &pValue);
    if (FAILED(hr))
    {
        return hr;
    }

    // This method is called by the expression evaluator when a visualized expression's children are
    // being expanded, or the value is being set. We just want to delegate this back to the C++ EE.
    // So we need to set `*pUseDefaultEvaluationBehavior` to true and return the evaluation result which would
    // be created if this custom visualizer didn't exist.
    //
    // NOTE: If this custom visualizer supported underlying strings (no DkmEvaluationResultFlags::RawString),
    // this method would also be called when that is requested.

    DkmRootVisualizedExpression* pRootVisualizedExpression = DkmRootVisualizedExpression::TryCast(pVisualizedExpression);
    if (pRootVisualizedExpression == nullptr)
    {
        // This sample doesn't provide child evaluation results, so only root expressions are expected
        return E_NOTIMPL;
    }

#define FLAGS                                                                  \
  DkmEvaluationFlags::ForceRealFuncEval | DkmEvaluationFlags::ForceEvaluationNow
    DkmInspectionContext* pParentInspectionContext = pVisualizedExpression->InspectionContext();

    CAutoDkmClosePtr<DkmLanguageExpression> pLanguageExpression;
    hr = DkmLanguageExpression::Create(
        pParentInspectionContext->Language(),
        FLAGS,
        pValue,
        DkmDataItem::Null(),
        &pLanguageExpression
        );

    if (FAILED(hr))
    {
        return hr;
    }

    // Create a new inspection context with 'DkmEvaluationFlags::ShowValueRaw' set. This is important because
    // the result of the expression is a FILETIME, and we don't want our visualizer to be invoked again. This
    // step would be unnecessary if we were evaluating a different expression that resulted in a type which
    // we didn't visualize.
    CComPtr<DkmInspectionContext> pInspectionContext;
    // If we are running in VS 16 or newer, use this overload...
    hr = DkmInspectionContext::Create(
        pParentInspectionContext->InspectionSession(),
        pParentInspectionContext->RuntimeInstance(),
        pParentInspectionContext->Thread(), pParentInspectionContext->Timeout(),
        FLAGS,
        pParentInspectionContext->FuncEvalFlags(),
        pParentInspectionContext->Radix(), pParentInspectionContext->Language(),
        pParentInspectionContext->ReturnValue(),
        (Evaluation::DkmCompiledVisualizationData *)nullptr,
        Evaluation::DkmCompiledVisualizationDataPriority::None,
        pParentInspectionContext->ReturnValues(),
        pParentInspectionContext->SymbolsConnection(), &pInspectionContext);
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr<DkmEvaluationResult> pEEEvaluationResult;
    hr = pVisualizedExpression->EvaluateExpressionCallback(
        pInspectionContext,
        pLanguageExpression,
        pVisualizedExpression->StackFrame(),
        &pEEEvaluationResult
        );
    if (FAILED(hr))
    {
        return hr;
    }

    *ppDefaultEvaluationResult = pEEEvaluationResult.Detach();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetChildren(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _In_ UINT32 InitialRequestSize,
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _Out_ DkmArray<Evaluation::DkmChildVisualizedExpression*>* pInitialChildren,
    _Deref_out_ Evaluation::DkmEvaluationResultEnumContext** ppEnumContext
    )
{
    // This sample delegates expansion to the C++ EE, so this method doesn't need to be implemented
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetItems(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _In_ Evaluation::DkmEvaluationResultEnumContext* pEnumContext,
    _In_ UINT32 StartIndex,
    _In_ UINT32 Count,
    _Out_ DkmArray<Evaluation::DkmChildVisualizedExpression*>* pItems
    )
{
    // This sample delegates expansion to the C++ EE, so this method doesn't need to be implemented
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::SetValueAsString(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _In_ DkmString* pValue,
    _In_ UINT32 Timeout,
    _Deref_out_opt_ DkmString** ppErrorText
    )
{
    // This sample delegates setting values to the C++ EE, so this method doesn't need to be implemented
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetUnderlyingString(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _Deref_out_opt_ DkmString** ppStringValue
    )
{
    // FILETIME doesn't have an underlying string (no DkmEvaluationResultFlags::RawString), so this method
    // doesn't need to be implemented
    return E_NOTIMPL;
}

void CCppCustomVisualizerService::entity_to_text(const entity &e,
                                                 CString &text) {
  text = _T("<unable to resolve entity>");
  //LPWSTR pBuffer = text.GetBuffer(allocLength);
  //text.ReleaseBuffer();
}

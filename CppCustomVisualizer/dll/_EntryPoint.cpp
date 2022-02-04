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

    DkmRootVisualizedExpression* pRootVisualizedExpression = nullptr;
    DkmVisualizedExpression* pTestVisualizedExpression = pVisualizedExpression;
    for (;;) {
      pRootVisualizedExpression = DkmRootVisualizedExpression::TryCast(pTestVisualizedExpression);
      if (pRootVisualizedExpression)
      {
        break;
      }

      DkmChildVisualizedExpression* pChildVisualizedExpression = DkmChildVisualizedExpression::TryCast( pTestVisualizedExpression );

      if ( pChildVisualizedExpression ) {
        pTestVisualizedExpression = pChildVisualizedExpression->Parent();
      }
      else {
        return E_NOTIMPL;
      }
    }

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
    (void)pVisualizedExpression;
    (void)ppDefaultEvaluationResult;
    *pUseDefaultEvaluationBehavior = false;
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

    CComPtr<DkmInspectionContext> pInspectionContext;
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

// NOTE: https://github.com/microsoft/cppwinrt/blob/master/natvis/object_visualizer.cpp has a good sample of this
HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetChildren(
    _In_ Evaluation::DkmVisualizedExpression* pVisualizedExpression,
    _In_ UINT32 InitialRequestSize,
    _In_ Evaluation::DkmInspectionContext* pInspectionContext,
    _Out_ DkmArray<Evaluation::DkmChildVisualizedExpression*>* pInitialChildren,
    _Deref_out_ Evaluation::DkmEvaluationResultEnumContext** ppEnumContext
    )
{
  HRESULT hr;
    Evaluation::DkmPointerValueHome* pPointerValueHome = Evaluation::DkmPointerValueHome::TryCast(pVisualizedExpression->ValueHome());
    if (pPointerValueHome == nullptr)
    {
        // This sample only handles visualizing in-memory FILETIME structures
        return E_NOTIMPL;
    }
    DkmProcess* pTargetProcess = pVisualizedExpression->RuntimeInstance()->Process();
    entity value;
    hr = pTargetProcess->ReadMemory(pPointerValueHome->Address(), DkmReadMemoryFlags::None, &value, sizeof(value), nullptr);
    if (FAILED(hr))
    {
        // If the bytes of the value cannot be read from the target process, just fall back to the default visualization
        return E_NOTIMPL;
    }

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

      const wchar_t num = *(wcschr( success_value->Value(), L'"' ) + 1);

      UINT32 max_out = InitialRequestSize;
      if ( num >= L'0' && num <= L'9' ) {
        max_out = UINT32(num - L'0');
      }
      else {
        max_out = 0;
      }

      CComPtr<DkmEvaluationResultEnumContext> pEnumContext;
      hr = DkmEvaluationResultEnumContext::Create(
          max_out, pVisualizedExpression->StackFrame(), pInspectionContext,
          DkmDataItem::Null(), &pEnumContext);
      DkmAllocArray(0, pInitialChildren);
      *ppEnumContext = pEnumContext.Detach();

      return S_OK;
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetItems(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _In_ Evaluation::DkmEvaluationResultEnumContext *pEnumContext,
    _In_ UINT32 StartIndex, _In_ UINT32 Count,
    _Out_ DkmArray<Evaluation::DkmChildVisualizedExpression *> *pItems) {
  HRESULT hr;
  hr = DkmAllocArray(Count, pItems);
  if (FAILED(hr)) {
    return hr;
  }

  Evaluation::DkmPointerValueHome *pPointerValueHome =
      Evaluation::DkmPointerValueHome::TryCast(
          pVisualizedExpression->ValueHome());
  for (UINT32 i = StartIndex; i < StartIndex + Count; ++i) {
    DkmChildVisualizedExpression **children = pItems->Members;
    DkmChildVisualizedExpression **pChild = &children[i];

    CComPtr<DkmEvaluationResult> pEEEvaluationResultOther;
    CString expr;
    expr.Format(L"\"Comp %i\"", i);
    hr = EvaluateOtherExpression(pVisualizedExpression, (LPCTSTR)expr,
                                 &pEEEvaluationResultOther);

    if ( pEEEvaluationResultOther->TagValue() == DkmEvaluationResult::Tag::SuccessResult ) {
      DkmSuccessEvaluationResult* success = DkmSuccessEvaluationResult::TryCast( pEEEvaluationResultOther );
      int i = 0;
      ++i;
    }

    DkmChildVisualizedExpression::Create(
        pVisualizedExpression->InspectionContext(),
        pVisualizedExpression->VisualizerId(),
        pVisualizedExpression->SourceId(), pVisualizedExpression->StackFrame(),
        pPointerValueHome, pEEEvaluationResultOther.Detach(),
        pVisualizedExpression, i, DkmDataItem::Null(), pChild );
  }

  return S_OK;
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


// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// _CppCustomVisualizerService.cpp : Implementation of CCppCustomVisualizerService

#include "stdafx.h"
#include "_EntryPoint.h"
#include "../TargetApp/entity.h"

using namespace std;

HRESULT STDMETHODCALLTYPE
CCppCustomVisualizerService::EvaluateVisualizedExpression(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult **ppResultObject) {
  HRESULT hr;

  Evaluation::DkmPointerValueHome *pPointerValueHome =
      Evaluation::DkmPointerValueHome::TryCast(
          pVisualizedExpression->ValueHome());
  if (pPointerValueHome == nullptr) {
    // This sample only handles visualizing in-memory FILETIME structures
    return E_NOTIMPL;
  }

  DkmRootVisualizedExpression *pRootVisualizedExpression = nullptr;
  DkmVisualizedExpression *pTestVisualizedExpression = pVisualizedExpression;
  for (;;) {
    pRootVisualizedExpression =
        DkmRootVisualizedExpression::TryCast(pTestVisualizedExpression);
    if (pRootVisualizedExpression) {
      break;
    }

    DkmChildVisualizedExpression *pChildVisualizedExpression =
        DkmChildVisualizedExpression::TryCast(pTestVisualizedExpression);

    if (pChildVisualizedExpression) {
      pTestVisualizedExpression = pChildVisualizedExpression->Parent();
    } else {
      return E_NOTIMPL;
    }
  }

  DkmProcess *pTargetProcess =
      pVisualizedExpression->RuntimeInstance()->Process();
  const UINT64 entity_address = pPointerValueHome->Address();
  entity value;
  hr = pTargetProcess->ReadMemory(entity_address, DkmReadMemoryFlags::None,
                                  &value, sizeof(value), nullptr);
  if (FAILED(hr)) {
    // If the bytes of the value cannot be read from the target process, just
    // fall back to the default visualization
    return E_NOTIMPL;
  }

  CString strValue;
  strValue = _T("<unable to resolve entity>");

  CComPtr<DkmEvaluationResult> pEEEvaluationResultOther;
  CString expr;
  expr.Format(L"*((struct entity*)0x%08x%08x)",
              static_cast<DWORD>(entity_address >> 32),
              static_cast<DWORD>(entity_address));
  hr = EvaluateOtherExpression(pVisualizedExpression,
                               DkmEvaluationFlags::ShowValueRaw, (LPCTSTR)expr,
                               &pEEEvaluationResultOther);
  if (FAILED(hr)) {
    return hr;
  }

  if (pEEEvaluationResultOther->TagValue() ==
      DkmEvaluationResult::Tag::SuccessResult) {
    DkmSuccessEvaluationResult *success =
        DkmSuccessEvaluationResult::TryCast(pEEEvaluationResultOther);

    CComPtr<DkmSuccessEvaluationResult> pNamedEEEvaluationResultOther;
    hr = DkmSuccessEvaluationResult::Create(
        success->InspectionContext(), success->StackFrame(),
        pRootVisualizedExpression->Name(),
        pRootVisualizedExpression->FullName(), success->Flags(),
        success->Value(), success->EditableValue(), success->Type(),
        success->Category(), success->Access(), success->StorageType(),
        success->TypeModifierFlags(), success->Address(),
        success->CustomUIVisualizers(), success->ExternalModules(),
        DkmDataItem::Null(), &pNamedEEEvaluationResultOther);

    pEEEvaluationResultOther = pNamedEEEvaluationResultOther.Detach();
  }

  *ppResultObject = pEEEvaluationResultOther.Detach();
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CCppCustomVisualizerService::UseDefaultEvaluationBehavior(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _Out_ bool *pUseDefaultEvaluationBehavior,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult *
        *ppDefaultEvaluationResult) {
  (void)pVisualizedExpression;
  (void)ppDefaultEvaluationResult;
  *pUseDefaultEvaluationBehavior = false;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::EvaluateOtherExpression(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _In_ DkmEvaluationFlags_t extra_flags, _In_ const wchar_t *expr,
    _Deref_out_opt_ Evaluation::DkmEvaluationResult *
        *ppDefaultEvaluationResult) {
  HRESULT hr;
  CComPtr<DkmString> pValue;
  hr = DkmString::Create(DkmSourceString(expr), &pValue);
  if (FAILED(hr)) {
    return hr;
  }

  const DkmEvaluationFlags_t FLAGS = DkmEvaluationFlags::ForceRealFuncEval |
                                     DkmEvaluationFlags::ForceEvaluationNow |
                                     DkmEvaluationFlags::NoRawView |
                                     extra_flags;
  DkmInspectionContext *pParentInspectionContext =
      pVisualizedExpression->InspectionContext();

  CAutoDkmClosePtr<DkmLanguageExpression> pLanguageExpression;
  hr = DkmLanguageExpression::Create(pParentInspectionContext->Language(),
                                     FLAGS, pValue, DkmDataItem::Null(),
                                     &pLanguageExpression);

  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<DkmInspectionContext> pInspectionContext;
  hr = DkmInspectionContext::Create(
      pParentInspectionContext->InspectionSession(),
      pParentInspectionContext->RuntimeInstance(),
      pParentInspectionContext->Thread(), pParentInspectionContext->Timeout(),
      FLAGS, pParentInspectionContext->FuncEvalFlags(),
      pParentInspectionContext->Radix(), pParentInspectionContext->Language(),
      pParentInspectionContext->ReturnValue(),
      (Evaluation::DkmCompiledVisualizationData *)nullptr,
      Evaluation::DkmCompiledVisualizationDataPriority::None,
      pParentInspectionContext->ReturnValues(),
      pParentInspectionContext->SymbolsConnection(), &pInspectionContext);
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<DkmEvaluationResult> pEEEvaluationResult;
  hr = pVisualizedExpression->EvaluateExpressionCallback(
      pInspectionContext, pLanguageExpression,
      pVisualizedExpression->StackFrame(), &pEEEvaluationResult);
  if (FAILED(hr)) {
    return hr;
  }

  *ppDefaultEvaluationResult = pEEEvaluationResult.Detach();
  return S_OK;
}

HRESULT
CCppCustomVisualizerService::evaluate_entity(
    Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    Evaluation::DkmPointerValueHome *pPointerValueHome,
    vector<name_and_expr> &out) {
  out.clear();
  HRESULT hr;
  const UINT64 entity_address = pPointerValueHome->Address();
  CComPtr<DkmEvaluationResult> pEEEvaluationResultOther;
  CString expr;
  expr.Format(L"entity_manager_get_visualizer_data(s_entity_manager,(const "
              L"struct entity*)0x%08x%08x)",
              static_cast<DWORD>(entity_address >> 32),
              static_cast<DWORD>(entity_address));
  hr = EvaluateOtherExpression(pVisualizedExpression, DkmEvaluationFlags::None,
                               (LPCTSTR)expr, &pEEEvaluationResultOther);
  if (FAILED(hr)) {
    return hr;
  }

  if (pEEEvaluationResultOther->TagValue() !=
      DkmEvaluationResult::Tag::SuccessResult) {
    return E_NOTIMPL;
  }

  DkmSuccessEvaluationResult *success =
      DkmSuccessEvaluationResult::TryCast(pEEEvaluationResultOther);
  DkmString *success_value = success->Value();
  const wchar_t *val = success_value->Value();
  val = wcschr(val, L'"');
  ++val;
  const wchar_t *res = val;
  wstring name;

  while ((res = wcspbrk(val, L"',\0"))) {
    if (*res == L'\'') {
      ++res;
      const wchar_t *end = wcschr(res, L'\'');
      //'end' must be valid
      name.assign(res, end);
      res = end + 1;
      val = res;
      continue;
    }
    if (res != val) {
      out.push_back({name, wstring(val, res)});
      name.clear();
    }
    if (*res == L'\0') {
      break;
    }
    val = res + 1;
  }

  return S_OK;
}

// NOTE:
// https://github.com/microsoft/cppwinrt/blob/master/natvis/object_visualizer.cpp
// has a good sample of this
HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetChildren(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _In_ UINT32 InitialRequestSize,
    _In_ Evaluation::DkmInspectionContext *pInspectionContext,
    _Out_ DkmArray<Evaluation::DkmChildVisualizedExpression *>
        *pInitialChildren,
    _Deref_out_ Evaluation::DkmEvaluationResultEnumContext **ppEnumContext) {
  HRESULT hr;
  Evaluation::DkmPointerValueHome *pPointerValueHome =
      Evaluation::DkmPointerValueHome::TryCast(
          pVisualizedExpression->ValueHome());
  if (pPointerValueHome == nullptr) {
    // This sample only handles visualizing in-memory FILETIME structures
    return E_NOTIMPL;
  }
  DkmProcess *pTargetProcess =
      pVisualizedExpression->RuntimeInstance()->Process();
  entity value;
  hr = pTargetProcess->ReadMemory(pPointerValueHome->Address(),
                                  DkmReadMemoryFlags::None, &value,
                                  sizeof(value), nullptr);
  if (FAILED(hr)) {
    // If the bytes of the value cannot be read from the target process, just
    // fall back to the default visualization
    return E_NOTIMPL;
  }

  vector<name_and_expr> comps;
  hr = evaluate_entity(pVisualizedExpression, pPointerValueHome, comps);
  if (FAILED(hr)) {
    return hr;
  }

  (void)InitialRequestSize;
  CComPtr<DkmEvaluationResultEnumContext> pEnumContext;
  hr = DkmEvaluationResultEnumContext::Create(
      UINT32(comps.size()), pVisualizedExpression->StackFrame(),
      pInspectionContext, DkmDataItem::Null(), &pEnumContext);
  DkmAllocArray(0, pInitialChildren);
  *ppEnumContext = pEnumContext.Detach();

  return S_OK;
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

  vector<name_and_expr> comps;
  hr = evaluate_entity(pVisualizedExpression, pPointerValueHome, comps);
  if (FAILED(hr)) {
    return hr;
  }

  for (UINT32 i = StartIndex; i < StartIndex + Count; ++i) {
    const name_and_expr &expr = comps[i];
    DkmChildVisualizedExpression **children = pItems->Members;
    DkmChildVisualizedExpression **pChild = &children[i];

    CComPtr<DkmEvaluationResult> pEEEvaluationResultOther;
    hr =
        EvaluateOtherExpression(pVisualizedExpression, DkmEvaluationFlags::None,
                                expr._expr.c_str(), &pEEEvaluationResultOther);
    CComPtr<DkmString> pName;
    hr = DkmString::Create(DkmSourceString(expr._name.c_str()), &pName);
    if (FAILED(hr)) {
      return hr;
    }

    /*
    if (pEEEvaluationResultOther->TagValue() ==
            DkmEvaluationResult::Tag::SuccessResult &&
        !expr._name.empty()) {
      DkmSuccessEvaluationResult *success =
          DkmSuccessEvaluationResult::TryCast(pEEEvaluationResultOther);

      CComPtr<DkmSuccessEvaluationResult> pNamedEEEvaluationResultOther;

      // Custom name
      hr = DkmSuccessEvaluationResult::Create(
          success->InspectionContext(), success->StackFrame(), success->Name(),
          success->FullName(), success->Flags(), success->Value(),
          success->EditableValue(), success->Type(), success->Category(),
          success->Access(), success->StorageType(),
          success->TypeModifierFlags(), success->Address(),
          success->CustomUIVisualizers(), success->ExternalModules(),
          DkmDataItem::Null(), &pNamedEEEvaluationResultOther);

      pEEEvaluationResultOther = pNamedEEEvaluationResultOther.Detach();
    }
    */

    DkmChildVisualizedExpression::Create(
        pVisualizedExpression->InspectionContext(),
        pVisualizedExpression->VisualizerId(),
        pVisualizedExpression->SourceId(), pVisualizedExpression->StackFrame(),
        pPointerValueHome, pEEEvaluationResultOther.Detach(),
        pVisualizedExpression, i, DkmDataItem::Null(), pChild);
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::SetValueAsString(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _In_ DkmString *pValue, _In_ UINT32 Timeout,
    _Deref_out_opt_ DkmString **ppErrorText) {
  // This sample delegates setting values to the C++ EE, so this method doesn't
  // need to be implemented
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCppCustomVisualizerService::GetUnderlyingString(
    _In_ Evaluation::DkmVisualizedExpression *pVisualizedExpression,
    _Deref_out_opt_ DkmString **ppStringValue) {
  // FILETIME doesn't have an underlying string (no
  // DkmEvaluationResultFlags::RawString), so this method doesn't need to be
  // implemented
  return E_NOTIMPL;
}

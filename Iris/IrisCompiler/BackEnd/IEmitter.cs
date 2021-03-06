// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using IrisCompiler.FrontEnd;
using System;
using System.Collections.Generic;

namespace IrisCompiler.BackEnd
{
    /// <summary>
    /// IEmitter is the interface for creating backends for the Iris compiler.  The output code is
    /// generated by calling various methods on an IEmitter implementation.
    /// </summary>
    public interface IEmitter : IDisposable
    {
        void Flush();

        void BeginProgram(string name, IEnumerable<string> references);
        void DeclareGlobal(Symbol symbol);
        void EndProgram();

        void BeginMethod(string name, IrisType returnType, Variable[] parameters, Variable[] locals, bool entryPoint);
        void EmitMethodLanguageInfo();
        void EmitLineInfo(SourceRange range, string filePath);
        void InitArray(Symbol arraySymbol, SubRange subRange);
        void EndMethod();

        void PushString(string s);
        void PushIntConst(int i);
        void PushArgument(int i);
        void PushArgumentAddress(int i);
        void StoreArgument(int i);
        void PushLocal(int i);
        void PushLocalAddress(int i);
        void StoreLocal(int i);
        void PushGlobal(Symbol symbol);
        void PushGlobalAddress(Symbol symbol);
        void StoreGlobal(Symbol symbol);

        void Dup();
        void Pop();
        void NoOp();

        void LoadElement(IrisType elementType);
        void LoadElementAddress(IrisType elementType);
        void StoreElement(IrisType elementType);

        void Label(int i);
        void Goto(int i);
        void BranchCondition(Operator condition, int i);
        void BranchTrue(int i);
        void BranchFalse(int i);
        void Call(Symbol methodSymbol);

        void Operator(Operator opr);
        void Load(IrisType type);
        void Store(IrisType type);
    }
}

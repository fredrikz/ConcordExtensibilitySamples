// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using Microsoft.VisualStudio.Debugger.Clr;
using System.Collections.Generic;

namespace IrisExtension
{
    /// <summary>
    /// Equality comparer to allow us to use DkmClrInstructionAddress as a dictionary key.
    /// </summary>
    internal class AddressComparer : IEqualityComparer<DkmClrInstructionAddress>
    {
        public static readonly AddressComparer Instance = new AddressComparer();

        /// <summary>
        /// Singleton .ctor.  Use AddressComparer.Instance to get the instance.
        /// </summary>
        private AddressComparer()
        {
        }

        public bool Equals(DkmClrInstructionAddress address1, DkmClrInstructionAddress address2)
        {
            if (address1.ILOffset != address2.ILOffset)
                return false;

            if (address1.MethodId != address2.MethodId)
                return false;
            
            // Also compare the module.  Reference equality works for comparing module instances.
            return object.ReferenceEquals(address1.ModuleInstance, address2.ModuleInstance);
        }

        public int GetHashCode(DkmClrInstructionAddress obj)
        {
            return (int)obj.ILOffset ^ obj.MethodId.GetHashCode();
        }
    }
}

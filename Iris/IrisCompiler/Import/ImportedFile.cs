// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

using System;
using System.IO;
using System.Reflection.PortableExecutable;

namespace IrisCompiler.Import
{
    /// <summary>
    /// This class handles reading and ownership of a PE file that is imported into the compiler
    /// </summary>
    internal sealed class ImportedFile : IDisposable
    {
        private PEReader _peReader;
        private Stream _peStream;

        private ImportedFile(Stream peStream)
        {
            _peReader = new PEReader(peStream);
            _peStream = peStream;
        }

        public ImportedModule Module
        {
            get
            {
                return new ImportedModule(_peReader.GetMetadata());
            }
        }

        public void Dispose()
        {
            _peReader.Dispose();
            _peStream.Dispose();
        }

        public static ImportedFile Create(string path)
        {
            Stream input = File.OpenRead(path);
            return new ImportedFile(input);
        }
    }
}

/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInputStream.h"

#include "PdfOutputStream.h"

using namespace std;
using namespace mm;

constexpr size_t BUFFER_SIZE = 4096;

InputStream::InputStream() { }

InputStream::~InputStream() { }

void InputStream::Read(char* buffer, size_t size)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid buffer");

    checkRead();
    bool eof;
    size_t read = 0;
    do
    {
        read += readBuffer(buffer + read, size - read, eof);
        if (read == size)
            return;

    } while (!eof);

    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Unexpected EOF when reading from stream");
}

char InputStream::ReadChar()
{
    checkRead();
    char ch;
    if (!readChar(ch))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Reached EOF while reading from the stream");

    return ch;
}

bool InputStream::Read(char& ch)
{
    checkRead();
    return readChar(ch);
}

size_t InputStream::Read(char* buffer, size_t size, bool& eof)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid buffer");

    checkRead();
    size_t read = 0;
    do
    {
        read += readBuffer(buffer + read, size - read, eof);
        if (read == size)
            return read;

    } while (!eof);

    return read;
}

void InputStream::CopyTo(OutputStream& stream)
{
    size_t read = 0;
    char buffer[BUFFER_SIZE];

    bool eof;
    do
    {
        read = readBuffer(buffer, BUFFER_SIZE, eof);
        stream.Write(buffer, read);
    } while (!eof);

    stream.Flush();
}

void InputStream::CopyTo(OutputStream& stream, size_t size)
{
    size_t read = 0;
    char buffer[BUFFER_SIZE];

    bool eof;
    do
    {
        read = readBuffer(buffer, std::min(BUFFER_SIZE, size), eof);
        size -= read;
        stream.Write(buffer, read);
    } while (size > 0 && !eof);

    stream.Flush();
}

bool InputStream::readChar(char& ch)
{
    ch = '\0';
    bool eof;
    do
    {
        if (readBuffer(&ch, 1, eof) == 1)
            return true;

    } while (!eof);

    return  false;
}

void InputStream::checkRead() const
{
    // Do nothing
}

size_t InputStream::ReadBuffer(InputStream& stream, char* buffer, size_t size, bool& eof)
{
    return stream.readBuffer(buffer, size, eof);
}

bool InputStream::ReadChar(InputStream& stream, char& ch)
{
    return stream.readChar(ch);
}

/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInfo.h"

#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#define PRODUCER_STRING "pdfmm - https://github.com/pdfmm/pdfmm"

using namespace std;
using namespace mm;

PdfInfo::PdfInfo(PdfObject& obj)
    : PdfDictionaryElement(obj)
{
}

PdfInfo::PdfInfo(PdfObject& obj, PdfInfoInitial initial)
    : PdfInfo(obj)
{
    init(initial);
}

void PdfInfo::init(PdfInfoInitial initial)
{
    PdfDate date;
    PdfString str = date.ToString();

    if ((initial & PdfInfoInitial::WriteCreationTime) == PdfInfoInitial::WriteCreationTime)
        this->GetObject().GetDictionary().AddKey("CreationDate", str);

    if ((initial & PdfInfoInitial::WriteModificationTime) == PdfInfoInitial::WriteModificationTime)
        this->GetObject().GetDictionary().AddKey("ModDate", str);

    if ((initial & PdfInfoInitial::WriteProducer) == PdfInfoInitial::WriteProducer)
        this->GetObject().GetDictionary().AddKey("Producer", PdfString(PRODUCER_STRING));
}

nullable<PdfString> PdfInfo::getStringFromInfoDict(const PdfName& name) const
{
    auto obj = this->GetObject().GetDictionary().FindKey(name);
    return obj != nullptr && obj->IsString() ? nullable<PdfString>(obj->GetString()) : nullptr;
}

const PdfName& PdfInfo::getNameFromInfoDict(const PdfName& name) const
{
    auto obj = this->GetObject().GetDictionary().FindKey(name);
    return  obj != nullptr && obj->IsName() ? obj->GetName() : PdfName::KeyNull;
}

void PdfInfo::SetAuthor(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Author", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Author");
}

void PdfInfo::SetCreator(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Creator", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Creator");
}

void PdfInfo::SetKeywords(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Keywords", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Keywords");
}

void PdfInfo::SetSubject(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Subject", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Subject");
}

void PdfInfo::SetTitle(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Title", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Title");
}

void PdfInfo::SetProducer(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Producer", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Producer");
}

void PdfInfo::SetTrapped(const PdfName& trapped)
{
    if ((trapped.GetString() == "True") || (trapped.GetString() == "False"))
        this->GetObject().GetDictionary().AddKey("Trapped", trapped);
    else
        this->GetObject().GetDictionary().AddKey("Trapped", PdfName("Unknown"));
}

nullable<PdfString> PdfInfo::GetAuthor() const
{
    return this->getStringFromInfoDict("Author");
}

nullable<PdfString> PdfInfo::GetCreator() const
{
    return this->getStringFromInfoDict("Creator");
}

nullable<PdfString> PdfInfo::GetKeywords() const
{
    return this->getStringFromInfoDict("Keywords");
}

nullable<PdfString> PdfInfo::GetSubject() const
{
    return this->getStringFromInfoDict("Subject");
}

nullable<PdfString> PdfInfo::GetTitle() const
{
    return this->getStringFromInfoDict("Title");
}

nullable<PdfString> PdfInfo::GetProducer() const
{
    return this->getStringFromInfoDict("Producer");
}

nullable<PdfDate> PdfInfo::GetCreationDate() const
{
    auto datestr = this->getStringFromInfoDict("CreationDate");
    if (datestr == nullptr)
        return nullptr;
    else
        return PdfDate::Parse(*datestr);
}

nullable<PdfDate> PdfInfo::GetModDate() const
{
    auto datestr = this->getStringFromInfoDict("ModDate");
    if (datestr == nullptr)
        return nullptr;
    else
        return PdfDate::Parse(*datestr);
}

const PdfName& PdfInfo::GetTrapped() const
{
    return this->getNameFromInfoDict("Trapped");
}

void PdfInfo::SetCreationDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("CreationDate", value->ToString());
    else
        this->GetObject().GetDictionary().RemoveKey("CreationDate");
}

void PdfInfo::SetModDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("ModDate", value->ToString());
    else
        this->GetObject().GetDictionary().RemoveKey("ModDate");
}

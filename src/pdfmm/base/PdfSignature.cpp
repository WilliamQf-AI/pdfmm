/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2011 by Petr Pytelka
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfSignature.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfData.h"

#include "PdfDocument.h"
#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace mm;

PdfSignature::PdfSignature(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent) :
    PdfField(acroform, PdfFieldType::Signature, parent),
    m_ValueObj(nullptr)
{
    init(acroform);
}

PdfSignature::PdfSignature(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent) :
    PdfField(widget, PdfFieldType::Signature, parent),
    m_ValueObj(nullptr)
{
    init(widget.GetDocument().GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAcroForm* acroform) :
    PdfField(obj, acroform, PdfFieldType::Signature),
    m_ValueObj(this->GetObject().GetDictionary().FindKey("V"))
{
    // NOTE: Do not call init() here
}

void PdfSignature::SetAppearanceStream(PdfXObjectForm& obj, PdfAppearanceType appearance, const PdfName& state)
{
    GetWidget()->SetAppearanceStream(obj, appearance, state);
    (void)this->GetOrCreateAppearanceCharacteristics();
}

void PdfSignature::init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.GetObject().GetDictionary().AddKey("SigFlags", (int64_t)3);
}

void PdfSignature::SetSignerName(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Name", text);
}

void PdfSignature::SetSignatureReason(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Reason", text);
}

void PdfSignature::SetSignatureDate(const PdfDate& sigDate)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfString dateStr = sigDate.ToString();
    m_ValueObj->GetDictionary().AddKey("M", dateStr);
}

void PdfSignature::PrepareForSigning(const string_view& filter,
    const string_view& subFilter, const std::string_view& type,
    const PdfSignatureBeacons& beacons)
{
    EnsureValueObject();
    auto& dict = m_ValueObj->GetDictionary();
    // This must be ensured before any signing operation
    dict.AddKey(PdfName::KeyFilter, PdfName(filter));
    dict.AddKey("SubFilter", PdfName(subFilter));
    dict.AddKey(PdfName::KeyType, PdfName(type));

    // Prepare contents data
    PdfData contentsData = PdfData(beacons.ContentsBeacon, beacons.ContentsOffset);
    m_ValueObj->GetDictionary().AddKey(PdfName::KeyContents, PdfVariant(std::move(contentsData)));

    // Prepare byte range data
    PdfData byteRangeData = PdfData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_ValueObj->GetDictionary().AddKey("ByteRange", PdfVariant(std::move(byteRangeData)));
}

void PdfSignature::SetSignatureLocation(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Location", text);
}

void PdfSignature::SetSignatureCreator(const PdfName& creator)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (m_ValueObj->GetDictionary().HasKey("Prop_Build"))
    {
        PdfObject* propBuild = m_ValueObj->GetDictionary().GetKey("Prop_Build");
        if (propBuild->GetDictionary().HasKey("App"))
        {
            PdfObject* app = propBuild->GetDictionary().GetKey("App");
            app->GetDictionary().RemoveKey("Name");
            propBuild->GetDictionary().RemoveKey("App");
        }

        m_ValueObj->GetDictionary().RemoveKey("Prop_Build");
    }

    m_ValueObj->GetDictionary().AddKey("Prop_Build", PdfDictionary());
    PdfObject* propBuild = m_ValueObj->GetDictionary().GetKey("Prop_Build");
    propBuild->GetDictionary().AddKey("App", PdfDictionary());
    PdfObject* app = propBuild->GetDictionary().GetKey("App");
    app->GetDictionary().AddKey("Name", creator);
}

void PdfSignature::AddCertificationReference(PdfCertPermission perm)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().RemoveKey("Reference");

    auto sigRef = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("SigRef");
    sigRef->GetDictionary().AddKey("TransformMethod", PdfName("DocMDP"));

    auto transParams = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("TransformParams");
    transParams->GetDictionary().AddKey("V", PdfName("1.2"));
    transParams->GetDictionary().AddKey("P", (int64_t)perm);
    sigRef->GetDictionary().AddKey("TransformParams", *transParams);

    auto& catalog = GetObject().GetDocument()->GetCatalog();
    PdfObject permObject;
    permObject.GetDictionary().AddKey("DocMDP", this->GetObject().GetDictionary().GetKey("V")->GetReference());
    catalog.GetDictionary().AddKey("Perms", permObject);

    PdfArray refers;
    refers.Add(*sigRef);

    m_ValueObj->GetDictionary().AddKey("Reference", PdfVariant(refers));
}

const PdfObject* PdfSignature::GetSignatureReason() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Reason");
}

const PdfObject* PdfSignature::GetSignatureLocation() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Location");
}

const PdfObject* PdfSignature::GetSignatureDate() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("M");
}

const PdfObject* PdfSignature::GetSignerName() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Name");
}

PdfObject* PdfSignature::getValueObject() const
{
    return m_ValueObj;
}

void PdfSignature::EnsureValueObject()
{
    if (m_ValueObj != nullptr)
        return;

    m_ValueObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Sig");
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    GetObject().GetDictionary().AddKey("V", m_ValueObj->GetIndirectReference());
}

PdfSignature* PdfSignature::GetParent()
{
    return GetParentTyped<PdfSignature>(PdfFieldType::Signature);
}

const PdfSignature* PdfSignature::GetParent() const
{
    return GetParentTyped<PdfSignature>(PdfFieldType::Signature);
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}

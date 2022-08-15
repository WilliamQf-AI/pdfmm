/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfField.h"

#include "PdfAcroForm.h"
#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfPage.h"
#include "PdfStreamedDocument.h"
#include "PdfXObject.h"
#include "PdfSignature.h"
#include "PdfPushButton.h"
#include "PdfRadioButton.h"
#include "PdfCheckBox.h"
#include "PdfTextBox.h"
#include "PdfComboBox.h"
#include "PdfListBox.h"

using namespace std;
using namespace mm;

void getFullName(const PdfObject& obj, bool escapePartialNames, string& fullname);

PdfField::PdfField(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget) :
    PdfDictionaryElement(widget == nullptr ? *doc.GetObjects().CreateDictionaryObject() : widget->GetObject()),
    m_FieldType(fieldType),
    m_Widget(widget)
{
}

PdfField::PdfField(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect)
    : PdfField(fieldType, page.GetDocument(), page.CreateAnnotation(PdfAnnotationType::Widget, rect))
{
    init(&page.GetDocument().GetOrCreateAcroForm());
}

PdfField::PdfField(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfField(fieldType, doc, widget)
{
    init(insertInAcroform ? &doc.GetOrCreateAcroForm() : nullptr);
}

PdfField::PdfField(PdfFieldType fieldType, PdfObject& obj, PdfAnnotation* widget)
    : PdfDictionaryElement(obj), m_FieldType(fieldType), m_Widget(widget)
{
}

PdfField::PdfField(PdfObject& obj, PdfAnnotation* widget)
    : PdfDictionaryElement(obj), m_FieldType(PdfFieldType::Unknown), m_Widget(widget)
{
    m_FieldType = GetFieldType(obj);
}

bool PdfField::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfField>& field)
{
    return tryCreateField(GetFieldType(obj), obj, nullptr, field);
}

bool PdfField::TryCreateFromAnnotation(PdfAnnotation& annot, unique_ptr<PdfField>& field)
{
    if (annot.GetType() != PdfAnnotationType::Widget)
    {
        field.reset();
        return false;
    }
    return tryCreateField(GetFieldType(annot.GetObject()), annot.GetObject(), &annot, field);
}

unique_ptr<PdfField> PdfField::CreateChildField()
{
    return createChildField(nullptr, PdfRect());
}

unique_ptr<PdfField> PdfField::CreateChildField(PdfPage& page, const PdfRect& rect)
{
    return createChildField(&page, rect);
}

unique_ptr<PdfField> PdfField::createChildField(PdfPage* page, const PdfRect& rect)
{
    PdfFieldType type = GetType();
    auto& doc = GetDocument();
    unique_ptr<PdfField> field;
    PdfObject* childObj;
    if (page == nullptr)
    {
        childObj = doc.GetObjects().CreateDictionaryObject();
        (void)tryCreateField(type, *childObj, nullptr, field);
    }
    else
    {
        PdfAnnotation* annot = page->CreateAnnotation(PdfAnnotationType::Widget, rect);
        childObj = &annot->GetObject();
        (void)tryCreateField(type, *childObj, annot, field);
    }

    auto& dict = GetDictionary();
    auto kids = dict.FindKey("Kids");
    if (kids == nullptr)
        kids = &dict.AddKey("Kids", PdfArray());

    auto& arr = kids->GetArray();
    arr.Add(childObj->GetIndirectReference());
    childObj->GetDictionary().AddKey("Parent", GetObject().GetIndirectReference());
    return field;
}

bool PdfField::tryCreateField(PdfFieldType type, PdfObject& obj, PdfAnnotation* annot,
    std::unique_ptr<PdfField>& field)
{
    switch (type)
    {
        case PdfFieldType::Unknown:
            field.reset(new PdfField(obj, annot));
            return true;
        case PdfFieldType::PushButton:
            field.reset(new PdfPushButton(obj, annot));
            return true;
        case PdfFieldType::CheckBox:
            field.reset(new PdfCheckBox(obj, annot));
            return true;
        case PdfFieldType::RadioButton:
            field.reset(new PdfRadioButton(obj, annot));
            return true;
        case PdfFieldType::TextBox:
            field.reset(new PdfTextBox(obj, annot));
            return true;
        case PdfFieldType::ComboBox:
            field.reset(new PdfComboBox(obj, annot));
            return true;
        case PdfFieldType::ListBox:
            field.reset(new PdfListBox(obj, annot));
            return true;
        case PdfFieldType::Signature:
            field.reset(new PdfSignature(obj, annot));
            return true;
        default:
            field.reset();
            return false;
    }
}

PdfFieldType PdfField::GetFieldType(const PdfObject& obj)
{
    PdfFieldType ret = PdfFieldType::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    auto ftObj = obj.GetDictionary().FindKeyParent("FT");
    if (ftObj == nullptr)
        return PdfFieldType::Unknown;

    auto& fieldType = ftObj->GetName();
    if (fieldType == "Btn")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdfButton::ePdfButton_PushButton) == PdfButton::ePdfButton_PushButton)
            ret = PdfFieldType::PushButton;
        else if ((flags & PdfButton::ePdfButton_Radio) == PdfButton::ePdfButton_Radio)
            ret = PdfFieldType::RadioButton;
        else
            ret = PdfFieldType::CheckBox;
    }
    else if (fieldType == "Tx")
    {
        ret = PdfFieldType::TextBox;
    }
    else if (fieldType == "Ch")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdChoiceField::ePdfListField_Combo) == PdChoiceField::ePdfListField_Combo)
        {
            ret = PdfFieldType::ComboBox;
        }
        else
        {
            ret = PdfFieldType::ListBox;
        }
    }
    else if (fieldType == "Sig")
    {
        ret = PdfFieldType::Signature;
    }

    return ret;
}

void PdfField::init(PdfAcroForm* parent)
{
    if (parent != nullptr)
    {
        // Insert into the parents kids array
        auto& fields = parent->GetOrCreateFieldsArray();
        fields.Add(GetObject().GetIndirectReference());
    }

    PdfDictionary& dict = GetDictionary();
    switch (m_FieldType)
    {
        case PdfFieldType::CheckBox:
            dict.AddKey("FT", PdfName("Btn"));
            break;
        case PdfFieldType::PushButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", (int64_t)PdfButton::ePdfButton_PushButton);
            break;
        case PdfFieldType::RadioButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", (int64_t)(PdfButton::ePdfButton_Radio | PdfButton::ePdfButton_NoToggleOff));
            break;
        case PdfFieldType::TextBox:
            dict.AddKey("FT", PdfName("Tx"));
            break;
        case PdfFieldType::ListBox:
            dict.AddKey("FT", PdfName("Ch"));
            break;
        case PdfFieldType::ComboBox:
            dict.AddKey("FT", PdfName("Ch"));
            dict.AddKey("Ff", (int64_t)PdChoiceField::ePdfListField_Combo);
            break;
        case PdfFieldType::Signature:
            dict.AddKey("FT", PdfName("Sig"));
            break;

        case PdfFieldType::Unknown:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
        break;
    }
}

PdfObject& PdfField::GetOrCreateAppearanceCharacteristics()
{
    auto mkObj = GetDictionary().FindKey("MK");
    if (mkObj != nullptr)
        return *mkObj;

    return GetDictionary().AddKey("MK", PdfDictionary());
}

PdfObject* PdfField::GetAppearanceCharacteristics()
{
    return GetDictionary().FindKey("MK");
}

const PdfObject* PdfField::GetAppearanceCharacteristics() const
{
    return const_cast<PdfField&>(*this).GetAppearanceCharacteristics();
}

PdfObject* PdfField::getValueObject() const
{
    return const_cast<PdfField&>(*this).GetDictionary().FindKey("V");
}

void PdfField::AssertTerminalField() const
{
    if (GetDictionary().HasKey("Kids"))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
    }
}

void PdfField::SetFieldFlag(int64_t value, bool set)
{
    // Retrieve parent field flags
    // CHECK-ME: It seems this semantics is not honored in all cases,
    // example for CheckBoxesRadioButton (s)
    int64_t curr = 0;
    auto fieldFlagsObj = GetDictionary().FindKeyParent("Ff");
    if (fieldFlagsObj != nullptr)
        curr = fieldFlagsObj->GetNumber();

    if (set)
    {
        curr |= value;
    }
    else
    {
        if ((curr & value) == value)
            curr ^= value;
    }

    GetDictionary().AddKey("Ff", curr);
}

bool PdfField::GetFieldFlag(int64_t value, bool defvalue) const
{
    int64_t flag;
    if (!GetFieldFlags(GetObject(), flag))
        return defvalue;

    return (flag & value) == value;
}


bool PdfField::GetFieldFlags(const PdfObject& obj, int64_t& value)
{
    auto flagsObject = obj.GetDictionary().FindKeyParent("Ff");
    if (flagsObject == nullptr)
    {
        value = 0;
        return false;
    }

    value = flagsObject->GetNumber();
    return true;
}

void PdfField::SetHighlightingMode(PdfHighlightingMode mode)
{
    PdfName value;

    switch (mode)
    {
        case PdfHighlightingMode::None:
            value = "N";
            break;
        case PdfHighlightingMode::Invert:
            value = "I";
            break;
        case PdfHighlightingMode::InvertOutline:
            value = "O";
            break;
        case PdfHighlightingMode::Push:
            value = "P";
            break;
        case PdfHighlightingMode::Unknown:
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidName);
            break;
    }

    GetDictionary().AddKey("H", value);
}

PdfHighlightingMode PdfField::GetHighlightingMode() const
{
    PdfHighlightingMode mode = PdfHighlightingMode::Invert;

    if (GetDictionary().HasKey("H"))
    {
        auto& value = GetDictionary().MustFindKey("H").GetName();
        if (value == "N")
            return PdfHighlightingMode::None;
        else if (value == "I")
            return PdfHighlightingMode::Invert;
        else if (value == "O")
            return PdfHighlightingMode::InvertOutline;
        else if (value == "P")
            return PdfHighlightingMode::Push;
    }

    return mode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double gray)
{
    PdfArray array;
    array.Add(gray);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double red, double green, double blue)
{
    PdfArray array;
    array.Add(red);
    array.Add(green);
    array.Add(blue);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double cyan, double magenta, double yellow, double black)
{
    PdfArray array;
    array.Add(cyan);
    array.Add(magenta);
    array.Add(yellow);
    array.Add(black);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BC", array);
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double gray)
{
    PdfArray array;
    array.Add(gray);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double red, double green, double blue)
{
    PdfArray array;
    array.Add(red);
    array.Add(green);
    array.Add(blue);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double cyan, double magenta, double yellow, double black)
{
    PdfArray array;
    array.Add(cyan);
    array.Add(magenta);
    array.Add(yellow);
    array.Add(black);

    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("BG", array);
}

void PdfField::SetName(const PdfString& name)
{
    GetDictionary().AddKey("T", name);
}

PdfObject* PdfField::GetValueObject()
{
    return getValueObject();
}

const PdfObject* PdfField::GetValueObject() const
{
    return getValueObject();
}

nullable<PdfString> PdfField::GetName() const
{
    auto name = GetDictionary().FindKeyParent("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

nullable<PdfString> PdfField::GetNameRaw() const
{
    auto name = GetDictionary().GetKey("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

string PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(GetObject(), escapePartialNames, fullName);
    return fullName;
}

void PdfField::SetAlternateName(const PdfString& name)
{
    GetDictionary().AddKey("TU", name);
}

nullable<PdfString> PdfField::GetAlternateName() const
{
    if (GetDictionary().HasKey("TU"))
        return GetDictionary().MustFindKey("TU").GetString();

    return { };
}

void PdfField::SetMappingName(const PdfString& name)
{
    GetDictionary().AddKey("TM", name);
}

nullable<PdfString> PdfField::GetMappingName() const
{
    if (GetDictionary().HasKey("TM"))
        return GetDictionary().MustFindKey("TM").GetString();

    return { };
}

void PdfField::addAlternativeAction(const PdfName& name, const PdfAction& action)
{
    auto aaObj = GetDictionary().FindKey("AA");
    if (aaObj == nullptr)
        aaObj = &GetDictionary().AddKey("AA", PdfDictionary());

    aaObj->GetDictionary().AddKey(name, action.GetObject().GetIndirectReference());
}

void PdfField::SetReadOnly(bool readOnly)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::ReadOnly), readOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool required)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::Required), required);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool exprt)
{
    this->SetFieldFlag(static_cast<int64_t>(PdfFieldFlags::NoExport), exprt);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int64_t>(PdfFieldFlags::NoExport), false);
}

PdfPage* PdfField::GetPage() const
{
    return m_Widget->GetPage();
}

void PdfField::SetMouseEnterAction(const PdfAction& action)
{
    this->addAlternativeAction("E", action);
}

void PdfField::SetMouseLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("X", action);
}

void PdfField::SetMouseDownAction(const PdfAction& action)
{
    this->addAlternativeAction("D", action);
}

void PdfField::SetMouseUpAction(const PdfAction& action)
{
    this->addAlternativeAction("U", action);
}

void PdfField::SetFocusEnterAction(const PdfAction& action)
{
    this->addAlternativeAction("Fo", action);
}

void PdfField::SetFocusLeaveAction(const PdfAction& action)
{
    this->addAlternativeAction("BI", action);
}

void PdfField::SetPageOpenAction(const PdfAction& action)
{
    this->addAlternativeAction("PO", action);
}

void PdfField::SetPageCloseAction(const PdfAction& action)
{
    this->addAlternativeAction("PC", action);
}

void PdfField::SetPageVisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PV", action);
}

void PdfField::SetPageInvisibleAction(const PdfAction& action)
{
    this->addAlternativeAction("PI", action);
}

void PdfField::SetKeystrokeAction(const PdfAction& action)
{
    this->addAlternativeAction("K", action);
}

void PdfField::SetValidateAction(const PdfAction& action)
{
    this->addAlternativeAction("V", action);
}

void getFullName(const PdfObject& obj, bool escapePartialNames, string& fullname)
{
    auto& dict = obj.GetDictionary();
    auto parent = dict.FindKey("Parent");
    if (parent != nullptr)
        getFullName(*parent, escapePartialNames, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != nullptr)
    {
        string name = nameObj->GetString().GetString();
        if (escapePartialNames)
        {
            // According to ISO 32000-1:2008, "12.7.3.2 Field Names":
            // "Because the PERIOD is used as a separator for fully
            // qualified names, a partial name shall not contain a
            // PERIOD character."
            // In case the partial name still has periods (effectively
            // violating the standard and Pdf Reference) the fullname
            // would be unintelligible, let's escape them with double
            // dots "..", example "parent.partial..name"
            size_t currpos = 0;
            while ((currpos = name.find('.', currpos)) != std::string::npos)
            {
                name.replace(currpos, 1, "..", 2);
                currpos += 2;
            }
        }

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}


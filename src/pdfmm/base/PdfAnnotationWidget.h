/*
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1
 */

#ifndef PDF_ANNOTATION_WIDGET_H
#define PDF_ANNOTATION_WIDGET_H

#include "PdfAnnotationActionBase.h"

namespace mm {

    class PdfField;

    class PDFMM_API PdfAnnotationWidget : public PdfAnnotationActionBase
    {
        friend class PdfAnnotation;
        friend class PdfField;
        friend class PdfPage;
    private:
        PdfAnnotationWidget(PdfPage& page, const PdfRect& rect);
        PdfAnnotationWidget(PdfObject& obj);
    public:
        const PdfField& GetField() const;
        PdfField& GetField();
    private:
        void SetField(const std::shared_ptr<PdfField>& field);
        const std::shared_ptr<PdfField>& GetFieldPtr() { return m_Field; }
    private:
        void initField();
    private:
        std::shared_ptr<PdfField> m_Field;
    };
}

#endif // PDF_ANNOTATION_WIDGET_H
/*
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1
 */

#ifndef PDF_ANNOTATION_COLLECTION_H
#define PDF_ANNOTATION_COLLECTION_H

#include "PdfAnnotation.h"
#include "PdfArray.h"

namespace mm
{
    class PdfPage;

    class PDFMM_API PdfAnnotationCollection final
    {
        friend class PdfPage;
        friend class PdfAnnotation;

    private:
        PdfAnnotationCollection(PdfPage& page);

    public:
        template <typename TAnnotation>
        TAnnotation& CreateAnnot(const PdfRect& rect);

        PdfAnnotation& CreateAnnot(PdfAnnotationType annotType, const PdfRect& rect);

        PdfAnnotation& GetAnnotAt(unsigned index);

        const PdfAnnotation& GetAnnotAt(unsigned index) const;

        void RemoveAnnotAt(unsigned index);

        void RemoveAnnot(const PdfReference& ref);

        unsigned GetCount() const;

    public:
        using AnnotationList = std::vector<std::unique_ptr<PdfAnnotation>>;

        template <typename TObject, typename TListIterator>
        class Iterator final
        {
            friend class PdfAnnotationCollection;
        public:
            using difference_type = void;
            using value_type = TObject*;
            using pointer = void;
            using reference = void;
            using iterator_category = std::forward_iterator_tag;
        public:
            Iterator() { }
        private:
            Iterator(const TListIterator& iterator) : m_iterator(iterator) { }
        public:
            Iterator(const Iterator&) = default;
            Iterator& operator=(const Iterator&) = default;
            bool operator==(const Iterator& rhs) const
            {
                return m_iterator == rhs.m_iterator;
            }
            bool operator!=(const Iterator& rhs) const
            {
                return m_iterator != rhs.m_iterator;
            }
            Iterator& operator++()
            {
                m_iterator++;
                return *this;
            }
            value_type operator*()
            {
                return m_iterator.get();
            }
            value_type operator->()
            {
                return m_iterator.get();
            }
        private:
            TListIterator m_iterator;
        };

        using iterator = Iterator<PdfAnnotation, AnnotationList::iterator>;
        using const_iterator = Iterator<const PdfAnnotation, AnnotationList::const_iterator>;

    public:
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;

    private:
        PdfAnnotation& createAnnotation(const std::type_info& typeInfo, const PdfRect& rect);
        PdfAnnotation& addAnnotation(std::unique_ptr<PdfAnnotation>&& annot);
        PdfArray* getAnnotationsArray() const;
        void initAnnotations();
        PdfAnnotation& getAt(unsigned index) const;
        void fixIndices(unsigned index);

    private:
        using AnnotationMap = std::map<PdfReference, unsigned>;

    private:
        AnnotationList m_Annots;
        std::unique_ptr<AnnotationMap> m_annotMap;
        PdfPage* m_Page;
        PdfArray* m_annotArray;
    };

    template<typename TAnnotation>
    TAnnotation& PdfAnnotationCollection::CreateAnnot(const PdfRect& rect)
    {
        return static_cast<TAnnotation&>(createAnnotation(typeid(TAnnotation), rect));
    }
}

#endif // PDF_ANNOTATION_COLLECTION_H
/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XREF_H
#define PDF_XREF_H

#include "PdfDefines.h"

#include "PdfReference.h"
#include "PdfXRefEntry.h"

namespace mm {

class PdfOutputDevice;
class PdfWriter;

/**
 * Creates an XRef table.
 *
 * This is an internal class of pdfmm used by PdfWriter.
 */
class PdfXRef
{
protected:
    struct XRefItem
    {
        XRefItem(const PdfReference& ref, uint64_t off)
            : Reference(ref), Offset(off) { }

        PdfReference Reference;
        uint64_t Offset;

        bool operator<(const XRefItem& rhs) const
        {
            return this->Reference < rhs.Reference;
        }
    };

    typedef std::vector<XRefItem> XRefItemList;
    typedef std::vector<PdfReference> ReferenceList;

    struct PdfXRefBlock
    {
        PdfXRefBlock()
            : First(0), Count(0) { }

        PdfXRefBlock(const PdfXRefBlock& rhs) = default;

        bool InsertItem(const PdfReference& ref, std::optional<uint64_t> offset, bool bUsed);

        bool operator<(const PdfXRefBlock& rhs) const
        {
            return First < rhs.First;
        }

        PdfXRefBlock& operator=(const PdfXRefBlock& rhs) = default;

        uint32_t First;
        uint32_t Count;
        XRefItemList Items;
        ReferenceList FreeItems;
    };

    typedef std::vector<PdfXRefBlock> XRefBlockList;

public:
    PdfXRef(PdfWriter& pWriter);
    virtual ~PdfXRef();

public:

    /** Add an used object to the XRef table.
     *  The object should have been written to an output device already.
     *
     *  \param ref reference of this object
     *  \param offset the offset where on the device the object was written
     *                if std::nullopt, the object will be accounted for
     *                 trailer's /Size but not written in the entries list
     */
    void AddInUseObject(const PdfReference& ref, std::optional<uint64_t> offset);

    /** Add a free object to the XRef table.
     *
     *  \param ref reference of this object
     *  \param offset the offset where on the device the object was written
     *  \param bUsed specifies whether this is an used or free object.
     *               Set this value to true for all normal objects and to false
     *               for free object references.
     */
    void AddFreeObject(const PdfReference& ref);

    /** Write the XRef table to an output device.
     *
     *  \param device an output device (usually a PDF file)
     *
     */
    void Write(PdfOutputDevice& device);

    /** Get the size of the XRef table.
     *  I.e. the highest object number + 1.
     *
     *  \returns the size of the xref table
     */
    uint32_t GetSize() const;

    /**
     * Mark as empty block.
     */
    void SetFirstEmptyBlock();

    /** Should skip writing for this object
     */
    virtual bool ShouldSkipWrite(const PdfReference& ref);

public:
    inline PdfWriter& GetWriter() const { return *m_writer; }

    /**
     * \returns the offset in the file at which the XRef table
     *          starts after it was written
     */
    inline virtual uint64_t GetOffset() const { return m_offset; }

protected:
    /** Called at the start of writing the XRef table.
     *  This method can be overwritten in subclasses
     *  to write a general header for the XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    virtual void BeginWrite(PdfOutputDevice& device);

    /** Begin an XRef subsection.
     *  All following calls of WriteXRefEntry belong to this XRef subsection.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     *  \param nFirst the object number of the first object in this subsection
     *  \param nCount the number of entries in this subsection
     */
    virtual void WriteSubSection(PdfOutputDevice& device, uint32_t nFirst, uint32_t nCount);

    /** Write a single entry to the XRef table
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     *  \param offset the offset of the object
     *  \param generation the generation number
     *  \param cMode the mode 'n' for object and 'f' for free objects
     *  \param objectNumber the object number of the currently written object if cMode = 'n'
     *                       otherwise undefined
     */
    virtual void WriteXRefEntry(PdfOutputDevice& device, const PdfXRefEntry& entry);

    /**  Sub classes can overload this method to finish a XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    virtual void EndWriteImpl(PdfOutputDevice& device);

private:
    void AddObject(const PdfReference& ref, std::optional<uint64_t> offset, bool inUse);

    /** Called at the end of writing the XRef table.
     *  Sub classes can overload this method to finish a XRef table.
     *
     *  \param device the output device to which the XRef table
     *                 should be written.
     */
    void EndWrite(PdfOutputDevice& device);

    const PdfReference* GetFirstFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const;
    const PdfReference* GetNextFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const;

    /** Merge all xref blocks that follow immediately after each other
     *  into a single block.
     *
     *  This results in slightly smaller PDF files which are easier to parse
     *  for other applications.
     */
    void MergeBlocks();

private:
    uint32_t m_maxObjCount;
    XRefBlockList m_blocks;
    PdfWriter* m_writer;
    uint64_t m_offset;
};

};

#endif // PDF_XREF_H
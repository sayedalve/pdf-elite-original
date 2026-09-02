#pragma once
#include "core/interfaces/dom/ICommand.h"
#include "../PdfDocument.h"
#include <fpdfview.h>
#include <fpdf_ppo.h>
#include <vector>

namespace pdf_engine {
namespace commands {

class DeletePageCommand : public core::interfaces::dom::ICommand {
public:
    DeletePageCommand(PdfDocument* doc, int pageIndex);
    ~DeletePageCommand() override;

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Delete Page"; }

private:
    PdfDocument* m_doc;
    int m_pageIndex;
    FPDF_DOCUMENT m_backupDoc = nullptr;
};

class InsertBlankPageCommand : public core::interfaces::dom::ICommand {
public:
    InsertBlankPageCommand(PdfDocument* doc, int pageIndex, double width, double height);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Insert Blank Page"; }

private:
    PdfDocument* m_doc;
    int m_pageIndex;
    double m_width;
    double m_height;
};

class RotatePageCommand : public core::interfaces::dom::ICommand {
public:
    RotatePageCommand(PdfDocument* doc, int pageIndex, int rotationDegrees);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Rotate Page"; }

private:
    PdfDocument* m_doc;
    int m_pageIndex;
    int m_rotationDegrees;
    int m_oldRotation = 0;
};

class MovePageCommand : public core::interfaces::dom::ICommand {
public:
    MovePageCommand(PdfDocument* doc, int sourceIndex, int destIndex);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Move Page"; }

private:
    PdfDocument* m_doc;
    int m_sourceIndex;
    int m_destIndex;
};

class CropPageCommand : public core::interfaces::dom::ICommand {
public:
    CropPageCommand(PdfDocument* doc, int pageIndex, float left, float top, float right, float bottom);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Crop Page"; }

private:
    PdfDocument* m_doc;
    int m_pageIndex;
    float m_left, m_top, m_right, m_bottom;
    float m_oldLeft, m_oldTop, m_oldRight, m_oldBottom;
};

class SetPageSizeCommand : public core::interfaces::dom::ICommand {
public:
    SetPageSizeCommand(PdfDocument* doc, int pageIndex, float width, float height);

    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Set Page Size"; }

private:
    PdfDocument* m_doc;
    int m_pageIndex;
    float m_width, m_height;
    float m_oldWidth, m_oldHeight;
};
} // namespace commands
} // namespace pdf_engine


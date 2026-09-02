#pragma once
#include "core/interfaces/dom/ICommand.h"
#include "core/interfaces/dom/ITextObject.h"
#include "core/interfaces/dom/IDocument.h"
#include <string>

namespace pdf_engine {
namespace commands {

class EditTextCommand : public core::interfaces::dom::ICommand {
public:
    EditTextCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, const std::wstring& oldText, const std::wstring& newText);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Edit Text"; }
    
private:
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    std::wstring m_oldText;
    std::wstring m_newText;
};

class MoveTextCommand : public core::interfaces::dom::ICommand {
public:
    MoveTextCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, const RectF& oldBounds, const RectF& newBounds);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Move Text"; }
    
private:
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    RectF m_oldBounds;
    RectF m_newBounds;
};

class DeleteTextCommand : public core::interfaces::dom::ICommand {
public:
    DeleteTextCommand(core::interfaces::dom::IDocument* doc, int pageIndex, std::shared_ptr<core::interfaces::dom::ITextObject> textObj);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Delete Text"; }
    
private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    bool m_deleted;
    
    std::wstring m_cachedText;
    RectF m_cachedBounds;
    std::string m_cachedFontName;
    float m_cachedFontSize;
    uint8_t m_cachedR, m_cachedG, m_cachedB, m_cachedA;
};

class AddTextCommand : public core::interfaces::dom::ICommand {
public:
    AddTextCommand(core::interfaces::dom::IDocument* doc, int pageIndex, const std::wstring& text, const RectF& bounds, const std::string& fontName, float fontSize, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    
    void SetDocument(core::interfaces::dom::IDocument* doc) { m_doc = doc; }
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Add Text"; }
    
private:
    core::interfaces::dom::IDocument* m_doc;
    int m_pageIndex;
    std::wstring m_text;
    RectF m_bounds;
    std::string m_fontName;
    float m_fontSize;
    uint8_t m_r, m_g, m_b, m_a;
    
    std::shared_ptr<core::interfaces::dom::ITextObject> m_addedObj;
};

class EditMultilineTextCommand : public core::interfaces::dom::ICommand {
public:
    EditMultilineTextCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, 
                             const std::vector<core::interfaces::dom::TextLineData>& oldLines,
                             const std::vector<core::interfaces::dom::TextLineData>& newLines);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Edit Multiline Text"; }
    
private:
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    std::vector<core::interfaces::dom::TextLineData> m_oldLines;
    std::vector<core::interfaces::dom::TextLineData> m_newLines;
};

class EditTextStyleCommand : public core::interfaces::dom::ICommand {
public:
    EditTextStyleCommand(std::shared_ptr<core::interfaces::dom::ITextObject> textObj, 
                         float oldSize, float newSize,
                         uint8_t oldR, uint8_t oldG, uint8_t oldB, uint8_t oldA,
                         uint8_t newR, uint8_t newG, uint8_t newB, uint8_t newA);
    
    bool Execute() override;
    bool Undo() override;
    std::string GetDescription() const override { return "Edit Text Style"; }
    
private:
    std::shared_ptr<core::interfaces::dom::ITextObject> m_textObj;
    float m_oldSize, m_newSize;
    uint8_t m_oldR, m_oldG, m_oldB, m_oldA;
    uint8_t m_newR, m_newG, m_newB, m_newA;
};

} // namespace commands
} // namespace pdf_engine

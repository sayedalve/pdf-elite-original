#pragma once
#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace core {
namespace models {

// Represents a PDF object property mapping (e.g. string, float, bool, color)
using PDFPropertyValue = std::variant<std::string, double, bool, std::vector<double>>;

class PDFObjectProperty {
public:
    std::string key; // The PDF dictionary key (e.g., "/Rect", "/C")
    std::string displayName; // UI display name (e.g., "Bounding Box", "Color")
    PDFPropertyValue value;
    bool isReadOnly;

    PDFObjectProperty(const std::string& k, const std::string& name, const PDFPropertyValue& v, bool ro = false)
        : key(k), displayName(name), value(v), isReadOnly(ro) {}
};

/**
 * @class PDFObjectEditorAbstractModel
 * @brief Inspired by PDF4QT, maps PDFium dictionary attributes to native C++ structures
 * for UI binding and serialization without coupling the UI to PDFium.
 */
class PDFObjectEditorAbstractModel {
protected:
    std::vector<PDFObjectProperty> properties;

public:
    virtual ~PDFObjectEditorAbstractModel() = default;

    // Load properties from the underlying PDF object (e.g. FPDF_ANNOTATION or FPDF_PAGEOBJECT)
    virtual void LoadFromPdfObject(void* pdfObject) = 0;

    // Apply the mapped properties back to the PDF object
    virtual void ApplyToPdfObject(void* pdfObject) = 0;

    const std::vector<PDFObjectProperty>& GetProperties() const {
        return properties;
    }

    bool SetProperty(const std::string& key, const PDFPropertyValue& value) {
        for (auto& prop : properties) {
            if (prop.key == key && !prop.isReadOnly) {
                prop.value = value;
                return true;
            }
        }
        return false;
    }
    
    // Abstract serialization for Undo/Redo tracking
    virtual std::string SerializeState() const = 0;
    virtual void DeserializeState(const std::string& state) = 0;
};

} // namespace models
} // namespace core

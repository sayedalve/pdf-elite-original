#pragma once

#include <string>
#include <vector>
#include "IAnnotation.h"

namespace core {
namespace interfaces {
namespace dom {

enum class FormFieldType {
    Unknown,
    PushButton,
    CheckBox,
    RadioButton,
    ComboBox,
    ListBox,
    TextField,
    Signature
};

class IFormField {
public:
    virtual ~IFormField() = default;

    virtual FormFieldType GetType() const = 0;
    virtual std::string GetName() const = 0;
    virtual std::string GetAlternateName() const = 0;
    virtual std::string GetMappingName() const = 0;
    virtual std::wstring GetValue() const = 0;
    virtual std::wstring GetDefaultValue() const = 0;
    virtual RectF GetBounds() const = 0;
    virtual bool IsReadOnly() const = 0;
    virtual bool IsRequired() const = 0;
    virtual bool IsNoExport() const = 0;
    virtual int GetPageIndex() const = 0;
    virtual int GetFlags() const = 0;
    
    // For Choice fields
    virtual std::vector<std::wstring> GetOptions() const = 0;
    virtual std::vector<int> GetSelectedIndices() const = 0;
};

} // namespace dom
} // namespace interfaces
} // namespace core

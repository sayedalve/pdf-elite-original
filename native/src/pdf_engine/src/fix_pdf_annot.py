import re

with open('PdfAnnotation.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('     if (m_page) FPDFPage_GenerateContent(m_page->GetHandle());`n}', ' };')
content = content.replace('        if (m_page) FPDFPage_GenerateContent(m_page->GetHandle());`n}', '        return;\n    }')
content = content.replace('        FS_QUADPOINTSF q = { quads[i].p1.x, quads[i].p1.y, quads[i].p2.x, quads[i].p2.y, quads[i].p3.x, quads[i].p3.y, quads[i].p4.x, quads[i].p4.y     if (m_page) FPDFPage_GenerateContent(m_page->GetHandle());`n};', '        FS_QUADPOINTSF q = { quads[i].p1.x, quads[i].p1.y, quads[i].p2.x, quads[i].p2.y, quads[i].p3.x, quads[i].p3.y, quads[i].p4.x, quads[i].p4.y };')

content = re.sub(r'    if \(m_page\) FPDFPage_GenerateContent\(m_page->GetHandle\(\)\);`n\}', '}\n', content)
content = re.sub(r'        if \(m_page\) FPDFPage_GenerateContent\(m_page->GetHandle\(\)\);`n\}', '}\n', content)

with open('PdfAnnotation.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

import os
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

os.makedirs('tests/fixtures/text', exist_ok=True)
os.makedirs('tests/fixtures/unicode', exist_ok=True)

# Basic Text PDF
c = canvas.Canvas('tests/fixtures/text/basic_text.pdf')
c.drawString(100, 700, 'Hello PDF Elite')
c.drawString(100, 680, 'This is a second line.')
c.drawString(100, 660, '1234567890')
c.showPage()
c.save()

# Unicode/Bangla PDF
# Using a standard Windows font that supports Unicode
try:
    pdfmetrics.registerFont(TTFont('Arial', 'C:/Windows/Fonts/arial.ttf'))
    c = canvas.Canvas('tests/fixtures/unicode/unicode_text.pdf')
    c.setFont('Arial', 12)
    c.drawString(100, 700, 'English and Bangla')
    c.drawString(100, 680, 'Hello World')
    # Let's use some dummy unicode if Bangla doesn't render perfectly, it's just extraction we are testing
    c.drawString(100, 660, u'বাংলা টেস্ট') # Bangla Test
    c.showPage()
    c.save()
    print('Unicode PDF generated successfully')
except Exception as e:
    print('Failed to generate Unicode PDF:', e)

print('Done')

#!/usr/bin/env python3
"""生成示例 xlsx 文件用于测试"""
import zipfile, os, sys

def make_xlsx(path):
    names = [
        ("张三", "2024001", "计算机一班", "男", "班委"),
        ("李四", "2024002", "计算机一班", "男", ""),
        ("王五", "2024003", "计算机一班", "女", ""),
        ("赵六", "2024004", "计算机一班", "男", ""),
        ("钱七", "2024005", "计算机一班", "女", ""),
        ("孙八", "2024006", "计算机一班", "男", ""),
        ("周九", "2024007", "计算机一班", "女", ""),
        ("吴十", "2024008", "计算机一班", "男", ""),
        ("郑十一", "2024009", "计算机一班", "男", ""),
        ("王十二", "2024010", "计算机一班", "女", ""),
        ("陈十三", "2024011", "计算机一班", "男", ""),
        ("褚十四", "2024012", "计算机一班", "女", ""),
        ("卫十五", "2024013", "计算机一班", "男", ""),
        ("蒋十六", "2024014", "计算机一班", "女", ""),
        ("沈十七", "2024015", "计算机一班", "男", ""),
        ("韩十八", "2024016", "计算机一班", "女", ""),
        ("杨十九", "2024017", "计算机一班", "男", ""),
        ("朱二十", "2024018", "计算机一班", "女", ""),
        ("秦二十一", "2024019", "计算机一班", "男", ""),
        ("尤二十二", "2024020", "计算机一班", "女", ""),
    ]
    headers = ["姓名", "学号", "班级", "性别", "备注"]
    rows = [headers] + [list(n) for n in names]
    flat = []
    for r in rows:
        flat.extend(r)
    ss_items = "".join(f"<si><t>{v}</t></si>" for v in flat)
    shared = f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
             f'<sst xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" count="{len(flat)}" uniqueCount="{len(flat)}">' \
             + ss_items + '</sst>'

    def col_letter(idx):
        s = ""
        while idx >= 0:
            s = chr(ord('A') + idx % 26) + s
            idx = idx // 26 - 1
        return s

    row_xml_parts = []
    for i, row in enumerate(rows, start=1):
        cells = ""
        for c, v in enumerate(row):
            cells += f'<c r="{col_letter(c)}{i}" t="s"><v>{i*len(headers) - len(headers) + c}</v></c>'
        row_xml_parts.append(f'<row r="{i}">{cells}</row>')
    rows_xml = "".join(row_xml_parts)

    sheet = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
            '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">' \
            '<sheetData>' + rows_xml + '</sheetData></worksheet>'

    workbook = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
               '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">' \
               '<sheets><sheet name="Sheet1" sheetId="1" r:id="rId1"/></sheets></workbook>'

    rels = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
           '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">' \
           '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>' \
           '<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings" Target="sharedStrings.xml"/>' \
           '</Relationships>'

    root_rels = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
                '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">' \
                '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>' \
                '</Relationships>'

    content_types = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
                    '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">' \
                    '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>' \
                    '<Default Extension="xml" ContentType="application/xml"/>' \
                    '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>' \
                    '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>' \
                    '<Override PartName="/xl/sharedStrings.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml"/>' \
                    '</Types>'

    with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as z:
        z.writestr('[Content_Types].xml', content_types)
        z.writestr('_rels/.rels', root_rels)
        z.writestr('xl/workbook.xml', workbook)
        z.writestr('xl/_rels/workbook.xml.rels', rels)
        z.writestr('xl/sharedStrings.xml', shared)
        z.writestr('xl/worksheets/sheet1.xml', sheet)

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else 'students.xlsx'
    make_xlsx(out)
    print(f'Created {out} ({os.path.getsize(out)} bytes)')

import re
import unittest
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DRAWING_SOURCE = ROOT / "src" / "drawing.cpp"
MAIN_SOURCE = ROOT / "src" / "main.cpp"
API_HEADER = ROOT / "include" / "apiwebserver.h"
XLSX_LAYOUT = ROOT / "leds.xlsx"

SHEET_NS = {"x": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}


def column_index(reference: str) -> int:
    letters = re.match(r"[A-Z]+", reference).group(0)
    value = 0
    for letter in letters:
        value = value * 26 + ord(letter) - ord("A") + 1
    return value - 1


def read_layout_cells():
    with zipfile.ZipFile(XLSX_LAYOUT) as workbook:
        root = ET.fromstring(workbook.read("xl/worksheets/sheet1.xml"))

    cells = {}
    for cell in root.findall(".//x:c", SHEET_NS):
        value = cell.find("x:v", SHEET_NS)
        if value is None:
            continue
        reference = cell.attrib["r"]
        x = column_index(reference)
        y = int(re.search(r"\d+", reference).group(0)) - 1
        cells[int(value.text)] = (x, y)
    return cells


def expected_coordinate(index: int):
    if index <= 18:
        return index, 0
    if index <= 31:
        return 18, index - 18
    if index <= 50:
        return 18 - (index - 32), 14
    if index <= 61:
        return 0, 13 - (index - 51)
    if index <= 78:
        return 1 + (index - 62), 3
    if index <= 85:
        return 17, 4 + (index - 79)
    if index <= 100:
        return 16 - (index - 86), 10
    if index <= 105:
        return 2, 9 - (index - 101)
    if index <= 111:
        return 3 + (index - 106), 5
    if index <= 113:
        return 8, 6 + (index - 112)
    return 9 + (index - 114), 7


class LedLayoutTests(unittest.TestCase):
    def test_excel_contains_every_strip_one_index_once(self):
        cells = read_layout_cells()
        self.assertEqual(set(cells), set(range(121)))
        self.assertEqual(len(cells), 121)

    def test_excel_matches_runtime_coordinate_path(self):
        cells = read_layout_cells()
        for index in range(121):
            self.assertEqual(cells[index], expected_coordinate(index), index)

    def test_jackpot_uses_visible_one_through_eight_order(self):
        source = DRAWING_SOURCE.read_text()
        match = re.search(
            r"kJackpotVisualToPhysical\s*\[kJackpotSegments\]\s*=\s*"
            r"\{([^}]+)\}",
            source,
        )
        self.assertIsNotNone(match)
        mapping = [int(value) for value in re.findall(r"\d+", match.group(1))]
        self.assertEqual(mapping, [7, 6, 5, 4, 3, 2, 1, 0])

    def test_only_renderer_calls_fastled_show(self):
        direct_calls = []
        for path in (ROOT / "src").glob("*.cpp"):
            if path.name == "ledrenderer.cpp":
                continue
            if "FastLED.show()" in path.read_text():
                direct_calls.append(path.name)
        self.assertEqual(direct_calls, [])

    def test_network_startup_is_backgrounded(self):
        source = MAIN_SOURCE.read_text()
        setup = source[source.index("void setup()"):source.index("void loop()")]
        self.assertIn("NetworkLoopTaskEntry", setup)
        self.assertNotIn("ConnectToWiFi(", setup)

    def test_status_endpoint_and_input_errors_are_exposed(self):
        source = API_HEADER.read_text()
        self.assertIn('"/status"', source)
        self.assertIn("beginResponse(status, contentType, content)", source)
        self.assertGreaterEqual(source.count("400"), 3)


if __name__ == "__main__":
    unittest.main()

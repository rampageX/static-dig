"""Fail-closed tests for the ISC ESV release selector."""

import unittest

from ci.release_source import current_920_esv


def row(version: str, stable: bool, esv: bool, source: str = "BIND 9") -> str:
    badges = '<div class="badge-download--stable">Current Stable</div>' if stable else ""
    badges += '<div class="badge-download--esv">ESV</div>' if esv else ""
    return (
        f'<tr><td><div class="download-version-text">{version}</div></td>'
        f'<td>{badges}</td><td>{source}</td></tr>'
    )


class ReleaseSelectorTests(unittest.TestCase):
    def test_selects_only_920_stable_esv(self):
        source = row("9.21.27", False, False) + row("9.20.30", True, True) + row("9.18.50", False, True)
        self.assertEqual(current_920_esv(source), "9.20.30")

    def test_rejects_development_only(self):
        with self.assertRaises(ValueError):
            current_920_esv(row("9.21.27", False, False))

    def test_rejects_non_bind_stable_software(self):
        with self.assertRaises(ValueError):
            current_920_esv(row("9.20.30", True, True, source="Kea"))

    def test_rejects_duplicate_stable_rows(self):
        with self.assertRaises(ValueError):
            current_920_esv(row("9.20.30", True, True) + row("9.20.31", True, True))


if __name__ == "__main__":
    unittest.main()

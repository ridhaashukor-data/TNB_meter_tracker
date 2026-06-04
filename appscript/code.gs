const TOKEN_PROPERTY_KEYS = ["DEVICE_TOKEN", "device_token"];
const REQUIRED_HEADERS = ["timestamp", "pulse_count", "kwh", "watts", "source"];

function doPost(e) {
  try {
    // 1) Validate body exists
    if (!e || !e.postData || !e.postData.contents) return textOut("bad_request");

    // 2) Parse JSON safely
    let data;
    try {
      data = JSON.parse(e.postData.contents);
    } catch (_) {
      return textOut("bad_request");
    }

    // 3) Required fields check
    const required = ["timestamp", "pulse_count", "kwh", "watts", "source", "token"];
    for (const key of required) {
      if (!(key in data)) return textOut("bad_request");
    }

    // 4) Token check (body token)
    const token = String(data.token || "");
    const expectedToken = getExpectedToken_();
    if (!expectedToken) return textOut("bad_request");
    if (token !== expectedToken) return textOut("unauthorized");

    // 5) Field validation
    const timestamp = String(data.timestamp || "");
    const pulseCount = Number(data.pulse_count);
    const kwh = Number(data.kwh);
    const watts = Number(data.watts);
    const source = String(data.source || "");

    if (!/^\d{2}-\d{2}-\d{4} \d{2}:\d{2}:\d{2}$/.test(timestamp)) return textOut("bad_request");
    if (!Number.isFinite(pulseCount) || !Number.isFinite(kwh) || !Number.isFinite(watts)) return textOut("bad_request");
    if (!source) return textOut("bad_request");

    // 6) Determine yearly tab from DD-MM-YYYY HH:MM:SS
    const year = timestamp.substring(6, 10); // YYYY
    if (!/^\d{4}$/.test(year)) return textOut("bad_request");

    const ss = SpreadsheetApp.getActiveSpreadsheet();
    let sheet = ss.getSheetByName(year);

    // 7) Auto-create tab if missing
    if (!sheet) {
      sheet = ss.insertSheet(year);
    }

    // 8) Auto-fix headers (row 1)
    ensureHeaderAutoFix_(sheet);

    // 9) Append row
    sheet.appendRow([timestamp, pulseCount, kwh, watts, source]);

    // 10) Exact success body
    return textOut("ok");
  } catch (_) {
    return textOut("bad_request");
  }
}

function ensureHeaderAutoFix_(sheet) {
  // Ensure at least 1 row and enough columns
  const minCols = REQUIRED_HEADERS.length;
  if (sheet.getMaxColumns() < minCols) {
    sheet.insertColumnsAfter(sheet.getMaxColumns(), minCols - sheet.getMaxColumns());
  }
  if (sheet.getLastRow() < 1) {
    sheet.insertRowBefore(1);
  }

  // Read current header row A1:E1
  const range = sheet.getRange(1, 1, 1, minCols);
  const current = range.getValues()[0].map(v => String(v).trim());

  // Check if header matches exactly
  let same = true;
  for (let i = 0; i < minCols; i++) {
    if (current[i] !== REQUIRED_HEADERS[i]) {
      same = false;
      break;
    }
  }

  // Auto-fix if mismatch
  if (!same) {
    range.setValues([REQUIRED_HEADERS]);
  }
}

function getExpectedToken_() {
  const scriptProperties = PropertiesService.getScriptProperties();

  for (const key of TOKEN_PROPERTY_KEYS) {
    const value = String(scriptProperties.getProperty(key) || "").trim();
    if (value) {
      return value;
    }
  }

  return "";
}

function textOut(body) {
  return ContentService
    .createTextOutput(body)
    .setMimeType(ContentService.MimeType.TEXT);
}
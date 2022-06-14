
/* 
 ******************************************************************** 
  ESP Data to Google Sheets project
  Upload data to Google Sheets from Espressif Systems ESP32 & ESP8266

  by Walid Amriou
  Github : https://github.com/walidamriou/ESP_Data_to_Google_Sheets

 ******************************************************************** 
*/

function enc(str)
{
    var encoded = [];
    for (i=0; i<str.length;i++) {
        var a = str.charCodeAt(i);
        var b = ~(a ^ 0xAA) & 0xFF;
        encoded.push(b);
    }
    return encoded;
}

function hexstring(lst)
{
  var hexString = lst
      .map(function(byte) {
          // Convert from 2's compliment
          var v = (byte < 0) ? 256 + byte : byte;
          // Convert byte to hexadecimal
          return ("0" + v.toString(16)).slice(-2);
      })
      .join("");
  return hexString;
}


function debug()
{

  let sheets_file = SpreadsheetApp.getActive();
  let sheet = sheets_file.getSheetByName("data");
  let ncols = sheet.getLastColumn();
  let nrows = sheet.getLastRow();
  let cell = sheet.getRange('a1');
  let output = "";

  for (var i = 0; i < nrows; ++i)
  {
    for (var j = 0; j < ncols; ++j)
    {
      output += cell.offset(i, j).getValue();
      if(j != ncols-1)
      {
        output += ",";
      }
    }
    if(i != nrows-1)
    {
      output += "\n";
    }
  }

  //console.log(output);

  var output_enc = enc(output);
  //console.log(output_enc);

  var output_enc_hex = hexstring(output_enc);
  console.log(output_enc_hex);

  return;

  //var signature = Utilities.computeDigest(Utilities.DigestAlgorithm.SHA_256, output);
  //console.log(signature);

  let current_date = new Date(1642622469*1000);

  console.log(current_date.getTime())

  var formattedDate = Utilities.formatDate(current_date, "America/New_York", "Y-M-D h:m:s");
  console.log(formattedDate)
}

/*
 About doGet(e) function
 When a user visits an app or a program sends the app an HTTP GET request, Apps Script
 runs the function doGet(e). When a program sends the app an  HTTP POST request, Apps Script runs
 doPost(e) instead. In both cases, the e argument represents an event parameter that can contain 
 information about any request parameters.
 more info here: https://developers.google.com/apps-script/guides/web
*/

function doGet(e)
{
  let sheets_file = SpreadsheetApp.getActive();
  let sheet = sheets_file.getSheetByName("data");
  let ncols = sheet.getLastColumn();
  let nrows = sheet.getLastRow();
  let cell = sheet.getRange('a1');
  let output = "";

  for (var i = 0; i < nrows; ++i)
  {
    for (var j = 0; j < ncols; ++j)
    {
      output += cell.offset(i, j).getValue();
      if(j != ncols-1)
      {
        output += ",";
      }
    }
    if(i != nrows-1)
    {
      output += "\n";
    }
  }

  //console.log(output);

  var output_enc = enc(output);
  //console.log(output_enc);

  var output_enc_hex = hexstring(output_enc);
  //console.log(output_enc_hex);

  return ContentService.createTextOutput(output_enc_hex);
}


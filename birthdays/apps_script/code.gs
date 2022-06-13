
/* 
 ******************************************************************** 
  ESP Data to Google Sheets project
  Upload data to Google Sheets from Espressif Systems ESP32 & ESP8266

  by Walid Amriou
  Github : https://github.com/walidamriou/ESP_Data_to_Google_Sheets

 ******************************************************************** 
*/

function debug()
{

  let sheets_file = SpreadsheetApp.getActive();
  let sheet = sheets_file.getSheetByName("test");
  let x = sheet.getRange('A1').getValue();
  x += "\n";
  x += sheet.getRange('A1').getValue();

  //let cols = sheet.getRange(1, 1, 1, sheet.getLastColumn()).getValues()[0];

  let ncols = sheet.getLastColumn();
  let nrows = sheet.getLastRow();

  x+="\n";
  x+=ncols;

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

  console.log(output);
  return;



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
  let sheet = sheets_file.getSheetByName("test");
  let x = sheet.getRange('A1').getValue();
  x += "\n";
  x += sheet.getRange('A1').getValue();

  //let cols = sheet.getRange(1, 1, 1, sheet.getLastColumn()).getValues()[0];

  let ncols = sheet.getLastColumn();
  let nrows = sheet.getLastRow();

  x+="\n";
  x+=ncols;

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
  //var params = JSON.stringify(e);
  //return HtmlService.createHtmlOutput(params);

  return ContentService.createTextOutput(output);
}

 

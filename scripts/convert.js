
const filename = process.argv[2];
const fs = require('fs')
fs.readFile(filename, 'utf8', function (err,data) {
  if (err) {
    return console.log(err);
  }
  const result = data.replace(/string to be replaced/g, 'replacement');
  console.log(result);
});